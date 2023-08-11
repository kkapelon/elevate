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
#ifndef GFX_H
#define GFX_H

void draw_fancy_rec(cairo_t *cr,double x0,double y0,double rect_width,double rect_height,double radius);

void draw_rounded_rect(cairo_t *cr,double x,double y,double w,double h,double r);

void show_text_message (cairo_t * cr, int font_size, int pos_x,int pos_y, const char *message,double alpha);

void draw_input_box (cairo_t * cr, const char *command_text,double fx_percent);

void draw_messages_box (cairo_t * cr, const char *command_text,int characters_visible);

#endif
