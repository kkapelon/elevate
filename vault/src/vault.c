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
#include <glib.h> 
#include <sqlite3.h>
#include "vault.h"
#include "db.h"
#include "logic.h"

#define PROGRAM_NAME "vault (Project Elevate) 0.2.0"

/*
 * The following variables form the command line options of the
 * vault. At the moment the vault has these capabilities
 *
 * 1. Build the database
 * 	vault --build
 * 2. Add files/folders
 * 	vault --add /incoming/report.pdf --tags sales,2008,report
 * 3. Search files
 * 	vault --search --tags video,movies
 * 4. Show tags already defined
 * 	vault --present
 * 5. Show statistics
 * 	vault --numbers
 * 6. Show recent files
 * 	vault --recent
 *
 * We also have
 *  --force for rebuilding the database when it exists
 *  --debug that shows more info
 */
static gboolean build = FALSE;
static gchar *add = NULL;
static gchar *tags = NULL;
static gboolean search = FALSE;
static gboolean present = FALSE;
static gboolean recent = FALSE;
static gboolean numbers = FALSE;
static gboolean version = FALSE;
static gboolean debug = FALSE;

static GOptionEntry entries[] = 
{
  { "build", 0, 0, G_OPTION_ARG_NONE, &build, "Build the database that holds tags", NULL },
  { "add", 'a', 0, G_OPTION_ARG_FILENAME, &add, "file/folder to add in the database", "path" },
  { "tags", 't', 0, G_OPTION_ARG_STRING, &tags, "Tags (separated with ,)", "keywords" },
  { "search", 's', 0, G_OPTION_ARG_NONE, &search, "Search the database", NULL },
  { "present", 'p', 0, G_OPTION_ARG_NONE, &present, "Show existing tags present in the database", NULL },
  { "recent", 'r', 0, G_OPTION_ARG_NONE, &recent, "Show recent files", NULL },
  { "numbers", 'n', 0, G_OPTION_ARG_NONE, &numbers, "Show statistics for the database", NULL },
  { "version", 'v', 0, G_OPTION_ARG_NONE, &version, "Show version and exit", NULL },
  { "debug", 'd', 0, G_OPTION_ARG_NONE, &debug, "Print debugging information", NULL },
  { NULL }
};

static void show_version(void);
static void show_stats(app_state *as);
static void show_recent(void);
static void search_files(app_state *as,gchar *keywords);
static void show_present(app_state *as);
static void add_file(app_state *as,gchar *filename,gchar *keywords);
static void build_db(app_state *as);
static app_state *init(gboolean debug);


static void start_working(app_state *as);
static void finish_working(app_state *as);

int main(int argc, char ** argv) 
{ 
	GError *error = NULL;
	GOptionContext *context;
	app_state *as = NULL;

	context = g_option_context_new ("- manage the vault tag database");
	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_set_description (context,"Report bugs to <kkapelon@freemail.gr>.");
	if (!g_option_context_parse (context, &argc, &argv, &error))
	{
		g_print("%s: %s\n", argv[0],error->message);
		g_print("Try `%s --help' for more information\n",argv[0]);
		return 1;
	}
	g_option_context_free(context);

	/*
	 * Prepare global application state.
	 * This will be then passes to all important
	 * functions of the programs
	 */
	as = init(debug);
	
	
	/* Take action depending on command line arguments */
	if(version == TRUE) show_version();
	else if(numbers == TRUE) show_stats(as);
	else if(recent == TRUE) show_recent();
	else if(search == TRUE && tags != NULL) 
	{
		search_files(as,tags);
	}
	else if(present == TRUE) show_present(as);
	else if (add != NULL && tags != NULL)
	{
		add_file(as,add,tags);
	}
	else if(build == TRUE)
	{
		build_db(as);
	}
	else
	{
		g_print("%s: Missing arguments\n", argv[0]);
		g_print("Try `%s --help' for more information\n",argv[0]);
	}
	return 0;
} 

static void show_version(void)
{
	g_print("%s\n",PROGRAM_NAME);
	g_print("Copyright (C) 2006-2008 Kapelonis Kostis.\n");
	g_print("This is free software.  You may redistribute copies of it under the terms of\n");
	g_print("the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n");
	g_print("There is NO WARRANTY, to the extent permitted by law.\n");
	g_print("\nWritten by Kapelonis Kostis\n");
}

static void show_stats(app_state *as)
{
	g_print("%s\n",PROGRAM_NAME);
	g_print("Database Statistics:\n");

	start_working(as);

	logic_print_statistics(as);

	finish_working(as);
}

static void show_recent(void)
{
	g_print("%s\n",PROGRAM_NAME);
	g_print("Recent files:\n");
}
static void show_present(app_state *as)
{
	g_print("%s\n",PROGRAM_NAME);
	g_print("Existing tags:\n");


	start_working(as);

	logic_print_tags(as);

	finish_working(as);
}

static void search_files(app_state *as,gchar *keywords)
{
	g_print("%s\n",PROGRAM_NAME);
	g_print("Tags are: %s\n",keywords);

	start_working(as);
	
	logic_search(as,keywords);

	finish_working(as);
}

static void add_file(app_state *as,gchar *filename,gchar *keywords)
{

	g_print("%s\n",PROGRAM_NAME);
	g_print("File is: %s\n",filename);
	g_print("Tags are: %s\n",keywords);


	start_working(as);
	
	logic_import_file(as,filename,keywords);

	finish_working(as);

}
static void build_db(app_state *as)
{
	gboolean exists = TRUE;
	g_print("%s\n",PROGRAM_NAME);
	
	/* first see if the database exists */
	exists = dbfs_exists(as);
	if(exists)
	{
		g_print("found an existing vault at %s\n",as->vault_path);
		g_print("please delete the whole %s directory and try again\n",as->vault_path);
		exit(1);
	}
	//since we are here we can create the db
	dbfs_create(as);
	g_printf("Database created! You can use now the other vault commands\n");
}

static app_state *init(gboolean debug)
{
	app_state *result = NULL;
	const gchar *path = NULL;
	
	result = g_new(app_state,1);
	result -> debug_mode = debug;

	/* First find the home directory of the user */
	path = g_getenv("HOME");
	if(path == NULL)
	{
		g_critical("Your $HOME variable is not set\n");
		exit(1);
	}
	if(debug) g_debug("Home is at %s\n",path);
	g_snprintf(result->vault_path,_POSIX_PATH_MAX,"%s/%s",path,VAULT_DIR);
	g_snprintf(result->db_path,_POSIX_PATH_MAX,"%s/%s/%s",path,VAULT_DIR,DB_NAME);
	g_snprintf(result->store_path,_POSIX_PATH_MAX,"%s/%s/%s",path,VAULT_DIR,STORE_NAME);
	return result;
}
static void start_working(app_state *as)
{
	gboolean exists = TRUE;

	/* First see if the database exists */
	exists = dbfs_exists(as);
	if(exists == FALSE)
	{
		g_print("no vault database found  at %s\n",as->vault_path);
		g_print("please rerun the command using the --build parameter and then try again\n");
		exit(1);
	}
	/* Open the existing database */
	dbfs_init(as);
}

static void finish_working(app_state *as)
{
	/* Finished work */
	dbfs_shutdown(as);
}
