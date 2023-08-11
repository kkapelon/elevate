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

/*
 * A physics integrator (See Visualizing Data book by Ben Fry)
 */

#include <gtk/gtk.h>
#include "integrator.h"

#define DAMPING 0.3
#define ATTRACTION 0.2f
#define DEFAULT_MASS 1

/* Default Constructor */
Integrator* Integrator_create(void)
{
	Integrator *integrator = g_new0(Integrator,1);

	integrator->targeting = FALSE;
	integrator->mass = DEFAULT_MASS;
	integrator->damping = DAMPING;
	integrator->attraction = ATTRACTION;


	return integrator;
}

/* Destructor */
void Integrator_free(Integrator *what)
{
	g_free(what);
}

void Integrator_set(Integrator *integrator,gfloat v)
{
	integrator->value = v;
}

gfloat Integrator_get(Integrator *integrator)
{
	return integrator->value;	
}

/* This where the magic happens */
void Integrator_update(Integrator *integrator)
{
	if (integrator->targeting) {
		integrator->force += integrator->attraction * (integrator->target - integrator->value);      
	}

	integrator->accel = integrator->force / integrator->mass;
	integrator->vel = (integrator->vel + integrator->accel) * integrator->damping;
	integrator->value += integrator->vel;

	integrator->force = 0;
}

/* Assign a new target */
void Integrator_target(Integrator *integrator, gfloat t)
{
	integrator->targeting = TRUE;
	integrator->target = t;
}

/* Disable integrator */
void Integrator_no_target(Integrator *integrator)
{
	integrator->targeting = FALSE;
}


