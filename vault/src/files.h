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

#ifndef FILES_H
#define FILES_H


/* 
 * Inside the database of the vault we can store the following:
 * Files : A single file
 * Groups : A directory/folder of files which is treated as one
 * Modules: Modules of the Elevate application TODO
 * Applications: Normal executables (usually with GUIs) TODO
 *
 * Notes:
 * - In the case of applications we do not store the actual
 *   executable but rather its path in the system. TODO
 * - In a similar manner modules contain meta-information
 *   on what executables they are depending on. TODO
 */
enum data_type
{
	file, /* a UNIX file */
	group, /* a UNIX directory */
	module, /* a module of Project elevate (.module file) */
	application /* a .desktop file */
};


/*
 * This is the main data structure of the whole program. It stores all the
 * information regarding anything (file/group e.t.c) that exists in the
 * database. Not all fields of the struct are valid at any given time, since
 * the struct essentially contains a superset of all things we need during the
 * life of a database object. We could use unions to denote this, but this is
 * just added complexity in our opinion.
 */
struct storage_node
{
	unsigned long long id; /* The object id inside the database */
	char name[255]; /* Base name of the object (no path) */
	char path[255]; /* Absolute path for incoming files. Relative for stored files */
	unsigned long int size; /* Size of the object */
	enum data_type type; /* Type of the object */
};



/* General functions */
int dbfs_init(void);
void dbfs_shutdown(void);
const char *dbfs_get_storage_path(void);

/* Functions called from the incoming dialog */
unsigned int dbfs_check_for_incoming(Ecore_List *results);
int dbfs_import_file(struct storage_node *what,const char *tags);
int dbfs_delete_file(const struct storage_node *what);

/* Functions called from the search dialog */
unsigned int dbfs_search(const char *pattern,Ecore_List *results);

/* Function called both from the search and the recent dialogs */
void dbfs_add_to_recent(unsigned long long fileid);

/* Function called by the recent dialog */
unsigned int dbfs_get_recent(Ecore_List *results);




#endif
