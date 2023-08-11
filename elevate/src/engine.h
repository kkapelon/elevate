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
 * Header for an modapp engine 
 */

#ifndef ENGINE_H
#define ENGINE_H

typedef struct descriptions
{
	gchar *description;
	gchar *type;
	gchar *command;
}Knowledge;

typedef struct Capabilities
{
	GSList *knowledge; /* Holds an array of Knowledge structs */
	GHashTable *apps;
	GHashTable *modules;
}Engine;

/* Constructor */
Engine* create_Engine(void);

void show_knowledge(Engine *eng);

//TODO see why a simple App does not work here
struct Application *find_application(Engine *eng,gchar *keyword);
void launch_application(Engine *eng,gchar *keyword);

//TODO again here on struct Module works
struct Module *find_modapp(Engine *eng,gchar *keyword);


/* Destructor */
void free_Engine(Engine *eng);




#endif
