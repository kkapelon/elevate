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
#include "lang.h"

static void feed(Language *lang,gchar *word);
static gboolean is_application(gchar *word,Engine *eng);
static gboolean is_launch_verb(gchar *word);
static gboolean is_open_verb(gchar *word);
static gboolean is_object(gchar *word);
static gboolean is_modapp(gchar *word,Engine *eng);

/* Constructor */
Language* create_Language(Engine *eng)
{
	Language *result = NULL;
	result = g_new(Language,1);
	result->eng = eng;
	result->sen = g_new(Sentence,1);
	result->sen->type = -1;


	return result;
}
/*
 * There are several kind of sentences that can be parsed.
 * Each keyword adds points to a specific kind of sentence.
 * When parsing is finished the sentence with the highest points
 * is assumed to be the correct.
 * 
 * Kinds of sentences
 * 
 * 0. Application: firefox
 * 1. Application: launch firefox
 * 2. File open: open lala.pdf
 * 3. Module: compress lala.pdf
 * 4. Vault: search all files tagged with video
 * 5. Information: show me the free memory
 * 
 * We need to be able to understand the difference between
 * show lala.pdf (sentence 3 runs xpdf lala.pdf) with
 * show the headers of lala.pdf (sentence 4 runs pdfinfo lala.pdf)
 */
void process(Language *lang,gchar *input)
{
	gchar** tokens = NULL;
	int i =0;
	int greatest = -1;
	int winner = -1;

	//Get all words
	tokens = g_strsplit(input," ",-1);
	g_debug("Found %d tokens",g_strv_length(tokens));

	/* 
	 * For each kind of possible sentence
	 * we keep a single counter.
	 */
	for(i=0;i<6;i++) lang->points[i] = 0;

	for(i=0;i<g_strv_length(tokens);i++)
	{
		g_strstrip(tokens[i]);
		feed(lang,tokens[i]);
	}

	//Find the sentence with largest score
	for(i =0;i<6;i++)
	{
		if(lang->points[i] >= greatest) 
		{
			greatest = lang->points[i];
			winner = i;
		}
	}
	if(greatest ==0) return; //No sentence

	lang->sen->type = winner;

	g_strfreev(tokens);

}


/* Destructor */
void free_Language(Language *lang)
{
	//TODO add code here for members of the struct
	g_free(lang);
}

static void feed(Language *lang,gchar *word)
{
	//Check for an application
	if(is_application(word,lang->eng))
	{
		g_debug("We have an application: %s ",word);
		lang->points[0]++;
		lang->points[1]++;
		lang->sen->application = g_strdup(word); //FIXME memory leak
		return;
	}
	//Check for launch keyword
	if(is_launch_verb(word))
	{
		g_debug("We have a launch verb!");
		lang->points[1]++;
		lang->points[2]++;
	}
	//Check for open keyword
	if(is_open_verb(word))
	{
		g_debug("We have an open verb!");
		lang->points[2]++;
	}
	//Check for object
	if(is_object(word))
	{
		g_debug("We have an object: %s ",word);
		lang->points[2]++;
		lang->points[3]++;
		lang->sen->object = g_strdup(word); //FIXME memory leak

		return;
	}
	//Check for module
	if(is_modapp(word,lang->eng))
	{
		g_debug("We have a module: %s ",word);
		lang->points[3]++;
		lang->sen->module = g_strdup(word); //FIXME memory leak
		return;
	}



}	
static gboolean is_application(gchar *word,Engine *eng)
{
	App *app = find_application(eng,word);
	if(app != NULL) return TRUE;
	else return FALSE;
}

static gboolean is_launch_verb(gchar *word)
{
	int i=0;
	//We need null as a last element so that g_strv_length works OK
	gchar *verbs[]={"open","start","launch","init",NULL};

	for(i=0;i<g_strv_length(verbs);i++)
	{
		if (g_ascii_strcasecmp(word,verbs[i]) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean is_open_verb(gchar *word)
{
	int i=0;
	//We need null as a last element so that g_strv_length works OK
	gchar *verbs[]={"edit","view","play","activate","show","display",NULL};

	for(i=0;i<g_strv_length(verbs);i++)
	{
		if (g_ascii_strcasecmp(word,verbs[i]) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean is_object(gchar *word)
{
	int maybe = -1;
	/*
	 * Examples
	 * open 1 (from a selection)
	 * open /etc/lala
	 * open yes
	 * open no.pdf
	 * open this
	 * open that
	 * open the current file
	 * open the previous file
	 * open the present file
	 */
	if(word[0] == '/') return TRUE;
	if(g_strrstr(word,".") != NULL) return TRUE;
	if(g_ascii_strcasecmp(word,"present") == 0) return TRUE;
	if(g_ascii_strcasecmp(word,"current") == 0) return TRUE;
	if(g_ascii_strcasecmp(word,"previous") == 0) return TRUE;
	if(g_ascii_strcasecmp(word,"this") == 0) return TRUE;
	if(g_ascii_strcasecmp(word,"that") == 0) return TRUE;
	maybe = g_ascii_strtoll(word,NULL,10);
	if(maybe != 0) return TRUE;


	return FALSE;
}
static gboolean is_modapp(gchar *word,Engine *eng)
{
	Modapp *modapp = find_modapp(eng,word);
	if(modapp != NULL) return TRUE;
	else return FALSE;
}

