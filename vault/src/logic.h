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

#ifndef LOGIC_H
#define LOGIC_H

/* Print various statistics */
void logic_print_statistics(app_state *as);

/* Show existing tags */
void logic_print_tags(app_state *as);

/* Index a file in the database */
void logic_import_file(app_state *as,gchar *filename,gchar *keywords);

/* Search files in the database */
void logic_search(app_state *as,gchar *keywords);
#endif
