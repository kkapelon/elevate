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
 * Project Elevate - Core 
 */

/*
 * Header for an application MODAPP
 */

#ifndef MODAPP_H
#define MODAPP_H

typedef struct Argument
{
	gchar *description;
	gchar **keyword;
	gboolean optional;
	gboolean implied;
	gboolean parameter;
	gchar *default_value;
	gchar *pattern;
}Arg;

typedef struct Module
{
	gchar *description;
	gchar **keyword;
	gchar *command;

	GHashTable *arguments;

}Modapp;

/* Constructor */
Arg* create_Arg(void);

/* Destructor */
void free_Arg(Arg *what);

/* Constructor */
Modapp* create_Modapp(void);

/* Destructor */
void free_Modapp(Modapp *what);




#endif
