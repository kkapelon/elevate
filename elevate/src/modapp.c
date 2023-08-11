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

#include "modapp.h"

/* Constructor */
Arg* create_Arg(void)
{
	Arg *result = NULL;
	result = g_new(Arg,1);
	return result;
}

/* Destructor */
void free_Arg(Arg *what)
{
	//TODO add code here for members of the struct
	g_free(what);
}


/* Constructor */
Modapp* create_Modapp(void)
{
	Modapp *result = NULL;
	result = g_new(Modapp,1);
	result->arguments = g_hash_table_new(g_str_hash,g_str_equal); 
	return result;
}

/* Destructor */
void free_Modapp(Modapp *what)
{
	//TODO add code here for members of the struct
	g_free(what);
}

