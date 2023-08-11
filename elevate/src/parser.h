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

/*
 * Header for the Command parser 
 */

#ifndef PARSER_H
#define PARSER_H

typedef struct command_parser
{
	gchar *description;
	gchar *type;
	gchar *command;
}Parser;

/* Constructor */
Parser* create_Parser(void);

void start_parsing(gchar *input,Engine *eng);

/* Destructor */
void free_Parser(Parser *par);




#endif
