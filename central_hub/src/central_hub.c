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
 * central_hub -- The launcher component of Project Elevate.
*/

#include <stdio.h> 
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include <cairo-xlib.h>

#include "sys.h"

#define TEXT_MAX 30
#define HEIGHT 800
#define WIDTH 600


struct application_state {
	GtkWidget *drawing_area; //Canvas
	gboolean internet_connected;
	gint processor_load; //0-100 normally
	glong free_space; //in GB
	gint percent_free;
	glong used_space; //in GB
	gint percent_used;

};

typedef struct application_state app_state;

static void update_stats(app_state *as);
static int processing_map(int value, int istart,int istop,int ostart,int ostop);
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
 * Prepares the cairo context each time
 * a frame is rendered
 */
static cairo_t * begin_paint (GdkDrawable *window)
{
	gint width, height;
	cairo_t *cr;

	gdk_drawable_get_size (window, &width, &height);

	cr = gdk_cairo_create(window);
	scale_for_aspect_ratio (cr, width, height);
	/*
	 * Scale the canvas so that all co-ordinates
	 * can be 0-100 regardless of the size of the window
	 */
	cairo_scale(cr,WIDTH/100,HEIGHT/100);
	/* Draw a black background as a starting canvas */
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_paint(cr);

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
 * Prints a text line anywhere on the canvas
 */
static void show_text_message (cairo_t * cr, int font_size, int pos_x,int pos_y, const char *message)
{
	double x, y;
	cairo_text_extents_t extents;

	cairo_save (cr);

	cairo_select_font_face (cr, "Consolas",
			CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

	cairo_set_font_size (cr, font_size);
	cairo_text_extents (cr, message, &extents);
	x = pos_x - (extents.width / 2 + extents.x_bearing);
	y = pos_y - (extents.height / 2 + extents.y_bearing);

	cairo_move_to (cr, x, y);
	cairo_show_text (cr, message);
	cairo_restore (cr);
}


/*
 * Draws the border around the window.
 */
static void draw_decors(cairo_t *cr)
{
	cairo_save(cr);

	cairo_set_source_rgba (cr, 0, 0, 1,1);
	cairo_set_line_width (cr, 0.5);
	cairo_rectangle (cr, 3,3,94,94);
	cairo_stroke(cr);

	cairo_restore(cr);
}

/*
 * Basic render function that draws
 * a single slice.
 */
static void draw_slice(cairo_t *cr,double xc, double yc,
		double radius,
		double angle1,
		double angle2,int width)
{
	angle1 = angle1 * (M_PI/180.0); 
	angle2 = angle2 * (M_PI/180.0);  /* in radians           */
	cairo_save(cr);


	//First the core
	cairo_new_path(cr);
	cairo_set_line_width (cr, 0.7);
	cairo_arc (cr, xc, yc, radius, angle1, angle2);

	cairo_arc_negative (cr, xc, yc, radius +width, angle2, angle1);
	cairo_close_path(cr);
	cairo_stroke (cr);

	cairo_restore(cr);
}

/*
 * Draws the "disk" that represents used and free space
 */
static void draw_space(cairo_t *cr,app_state *as)
{
	int used = 0;
	char text[TEXT_MAX];

	cairo_set_source_rgb (cr, 1, 1, 0);
	show_text_message(cr, 5,50,40,"Space");


	used = processing_map(as->percent_used,0, 100, 0 , 360);
	//Free Space
	cairo_set_source_rgba (cr, 0.592, 1, 1, 1);
	draw_slice(cr,50,40,10,used-90,270,20); 
	snprintf(text,TEXT_MAX,"%ldGB free (%d%%)",as->free_space,as->percent_free);
	show_text_message(cr, 3,20,8,text);

	//Used space
	cairo_set_source_rgba (cr, 1, 0.647, 0, 1);
	draw_slice(cr,50,40,11,270,used-90,21); 
	snprintf(text,TEXT_MAX,"%ldGB used (%d%%)",as->used_space,as->percent_used);
	show_text_message(cr, 3,80,8,text);
}

/*
 * Draws the average load as a gradient bar 
 */
static void draw_load(cairo_t *cr,app_state *as)
{

	cairo_pattern_t *pat;
	char load_text[15];
	int pin_position = 7; //7 is the minimum
	int load = as->processor_load;

	cairo_save(cr);

	//First the gradient that is always there
	pat = cairo_pattern_create_linear (0.0, 0.0,  50, 0.0);
	cairo_pattern_add_color_stop_rgb (pat, 0, 0, 1, 0);
	cairo_pattern_add_color_stop_rgb (pat, 0.6, 1, 1, 0);
	cairo_pattern_add_color_stop_rgb (pat, 1, 1, 0, 0);
	cairo_rectangle (cr, 7, 80, 50, 10);
	cairo_set_source (cr, pat);
	cairo_fill (cr);

	cairo_pattern_destroy (pat);

	//Borders and messages
	cairo_set_line_width (cr, 0.5);
	cairo_set_source_rgb (cr, 0.7, 0.3, 1.0);
	cairo_rectangle (cr, 7, 80, 50, 10);
	cairo_stroke(cr);
	cairo_set_source_rgb (cr, 1, 1, 0);
	show_text_message(cr, 3,8,77,"0%");
	show_text_message(cr, 3,40,77,"100%");
	show_text_message(cr, 3,57,77,"150%");
	snprintf(load_text,15,"Load: %d%%",load);
	show_text_message(cr, 4,30,93,load_text);

	//Finally the load "pin"
	cairo_set_source_rgb (cr, 0.7, 0.3, 1.0);
	//7 is minimum and 55 is maximum 38 is 100%
	if(load > 150)
	{
		load = 150; //over 150% it is off the chart
	}
	pin_position = processing_map(load,0,150,7,55);
	cairo_rectangle (cr, pin_position, 81, 2, 8);
	cairo_fill(cr);

	cairo_restore(cr);
}

/*
 * Draws a text box that shows internet status
 */
static void draw_network(cairo_t *cr,app_state *as)
{
	cairo_set_line_width (cr, 0.7);
	cairo_set_source_rgb (cr, 0.7, 0.3, 1.0);
	cairo_rectangle (cr, 65,78,28,15);
	cairo_stroke(cr);
	cairo_set_source_rgb (cr, 1, 1, 0);
	if(as->internet_connected == TRUE)
	{
		show_text_message(cr, 5,79,85,"Online");
	}
	else
	{
		show_text_message(cr, 5,79,85,"Offline");
	}
}


/*
 * Renders the content of the main window
 */
static void draw_example(GtkWidget *widget,app_state *as)
{
	cairo_t *cr;
	int i = 0;

	cr = begin_paint (widget->window);


	draw_decors(cr);

	draw_space(cr,as);
	draw_load(cr,as);
	draw_network(cr,as);

	end_paint (cr);
}


/*
 * Callback that is run by GTK whenever there is an expose
 * event
 */
static gboolean win_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	app_state *closure = (app_state *)data;

	draw_example(widget,closure);

	return TRUE;
}


/*
 * Creates the main GTK window and 
 * sets some basic window operations
 */
static GtkWidget *create_window (app_state *as)
{
	GtkWidget *window;
	GtkWidget *da;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Central Hub");

	g_signal_connect (window, "destroy",
			G_CALLBACK (gtk_main_quit), &window);

	da = gtk_drawing_area_new ();
	/* set a minimum size */
	gtk_widget_set_size_request (da, HEIGHT/2, WIDTH/2);
	gtk_window_set_default_size(GTK_WINDOW(window),HEIGHT,WIDTH);

	gtk_container_add (GTK_CONTAINER (window), da);


	g_signal_connect (da, "expose_event", G_CALLBACK (win_expose_event), as);


	return da;
}

/*
 * Animation thread. It just
 * redraws the main window
 */
static gint timeout_callback (gpointer data)
{
	app_state *closure = data;

	//printf("Updating stats...\n");
	update_stats(closure);
	gtk_widget_queue_draw (closure->drawing_area);

	return TRUE; //Keep this timer running
}


/*
 * Main function 
 * and creates the main GUI window
 */
int main (int argc, char *argv[])
{
	app_state closure;

	gtk_init (&argc, &argv);


		//closure.filename = g_strdup("No file chosen");
	//closure.paused = FALSE;

	closure.drawing_area = create_window (&closure);
	closure.processor_load = 0;
	closure.internet_connected = FALSE;
	closure.percent_free = 0;
	closure.free_space = 0;
	closure.percent_used = 0;
	closure.used_space = 0;


	/* Show the GUI to the user */
	gtk_widget_show_all (gtk_widget_get_toplevel (closure.drawing_area));


	/* Create animation thread if everything is ok*/
	g_timeout_add (3000, timeout_callback, &closure);

	gtk_main();

	return 0;
}



	/* Central hub contains a special argument (-d). If it is set by the
	 * user then we take over the desktop in a similar way that nautilus
	 * does for GNOME.  The is tested with Matchbox mainly. It is not a
	 * good idea to run central_hub like this (yet). Memory leaks may be
	 * still present and running the application all the time will probably
	 * be a disaster. You have been warned! 
	 * */

	///* Update the stats the first time */
	//update_stats(NULL); 
	///*Then update them every 5 seconds */
	//ecore_timer_add(5.0, update_stats,NULL); 



/* Update the 3 boxes that show system statistics */
static void update_stats(app_state *as)
{
	int ret;
	/* For system stats */
	char text[TEXT_MAX];
	int load5 = 0;
	long free_space = 0;
	int percent_free = 0;
	int online = 0;

	/* First the processor load */
	load5=get_5min_load();
	as->processor_load = load5;


	/* Then free space on the home partition */
	get_free_space(&free_space,&percent_free);
//	snprintf(text,TEXT_MAX,"%ldGB free(%d%%)",free_space,percent_free);
	//printf("Free is  %s\n",text);

	as->percent_free = percent_free;
	as->percent_used = 100 - percent_free;
	as->free_space = free_space;
	as->used_space = (free_space * 100.0)/percent_free - free_space;
	//snprintf(text,TEXT_MAX,"%ldGB used(%d%%)",as->used_space,as->percent_used);
	//printf("Used is  %s\n",text);

	/* Finally the network status */
	online=is_online_now();
	if(online)
	{
		//printf("Online!\n");
		as->internet_connected = TRUE;
	}
	else
	{
		//printf("Offline\n");
		as->internet_connected = FALSE;
	}
}


/*
 * Converts a value from a given ranget to another 
 * range. Taken from Processing.org
 */
static int processing_map(int value, int istart,int istop,int ostart,int ostop)
{
	if(istop <= 0 || value < 0 || ostop <= 0) return 0;
	//g_print("Val is %d, current = %d, requested = %d\n",value,istop, ostop);
	return ostart + (((ostop - ostart) * (value - istart)) / (istop - istart));
}

