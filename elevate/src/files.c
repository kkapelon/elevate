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
 * Code for reading the .desktop like files that contain modapps.
 * These can be either in the system directory or in the user
 * directory
 */

#include <glib.h>
#include <glib/gutils.h>
#include <glib/gstdio.h>

#include "mod_strings.h"
#include "engine.h"
#include "files.h"
#include "app.h"
#include "modapp.h"



#define APP_DIR ".elevate"
#define MOD_DIR "modules"

static void load_system_modules(Engine *eng);
static void load_user_modules(Engine *eng);
static void load_modules_at(Engine *eng,const gchar *path);
static void load_module(const gchar *filename,Engine *eng);

static void load_mod(GKeyFile *mod_file,Engine *eng);
static void load_app(GKeyFile *mod_file,Engine *eng);
static void load_info(GKeyFile *mod_file,Engine *eng);




Engine *create_capabilities(void)
{
	Engine *result = NULL;
	g_message("Loading modules...");

	result = create_Engine();
	load_system_modules(result);
	load_user_modules(result);

	show_knowledge(result);

	return result;
}
static void load_modules_at(Engine *eng,const gchar *path)
{
	GDir *mod_dir = NULL;

	g_debug("Modules will be searched at %s",path);

	// Read all module files 
	mod_dir = g_dir_open(path,0,NULL); 
	if(mod_dir == NULL)
	{
		g_warning("Could not read directory %s",path);
		return;
	}
	while(TRUE)
	{
		gchar *full_path = NULL;
		const gchar *next_file = g_dir_read_name (mod_dir);

		if(next_file == NULL) break;

		full_path = g_build_filename(path,next_file,NULL);
		load_module(full_path,eng);
		g_free(full_path);
	}

	// Clean up 
	g_dir_close(mod_dir);
}

static void load_system_modules(Engine *eng)
{
	gchar *mod_dir_path = NULL;

	mod_dir_path = g_build_filename(PKGDATADIR,MOD_DIR,NULL);
	load_modules_at(eng,mod_dir_path);

	g_free(mod_dir_path);
}

static void load_user_modules(Engine *eng)
{
	gchar *mod_dir_path = NULL;


	// Find the home directory of the user 
	const gchar *homedir = g_getenv ("HOME");
	if (!homedir)
	{
		g_warning("Your $HOME variable is not set.");
		homedir = g_get_home_dir ();
	}
	if (!homedir)
	{
		g_warning("Could not locate your home directory.");
		return;
	}
	g_message("Home is at %s",homedir);
	mod_dir_path = g_build_filename(homedir,APP_DIR,MOD_DIR,NULL);

	load_modules_at(eng,mod_dir_path);
	g_free(mod_dir_path);
}

static void load_module(const gchar *filename,Engine *eng)
{
	GKeyFile *possible = NULL;
	gboolean success = FALSE;
	GError *open_status = NULL;
	gchar *type = NULL;

	//g_debug("Reading module %s",filename);
	possible = g_key_file_new();
	success = g_key_file_load_from_file(possible,filename,G_KEY_FILE_NONE,&open_status);
	if(!success)
	{
		g_warning("Could not load file %s",filename);
		g_warning("error is %s",open_status->message);
		g_error_free(open_status);
		return;
	}
	//Check that this file is indeed a module
	success = g_key_file_has_group(possible,GENERAL_G);
	if(!success)
	{
		g_warning("File %s is not a valid module",filename);
		g_key_file_free(possible);
		return;
	}


	//Find out type of module
	type = g_key_file_get_string(possible,GENERAL_G,TYPE_P,NULL);
	if(type == NULL)
	{
		g_message("Unknown type %s for module  %s",type,filename);
		g_key_file_free(possible);
		return;
	}

	//String based comparison is a bit tricky
	g_strstrip(type);
	if(g_ascii_strcasecmp(type,APP_V) == 0)
	{
		load_app(possible,eng);
	}
	else if(g_ascii_strcasecmp(type,MOD_V) == 0)
	{
		load_mod(possible,eng);
	}

	g_free(type);
	g_key_file_free(possible);
}
static void load_mod(GKeyFile *mod_file,Engine *eng)
{
	Modapp *mod_app = NULL;
	gchar *temp = NULL;
	Knowledge *knowbit = NULL;
	int i = 0;
	int arg_n = 0;

	/*
	 * We have a mod app. Modules are the most
	 * complicated components of Project elevate
	 * since they comprise the bulk of the capabilities
	 * of the system.
	 *
	 * A Module has essentially several arguments
	 * Each argument maps into a switch of the
	 * command line executable wrapped by this module
	 *
	 * For example
	 * for the cdrecord command we need several
	 * arguments that define
	 * -target speed
	 * -cdrecorder device
	 * -iso image to write e.t.c
	 *
	 * Each of the arguments has properties on its
	 * own that define if it optional, if it is a 
	 * parameter e.t.c The most important
	 * property for an argument is the pattern
	 * that specifies how this argument affects
	 * the command line execution of the module.
	 * 
	 * For example the respective patterns are
	 * -speed=%p
	 * dev=%p
	 * %p
	 */
	mod_app = create_Modapp();

	//Description
	mod_app->description = g_key_file_get_string(mod_file,GENERAL_G,DESC_P,NULL);
	//Keyword (1 or more)
	temp = g_key_file_get_string(mod_file,GENERAL_G,KEYWORD_P,NULL);
	mod_app->keyword = g_strsplit(temp,SEP,-1); //=1 means no maximum length
	g_free(temp);
	//Command
	mod_app->command = g_key_file_get_string(mod_file,MODAPP_G,COMM_P,NULL);

	//g_debug("Command %s has %d keywords",mod_app->command,g_strv_length(mod_app->keyword));

	/*
	 * Arguments can be 0 or more. We do not know
	 * beforehand how many there are. We need to
	 * look for all key groups that have names
	 * like Argument1, Argument2 e.t.c.
	 */
	arg_n = 1;
	while(TRUE)
	{
		gboolean success = FALSE;
		Arg *arg = NULL;


		gchar *header = g_strdup_printf("%s%d",ARG_G,arg_n);
		//g_debug("Header is %s",header);
		success = g_key_file_has_group(mod_file,header);
		if(success == FALSE)
		{
			g_free(header);
			break;
		}

		arg = create_Arg();
		//Description
		arg->description = g_key_file_get_string(mod_file,header,DESC_P,NULL);
		//Keyword (1 or more)
		temp = g_key_file_get_string(mod_file,header,KEYWORD_P,NULL);
		arg->keyword = g_strsplit(temp,SEP,-1); //=1 means no maximum length
		g_free(temp);

		//Optional (or required argument)
		temp = g_key_file_get_string(mod_file,header,OPT_P,NULL);
		g_strstrip(temp);
		if(g_ascii_strcasecmp(temp,YES_V) == 0)
			arg->optional = TRUE;
		else 
			arg->optional = FALSE;
		g_free(temp);

		//Implied (even if no keyword is given this argument is assumed) 
		temp = g_key_file_get_string(mod_file,header,IMP_P,NULL);
		g_strstrip(temp);
		if(g_ascii_strcasecmp(temp,YES_V) == 0)
			arg->implied = TRUE;
		else 
			arg->implied = FALSE;
		g_free(temp);

		//Parameter (simple switch or need a value)
		temp = g_key_file_get_string(mod_file,header,PAR_P,NULL);
		g_strstrip(temp);
		if(g_ascii_strcasecmp(temp,YES_V) == 0)
			arg->parameter = TRUE;
		else 
			arg->parameter = FALSE;
		g_free(temp);

		//Default value for this argument
		arg->default_value = g_key_file_get_string(mod_file,header,DEF_P,NULL);

		//Pattern (%p is the value if any)
		arg->pattern = g_key_file_get_string(mod_file,header,PAT_P,NULL);


		//Add this argument to the rest
		for(i=0;i<g_strv_length(arg->keyword);i++)
			g_hash_table_insert(mod_app->arguments,arg->keyword[i],arg);

		g_free(header);
		arg_n++;
	}	
	
	//After everything is finished add the mod structure with the others
	for(i=0;i<g_strv_length(mod_app->keyword);i++)
		g_hash_table_insert(eng->modules,mod_app->keyword[i],mod_app);

	//Also create the knowledge entry
	knowbit = g_new(Knowledge,1);
	knowbit->description = mod_app->description;
	knowbit->type = g_strdup(MOD_V);
	knowbit->command = mod_app->command;
	//Add it to the list
	eng->knowledge = g_slist_append(eng->knowledge,knowbit);

}

static void load_app(GKeyFile *mod_file,Engine *eng)
{
	App *app = NULL;
	gchar *temp = NULL;
	gchar *accepts = NULL;
	Knowledge *knowbit = NULL;
	int i = 0;
	/*
	 * We have an application. Applications are simple.
	 * They contain a command (e.g. xpdf,gftp) and if
	 * they can open files they also have information
	 * on extensions (e.g. pdf,xml,jpg).
	 */
	app = create_App();

	//Description
	app->description = g_key_file_get_string(mod_file,GENERAL_G,DESC_P,NULL);
	//Keyword (1 or more)
	temp = g_key_file_get_string(mod_file,GENERAL_G,KEYWORD_P,NULL);
	app->keyword = g_strsplit(temp,SEP,-1); //=1 means no maximum length
	g_free(temp);
	//Command
	app->command = g_key_file_get_string(mod_file,APPLICATION_G,COMM_P,NULL);

	//g_debug("Command %s has %d keywords",app->command,g_strv_length(app->keyword));
	
	/*
	 * An application can also accept arguments (files) in order to 
	 * open them. For example xpdf can open a specific xpdf file
	 * by argument. Other applications such as xload do not
	 * take any meaningful arguments.
	 *
	 * Finally some application (such a feh) can accept multiple
	 * arguments and open them at once. In the future support
	 * for opening whole directories may be added if needed.
	 *
	 * So before we do anything we read the "accepts" fields
	 * which shows what this application does.
	 */
	accepts = g_key_file_get_string(mod_file,FILETYPES_G,ACC_P,NULL);
	if(g_ascii_strcasecmp(accepts,NON_V) !=0)
	{
		//This application accepts arguments so load them
		temp = g_key_file_get_string(mod_file,FILETYPES_G,EXT_P,NULL);
		app->extensions = g_strsplit(temp,SEP,-1);
		g_free(temp);

		temp = g_key_file_get_string(mod_file,FILETYPES_G,TRIG_P,NULL);
		app->triggers = g_strsplit(temp,SEP,-1);
		g_free(temp);

		//g_debug("Command %s has %d extension",app->command,g_strv_length(app->extensions));
		//g_debug("Command %s has %d triggers",app->command,g_strv_length(app->triggers));
		
		//Finally check if this application accepts multiple arguments
		if(g_ascii_strcasecmp(accepts,MUL_V) ==0)
			app->accept_multiple = TRUE;
		else
			app->accept_multiple = FALSE;

	}
	//After everything is finished add the app structure with the others
	for(i=0;i<g_strv_length(app->keyword);i++)
		g_hash_table_insert(eng->apps,app->keyword[i],app);

	//Also create the knowledge entry
	knowbit = g_new(Knowledge,1);
	knowbit->description = app->description;
	knowbit->type = g_strdup(APP_V);
	knowbit->command = app->command;
	//Add it to the list
	eng->knowledge = g_slist_append(eng->knowledge,knowbit);

	
	
}
static void load_info(GKeyFile *mod_file,Engine *eng)
{
}
