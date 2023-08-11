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
#include <cairo.h>
#include <math.h>
#include <string.h>
#include <glib.h>

#include "gfx.h"

#define MAX_CMD 35

void draw_fancy_rec(cairo_t *cr,double x0,double y0,double rect_width,double rect_height,double radius)
{
	double x1,y1;

	x1=x0+rect_width;
	y1=y0+rect_height;
	if (!rect_width || !rect_height)
		return;
	if (rect_width/2<radius) {
		if (rect_height/2<radius) {
			cairo_move_to  (cr, x0, (y0 + y1)/2);
			cairo_curve_to (cr, x0 ,y0, x0, y0, (x0 + x1)/2, y0);
			cairo_curve_to (cr, x1, y0, x1, y0, x1, (y0 + y1)/2);
			cairo_curve_to (cr, x1, y1, x1, y1, (x1 + x0)/2, y1);
			cairo_curve_to (cr, x0, y1, x0, y1, x0, (y0 + y1)/2);
		} else {
			cairo_move_to  (cr, x0, y0 + radius);
			cairo_curve_to (cr, x0 ,y0, x0, y0, (x0 + x1)/2, y0);
			cairo_curve_to (cr, x1, y0, x1, y0, x1, y0 + radius);
			cairo_line_to (cr, x1 , y1 - radius);
			cairo_curve_to (cr, x1, y1, x1, y1, (x1 + x0)/2, y1);
			cairo_curve_to (cr, x0, y1, x0, y1, x0, y1- radius);
		}
	} else {
		if (rect_height/2<radius) {
			cairo_move_to  (cr, x0, (y0 + y1)/2);
			cairo_curve_to (cr, x0 , y0, x0 , y0, x0 + radius, y0);
			cairo_line_to (cr, x1 - radius, y0);
			cairo_curve_to (cr, x1, y0, x1, y0, x1, (y0 + y1)/2);
			cairo_curve_to (cr, x1, y1, x1, y1, x1 - radius, y1);
			cairo_line_to (cr, x0 + radius, y1);
			cairo_curve_to (cr, x0, y1, x0, y1, x0, (y0 + y1)/2);
		} else {
			cairo_move_to  (cr, x0, y0 + radius);
			cairo_curve_to (cr, x0 , y0, x0 , y0, x0 + radius, y0);
			cairo_line_to (cr, x1 - radius, y0);
			cairo_curve_to (cr, x1, y0, x1, y0, x1, y0 + radius);
			cairo_line_to (cr, x1 , y1 - radius);
			cairo_curve_to (cr, x1, y1, x1, y1, x1 - radius, y1);
			cairo_line_to (cr, x0 + radius, y1);
			cairo_curve_to (cr, x0, y1, x0, y1, x0, y1- radius);
		}
	}
	cairo_close_path (cr);

	cairo_set_source_rgb (cr, 0.5, 0.5, 1);
	cairo_fill_preserve (cr);
	cairo_set_source_rgba (cr, 0.5, 0, 0, 0.5);
	cairo_set_line_width (cr, 0.008);
	cairo_stroke (cr);
}

/*
 * Draws a rectange with arcs in the corners
 */
static void draw_rounded(cairo_t *cr,double x,double y,double w,double h,double margin)
{

	cairo_new_path(cr);
	cairo_set_line_width (cr, 0.8);

	cairo_translate(cr,x,y);
	cairo_arc (cr, margin, margin, margin, M_PI,3*M_PI/2);
	cairo_rel_line_to(cr,w-(2*margin),0);
	cairo_arc (cr, w - margin, margin, margin, 3*M_PI/2,0);
	cairo_rel_line_to(cr,0,h-(2*margin));
	cairo_arc (cr, w-margin, h-margin, margin, 0, M_PI/2);
	cairo_rel_line_to(cr,-(w-(2*margin)),0);
	cairo_arc (cr, margin, h-margin, margin, M_PI/2,M_PI);
	cairo_rel_line_to(cr,0,-(h-(2*margin)));
	cairo_close_path (cr);
}

/*
 * Draws a rectange with arcs in the corners (outline only)
 */
void draw_rounded_rect(cairo_t *cr,double x,double y,double w,double h,double margin)
{
	cairo_save(cr);

	draw_rounded(cr,x,y,w,h,margin);
	cairo_stroke(cr);


	cairo_restore(cr);
}

/*
 * Draws a rectange with arcs in the corners (filled)
 */
void draw_rounded_rect_filled(cairo_t *cr,double x,double y,double w,double h,double margin,double alpha)
{
	cairo_save(cr);

	cairo_new_path(cr);
	draw_rounded(cr,x,y,w,h,margin);
	cairo_fill_preserve(cr);
	cairo_set_source_rgba (cr, 0, 0, 0,alpha);
	cairo_set_line_width (cr, 0.4);
	cairo_stroke(cr);
	cairo_close_path (cr);

	cairo_restore(cr);
}
/*
 * Prints a text line centered on a specific point
 */
void show_text_message (cairo_t * cr, int font_size, int pos_x,int pos_y, const char *message,double alpha)
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

	cairo_set_source_rgba (cr, 0, 0, 0, alpha);
	cairo_move_to (cr, x, y);
	cairo_show_text (cr, message);
	cairo_restore (cr);
}

/* Show the command line box where users can type something */
void draw_input_box (cairo_t * cr, const char *command_text,double fx_percent)
{
	int length = strlen(command_text);
	cairo_save (cr);

	/* If we just scale then the text goes at the bottom right.
	 * Therefore we also need to translate in order to get it centered
	 */
	cairo_translate(cr, -50.0 * (1.0 - fx_percent), -50.0 * (1.0 -fx_percent));
	cairo_scale(cr,2.0 - fx_percent,2.0 - fx_percent);

	cairo_set_source_rgba (cr, 1, 0.647, 0,fx_percent);
	draw_rounded_rect_filled(cr,5,46,90,8,2,fx_percent);

	/* Trim text if it is bigger than the screen */
	if(length > MAX_CMD)
	{
		gchar *trimmed = NULL;
		command_text = command_text +(length - MAX_CMD + 6);  //3 because of ...
		trimmed = g_strconcat("...",command_text,NULL);
		show_text_message(cr,5,50,50,trimmed,fx_percent);
		g_free(trimmed);
	}
	else
	{
		show_text_message(cr,5,50,50,command_text,fx_percent);
	}

	cairo_restore(cr);

}

void draw_messages_box (cairo_t * cr, const char *command_text,int characters_visible)
{
	cairo_save (cr);

	cairo_set_source_rgba (cr, 0, 0, 1.0,0.2);
	draw_rounded_rect_filled(cr,5,8,90,80,2,0.2);

	show_text_message(cr,5,50,11,command_text,1.0);

	cairo_restore(cr);

}

