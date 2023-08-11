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

#ifndef DB_H
#define DB_H

/* Check if a database is present */
gboolean dbfs_exists(app_state *as);

/* Create a new database */
void dbfs_create(app_state *as);

/* Connect to the database*/
int dbfs_init(app_state *as);
/* Disconnect from the database */
void dbfs_shutdown(app_state *as);

#endif
