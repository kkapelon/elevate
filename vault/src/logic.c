/*
 * Copyright (c) 2006-2008 Kapelonis Kostis  <kkapelon@freemail.gr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Vault -- The storage component of Project Elevate.
*/
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glib.h>
#include <sqlite3.h>
#include "vault.h"
#include "logic.h"

#define DELIM ","
#define MAX_TAGS 10

/* More files than this have their own directory */
#define CATEGORY_LIMIT 2

static gchar *index_file(app_state *as,const gchar *filename,gchar **tags);
static gint tag_count_compare(gconstpointer item1, gconstpointer item2);
static gboolean tag_path_exists(app_state *as,gchar *tag_path);
static void insert_new_tag(app_state *as,gchar *tag);
static int insert_if_new(app_state *as,char *tag);
static gboolean store_file(app_state *as,gchar *filename,gchar *vault_path);
static const gchar *find_basename(gchar *filename);
static void create_category(app_state *as,gchar *tag_path); 
static gchar *replace(gchar *string,const gchar *separator,const gchar *replacement);

/*
 * In order to import a file we need to perform
 * two major operations.
 * 1. Insert it into the database
 * 2. Move it physically into the vault directory
 */
void logic_import_file(app_state *as,gchar *filename,gchar *keywords)
{
	gchar **tags = NULL;
	gchar *target = NULL;
	gboolean stored_OK = FALSE;
	const gchar *basename = NULL;

	if(as->debug_mode) g_debug("Decide what to do\n");

	/* Before doing anything check that this file 
	 * actually exists. It can also be a directory
	 */
	if(g_file_test(filename,G_FILE_TEST_EXISTS) == FALSE)
	{
		g_print("%s: No such file or directory\n",filename);
		return; 
	}
	basename = find_basename(filename);
	/*
	 * Insert the file into the database.
	 * This function returns the relative path
	 * inside the vault where the physical file
	 * should be moved.
	 */
	tags = g_strsplit(keywords,DELIM,MAX_TAGS);
	target = index_file(as,basename,tags);
	g_strfreev(tags); 

	/* Finally move the actual file into the vault */
	if(target == NULL) 
	{
		g_error("Could not index file %s\n",filename);
		return;
	}
	stored_OK = store_file(as,filename,target);
	if(stored_OK == FALSE) 
	{
		g_error("Could not move file %s into the vault\n",filename);
		return;
	}

}

/*
 * Now we reach the most useful feature of the Vault application.  The idea
 * behind the vault is that the user should no longer keep a hierarchy of her
 * files in her home directory. The computer should be smart enough to maintain
 * automatically a hierarchy on the filestem. The user is only presented a
 * search engine for her files. But the files themselves are really on the disk
 * organised in a dymanic structure according to the tags that describe them.
 *
 * Of course a trivial implementation would be for the vault to have all user
 * files in a flat way in a single directory and leave the database responsible
 * for having a "virtual" filesystem of the files. This idea suffers from
 * several evident problems. One of them is scalability. We cannot simply hold
 * all files in single directory.
 *
 * Ideally we should devise an algorithm which automatically creates/moves
 * directories around attempting to mimic a real human who would do the same
 * thing with her computer. This seems to be too complicated for now.
 * If you have other ideas contact me.
 *
 * We do something simpler.
 * Initially all files are dumped in the main vault directory (currently set in
 * .vault/storage) defined at the start of this file. The application however
 * monitors the tags the user enters for incoming files. Using them as hints it
 * creates 1-level directories for popular tags.
 *
 * We assume that tags that are very popular show big categories of files. For
 * example is the user marks too many files as "video" we assume that this
 * particular user has a lot of video files. A logical thing to do would be to
 * create a "video" subdirectory under the main vault directory and use it
 * store all these files. 
 *
 * We select a number (threshold) which shows how popular a category. The
 * number is  defined to be CATEGORY_LIMIT. That means that once CATEGORY_LIMIT files
 * are marked with the same tag, this tag is considered to be an 
 * important category for the user and therefore a new subdirectory
 * should be created explicitly for these files.
 *
 */

struct popularity
{
	gchar *tag;
	gint count;
};


static gchar *index_file(app_state *as,const gchar *filename,gchar **tags)
{
	int rc; 
	gchar sql[300];
	sqlite3_stmt *statement = NULL;
	unsigned long long last; /* Primary key of the file inserted */
	int popular=0;
	gchar *dominant=NULL;
	int i;
	struct stat fileInfo;
	unsigned long file_size = 0;
	GSList* tag_tree = NULL;
	GSList* iterator = NULL;
	GSList* tag_paths = NULL;
	struct popularity *current = NULL;

	rc = g_stat(filename,&fileInfo);
	if(rc != -1) file_size = fileInfo.st_size;

	/* In order to get its id
	 * we need to put the file into the database first 
	 */
	if(as->debug_mode) g_debug("We have %d tags for file %s\n",g_strv_length (tags),filename);

	g_snprintf(sql,300,"%s","insert into files (fileid, dominant, filename, size,description) values (NULL,NULL,? ,? ,?)");
	rc = sqlite3_prepare_v2(as->db,&sql[0],300,&statement,NULL);
	if(rc != SQLITE_OK) return NULL;

	rc = sqlite3_bind_text(statement,1,filename,-1,SQLITE_STATIC);
	if(rc != SQLITE_OK) return NULL;
	rc = sqlite3_bind_int(statement,2,file_size);
	if(rc != SQLITE_OK) return NULL;
	rc = sqlite3_bind_text(statement,3,"Unused",-1,SQLITE_STATIC);
	if(rc != SQLITE_OK) return NULL;

	rc = sqlite3_step(statement);
	if(rc != SQLITE_DONE) return NULL;

	sqlite3_finalize(statement);

	last = sqlite3_last_insert_rowid(as->db);
	//if(as->debug_mode) g_debug("Last file id was  %d\n",last);

	/* Now that we have the id of the inserted
	 * file we can deal with tags.
	 *
	 * Here is the algorithm
	 *
	 * 1. Find the counts of all tags (e.g. video = 5, trailers = 2, multimedia = 10)
	 * 2. Sort them with descending order (multimedia_video_trailers)
	 * 3. Starting from the full path and going to top level find which directory exists (until multimedia)
	 * 4. See the count for the hierarchy (e.g multimedia_video = 5)
	 * 4a. if count <  CATEGORY_LIMIT just copy the file there
	 * 4b. if count = CATEGORY_LIMIT create the deeper directory (e.g. multimedia_video_trailers) and copy all other files there.
	 *
	 */
	for(i = 0; i < g_strv_length(tags);i++)
	{
		int count;
		gchar *tag = tags[i];
		/* See how popular this tag is */
		/* This is step 1 */
		count = insert_if_new(as,tag);
		if(as->debug_mode) g_debug("Tag %s has %d matches\n",tag,count);
		g_snprintf(sql,300,"%s","insert into match (tag, fileid) values(?,?)");
		rc = sqlite3_prepare_v2(as->db,&sql[0],300,&statement,NULL);
		if(rc != SQLITE_OK) return NULL;

		rc = sqlite3_bind_text(statement,1,tag,-1,SQLITE_STATIC);
		if(rc != SQLITE_OK) return NULL;
		rc = sqlite3_bind_double(statement,2,last);
		if(rc != SQLITE_OK) return NULL;
		rc = sqlite3_step(statement);
		if(rc != SQLITE_DONE) return NULL;

		sqlite3_finalize(statement);

		current = g_new0(struct popularity,1);
		current->count = count;
		current->tag = tag;
		tag_tree = g_slist_append(tag_tree,current);
	}
	//Sort all tags. This is step 2
	tag_tree = g_slist_sort(tag_tree,(GCompareFunc)tag_count_compare);
	dominant = g_strdup("");
	for(iterator = tag_tree;iterator;iterator = iterator->next)
	{
		struct popularity *now = iterator ->data;
		g_debug("We have %s with count %d",now->tag,now->count);
		dominant = g_strconcat(dominant,"_",now->tag,NULL);
		now->tag = dominant;
		tag_paths = g_slist_prepend(tag_paths,now);
	}
	//Find dominant tag. This is step 3
	for(iterator = tag_paths;iterator;iterator = iterator->next)
	{
		gboolean found = FALSE;
		current = iterator ->data;
		//Remove the leading undescore created in step 2
		current->tag++;
		g_debug("Tag tree %s and count %d",current->tag,current->count);
		found = tag_path_exists(as,current->tag);
		if(found) break;
		dominant = current->tag;
	}
	//Step 4
	if(current->count < CATEGORY_LIMIT)
	{
		g_debug("Normal case since count is %d for %s",current->count,current->tag);
		return "";
	}
	else
	{
		g_debug("Boundary case since count is %d for %s",current->count,current->tag);
		create_category(as,current->tag);
		return replace(current->tag,"_","/");
	}

	
	//Since we are here we know the final tag
	
	//update_dominant_tag(as,last,dominant); TODO

	/* Just for now we return the root directory of the vault */
	//return "";
}
static gint tag_count_compare(gconstpointer item1, gconstpointer item2)
{
	struct popularity *first = (struct popularity *)item1;
	struct popularity *second = (struct popularity *)item2;

	if(first->count < second->count) return +1;
	else if(first->count > second->count) return -1;
	else return 0;
}

static gboolean tag_path_exists(app_state *as,gchar *tag_path)
{
	int rc;
	gchar sql[300];
	sqlite3_stmt *statement = NULL;
	gboolean found = FALSE;
	/* check if the tag exists */
	g_snprintf(sql,300,"%s","select name from tags where name = ?");

	rc = sqlite3_prepare_v2(as->db,&sql[0],300,&statement,NULL);
	if(rc != SQLITE_OK) return -1;

	rc = sqlite3_bind_text(statement,1,tag_path,-1,SQLITE_STATIC);
	if(rc != SQLITE_OK) return -1;

	rc = sqlite3_step(statement);
	/* If a row is returned then the tag exists */
	if(rc ==  SQLITE_ROW) found = TRUE;
	sqlite3_finalize(statement);

	return found;
}
/* Put a new tag in the tags table of the database */
static int insert_if_new(app_state *as,gchar *tag)
{
	int rc;
	gchar sql[300];
	sqlite3_stmt *statement = NULL;
	gboolean found = FALSE;

	found = tag_path_exists(as,tag);

	if(found == TRUE )
	{
		int count = 0 ;
		if(as->debug_mode) g_debug("Tag %s already exists\n",tag);


		g_snprintf(sql,300,"%s","select count(*) from match where tag = ?");

		rc = sqlite3_prepare_v2(as->db,&sql[0],300,&statement,NULL);
		if(rc != SQLITE_OK) return -1;

		rc = sqlite3_bind_text(statement,1,tag,-1,SQLITE_STATIC);
		if(rc != SQLITE_OK) return -1;

		rc = sqlite3_step(statement);
		/* If a row is returned then the tag exists */
		if(rc !=  SQLITE_ROW) return 0; /* Should never happen normally */;
		count = sqlite3_column_int(statement,0);
		sqlite3_finalize(statement);

		return count;
	}
	insert_new_tag(as,tag);
	return 0; /* Obviously a new tag has no files associated */
}

static void insert_new_tag(app_state *as,gchar *tag)
{
	int rc;
	gchar sql[300];
	sqlite3_stmt *statement = NULL;

	/* Insert this tag */
	if(as->debug_mode) g_debug("%s is a new tag! Inserting it now\n",tag);

	g_snprintf(sql,300,"%s","insert into tags (name, relpath)  values(?,NULL)");
	rc = sqlite3_prepare_v2(as->db,&sql[0],300,&statement,NULL);
	if(rc != SQLITE_OK) return;

	rc = sqlite3_bind_text(statement,1,tag,-1,SQLITE_STATIC);
	if(rc != SQLITE_OK) return;
	rc = sqlite3_step(statement);
	if(rc != SQLITE_DONE) return;

	sqlite3_finalize(statement);
}

static void create_category(app_state *as,gchar *tag_path)
{
	gchar targetpath[_POSIX_PATH_MAX];

	g_debug("New directory structure for %s",tag_path);
	g_snprintf(targetpath,_POSIX_PATH_MAX,"%s/%s",as->store_path,tag_path);
	g_debug("Creating directory %s",targetpath);
	g_mkdir_with_parents(targetpath,0755); /* Maybe check return value as well */
	
	
}
void logic_print_statistics(app_state *as)
{
	int rc;
	gchar sql[300];
	sqlite3_stmt *statement = NULL;
	unsigned long total_tags = 0;
	unsigned long total_files = 0;
	double total_size = 0;

	if(as->debug_mode) g_debug("Generating statistics for the database\n");
	g_print("\nEmbedded database is located: %s\n",as->db_path);
	g_print("Managed filesystem can be found: %s\n",as->store_path);

	g_snprintf(sql,300,"%s","select count(*) from tags");
	rc = sqlite3_prepare_v2(as->db,&sql[0],300,&statement,NULL);
	if(rc == SQLITE_OK)
	{
		rc = sqlite3_step(statement);
		if(rc ==  SQLITE_ROW) 
			total_tags = sqlite3_column_int(statement,0);
	}
	sqlite3_finalize(statement);


	g_snprintf(sql,300,"%s","select count(*) from files");
	rc = sqlite3_prepare_v2(as->db,&sql[0],300,&statement,NULL);
	if(rc == SQLITE_OK)
	{
		rc = sqlite3_step(statement);
		if(rc ==  SQLITE_ROW) 
			total_files = sqlite3_column_int(statement,0);
	}
	sqlite3_finalize(statement);

	g_snprintf(sql,300,"%s","select sum(size) from files");
	rc = sqlite3_prepare_v2(as->db,&sql[0],300,&statement,NULL);
	if(rc == SQLITE_OK)
	{
		rc = sqlite3_step(statement);
		if(rc ==  SQLITE_ROW) 
			total_size = sqlite3_column_int(statement,0);
	}
	sqlite3_finalize(statement);

	/* Convert bytes to megabytes */
	total_size = total_size / 1000.0;
	total_size = total_size / 1000.0;

	g_print("\nTotal number of tags: %ld\n",total_tags);
	g_print("Total files managed: %ld\n",total_files);
	g_print("Total database size (MB): %f\n",total_size);
}

void logic_print_tags(app_state *as)
{
	int rc;
	gchar sql[300];
	sqlite3_stmt *statement = NULL;

	if(as->debug_mode) g_debug("Calculating tag popularity\n");

	g_print("Tag\t\tFiles\n");

	g_snprintf(sql,300,"%s","select tag,count(tag) from match group by tag order by count(tag) desc");
	rc = sqlite3_prepare_v2(as->db,&sql[0],300,&statement,NULL);
	if(rc == SQLITE_OK)
	{
		const unsigned char *next_tag = NULL;
		unsigned long rank = 0;
		while(TRUE)
		{
			rc = sqlite3_step(statement);
			if(rc !=  SQLITE_ROW) break; 
			next_tag = sqlite3_column_text(statement,0);
			rank = sqlite3_column_int(statement,1);
			g_print("%s\t\t%ld\n",next_tag,rank);
		}
	}
	sqlite3_finalize(statement);
}

/* Copies a file into the vault.
 * The third argument is a path relative to the vault
 * directory. "" is the root of the vault.
 */
static gboolean store_file(app_state *as,gchar *filename,gchar *vault_path)
{
	const gchar* basename = NULL;
	gchar targetpath[_POSIX_PATH_MAX];
	gint moved_OK =-1;

	basename = find_basename(filename);
	g_snprintf(targetpath,_POSIX_PATH_MAX,"%s/%s/%s",as->store_path,vault_path,basename);

	if(as->debug_mode) g_debug("Moving file %s to  path %s\n",filename,targetpath);
	moved_OK = g_rename(filename,targetpath);
	if(moved_OK == 0 ) return TRUE;
	else return FALSE;
}

static const gchar *find_basename(gchar *filename)
{
	gchar *slash_position = NULL;
	const gchar *basename = NULL;
	/*
	 * The file name can be absolute or relative. It can be
	 *  bar/foo.c or even ../foo.c
	 *  We just need the filename.
	 *
	 *  There might be better ways to do this. But for now we
	 *  just get the part of the string that comes after the last
	 *  slash as the filename. We also need to sanitise the file name.
	 */
	slash_position = g_strrstr(filename,"/");
	if(slash_position == NULL)
		basename = filename;
	else
		basename = slash_position + 1; /* Since slash position includes the slash character as well */

	return basename;
}
void logic_search(app_state *as,gchar *keywords)
{
	int rc;
	int i;
	gchar sql[1024]; /* There should be a better way than this FIXME */
	sqlite3_stmt *statement = NULL;
	gchar **tags = NULL;
	const gchar *result = NULL;

	if(as->debug_mode) g_debug("Searching vault for tag(s) %s\n",keywords);

	tags = g_strsplit(keywords,DELIM,MAX_TAGS);
	/*
	 * Unfortunately this SQL commands is of variable size.
	 * It grows as more tags are entered by the user. We should
	 * make sure that the buffer holds everything
	 */

	g_snprintf(sql,300,"%s","select distinct filename,size from match,files where match.fileid=files.fileid and (");
	for(i = 0; i < g_strv_length(tags);i++)
	{
		int position = 0;
		gchar *tag = tags[i];
		position = g_strlcat(sql," match.tag LIKE ? or",1024);
	}
	g_strlcat(sql," 1=0)",1024);
	if(as->debug_mode) g_debug("We have %s\n",sql);
	rc = sqlite3_prepare_v2(as->db,&sql[0],1024,&statement,NULL);
	if(rc != SQLITE_OK) return ;
	for(i = 0; i < g_strv_length(tags);i++)
	{
		gchar *tag = tags[i];
		gchar *tag_string = g_strdup_printf("%%%s%%",tag); /* For the match operator */
		rc = sqlite3_bind_text(statement,i+1,tag_string,-1,SQLITE_TRANSIENT); /* SQL variables start from 1 */
		g_free(tag_string);
		if(rc != SQLITE_OK) break;
	}
	while(TRUE)
	{
		rc = sqlite3_step(statement);
		if(rc !=  SQLITE_ROW) break; 
		result = sqlite3_column_text(statement,0);
		g_print("%s/%s\n",as->store_path,result);
	}
	sqlite3_finalize(statement);
}

static gchar *replace(gchar *string,const gchar *separator,const gchar *replacement)
{
	 return g_strjoinv(replacement, g_strsplit(string, separator, -1)); 
}
