/*
 * Copyright (c) 2010 Kapelonis Kostis  <kkapelon@freemail.gr>
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
 * Syndesis -- Several stand-alone utilities for Project Elevate.
 */

#include <gtk/gtk.h>
#include <cairo.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include <string.h>

#define HEIGHT 400
#define WIDTH 400

#define LINUX_NETWORK "/proc/net/dev"
#define DEFAULT_PREFIX "eth" //Interfaces that start with this will be shown by default

#define HISTORY_SIZE 15 //How many seconds in the past we look for the maximum speed

#include "common.h"
#include "integrator.h"

typedef struct speed_arrow arrow;

struct application_state {
	GtkWidget *drawing_area; //Canvas
	gchar *selected_interface; /* eth0, eth1, lo e.t.c */
	GIOChannel *proc_reader; /* Channel for reading /proc/net/dev */
	GList *all_interfaces; /* A list of all network interfaces found */
	Integrator *title_integrator; /* Used for the zooming effect of the title */
	gchar *speed_in; /* Text to be presented on screen for incoming traffic */
	gchar *speed_out; /* Text to be presented on screen for outgoind traffic */
	gulong lastbytes_in; /* Bytes in as measured in the second before */
	gulong lastbytes_out; /* Bytes out as measured in the second before */
	gulong history_in[HISTORY_SIZE]; /* Historical data for incoming traffic */
	gulong history_out[HISTORY_SIZE]; /* Historical data for outgoind traffic */
	gint last_history_pos; /* Last Position used in history */
	gint arrows_top_in; /* Y position of the top arrow for incoming traffic */
	gint arrows_bottom_out; /* Y position of the bottom arrow for outgoing traffic */
	Integrator *status_in; /* Percent of current ingoing speed over maximum */
	Integrator *status_out; /* Percent of current outoing speed over maximum */
};

typedef struct application_state app_state;

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

	cairo_set_source_rgba (cr, 1, 1, 0, 1);
	cairo_move_to (cr, x, y);
	cairo_show_text (cr, message);
	cairo_restore (cr);
}

/*
 * Draws the fancy corners of the frame
 */
static void draw_decors(cairo_t *cr,const gchar* file,gint x_position)
{
	cairo_save(cr);

	cairo_set_source_rgb (cr, 0, 0, 1);
	cairo_set_line_width (cr, 5);
	/* Small angle on the top left corner */
	cairo_move_to (cr, 10,40);
	cairo_line_to (cr, 10, 10);
	cairo_line_to (cr, 40, 10);
	cairo_stroke(cr);
	/* Similar angle to other side */
	cairo_move_to (cr, 390,360);
	cairo_line_to (cr, 390,390);
	cairo_line_to (cr, 360,390);
	cairo_stroke(cr);

	show_text_message(cr,12,x_position+70,30,file);

	cairo_restore(cr);
}
/*
 * Draws the text for IN/OUTe
 */
static void draw_speed_text(cairo_t *cr,const gchar* speed_in,const gchar *speed_out,gint x_position)
{
	cairo_save(cr);

	cairo_select_font_face (cr, "Consolas",
			CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

	cairo_set_font_size (cr, 20);
	cairo_set_source_rgba (cr, 1, 0.647, 0, 1);
	cairo_move_to (cr, x_position, 100);
	cairo_show_text (cr, speed_in);
	
	cairo_set_source_rgba (cr, 0.592, 1, 1, 1);
	cairo_move_to (cr, x_position, 150);
	cairo_show_text (cr, speed_out);

	cairo_restore(cr);
}

/*
 * Draws an arrow for incoming traffic. Arrows go downwards and have
 * the same colour as the incoming speed text 
 */
static void draw_arrow_in(cairo_t *cr,float angleNumber,int x1,int y1, int width)
{
	float angle = angleNumber * M_PI/180;
  
	int x = width * cos(angle);
	int y = width * sin(angle);

	cairo_save(cr);
	cairo_set_source_rgba (cr, 1, 0.647, 0, 1);
	cairo_set_line_width (cr, 10);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND); 

	//g_print("X is %d\n",x);
	//g_print("Y is %d\n",y);
  
  
	cairo_translate(cr,x1,y1);
	cairo_move_to(cr,-x,-y);
	cairo_line_to(cr,0,0);
	cairo_line_to(cr,x,-y);
	cairo_stroke(cr);

	cairo_restore(cr);
}

/*
 * Draws an arrow for outgoing traffic. Arrows go uppwards and have
 * the same colour as the outgoing speed text 
 */
static void draw_arrow_out(cairo_t *cr,float angleNumber,int x1,int y1, int width)
{
	float angle = angleNumber * M_PI/180;
  
	int x = width * cos(angle);
	int y = width * sin(angle);

	cairo_save(cr);
	cairo_set_source_rgba (cr, 0.592, 1, 1, 1);
	cairo_set_line_width (cr, 10);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND); 

	//g_print("X is %d\n",x);
	//g_print("Y is %d\n",y);
  
  
	cairo_translate(cr,x1,y1);
	cairo_move_to(cr,-x,y);
	cairo_line_to(cr,0,0);
	cairo_line_to(cr,x,y);
	cairo_stroke(cr);

	cairo_restore(cr);
}

/*
 * Renders the content of the main window
 */
static void draw_now(GtkWidget *widget,app_state *as)
{
	cairo_t *cr;
	gchar *title = NULL;
	gint title_position = 0;
	GSList *current = NULL;
	int i;
	int y;
	gfloat angle_in = 15; //Min = 60 Degrees max = 15 Degrees
	gfloat width_in = 80; //Min = 20 max = 80
	gfloat angle_out = 15; //Min = 60 Degrees max = 15 Degrees
	gfloat width_out = 60; //Min = 20 max = 60
	int percent_in = 0;
	int percent_out = 0;

	cr = begin_paint (widget->window);

	/* Animation (if present) */
	title_position = (int)Integrator_get(as->title_integrator);
	percent_in = (int)Integrator_get(as->status_in);
	percent_out = (int)Integrator_get(as->status_out);

	angle_in = processing_map(percent_in,0,100,0,45);
	angle_in = 60 - angle_in;
	width_in = processing_map(percent_in,0,100,20,80);


	angle_out = processing_map(percent_out,0,100,0,45);
	angle_out = 60 - angle_out;
	width_out = processing_map(percent_in,0,100,20,60);


	/* Step 1 Arrows for incoming traffic */
	y = as->arrows_top_in;
	for(i = 0; i < 20; i++) //20 arrows are enough to fill the screen
	{
		draw_arrow_in(cr,angle_in,title_position-100,y,width_in);
		y += 30;
	}
	/* Step 2 Arrows for outgoing traffic */
	y = as->arrows_bottom_out;
	for(i = 0; i < 8; i++) //8 arrows are enough to fill the half screen
	{
		draw_arrow_out(cr,angle_out,title_position+70,y,width_out);
		y -= 30;
	}

	/* Step 3 decors and window title */
	title = g_strconcat("Network status: ",as->selected_interface,NULL);
	draw_decors(cr,title,title_position);
	g_free(title);

	/* Step 4 Speed text */
	draw_speed_text(cr,as->speed_in,as->speed_out,title_position);

	end_paint (cr);
}


/*
 * Callback that is run by GTK whenever there is an expose
 * event
 */
static gboolean win_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	app_state *closure = (app_state *)data;



	draw_now(widget,closure);

	return TRUE;
}

static void close_file(app_state *as)
{
	GError *error = NULL;

	g_io_channel_shutdown(as->proc_reader,FALSE,&error);
	g_io_channel_unref(as->proc_reader);

}

/*
 * Finds selected interface. Returns NULL if there is only 
 * one.
 */
static GList *locate_current_interface(app_state *as)
{
	
	guint total = g_list_length(as->all_interfaces);
	int i = 0;

	if(total == 1)
		return NULL;
	
	return g_list_find(as->all_interfaces,as->selected_interface);
}

/* 
 * Finds the next interface. It there is none
 * it wraps around to the beginning
 */
static void next_interface(app_state *as)
{
	GList *next = NULL;
	GList *now = locate_current_interface(as);
	if(now == NULL) //Only 1 interface nothing to do
		return;
	
	next = g_list_next(now);
	if(next == NULL) //We are at the end of the list
		next = g_list_first(as->all_interfaces);
	as->selected_interface = next->data;

	Integrator_set(as->title_integrator,400);
	//Also reset history 
	as->lastbytes_out = 0;
	as->lastbytes_in = 0;
	memset(&as->history_in[0], 0, sizeof(as->history_in));
	memset(&as->history_out[0], 0, sizeof(as->history_out));

}
/*
 * Finds the previous intercace. If there is none
 * it reverts to the end of the list.
 */
static void previous_interface(app_state *as)
{

	GList *prev = NULL;
	GList *now = locate_current_interface(as);
	if(now == NULL) //Only 1 interface nothing to do
		return;

	prev = g_list_previous(now);
	if(prev == NULL) //We are at the start of the list
		prev = g_list_last (as->all_interfaces);
	as->selected_interface = prev->data;

	Integrator_set(as->title_integrator,0);
	//Also reset history 
	as->lastbytes_out = 0;
	as->lastbytes_in = 0;
	memset(&as->history_in[0], 0, sizeof(as->history_in));
	memset(&as->history_out[0], 0, sizeof(as->history_out));
}
/*
 * Callback that is run when a key is pressed 
 */
static gint on_key_press (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
	app_state *closure = (app_state *)data;
	switch (event->keyval)
	{
		case GDK_Escape:
			close_file(closure);
			gtk_main_quit ();
			break;
		case GDK_Left:
			previous_interface(closure);
			break;
		case GDK_Right:
			next_interface(closure);
			break;
	}
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
	gtk_window_set_title (GTK_WINDOW (window), "Syndesis Network");

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
 * Animation thread. It advances all integrators and 
 * redraws the main window.
 */
static gint timeout_callback (gpointer data)
{
	app_state *closure = data;
	GSList *current = NULL;

	//g_print("Next\n");
	Integrator_update(closure->title_integrator);
	Integrator_update(closure->status_in);
	Integrator_update(closure->status_out);


	closure->arrows_top_in +=1;
	/* See if top arrows need to be recycled */
	if(closure->arrows_top_in == -10)
		closure->arrows_top_in = -40;

	closure->arrows_bottom_out -=1;
	/* See if bottom arrows need to be recycled */
	if(closure->arrows_bottom_out == 410)
		closure->arrows_bottom_out = 440;



	gtk_widget_queue_draw (closure->drawing_area);

	return TRUE;
}

/* Called every second. Finds the current throughput for the selected interface
 * as well as maximum speed for the last 30 seconds
 */
static void perform_calculations(app_state *as, gulong totalbytes_in, gulong totalbytes_out)
{
	//g_debug("we have %ld In and %ld OUT",totalbytes_in,totalbytes_out);
	gulong max_in = 0;
	gulong max_out = 0;
	gulong diffbytes_in = 0;
	gulong diffbytes_out = 0; 
	int i;
	
	/* Remove old text */
	g_free(as->speed_in);
	g_free(as->speed_out);

	gulong lastdiff_in = as->history_in[as->last_history_pos];
	gulong lastdiff_out = as->history_out[as->last_history_pos];

	if(as->lastbytes_in == 0)
	{
		//No history yet!
		as->speed_in = g_strdup("Please wait...");
		as->speed_out = g_strdup(" ");
	}
	else
	{
		diffbytes_in += totalbytes_in - as->lastbytes_in;
		diffbytes_out += totalbytes_out - as->lastbytes_out;
		as->speed_in = g_strconcat("In: ",human_readable(diffbytes_in),"b/s",NULL);
		as->speed_out = g_strconcat("Out: ",human_readable(diffbytes_out),"b/s",NULL);

	}

	/* Keep total bytes for the next time */
	as->lastbytes_in = totalbytes_in;
	as->lastbytes_out = totalbytes_out;

	/* Record this diff entry to the next position */
	as->last_history_pos += 1;
	if(as->last_history_pos == HISTORY_SIZE)
		as->last_history_pos = 0; //Start again in the history array

	as->history_in[as->last_history_pos] = diffbytes_in;
	as->history_out[as->last_history_pos] = diffbytes_out;
	


	/* Find maximum speed over the last HISTORY_SIZE seconds */
	for(i = 0; i < HISTORY_SIZE ; i++)
	{
		gulong current_in = as->history_in[i];
		gulong current_out = as->history_out[i];
		if(current_in > max_in) max_in = current_in;
		if(current_out > max_out) max_out = current_out;
	}
	//g_debug("In is %d out of %d meaning %d",diffbytes_in, max_in,processing_map(diffbytes_in,0,max_in,50,100));
	//g_debug("Out is %d out of %d meaning %d",diffbytes_out, max_out,processing_map(diffbytes_out,0,max_out,50,100));


	Integrator_target(as->status_in,processing_map(diffbytes_in,0,max_in,0,100));
	Integrator_target(as->status_out,processing_map(diffbytes_out,0,max_out,0,100));



}
/*
 * Monitoring thread. Runs every second. Update the speed information
 * on selected interface 
 */
static gint monitor_callback (gpointer data)
{
	app_state *as = data;
	GError *error = NULL;
	gchar *line;
	gsize tps = 0;
	GIOStatus result;
	gboolean found = FALSE;

	//Skip first two lines since they contain no usable information
	g_io_channel_read_line(as->proc_reader,&line,NULL,&tps,&error);
	g_free(line);
	g_io_channel_read_line(as->proc_reader,&line,NULL,&tps,&error);
	g_free(line);

	while(!found)
	{
		gchar *interface = NULL;
		gchar *ptr = NULL;

		result =  g_io_channel_read_line(as->proc_reader,&line,NULL,&tps,&error);

		if(result != G_IO_STATUS_NORMAL)
		{
			break;
		}		
		/* Format is >    etho0:45353454< */
		ptr = line;
		while(*ptr == ' ') ptr++;
		interface = ptr;
                while(*ptr != ':') ptr++;
                *ptr = '\0';
		if(!g_strcmp0(interface,as->selected_interface))
		{
			gulong totalbytes_in = strtoul(&line[7], NULL, 10);
			gulong totalbytes_out = strtoul(&line[67], NULL, 10);
			perform_calculations(as,totalbytes_in, totalbytes_out);
			found = TRUE;
		}
		g_free(line);
		
	}
	
	/* reset the file pointer back to the start of the file */
	g_io_channel_seek_position(as->proc_reader,0,G_SEEK_SET,&error);

	return TRUE; //Run all the time
}
/*
 * Finds the list of interfaces. Runs once during startup. Return TRUE if at least
 * one network interface was found. In any other case it return FALSE.
 */
static gboolean prepare_file(app_state *as)
{
	GError *error = NULL;
	gchar *line;
	gboolean valid_file = FALSE;
	gsize tps = 0;
	GIOStatus result;
	GSList *total_lines = NULL;
	GList *interfaces_detected = NULL;
	int i;

	/* See if this actually a file */
	valid_file = g_file_test(LINUX_NETWORK,G_FILE_TEST_IS_REGULAR);
	if(valid_file == FALSE)
	{
		return FALSE;
	}


	/* Now open it */
	as->proc_reader = g_io_channel_new_file(LINUX_NETWORK,"r",&error);
	if(as->proc_reader == NULL)
	{
		return FALSE;
	}

	while(TRUE)
	{
		gchar *interface = NULL;
		result =  g_io_channel_read_line(as->proc_reader,&line,NULL,&tps,&error);

		if(result != G_IO_STATUS_NORMAL)
		{

			break;
		}
		interface = g_strdup(line);
		g_free(line);
		total_lines = g_slist_prepend(total_lines,interface);
		
	}
	if(total_lines == NULL || g_slist_length(total_lines) < 3)
	{
		close_file(as);
		return FALSE;
	}

	/* Read all lines apart from the last 2 since they contain the information messages.
	 * Notice that since we used g_slist_prepend the list is reversed therefore
	 * we disregard the last two lines instead of the first two.
	 */
	for (i = 0; i< g_slist_length(total_lines) - 2 ; i++)
	{
		gchar *interface_name = NULL;
		gchar *interface_found = NULL;
		GSList *current = g_slist_nth (total_lines, i);
		/* Format is >    etho0:45353454< */
		gchar *ptr = current->data;
		while(*ptr == ' ') ptr++;
		interface_found = ptr;
                while(*ptr != ':') ptr++;
                *ptr = '\0';
		interface_name = g_strdup(interface_found);
		g_message("Detected Interface %s",interface_name);
		interfaces_detected = g_list_prepend(interfaces_detected,interface_name);

		g_free(current->data);
	}
	g_slist_free(total_lines);

	as->all_interfaces = interfaces_detected;
	//g_debug("Selecting default interface out of %d interfaces",g_list_length(as->all_interfaces));
	/* 
	 * At this point we have to select the interface that will be monitored by default
	 * We choose to show the Ethernet interface which in Linux is named eth0, eth1 e.t.c.
	 * If we do not find anything like that we just select the first interface
	 */
	as->selected_interface = (gchar *)interfaces_detected->data; //Default selection
	for (i = 0; i < g_list_length(interfaces_detected); i++)
	{
		GList *current = g_list_nth(interfaces_detected, i);
		if (g_str_has_prefix(current->data,DEFAULT_PREFIX))
		{
			as->selected_interface = (gchar *)current->data;
			break;
		}
	}
	//g_debug("Selected interface is %s",as->selected_interface);

	/* reset the file pointer back to the start of the file */
	g_io_channel_seek_position(as->proc_reader,0,G_SEEK_SET,&error);
	
	/* Everything is ok */
	return TRUE;

}


/*
 * Main function 
 */
int main (int argc, char *argv[])
{
	app_state closure;
	gboolean found_network = FALSE;

	gtk_init (&argc, &argv);


	/* Attempt to find network interfaces */
	found_network = prepare_file(&closure);

	if(found_network == FALSE)
	{
		g_printerr ("Invalid or empty %s. Are you running Linux?\n",LINUX_NETWORK);
		return 1;
	}


	/* Init stuff */
	closure.drawing_area = create_window (&closure);
	closure.title_integrator = Integrator_create();
	Integrator_set(closure.title_integrator,0);
	Integrator_target(closure.title_integrator,200); //These are pixels

	closure.status_in = Integrator_create();
	Integrator_set(closure.status_in,0);
	Integrator_target(closure.status_in,100); //This is percent

	closure.status_out = Integrator_create();
	Integrator_set(closure.status_out,0);
	Integrator_target(closure.status_out,100); //This is percent

	closure.speed_in = g_strdup("Please wait..");
	closure.speed_out = g_strdup(" ");
	memset(&closure.history_in[0], 0, sizeof(closure.history_in));
	memset(&closure.history_in[0], 0, sizeof(closure.history_in));
	closure.last_history_pos = 0;
	closure.lastbytes_in = 0;
	closure.lastbytes_out = 0;
	closure.arrows_top_in = -30; //30 pixels above the top of the screen
	closure.arrows_bottom_out = 430; //30 pixels below the bottom of the screen

	/* Show the GUI to the user */
	gtk_widget_show_all (gtk_widget_get_toplevel (closure.drawing_area));

	/* Create animation thread */
	g_timeout_add (40, timeout_callback, &closure);
	/* Create monitoring thread */
	g_timeout_add (1000, monitor_callback, &closure);


	gtk_main();

	return 0;
}
