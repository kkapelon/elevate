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
#include "schema.h"

/* Check if a database is present */
gboolean dbfs_exists(app_state *as)
{
	if(as->debug_mode) g_debug("See if the database exists\n");
	return g_file_test(as->vault_path,G_FILE_TEST_EXISTS);
}

static void create_table(sqlite3 *db,gchar *sql)
{
	int rc; 
	gchar *zErrMsg = NULL;

	rc = sqlite3_exec(db,sql,NULL,NULL,&zErrMsg);
	if( rc!=SQLITE_OK ){
		g_critical("SQL error: %s\n", zErrMsg);
		exit(1);
	}
	sqlite3_free(zErrMsg);
}

/* Create a new database */
void dbfs_create(app_state *as)
{
	int rc; 
	gchar sql[300];

	if(as->debug_mode) g_debug("Creating new database at %s\n",as->db_path);

	rc = g_mkdir(as->vault_path,0755);
	if(rc !=0)
	{
		g_critical("Could not create directory %s\n",as->vault_path);
		exit(1);
	}	
	rc = g_mkdir(as->store_path,0755);
	if(rc !=0)
	{
		g_critical("Could not create directory %s\n",as->vault_path);
		exit(1);
	}

	rc = sqlite3_open_v2(as->db_path,&as->db,SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	if(rc != SQLITE_OK) 
	{
		g_critical("Could not create database!");
		exit(1);
	}
	/* Fill the database with tables */
	g_snprintf(sql,300,"%s",TABLE_TAGS);
	if(as->debug_mode) g_debug("SQL:%s\n",sql);
	create_table(as->db,&sql[0]);

	g_snprintf(sql,300,"%s",TABLE_MATCH);
	if(as->debug_mode) g_debug("SQL:%s\n",sql);
	create_table(as->db,&sql[0]);

	g_snprintf(sql,300,"%s",TABLE_FILE);
	if(as->debug_mode) g_debug("SQL:%s\n",sql);
	create_table(as->db,&sql[0]);

	g_snprintf(sql,300,"%s",TABLE_RECENT);
	if(as->debug_mode) g_debug("SQL:%s\n",sql);
	create_table(as->db,&sql[0]);

	sqlite3_close(as->db);
}

/* Connect to the database */
int dbfs_init(app_state *as)
{
	int rc; 
	if(as->debug_mode) g_debug("Opening database at %s\n",as->db_path);

	rc = sqlite3_open_v2(as->db_path,&as->db,SQLITE_OPEN_READWRITE, NULL);
	if(rc != SQLITE_OK) 
	{
		g_critical("Could not open database!");
		exit(1);
	}
}
/* Disconnect from the database */
void dbfs_shutdown(app_state *as)
{
	if(as->debug_mode) g_debug("Closing database at %s\n",as->db_path);
	sqlite3_close(as->db);
}

