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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include <cairo.h>

#include "gfx.h"
#include "integrator.h"
#include "engine.h"
#include "parser.h"
#include "files.h"


#define HEIGHT 800
#define WIDTH 800

/* 
 * Non-Gui stuff goes here 
 */
typedef struct data_model
{
	char cmd[80];
	Engine *engine;
}dm_t;

/*
 * GUI stuff goes here
 */
typedef struct win {
	GtkWidget *drawing_area; //Canvas
	dm_t dm; //Data model
	gint mode; //What state is the interface now (e.g. input or not)
	Integrator *input_box_fx; /* Percent*/
} win_t;

/* Possible modes */
#define MODE_NORMAL 1
#define MODE_INPUT 2



static void scale_for_aspect_ratio (cairo_t * cr, int widget_width, int widget_height);
static cairo_t * begin_paint (GdkDrawable *window);
static void end_paint (cairo_t *cr);
static void draw_decors(cairo_t *cr,const gchar* file);
static void draw_canvas(GtkWidget *widget,win_t *win);
static gboolean win_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data);
static gint on_key_press (GtkWidget * widget, GdkEventKey * event, gpointer data);
static GtkWidget *create_window (win_t *as);
static gint timeout_callback (gpointer data);
static void draw_status(cairo_t *cr,win_t *win);

/*
 * Reads all modules from the filesystem 
 * and creates the application window
 */
int main (int argc, char *argv[])
{
	win_t closure;

	gtk_init (&argc, &argv);


	/* Elevate modules */
	closure.dm.engine = create_capabilities();
	closure.dm.cmd[0]='\0';
	/* GUI init */
	closure.drawing_area = create_window (&closure);
	closure.mode = MODE_NORMAL;
	/* Integrators */

	closure.input_box_fx = Integrator_create();
	Integrator_set(closure.input_box_fx,0);
	Integrator_target(closure.input_box_fx,101); //this is percent that we need to reach until 100

	/* Show the GUI to the user */
	gtk_widget_show_all (gtk_widget_get_toplevel (closure.drawing_area));
	
	/* Create animation thread at 25fps*/
	g_timeout_add (40, timeout_callback, &closure);

	gtk_main();

	return 0;
}


/*
 * Draws the static background for the window
 */
static void draw_background(cairo_t *cr)
{
	cairo_pattern_t *pat;

	cairo_save(cr);

	/* Gradient pattern from white to light blue */
	pat = cairo_pattern_create_linear (0.0, 0.0,  0.0,100);
	cairo_pattern_add_color_stop_rgb(pat, 0, 0.98, 0.98, 0.98);
	cairo_pattern_add_color_stop_rgb(pat, 100, 0.65, 0.77, 0.79);
	cairo_rectangle (cr, 0, 0, 100, 100); //Covers the whole screen
	cairo_set_source (cr, pat);
	cairo_fill (cr);

	cairo_pattern_destroy (pat); 
	
	/* Decors on the corners */
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_set_line_width (cr, 0.5);

	cairo_move_to(cr,2,7);
	cairo_rel_line_to(cr,0,-5);
	cairo_rel_line_to(cr,5,0);
	cairo_stroke(cr);

	cairo_move_to(cr,98,7);
	cairo_rel_line_to(cr,0,-5);
	cairo_rel_line_to(cr,-5,0);
	cairo_stroke(cr);

	/* Window status */
	draw_rounded_rect(cr,20,-10,60,16,3);

	/* Current Context */
	draw_rounded_rect(cr,1,91,98,8,4);

	cairo_restore(cr);

}
/*
 * Renders the content of the main window
 */
static void draw_canvas(GtkWidget *widget,win_t *win)
{
	cairo_t *cr;

	cr = begin_paint (widget->window);
	/*
	 * Scale the canvas so that all co-ordinates
	 * can be 0-100 regardless of the size of the window
	 */
	cairo_scale(cr,WIDTH/100,HEIGHT/100);


	/* Step 1 Static window contents */
	draw_background(cr);

	/* Step 2 Draw messages, tasks and context */
	draw_status(cr,win);

	end_paint (cr);
}

static void draw_status(cairo_t *cr,win_t *win)
{
	/* First the output messages */
	draw_messages_box(cr,"Messages go here",5);



	/* No need to draw command line if it is not needed */
	if(win->mode == MODE_INPUT)
	{
		gfloat fx_number = Integrator_get(win->input_box_fx);
		draw_input_box (cr, win->dm.cmd, fx_number /100.0); //Convert it to percent
	}
}

/*
 * Callback that is run when a key is pressed 
 */
static gint on_key_press (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
	win_t *closure = (win_t *)data;
	gchar *input = NULL;

	switch (event->keyval)
	{
		case GDK_Escape:
			memset(closure->dm.cmd,0,80); //Clear the command line
			closure->mode = MODE_NORMAL;
			break;
		case GDK_BackSpace:
			if(strlen(closure->dm.cmd) > 0)
				closure->dm.cmd[strlen(closure->dm.cmd)-1] = '\0';
			break;
		case GDK_Return:
			input = g_strdup(closure->dm.cmd);
			memset(closure->dm.cmd,0,80); //Clear the command line
			start_parsing(input,closure->dm.engine);
			g_free(input);
			closure->mode = MODE_NORMAL;
			break;
		default:
			g_print("Got %s\n",event->string);
			strncat(closure->dm.cmd,event->string,80);
			closure->mode = MODE_INPUT;
			Integrator_set(closure->input_box_fx,0);
			break;
	}
	g_print("command is now %s\n",closure->dm.cmd);
	return TRUE;
}



/*
 * Scales the window each time it is resized
 */
static void scale_for_aspect_ratio (cairo_t * cr, int widget_width, int widget_height)
{
	double scalex;
	double scaley;

	scaley = ((double) widget_height) / HEIGHT;
	scalex = ((double) widget_width) / WIDTH;

	cairo_scale (cr, scalex, scaley);
}


/*
 * Prapares the cairo context each time
 * a frame is rendered
 */
static cairo_t * begin_paint (GdkDrawable *window)
{
	gint width, height;
	cairo_t *cr;

	gdk_drawable_get_size (window, &width, &height);

	cr = gdk_cairo_create(window);
	scale_for_aspect_ratio (cr, width, height);

	return cr;
}

/*
 * Cleans up the cairo context when the frame
 * is finished
 */
static void end_paint (cairo_t *cr)
{
	cairo_destroy (cr);
}

/*
 * Callback that is run by GTK whenever there is an expose
 * event
 */
static gboolean win_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	win_t *closure = (win_t *)data;

	draw_canvas(widget,closure);

	return TRUE;
}

/*
 * Creates the main GTK window and 
 * sets some basic window operations
 */
static GtkWidget *create_window (win_t *as)
{
	GtkWidget *window;
	GtkWidget *da;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Project Elevate");

	g_signal_connect (window, "destroy",
			G_CALLBACK (gtk_main_quit), &window);

	da = gtk_drawing_area_new ();
	/* set a minimum size */
	gtk_widget_set_size_request (da, HEIGHT/2, WIDTH/2);
	gtk_window_set_default_size(GTK_WINDOW(window),HEIGHT,WIDTH);

	gtk_container_add (GTK_CONTAINER (window), da);


	g_signal_connect (da, "expose_event", G_CALLBACK (win_expose_event), as);

	g_signal_connect (window, "key_press_event", G_CALLBACK (on_key_press), as);



	return da;
}
/*
 * Animation thread. It just
 * redraws the main window
 */
static gint timeout_callback (gpointer data)
{
	win_t *closure = data;

	/* Advance all integrators */
	Integrator_update(closure->input_box_fx);

	gtk_widget_queue_draw (closure->drawing_area);

	return TRUE; //Keep this timer active
}


