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

#include <string.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define HEIGHT 400
#define WIDTH 400

#define LINES 30

#define FONT_SIZE 9
#define LEFT_MARGIN 11
#define VERTICAL_SPACING 14
#define MAX_LINE_LENGTH 78

#include "common.h"

struct text_line {
	gint y; //vertical position
	gchar* text; //text for this line;
	gboolean valid; //Text contains actual content or not
	gboolean wrap; //If this was part of a multiline
};

struct application_state {
	GtkWidget *drawing_area; //Canvas
	gchar *filename; /* File downloaded (basename of URL */
	gchar *file_path; /* Absolute path of the file */
	GIOChannel *file_reader; /* Text file that will be shown*/
	struct text_line text_lines[LINES];
	gint top_line; /* What is the "first" line shown */
	gboolean finished;
	gboolean paused;
	gchar *multiline; /* Temporary buffer in case of long lines */
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
 * Prints a text line aligned at the start of the canvas
 */
static void show_text_line (cairo_t * cr, int font_size,  struct text_line line)
{
	double x, y;
	cairo_text_extents_t extents;

	cairo_save (cr);

	//Draw a small caret if this is part of a new line
	if(line.wrap == TRUE)
	{
		cairo_set_source_rgba (cr, 1, 1, 0, 1);
		cairo_set_line_width (cr, 1);
		cairo_move_to (cr, LEFT_MARGIN-7, line.y -7); //Vertical line
		cairo_line_to (cr, LEFT_MARGIN-7, line.y -2); //Horizonal line
		cairo_line_to (cr, LEFT_MARGIN-2, line.y -2); //Arrowhead position
		cairo_move_to (cr, LEFT_MARGIN-5, line.y -5); //Upper part of the arrow
		cairo_line_to (cr, LEFT_MARGIN-2, line.y -2); //Arrowhead position
		cairo_move_to (cr, LEFT_MARGIN-5, line.y +1); //Lower part of the arrow
		cairo_line_to (cr, LEFT_MARGIN-2, line.y -2); //Arrowhead position
		cairo_stroke(cr);
	}

	//Now draw the line itself
	cairo_select_font_face (cr, "Consolas",
			CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

	cairo_set_font_size (cr, font_size);

	cairo_set_source_rgb (cr, 1, 0.498, 0.141);
	cairo_move_to (cr, LEFT_MARGIN, line.y);
	cairo_show_text (cr, line.text);
	cairo_restore (cr);
}


/*
 * Draws the fancy corners of the frame
 */
static void draw_decors(cairo_t *cr,const gchar* file)
{
	cairo_save(cr);

	cairo_set_source_rgba (cr, 0, 0, 1,0.75);
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
 * Renders the content of the main window
 */
static void draw_example(GtkWidget *widget,app_state *as)
{
	cairo_t *cr;
	int i = 0;

	cr = begin_paint (widget->window);



	for(i = 0; i< LINES;i++)
	{
		int position = (i + as->top_line) % LINES;
		if(as->text_lines[position].valid == FALSE) continue;
		show_text_line(cr,FONT_SIZE, as->text_lines[position]);

	}
	draw_decors(cr,as->filename);

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

static void close_file(app_state *as)
{
	GError *error = NULL;

	if(as->finished == TRUE) return; //File already closed
	g_io_channel_shutdown(as->file_reader,FALSE,&error);
	g_io_channel_unref(as->file_reader);
	as->finished = TRUE;

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
		case GDK_space:
			closure->paused = !closure->paused;
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
	gtk_window_set_title (GTK_WINDOW (window), "Syndesis View");

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
 * Replace non-printable characters of the file with spaces
 */
static void normalize_text(gchar *input, gsize length)
{
	int i = 0;

	for(i = 0; i < length; i++)
	{
		if(!g_ascii_isprint(input[i])) input[i] = ' ';
	}
}

/*
 * Reads the next 80 characters from the multiline buffer
 * if there are none it returns NULL;
 */
static gchar *rest_of_line(app_state *as)
{
	gchar *result = NULL;

	if(as->multiline == NULL) return NULL;

	if(strlen(as->multiline) < MAX_LINE_LENGTH)
	{
		//g_message("Multiline buffer is finished");
		result = as->multiline;
		as->multiline = NULL;
		return result;
	}
	//g_message("Multiline buffer is long itself");
	//Since we are here the buffer itself is a multiline
	//Return only the first 80 chars this time
	result = g_malloc0(MAX_LINE_LENGTH +1);
	g_strlcpy(result,as->multiline, MAX_LINE_LENGTH +1);
	//g_message("Passing back only :%s",result);
	//For the next time skip these chars
	g_strlcpy(as->multiline,as->multiline + MAX_LINE_LENGTH ,strlen(as->multiline) - MAX_LINE_LENGTH +1);
	//g_message("Rest is :%s",as->multiline);

	return result;

}

/*
 * Reads the next line either from the file or from
 * the multiline buffer in case a line is more than 
 * 80 chars long
 */
static gchar *read_next_line(app_state *as)
{
	gchar *line;
	GError *error = NULL;
	gsize tps = 0;

	/*
	 * Before reading the file check the multiline buffer.
	 * Maybe the last line read was multi-line and it still
	 * contains some text
	 */
	if(as->multiline !=NULL)
	{
		line = rest_of_line(as);
	}
	else
	{
		GIOStatus result =  g_io_channel_read_line(as->file_reader,&line,NULL,&tps,&error);

		if(result != G_IO_STATUS_NORMAL)
		{

			close_file(as);
			return NULL;
		}
		normalize_text(line,tps);
		line[tps] = '\0'; //Remove new line character
		// See if this a multiline 
		if(strlen(line) > MAX_LINE_LENGTH )
		{
			//g_message("Long line %s",line);
			//Store it in the multibuffer
			as->multiline = g_strdup(line);
			line = rest_of_line(as);
			//g_message("Passing back only %s",line);
			//g_message("Rest is %s",as->multiline);
		}
	}
	return line;
}

/* 
 * Read the file dynamically to the lines shown on screen
 */
static void fill_lines(app_state *as)
{
	int i = 0;
	int y = 300; //Text will start scrolling from here
	gchar *line;
	GError *error = NULL;
	gsize tps = 0;

	gboolean previous_line_exists = FALSE;

	if(as->text_lines[0].valid == TRUE) return; //Nothing to do

	for(i = 0; i <LINES; i++)
	{
		//Peak ahead to see if this line is part of a multiline 
		if(as->multiline != NULL) previous_line_exists = TRUE;
		line = read_next_line(as);
		if(line == NULL) break;

		as->text_lines[i].text = line;
		as->text_lines[i].y = y;
		as->text_lines[i].valid = TRUE;
		as->text_lines[i].wrap = previous_line_exists;
		y = y +VERTICAL_SPACING;
	}

	as->top_line = 0;

}


/*
 * Reads the next line from the file 
 * and places it at the bottom
 */
static void render_next_line(app_state *as)
{
	gchar *line;
	GError *error = NULL;
	gsize tps = 0;
	gint next_line = as->top_line +1;
	gint previous_line = as->top_line -1;

	gboolean previous_line_exists = FALSE;

	//g_message("Loading line %d (prev %d, next %d)",as->top_line,previous_line, next_line);
	if(next_line == LINES) next_line = 0;
	if(previous_line == -1) previous_line = LINES - 1;

	//Peak ahead to see if this line is part of a multiline 
	if(as->multiline != NULL) previous_line_exists = TRUE;
	line = read_next_line(as);
	if(line == NULL) return;

	g_free(as->text_lines[as->top_line].text);
	as->text_lines[as->top_line].text = line;
	as->text_lines[as->top_line].y = as->text_lines[previous_line].y + VERTICAL_SPACING;
	as->text_lines[as->top_line].wrap = previous_line_exists;
	as->top_line = next_line;
}


/* 
 * When the first line at the top exits the screen, then read the next
 * line from the file and show it at the bottom
 */
static void update_text_shown(app_state *as)
{
	int i =0;

	fill_lines(as);

	/* Step 1 move all the lines upwards */
	for(i = 0; i <LINES; i++)
	{
		as->text_lines[i].y = as->text_lines[i].y -1;
	}

  /* Step 2 If the top line is no longer visible then read more lines from the file */
  //g_print("Top line is at %d\n",as->text_lines[as->top_line].y);
  if(as->text_lines[as->top_line].y < -VERTICAL_SPACING)
  	render_next_line(as);

}

/*
 * Animation thread. It just
 * redraws the main window
 */
static gint timeout_callback (gpointer data)
{
	app_state *closure = data;

	if(closure->paused == TRUE) return TRUE;

	update_text_shown(closure);
	gtk_widget_queue_draw (closure->drawing_area);

	if(closure->finished == FALSE)
		return TRUE;
	else
		return FALSE; /* animation is no longer needed */
}

/*
 * Opens the file for writing
 */
static gboolean prepare_file(app_state *as)
{
	GError *error = NULL;
	gchar *line;
	gboolean valid_file = FALSE;


	/* If no argument was given then there is nothing to do */
	if(as->finished == TRUE) return FALSE;

	/* See if this actually a file */
	valid_file = g_file_test(as->file_path,G_FILE_TEST_IS_REGULAR);
	if(valid_file == FALSE)
	{

		g_free(as->filename);
		as->filename = g_strdup("Invalid file");
		return FALSE;
	}


	/* Now open it */
	as->file_reader = g_io_channel_new_file(as->file_path,"r",&error);
	if(as->file_reader == NULL)
	{
		g_free(as->filename);
		as->filename = g_strdup(error->message);
		return FALSE;
	}
	//g_message("Outside of loop");
	//g_io_channel_shutdown(as->file_reader,FALSE,&error);
	//g_io_channel_unref(as->file_reader);
	//g_message("closing file");


	/* Everything is ok */
	return TRUE;

}

/*
 * Main function reads the URL that is passed as an argument
 * and creates the main GUI window
 */
int main (int argc, char *argv[])
{
	app_state closure;
	gboolean valid_file = FALSE;
	int i = 0;

	gtk_init (&argc, &argv);


	if(argc > 1)
	{
		closure.filename = g_strdup(find_basename(argv[1]));
		closure.file_path = g_strdup(argv[1]);
		closure.finished = FALSE;
	}
	else
	{
		closure.filename = g_strdup("No file chosen");
		closure.finished = TRUE;
	}


	/* Clear text lines */
	for(i = 0; i <LINES; i++)
	{
		closure.text_lines[i].y = 0;
		closure.text_lines[i].valid = FALSE;
		closure.text_lines[i].text = NULL;
	}
	closure.top_line = 0;
	closure.paused = FALSE;
	closure.multiline = NULL;

	/* Open text file */
	valid_file = prepare_file(&closure);

	closure.drawing_area = create_window (&closure);


	/* Show the GUI to the user */
	gtk_widget_show_all (gtk_widget_get_toplevel (closure.drawing_area));
	




	/* Create animation thread if everything is ok*/
	if(valid_file == TRUE)
	{
		g_timeout_add (40, timeout_callback, &closure);
	}

	gtk_main();

	return 0;
}

