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
 * Header for the integrator (See Visualizing Data book by Ben Fry)
 */

#ifndef INTEGRATOR_H
#define INTEGRATOR_H

typedef struct physics_integrator
{
	gfloat value;
	gfloat vel;
	gfloat accel;
	gfloat force;
	gfloat mass;

	gfloat damping;
	gfloat attraction;
	gboolean targeting;
	gfloat target;
}Integrator;

/* Constructor */
Integrator* Integrator_create(void);

/* Destructor */
void Integrator_free(Integrator *what);

/* Use after constructor */
void Integrator_set(Integrator *integrator,gfloat v);

/* Convenience method instead of accessing the public field */
gfloat Integrator_get(Integrator *integrator);

/* Must be called before each frame */
void Integrator_update(Integrator *integrator);

/* Assign a new target */
void Integrator_target(Integrator *integrator, gfloat t);

/* Disable integrator */
void Integrator_no_target(Integrator *integrator);




#endif
