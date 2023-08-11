/*
 * Copyright (c) 2006 Kapelonis Kostis  <kkapelon@freemail.gr>
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sqlite3.h>
#include <sys/types.h>
#include <dirent.h>

#include <Ecore_Data.h>
#include <Ecore_File.h>

#include "files.h"

#define VAULT_DIR ".vault"
#define DB_NAME "vault.db"
#define IN_NAME "incoming"
#define STORE_NAME "storage"

/* Size of the recent file list */
#define MAX_RECENT 20

/* More files than this have their own directory */
#define CATEGORY_LIMIT 3

static int database_exists(void);
static char *index_file(const struct storage_node *what,const char *tags);
static void panic(char *zErrMsg);
static void sanitize_filename(char *filename, short slash);

/* Global variable FIXME */
struct global_info
{
	sqlite3 *db; /* The handle for the database */
	char db_path[_POSIX_PATH_MAX]; /* Path to vault.db file. Defined by VAULT_DIR */
	char inc_path[_POSIX_PATH_MAX]; /* Path for incoming files. Defined by IN_NAME */
	char store_path[_POSIX_PATH_MAX];/* Path for stored files. Defined by STORE_NAME */
} dbfs_info;

/* 
 * Opens that sqlite 3 database and sets the various paths.
 */
int dbfs_init(void)
{
	int ret;
	char *path=NULL;

	/* First find the home directory of the user */
	path=getenv("HOME");
	if(path==NULL)
	{
		printf("Your $HOME variable is not set\n");
		return 1;
	}
	printf("Home is at %s\n",path);
	snprintf(dbfs_info.db_path,_POSIX_PATH_MAX,"%s/%s/%s",path,VAULT_DIR,DB_NAME);
	snprintf(dbfs_info.inc_path,_POSIX_PATH_MAX,"%s/%s/%s",path,VAULT_DIR,IN_NAME);
	snprintf(dbfs_info.store_path,_POSIX_PATH_MAX,"%s/%s/%s",path,VAULT_DIR,STORE_NAME);
	ret=database_exists();
	if(ret==0) return 0;
	
	return 1;
}


/* Close the database. FIXME see if anything 
 * else should be freed 
 */
void dbfs_shutdown(void)
{
	printf("Cleaning up\n");
	sqlite3_close(dbfs_info.db);
}

/*
 * Used by the launch function of the 3 dialogs
 */
const char *dbfs_get_storage_path(void)
{
	return dbfs_info.store_path;
}

int database_exists(void)
{
	int rc;

	printf("Checking for database\n");
	printf("Attemting to open %s\n",dbfs_info.db_path);

	rc = sqlite3_open(dbfs_info.db_path, &dbfs_info.db);
	if( rc ){
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(dbfs_info.db));
		fprintf(stderr, "Run the %s/db/vault_init script first\n",PKGDATADIR);
		sqlite3_close(dbfs_info.db);
		return 0;
	}

	return 1;
}

/* Called by the incoming dialog.
 * Our task is to scan the incoming dictory and
 * present to the users the files/folders that reside
 * there.
 */
unsigned int dbfs_check_for_incoming(Ecore_List *inc_list)
{
	DIR *in_directory;

	unsigned int found=0;
	printf("Checking for incoming files\n");
	printf("Attemting to open %s\n",dbfs_info.inc_path);
	in_directory=opendir(dbfs_info.inc_path);
	if(in_directory==NULL)
	{
		perror("Cannot open directory: ");
		return 0;
	}
	/* Since we are here we are ok */
	printf("Getting content\n");
	while(1)
	{
		struct storage_node *data;
		struct dirent *current=NULL;
		current=readdir(in_directory); /* get the next directory entry */
		if(current==NULL)
		{
			perror("End of reading: ");
			break;
		}
		/* Skip . and .. and all hidden files */
		if(strncmp(current->d_name,".",1)==0) continue;
		/* OK we have something */
		found++;
		data=malloc(sizeof(struct storage_node));
		printf("Have %s with type %d(%d)\n",current->d_name,current->d_type,current->d_reclen);
		if(current->d_type==DT_DIR) /* A directory */
		{
			strncpy(data->name,current->d_name,_POSIX_PATH_MAX);
			strncpy(data->path,dbfs_info.inc_path,_POSIX_PATH_MAX);
			data->type=group;
			printf("%s is to be in\n",data->name);
			ecore_list_prepend(inc_list,data);
		}
		else if(current->d_type==DT_REG) /* A file */
		{
			strncpy(data->name,current->d_name,_POSIX_PATH_MAX);
			strncpy(data->path,dbfs_info.inc_path,_POSIX_PATH_MAX);
			data->type=file;
			printf("%s is to be in\n",data->name);
			ecore_list_prepend(inc_list,data);
		}
	}

	closedir(in_directory);

	printf("*****Got %d files\n",found);
	return found;
}


/* 
 * The user has decided that this file should
 * not be imported into the vault. We
 * need to delete it
 */
int dbfs_delete_file(const struct storage_node *what)
{
	char fullname[_POSIX_PATH_MAX];

	sprintf(fullname,"%s/%s",what->path,what->name);
	printf("Removing Object %s\n",fullname);
	/* If it a directory */
	if(what->type==group)
	{
		printf("It is a directory\n");
		return ecore_file_recursive_rm(fullname);
	}
	/* It is a file/application/module */
	else
	{
		printf("It is NOT a directory\n");
		return ecore_file_unlink(fullname);
	}

	return 0;
}

static int search_callback(void *list, int argc, char **argv, char **azColName)
{
	int i;
	Ecore_List *results;
	struct storage_node *data;

	results=(Ecore_List *)list;

	for(i=0; i<argc; i=i+3){
		data=malloc(sizeof(struct storage_node));
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		/* First is fileid */
		data->id=atol(argv[i]);
		/* Second is the filename */
		printf("%s = %s\n", azColName[i+1], argv[i+1] ? argv[i+1] : "NULL");
		strncpy(data->name,argv[i+1],_POSIX_PATH_MAX);
		/* Third is the relative path/tag */
		printf("%s = %s\n", azColName[i+2], argv[i+2] ? argv[i+2] : "NULL");
		strncpy(data->path,argv[i+2],_POSIX_PATH_MAX);
		ecore_list_prepend(results,data);
	}
	printf("\n");
	return 0;
}

/* Get a list of tags, and returns files in the database that match */
unsigned int dbfs_search(const char *pattern,Ecore_List *results)
{
	int rc;
	char *zErrMsg = NULL;
	char *pat;
	char *search_term;
	char query[255];
	printf("Searching for %s\n",pattern);

	pat=strdup(pattern);
	search_term=strtok(pat," ");
	sprintf(query,"select distinct files.fileid,filename,relpath from match,files where match.fileid=files.fileid and (");
	while(1)
	{
		if(search_term==NULL) break;
		printf("Search term is is %s\n",search_term);
		sprintf(query,"%s match.tag LIKE '%%%s%%' or",query,search_term);
		search_term=strtok(NULL," "); /* get next tag */
	}
	sprintf(query,"%s 1=0)",query);
	printf("Running query %s\n",query);
	rc=sqlite3_exec(dbfs_info.db,query,search_callback,(void *)results,&zErrMsg);
	if(rc!=SQLITE_OK) panic(zErrMsg);
	return 0;
}

static int get_recent_callback(void *list, int argc, char **argv, char **azColName)
{
	int i;
	Ecore_List *results;
	struct storage_node *data;

	results=(Ecore_List *)list;

	for(i=0; i<argc; i=i+3){
		data=malloc(sizeof(struct storage_node));
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		/* First is fileid */
		data->id=atol(argv[i]);
		/* Second is the filename */
		printf("%s = %s\n", azColName[i+1], argv[i+1] ? argv[i+1] : "NULL");
		strncpy(data->name,argv[i+1],_POSIX_PATH_MAX);
		/* Third is the relative path/tag */
		printf("%s = %s\n", azColName[i+2], argv[i+2] ? argv[i+2] : "NULL");
		strncpy(data->path,argv[i+2],_POSIX_PATH_MAX);
		ecore_list_prepend(results,data);
	}
	printf("\n");
	return 0;
}

/* Read the recent table from the database. Notice that
 * the GUI will show only MAX_RECENT results. 
 */
unsigned int dbfs_get_recent(Ecore_List *results)
{
	int rc;
	char *zErrMsg = NULL;
	char query[_POSIX_PATH_MAX];
	printf("Printing Recent files");

	sprintf(query,"select recent.fileid,filename,relpath from recent,files where recent.fileid=files.fileid order by popularity asc");

	printf("Running query %s\n",query);
	rc=sqlite3_exec(dbfs_info.db,query,get_recent_callback,(void *)results,&zErrMsg);
	if(rc!=SQLITE_OK) panic(zErrMsg);
	if (results==NULL) return -1;
	return 0;
}

/*
 * In order to import a file we need to perform
 * two major operations.
 * 1. Insert it into the database
 * 2. Move it physically into the vault directory
 *
 */
int dbfs_import_file(struct storage_node *what,const char *tags)
{
	int ret;
	char targetpath[_POSIX_PATH_MAX];
	char sourcepath[_POSIX_PATH_MAX];
	char *tree=NULL;

	snprintf(sourcepath,_POSIX_PATH_MAX,"%s/%s",what->path,what->name);
	sanitize_filename(what->name,1);
	printf("File to go in %s with tags %s\n",what->name,tags);

	/* This is operation 1
	 * First see where the file goes
	 */
	tree=index_file(what,tags);

	/* Then move the file too */
	snprintf(targetpath,_POSIX_PATH_MAX,"%s/%s/%s",dbfs_info.store_path,tree,what->name);
	printf("Moving to  %s ",targetpath);
	printf(" from %s\n",sourcepath);
	printf("to level >>%s<<\n",tree);
	/* Move the file to the local storage
	 * effectively renaming it
	 * This is operation 2
	 */
	ret=rename(sourcepath,targetpath);
	if(ret<0) 
	{
		perror("Could not move file!");
	}
	return ret;
}

static int already_in(void *data, int argc, char **argv, char **azColName)
{
	int *count=(int *)data;

	/* The first field is the name of the tag.
	 * We do not need it.
	 */
	printf("%s = %s\n", azColName[0], argv[0] ? argv[0] : "NULL");
	/* The second is the number of files that are marked with this tag */
	printf("Popularity: %s = %s\n", azColName[1], argv[1] ? argv[1] : "NULL");
	*count=atoi(argv[1]);

	return 0;
}

/* Put a new tag in the tags table of the database */
static int insert_if_new(char *tag)
{
	int rc;
	char query[255];
	int found;
	char *zErrMsg = NULL;
	/* First check if the tag exists */
	sprintf(query,"Select name,count from tags where name = '%s'",tag);
	printf("Running query %s\n",query);
	found=-1;
	rc=sqlite3_exec(dbfs_info.db,query,already_in,(void *)&found,&zErrMsg);
	if(rc!=SQLITE_OK) panic(zErrMsg);
	if(found > -1) 
	{
		printf("%s is already in\n",tag);
		/* Add one more to the count field  since we will insert a file
		 * with this tag
		 */
		sprintf(query,"update tags set count=count + 1 where name = '%s'",tag);
		printf("Running query %s\n",query);
		rc=sqlite3_exec(dbfs_info.db,query,NULL,NULL,&zErrMsg);
		if(rc!=SQLITE_OK) panic(zErrMsg);

		return found;
	}
	/* Insert it */
	printf("%s is a new tag! Inserting it now\n",tag);
	sprintf(query,"insert into tags values('%s',1)",tag);
	rc=sqlite3_exec(dbfs_info.db,query,NULL,NULL,&zErrMsg);
	if(rc!=SQLITE_OK) panic(zErrMsg);

	return found;
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
 * Initially all files are dumped in the main vault directory currently set in
 * (.vault/storage) defined at the start of this file. The application however
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

/* If a tag is popular and a new directory is created, the rest of the files
 * marked with this tag must be moved there too.
 */
static int relocate_callback(void *where,int argc, char **argv, char **azColName)
{
	int i;
	int ret;
	char targetpath[_POSIX_PATH_MAX];
	char sourcepath[_POSIX_PATH_MAX];

	char *dest=(char *)where;
	for(i=0; i<argc; i=i+2)
	{
		unsigned long long  fid;
		/* First is the fileid */
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		/* Then it is the actual name of the file */
		printf("%s = %s\n", azColName[i+1], argv[i+1] ? argv[i+1] : "NULL");
		fid=atol(argv[i]);
		snprintf(sourcepath,_POSIX_PATH_MAX,"%s/%s",dbfs_info.store_path,argv[i+1]);
		snprintf(targetpath,_POSIX_PATH_MAX,"%s/%s/%s",dbfs_info.store_path,dest,argv[i+1]);
		printf(">>>>Moving file %s(%llu) to dir %s\n",sourcepath,fid,targetpath);
		ret=rename(sourcepath,targetpath);
		if(ret<0) 
		{
			perror("Could not move file!");
		}

	}



	return 0;
}
static void relocate_files(const char *relpath)
{
	int rc;
	char query[255]; 
	char *zErrMsg = NULL;

	sprintf(query,"select distinct files.fileid,filename from files,match where files.fileid=match.fileid and relpath='' and tag='%s'",relpath);
	printf("Running query %s\n",query);
	rc=sqlite3_exec(dbfs_info.db,query,relocate_callback,(void *)relpath,&zErrMsg);
	if(rc!=SQLITE_OK) panic(zErrMsg);

	/* Assume that everything is OK */
	/* Update the database too */

	sprintf(query,"update files set relpath='%s' where fileid in (select distinct files.fileid from files,match where files.fileid=match.fileid and tag='%s' and relpath='')",relpath,relpath);
	printf("Running query >%s<\n",query);
	rc=sqlite3_exec(dbfs_info.db,query,NULL,NULL,&zErrMsg);
	if(rc!=SQLITE_OK) panic(zErrMsg);
}
	
/*
 * The first operation is split to the following steps 
 * a. Insert the details of the file 
 * b. Insert the tags that this file is expressed with (the new ones)
 * c. Update the counters for the old ones TODO 
 * d. Insert match entries (one for each tag) 
 * e. Update general stats TODO
 */
static char *index_file(const struct storage_node *what,const char *tags)
{ 
	int rc; 
	char query[255]; 
	char *zErrMsg = NULL;
	char *all_tags; 
	char *tag=NULL; 
	unsigned long long last; 


	int popular=0;
	char *popular_tag=NULL;
	char *newpath=NULL;

	printf("Deciding what to do %s(%s)\n",what->name,tags);

	/* This is step a */
	sprintf(query,"insert into files values(NULL,NULL,'%s','%s')",what->name,"Unused");
	printf("Running query %s\n",query);
	rc=sqlite3_exec(dbfs_info.db,query,NULL,NULL,&zErrMsg);
	if(rc!=SQLITE_OK) panic(zErrMsg);

	last=sqlite3_last_insert_rowid(dbfs_info.db);
	all_tags=strdup(tags);
	tag=strtok(all_tags," ");
	while(1)
	{
		int count;
		if(tag==NULL) break;
		printf("New tag is %s\n",tag);
		/* See how popular this tag is */
		/* This is step b */
		count=insert_if_new(tag);
		printf(">>>>>>>>>>>Pop = %d\n",count);
		if (count > popular) 
		{
			popular=count;
			if(popular_tag!=NULL) free(popular_tag);
			popular_tag=strdup(tag);
		}
		/* This is step d */
		sprintf(query,"insert into match values('%s',%llu)",tag,last);
		rc=sqlite3_exec(dbfs_info.db,query,NULL,NULL,&zErrMsg);
		if(rc!=SQLITE_OK) panic(zErrMsg);
		tag=strtok(NULL," ");
	}
	printf(">>>>>>>>>>>Vault logic reporting\n");
	printf("Highest count was %d\n",popular);
	printf("Highest tag was %s\n",popular_tag);
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	if(popular > CATEGORY_LIMIT)
	{
		printf(">>>>>>>>Directory already created\n");
		/* A directory for this category of files 
		 * already exists.
		 */
		newpath=strdup(popular_tag);
	}
	else if(popular == CATEGORY_LIMIT)
	{
		char *path;
		char targetpath[_POSIX_PATH_MAX];

		printf(">>>>>>>>>>>Creating a new directory\n");
		path=getenv("HOME");
		if(path==NULL)
		{
			printf("Your $HOME variable is not set\n");
			exit(1); /* FIXME */
		}
		printf("Home is at %s\n",path);
		sprintf(targetpath,"%s/%s/%s/%s",path,VAULT_DIR,STORE_NAME,popular_tag);
		printf("Creating %s\n",targetpath);
		ecore_file_mkdir(targetpath);
		/* Now we need to move into the new directory all the other files
		 * that also have this tag.
		 */
		newpath=strdup(popular_tag);
		relocate_files(newpath);
	}
	else 
	{

		printf(">>>>>>>>>Normal case\n");
		newpath=strdup("");
	}

	sprintf(query,"update files set relpath='%s' where fileid=%llu",newpath,last);
	printf("Running query %s\n",query);
	rc=sqlite3_exec(dbfs_info.db,query,NULL,NULL,&zErrMsg);
	if(rc!=SQLITE_OK) panic(zErrMsg);

	return newpath;


}

static int already_recent(void *data, int argc, char **argv, char **azColName)
{
	int *i=(int *)data;
	*i=1;

	return 0;
}

static int is_recent_full(void *data, int argc, char **argv, char **azColName)
{
	int entries;
	int *result=(int *)data;

	printf("%s = %s\n", azColName[0], argv[0] ? argv[0] : "NULL");
	entries=atoi(argv[0]);
	if(entries >= MAX_RECENT)
		*result=1; /* Recent list is full */
	else 
		*result=0; /* Recent list is NOT full */

	return 0;
}
/*
 * User has opened a file. We need to add it to the recent
 * list. If it is already in the recent list, just
 * increase its counter so that we know that it
 * is popular.
 *
 * The maximum number of files that we keep in the recent list
 * is MAX_RECENT (defined at the start of this file). So if
 * the recent list is already full we need to take out
 * the least popular file before inserting the new one.
 */
void dbfs_add_to_recent(unsigned long long fileid)
{
	int rc;
	char *zErrMsg = NULL;
	char query[255];
	int found; /* If it is already in the recent list */
	int full; /* if the recent list is full */

	printf("Got a hit counter on %llu\n",fileid);

	found=0;
	sprintf(query,"Select fileid from recent where fileid = %llu",fileid);
	printf("Running query %s\n",query);
	rc=sqlite3_exec(dbfs_info.db,query,already_recent,(void *)&found,&zErrMsg);
	if(rc!=SQLITE_OK) panic(zErrMsg);
	if(found==1) 
	{
		printf("%llu is already in\n",fileid);
		/* So we increase it counter */
		sprintf(query,"update recent set popularity=popularity+1 where fileid=%llu",fileid);
		rc=sqlite3_exec(dbfs_info.db,query,NULL,NULL,&zErrMsg);
		if(rc!=SQLITE_OK) panic(zErrMsg);

		return;
	}
	/* Since we are here the file is NOT in the recent list
	 * Before adding it, let's check if the list is full
	 */
	full=0;
	sprintf(query,"select count(fileid) from recent");
	rc=sqlite3_exec(dbfs_info.db,query,is_recent_full,(void *)&full,&zErrMsg);
	if(rc!=SQLITE_OK) panic(zErrMsg);
	if(full==1)
	{
		printf("Recent list is full\n");
		/* First delete the least popular entry in the recent list */
		sprintf(query,"delete from recent where popularity=(select min(popularity) from recent)");
		rc=sqlite3_exec(dbfs_info.db,query,NULL,NULL,&zErrMsg);
		if(rc!=SQLITE_OK) panic(zErrMsg);
	}

	/* Everything is fine. We can finally add it to the recent list */
	sprintf(query,"insert into recent values(%llu,1)",fileid);

	rc=sqlite3_exec(dbfs_info.db,query,NULL,NULL,&zErrMsg);
	if(rc!=SQLITE_OK) panic(zErrMsg);
}

/*
 * Function to create a safe name for a file, discarding all
 * dangerous characters. Functions borrowed from the source
 * code of the Mutt email client under the GPL licence.
 * Originally taken from lib.c from mutt 1.4.2.2
 */
static void sanitize_filename(char *filename, short slash)
{
	/* These are allowed character for a filename */
	char safe_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+@{}._-:%/";
	if (!filename) return;

	/* Scan all letters of the filename */
	for (; *filename; filename++)
	{
		if ((slash && *filename == '/') || !strchr (safe_chars, *filename))
			*filename = '_';/* Replace dangerous characters with _ */
	}
}

/* 
 * FIXME better error reporting is obviously needed!
 */
static void panic(char *zErrMsg)
{
	printf("Error in database!!!!!!");
	fflush(stdout);
	sqlite3_close(dbfs_info.db);
	fprintf(stderr, "SQL error: %s\n", zErrMsg);
	fflush(stderr);
	exit(1);
}
