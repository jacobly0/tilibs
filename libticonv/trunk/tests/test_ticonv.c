/* Hey EMACS -*- linux-c -*- */
/* $Id: test_tifiles_2.c 1798 2006-02-03 08:27:54Z roms $ */

/*  libtifiles - Ti File Format library, a part of the TiLP project
 *  Copyright (C) 1999-2004  Romain Lievin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
  This program contains a lot of routines for testing various part of the
  library as well as checking file support.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <glib.h>
#ifdef __WIN32__
#include <conio.h>
#endif

#ifndef _DEBUG
# define _DEBUG
#endif
#include "../src/ticonv.h"

const unsigned long* charsets[8];

/*
  The main function
*/
int main(int argc, char **argv)
{
	int i, j;
	int n = 0;
	int is_utf8 = g_get_charset(NULL);

	charsets[0] = ti73_charset;
	charsets[1] = ti82_charset;
	charsets[2] = ti83_charset;
	charsets[3] = ti83p_charset;
	charsets[4] = ti85_charset;
	charsets[5] = ti86_charset;
	charsets[6] = ti9x_charset;

	// test ticonv.c
	printf("Library version : <%s>\n", ticonv_version_get());
	printf("--\n");

	printf("Choose your charset: ");
	if(!scanf("%i", &n))
	    n = 0;
	if(n >= 7)
	    n = 6;

	printf("  0 1 2 3 4 5 6 7 8 9 A B C D E F\n");

	for(i = 0; i < 16; i++)
	{
		printf("%x ", i);
		for(j = 0; j < 16; j++)
		{
		    unsigned long wc = charsets[n][16*i+j];
		    gchar *str = NULL;

		    if(wc && wc != '\n')
		    {
				gunichar2 buf[4] = { 0 };

				buf[0] = (gunichar2)wc;
				str = ticonv_utf16_to_utf8(buf);

				if(!is_utf8 && str)
				{
					gchar *tmp = g_locale_from_utf8(str, -1, NULL, NULL, NULL);
					g_free(str);
					str = tmp;
				}
		    }
		    else
		    {
				str = NULL;
		    }

		    if(str)
				printf("%s ", str);
		    else
				printf("");
		    
		    g_free(str);
		}
		printf("\n");
	}

	{
		char ti82_varname[9] = { 0 };
		char ti92_varname[9] = { 0 };
		char *utf8;
		char *filename;

		ti82_varname[0] = 0x5d;
		ti82_varname[1] = 0x01;
		ti92_varname[0] = (char)132;	//delta
		ti92_varname[1] = (char)'�';

		// TI -> UTF-8
		utf8 = ticonv_varname_to_utf8(CALC_TI82, ti82_varname, -1);
		g_free(utf8);

		// TI -> filename
		filename = ticonv_varname_to_filename(CALC_TI92, ti92_varname);
		printf("filename: <%s>\n", filename);
		g_free(filename);

		filename = ticonv_varname_to_filename(CALC_TI82, ti82_varname);
		printf("filename: <%s>\n", filename);
		g_free(filename);
	}

#ifdef __WIN32__
	while(!_kbhit());
#endif

	return 0;
}