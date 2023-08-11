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
#include <glib.h>

#include "app.h"
#include "modapp.h"
#include "engine.h"


/* Constructor */
Engine* create_Engine(void)
{
	Engine *result = NULL;
	result = g_new(Engine,1);

	result->knowledge = NULL;
	result->apps = g_hash_table_new(g_str_hash,g_str_equal); 
	result->modules = g_hash_table_new(g_str_hash,g_str_equal); 

	return result;
}

/* Print knowledge information (summary of all modapps) */
void show_knowledge(Engine *eng)
{
	int i;
	int nlines;
	g_debug("Dumping Knowledge");

	nlines = g_slist_length(eng->knowledge);
	g_debug("Knowledge has %d entries",nlines);
	for(i=0 ; i < nlines ; i++)
	{
		Knowledge *knowbit = (Knowledge *)g_slist_nth_data(eng->knowledge,i);
		g_message("I have %s/%s/%s",knowbit->type,knowbit->command,knowbit->description);
	}
}

App *find_application(Engine *eng,gchar *keyword)
{
	App *result = NULL;
	g_debug("Searching for application %s",keyword);
	result = (App *)g_hash_table_lookup(eng->apps,keyword);
	return result;
}

void launch_application(Engine *eng,gchar *keyword)
{
	gboolean success = FALSE;
	App *found = find_application(eng,keyword);
	success = g_spawn_command_line_async(found->command,NULL);
	if(success == FALSE)
		g_warning("Launching application %s has failed",found->command);
}

Modapp *find_modapp(Engine *eng,gchar *keyword)
{
	Modapp *result = NULL;
	g_debug("Searching for module %s",keyword);
	result = (Modapp *)g_hash_table_lookup(eng->modules,keyword);
	return result;
}
/* Destructor */
void free_Engine(Engine *eng)
{
	//TODO add code here for members of the struct
	g_free(eng);
}

