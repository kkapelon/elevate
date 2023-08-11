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
#ifndef COMMON_H
#define COMMON_H

const gchar *find_basename(gchar *filename);

const gchar* human_readable (long n);

int processing_map(int value, int current_low, int current_high,int requested_low, int requested_high);


#endif
