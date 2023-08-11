/*
 * Copyright (c) 2006-2007 Kapelonis Kostis  <kkapelon@freemail.gr>
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

#include "engine.h"
#include "parser.h"
#include "lang.h"



/* Constructor */
Parser* create_Parser(void)
{
	Parser *result = NULL;
	result = g_new(Parser,1);

	return result;
}

void start_parsing(gchar *input,Engine *eng)
{
	int i=0;
	Language *lang = NULL;
	int type;
	
	g_debug("Got %s",input);
	lang= create_Language(eng);
	process(lang,input);

	//Finished processing print table
	for(i=0;i<6;i++)
		g_debug("Sentence %d got %d",i,lang->points[i]);

	//Interpret sentence
	type = lang->sen->type;
	g_debug("Type is %d",type);

	switch(type)
	{
		case 0:
		case 1:
			//capable.launchApplication(complete.getApplication());
			g_debug("Launching application: %s",lang->sen->application);
			launch_application(eng,lang->sen->application);
			break;
		case 2:
			//capable.openObject(complete.getObject());
			g_debug("Opening %s",lang->sen->object);
			break;
		case 3:
			//output.append("Using module "+complete.getModule());
			g_debug("Module is %s",lang->sen->module);
			g_debug("Object is %s",lang->sen->object);
			break;
		case 4:
			g_debug("Using vault...");
			break;
		case 5:
			g_debug("Information mode...");
			break;
		default:
			g_debug(">>>>>>>>>>>>Could not understand sentence<<<<<<<");
			break;

	}

}

/* Destructor */
void free_Parser(Parser *par)
{
	//TODO add code here for members of the struct
	g_free(par);
}

