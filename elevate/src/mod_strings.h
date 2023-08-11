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
 * Keywords expected in the .desktop like files
 * that project elevate uses for modapps
 */

#ifndef MOD_STRINGS_H
#define MOD_STRINGS_H

/* Part 1 - Groups in the module file */

#define GENERAL_G "General"
#define APPLICATION_G "app"
#define FILETYPES_G "filetypes"
#define MODAPP_G "modapp"
#define ARG_G "Argument"

/* Part 2 - Properties */

/* Common properties for General section */
#define DESC_P "description"
#define KEYWORD_P "keyword"
#define TYPE_P "type"

/* properties for the app section */
#define COMM_P "command"

/* properties for the filetype section */
#define EXT_P "extensions"
#define ACC_P "accepts"
#define TRIG_P "triggers"

/* properties for the argument section */
#define OPT_P "optional"
#define IMP_P "implied"
#define PAR_P "parameter"
#define DEF_P "default"
#define PAT_P "pattern"

/* Part 3 - VALUES */

/* Separator for multiple value */
#define SEP ","

/* Values for type */
#define MOD_V "module"
#define APP_V "application"
#define INF_V "information"

/* Values for accepts */
#define SIN_V "single"
#define MUL_V "multiple"
#define NON_V "none"

/* Values for the module arguments */
#define YES_V "yes"
#define NO_V "no"






#endif
