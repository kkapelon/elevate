/*
 * Copyright (c) 2008 Kapelonis Kostis  <kkapelon@freemail.gr>
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

#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <gdk/gdkx.h>
#include <math.h>
#include <curl/curl.h>
#include <curl/types.h> /* new for v7 */
#include <curl/easy.h> /* new for v7 */

#define HEIGHT 400
#define WIDTH 400

#include "common.h"

struct application_state {
	GtkWidget *drawing_area; //Canvas
	gint i; //Frame number
	gint progress; //1 -100
	gchar *filename; /* File downloaded (basename of URL */
	gdouble alpha; /* Transparency of core */
	gchar *url; /* Full URL of the file downloaded */
	gdouble total; /* Total bytes */
	gdouble downloaded; /* Bytes retrieved so far */
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

	cairo_select_font_face (cr, "Serif",
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
static void draw_decors(cairo_t *cr,const gchar* file)
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

	show_text_message(cr,12,200,30,file);

	cairo_restore(cr);
}

/*
 * Renders the central circle that 
 * contains the percent of the download
 */
static void draw_core(cairo_t *cr,app_state *as)
{
	gdouble current_alpha;
	gchar *text = g_strdup_printf("%d%%",as->progress);
	gchar *d = g_strdup(human_readable(as->downloaded));
	gchar *t = g_strdup(human_readable(as->total));
	gchar *status = g_strdup_printf("%s (Total %s)",d,t);

	cairo_save(cr);

	//
	//First the core
	cairo_new_path(cr);
	/* Advance the alpha otherwise keep it maximum */
	if(as->progress == 100)
	{
		current_alpha = 1.0;
	}
	else
	{
		current_alpha = as->alpha + 0.04;
		if(current_alpha > 1.0) current_alpha = 0.0;
	}
	as->alpha = current_alpha;
	cairo_arc (cr, 200, 200, 30, 0, 2*M_PI);
	cairo_set_source_rgba (cr, 0, 0, 1, current_alpha);
	cairo_fill (cr);

	//The Text on the top which shows the name of the file
	cairo_set_source_rgb (cr, 0, 0, 1);
	show_text_message(cr,16,200,200,text);
	g_free(text);
	g_free(d);
	g_free(t);

	//And finally the text on the bottom that shows size
	show_text_message(cr,16,200,380,status);
	g_free(status);

	cairo_restore(cr);
}

/*
 * Draws the ring the surrounds the core
 */
static void draw_speed(cairo_t *cr,int now,app_state *as)
{
	double angle1 = now * (M_PI/180.0); 
	double angle2 = (300 +now) * (M_PI/180.0);  /* in radians           */
	cairo_save(cr);

	//First the core
	cairo_new_path(cr);
	cairo_set_line_width (cr, 5.0);
	cairo_arc (cr, 200, 200, 40, angle1, angle2);
	cairo_set_source_rgb (cr, 0, 0.3, 0.2);
	cairo_stroke (cr);

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
	cairo_save(cr);

	//
	//First the core
	cairo_new_path(cr);
	cairo_set_line_width (cr, 2.0);
	cairo_arc (cr, xc, yc, radius, angle1, angle2);
	cairo_set_source_rgb (cr, 0.7, 0.3, 0.2);

	cairo_arc_negative (cr, xc, yc, radius +width, angle2, angle1);
	cairo_close_path(cr);
	cairo_stroke (cr);

	cairo_restore(cr);
}

/*
 * Draws the beam that extends from the core
 * to the file chunks
 */
static void draw_transfer(cairo_t *cr,int now,app_state *as)
{
	double how = now %100;
	double angle1 = (now +310) * (M_PI/180.0); 
	double angle2 = (now +350 ) * (M_PI/180.0);  /* in radians           */

	draw_slice (cr, 200, 200, 30, angle1, angle2,110-how);
}

/*
 * Draws the file chunks that go towards the center
 * as the file is downloaded
 */
static void draw_chunks(cairo_t *cr,int now,app_state *as)
{
	double space = (as->progress) * 7.0 /100.0;
	int i=0;

	/* If the file is downloaded show a full circle */
	if(as->progress == 100)
	{

		draw_slice (cr, 200, 200, 145 -now, 0, 2*M_PI,15);
		return;
	}

	for(i = 0; i< 12;i++)
	{
		double angle1 = (i*30 -now) * (M_PI/180.0); 
		double angle2 = (i*30 -now +20 +space) * (M_PI/180.0);  /* in radians           */
		draw_slice (cr, 200, 200, 145 -now, angle1, angle2,15);
	}
}

/*
 * Renders the content of the main window
 */
static void draw_example(GtkWidget *widget,int now,app_state *as)
{
	cairo_t *cr;

	cr = begin_paint (widget->window);

	draw_decors(cr,as->filename);
	draw_core(cr,as);

	if(now < 100)
	{
		draw_speed(cr,now,as);
		draw_transfer(cr,now,as);
	}
	draw_chunks(cr,now,as);

	cairo_new_path(cr);
	end_paint (cr);
}


/*
 * Callback that is run by GTK whenever there is an expose
 * event
 */
static gboolean win_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	static int now = 0;
	app_state *closure = (app_state *)data;
	now++;

	gdk_threads_enter();
	draw_example(widget,closure->progress,closure);
	gdk_threads_leave();

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
	gtk_window_set_title (GTK_WINDOW (window), "Syndesis Download");

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

	gtk_widget_queue_draw (closure->drawing_area);

	return TRUE; //Keep animation running
}

/*
 * Callback by curl that saves the file to the disk
 */
size_t my_write_func(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  return fwrite(ptr, size, nmemb, stream);
}

/*
 * Callback by curl that reads the file from the network
 */
size_t my_read_func(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  return fread(ptr, size, nmemb, stream);
}

/*
 * Callback that is run automatically by
 * curl whenever progress is made
 */
int my_progress_func(void *client_data,
                     double t, /* dltotal */
                     double d, /* dlnow */
                     double ultotal,
                     double ulnow)
{
  int percent = d*100/t;
  app_state *as = (app_state *)client_data;
  //g_print("%lf / %lf (%d %%)\n", d, t, percent);

  gdk_threads_enter();
  as->progress = percent;
  as->total = t;
  as->downloaded = d;
  gdk_threads_leave();
  return 0;
}


/* 
 * Download thread. Saves the file locally
 * using the curl library
 */
void *my_thread(void *ptr)
{
  CURL *curl;
  CURLcode res;
  FILE *outfile;
  app_state *as = (app_state *)ptr;
  gchar *url = as->url;

  curl = curl_easy_init();
  if(curl)
  {
    outfile = fopen(as->filename, "w");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1); //Act like a browser
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, outfile);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_write_func);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, my_read_func);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, my_progress_func);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, as);
    

    res = curl_easy_perform(curl);

    fclose(outfile);

    if(res != CURLE_OK)
    {
    	g_remove(as->filename);
	as->progress = 100;
	/* Print message to the user */
    	g_free(as->filename);
	as->filename = g_strdup(curl_easy_strerror(res));
    }

    /* always cleanup */
    curl_easy_cleanup(curl);
  }

  return NULL;
}

/*
 * Main function reads the URL that is passed as an argument
 * and creates the main GUI window
 */
int main (int argc, char *argv[])
{
	app_state closure;

	/* Must initialize libcurl before any threads are started */
	curl_global_init(CURL_GLOBAL_ALL);

	/* Init thread */
	g_thread_init(NULL);

	gtk_init (&argc, &argv);


	if(argc > 1)
	{
		closure.filename = g_strdup(find_basename(argv[1]));
		closure.url = g_strdup(argv[1]);
	}
	else
	{
		g_print("Missing argument. Expecting a valid URL\n");
		return 0;
	}


	closure.drawing_area = create_window (&closure);
	closure.i = 0;
	closure.progress = 0;
	closure.alpha = 0.0;

	/* Show the GUI to the user */
	gtk_widget_show_all (gtk_widget_get_toplevel (closure.drawing_area));

	/* Create animation thread */
	g_timeout_add (100, timeout_callback, &closure);

	/* Create download thread */
	if (!g_thread_create(&my_thread, &closure, FALSE, NULL) != 0)
		g_warning("can't create the thread");

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	return 0;
}
