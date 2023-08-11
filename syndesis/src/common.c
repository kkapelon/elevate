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

#include <glib.h>

#include "common.h"

const gchar *find_basename(gchar *filename)
{
	gchar *slash_position = NULL;
	const gchar *basename = NULL;
	/*
	 *  There might be better ways to do this. But for now we
	 *  just get the part of the string that comes after the last
	 *  slash as the filename. We also need to sanitise the file name.
	 */
	slash_position = g_strrstr(filename,"/");
	if(slash_position == NULL)
		basename = filename;
	else
		basename = slash_position + 1; /* Since slash position includes the slash character as well */

	return basename;
}

/* N, a byte quantity, is converted to a human-readable abberviated
   form a la sizes printed by `ls -lh'.  The result is written to a
   static buffer, a pointer to which is returned.

   Unlike `with_thousand_seps', this approximates to the nearest unit.
   Quoting GNU libit: "Most people visually process strings of 3-4
   digits effectively, but longer strings of digits are more prone to
   misinterpretation.  Hence, converting to an abbreviated form
   usually improves readability."

   This intentionally uses kilobyte (KB), megabyte (MB), etc. in their
   original computer science meaning of "powers of 1024".  Powers of
   1000 would be useless since Wget already displays sizes with
   thousand separators.  We don't use the "*bibyte" names invented in
   1998, and seldom used in practice.  Wikipedia's entry on kilobyte
   discusses this in some detail.  */

const gchar* human_readable (long n)
{
  /* These suffixes are compatible with those of GNU `ls -lh'. */
  static char powers[] =
    {
      'K',			/* kilobyte, 2^10 bytes */
      'M',			/* megabyte, 2^20 bytes */
      'G',			/* gigabyte, 2^30 bytes */
      'T',			/* terabyte, 2^40 bytes */
      'P',			/* petabyte, 2^50 bytes */
      'E',			/* exabyte,  2^60 bytes */
    };
  static gchar buf[8];
  int i;

  /* If the quantity is smaller than 1K, just print it. */
  if (n < 1024)
    {
      g_snprintf (buf, sizeof (buf), "%d", (int) n);
      return buf;
    }

  /* Loop over powers, dividing N with 1024 in each iteration.  This
     works unchanged for all sizes of wgint, while still avoiding
     non-portable `long double' arithmetic.  */
  for (i = 0; i < 6 ; i++)
    {
      /* At each iteration N is greater than the *subsequent* power.
	 That way N/1024.0 produces a decimal number in the units of
	 *this* power.  */
      if ((n >> 10) < 1024 || i == 5)
	{
	  double val = n / 1024.0;
	  /* Print values smaller than 10 with one decimal digits, and
	     others without any decimals.  */
	  g_snprintf (buf, sizeof (buf), "%.*f%c",
		    val < 10 ? 1 : 0, val, powers[i]);
	  return buf;
	}
      n >>= 10;
    }
  return NULL;			/* unreached */
}

/*
 * Converts a value from a given ranget to another 
 * range. Taken from Processing.org
 */
int processing_map(int value, int istart,int istop,int ostart,int ostop)
{
	if(istop <= 0 || value < 0 || ostop <= 0) return 0;
	//g_print("Val is %d, current = %d, requested = %d\n",value,istop, ostop);
	return ostart + (((ostop - ostart) * (value - istart)) / (istop - istart));
}

