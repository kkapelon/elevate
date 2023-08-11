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
#ifndef SCHEMA_H
#define SCHEMA_H

#define TABLE_TAGS "CREATE TABLE tags (name VARCHAR(50) PRIMARY KEY, relpath VARCHAR(255))"

#define TABLE_MATCH "CREATE TABLE match (tag VARCHAR(50), fileid INTEGER)"

#define TABLE_FILE "CREATE TABLE files (fileid INTEGER PRIMARY KEY, dominant VARCHAR(50), filename VARCHAR(255), size INTEGER, description VARCHAR(100))"

#define TABLE_RECENT "CREATE TABLE recent(fileid INTEGER PRIMARY KEY, popularity INTEGER)"

#endif
