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

#ifndef VAULT_H
#define VAULT_H

#define VAULT_DIR ".vault"
#define DB_NAME "vault.db"
#define STORE_NAME "storage"

/* Size of the recent file list */
#define MAX_RECENT 20


/*
 * This is the main data structure of the whole program. It stores all the
 * information regarding the state of the application.
 */
typedef struct application_state
{
	sqlite3 *db; /* The handle for the database */
	gchar vault_path[_POSIX_PATH_MAX]; /* Path to vault directory. Defined by VAULT_DIR */
	gchar db_path[_POSIX_PATH_MAX]; /* Path to vault.db file. Defined by VAULT_DIR and DB_NAME*/
	gchar store_path[_POSIX_PATH_MAX];/* Path for stored files. Defined by STORE_NAME */
	gboolean debug_mode; /* Defined by command line parameters */
}app_state;

#endif
