/* 
 * Copyright (C) 2007 Bastien Nocera <hadess@hadess.net>
 *
 * Authors: Bastien Nocera <hadess@hadess.net>
 * Thomas Köckerbauer <tkoecker@gmx.net>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <string.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/* From specs at http://www.bottledlight.com/ds/index.php/FileFormats/NDSFormat
 * and code at http://www.kde-apps.org/content/show.php?content=39247 */

#define CHECK_BOUND(x) {											\
	if (x >= g_mapped_file_get_length (map) || x < 0) {							\
		g_warning ("Couldn't access file data at 0x%x, probably not a NDS ROM", x);			\
		return 1;											\
	}													\
}

#define TILE_DATA_OFFSET 32
#define TILE_DATA_LENGTH 512
#define PALETTE_DATA_OFFSET TILE_DATA_OFFSET + 512
#define PALETTE_DATA_LENGTH 32

struct palette_i {
	guint8 r;
	guint8 g;
	guint8 b;
	guint8 a;
};

static void
put_pixel (guchar *pixels, int rowstride, int x, int y, struct palette_i item)
{
	int n_channels;
	guchar *p;

	n_channels = 4;

	p = pixels + y * rowstride + x * n_channels;
	p[0] = item.r;
	p[1] = item.g;
	p[2] = item.b;
	p[3] = item.a;
}

GdkPixbuf *
load_icon (gchar *tile_data, guint16 *palette_data)
{
	struct palette_i palette[16];
	int pos, j, i, y, x;

	GdkPixbuf *pixbuf;
	guchar *pixels;
	int rowstride;

	/* Parse the palette */
	for (i = 0; i < 16; i++) {
		palette[i].r = (palette_data[i] & 0x001F) << 3;
		palette[i].g = (palette_data[i] & 0x03E0) >> 2;
		palette[i].b = (palette_data[i] & 0x7C00) >> 7;
		palette[i].a = (i == 0) ? 0x00 : 0xFF;
	}

	/* Create the pixbuf */
	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 32, 32);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);

	/* Put the grid of icon data into the pixbuf */
	pos = 0;
	for (j = 0; j < 4; j++) {
		for (i = 0; i < 4; i++) {
			for (y = 0; y < 8; y++) {
				for (x = 0; x < 4; x++) {
					put_pixel (pixels, rowstride, x * 2 + 8 * i,  y + 8 * j, palette[tile_data[pos] & 0x0F]);
					put_pixel (pixels, rowstride, x * 2 + 1 + 8 * i, y + 8 * j, palette[(tile_data[pos] & 0xF0)>>4]);
					pos++;
				}
			}
		}
	}

	return pixbuf;
}

static int output_size = 64;
static gboolean g_fatal_warnings = FALSE;
static char **filenames = NULL;

static const GOptionEntry entries[] = {
	{ "size", 's', 0, G_OPTION_ARG_INT, &output_size, "Size of the thumbnail in pixels", NULL },
	{"g-fatal-warnings", 0, 0, G_OPTION_ARG_NONE, &g_fatal_warnings, "Make all warnings fatal", NULL},
 	{ G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, NULL, "[FILE...]" },
	{ NULL }
};

int main (int argc, char **argv)
{
	GMappedFile *map;
	guint32 offset;
	gchar *base;
	gchar *tile_data;
	guint16 *palette_data;

	GdkPixbuf *pixbuf, *scaled;
	GError *error = NULL;
	GOptionContext *context;
	char *input, *output;

	/* Options parsing */
	context = g_option_context_new ("Thumbnail Nintendo DS ROMs");
	g_option_context_add_main_entries (context, entries, NULL);
	g_type_init ();

	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
		g_warning ("Couldn't parse command-line options: %s", error->message);
		g_error_free (error);
		return 1;
	}

	/* Set fatal warnings if required */
	if (g_fatal_warnings) {
		GLogLevelFlags fatal_mask;

		fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
		fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
		g_log_set_always_fatal (fatal_mask);
	}

	if (filenames == NULL || g_strv_length (filenames) != 2) {
		g_print ("Expects an input and an output file\n");
		return 1;
	}
	input = filenames[0];
	output = filenames[1];

	map = g_mapped_file_new (input, FALSE, &error);
	if (!map) {
		g_warning ("Couldn't map %s: %s", input, error->message);
		g_error_free (error);
		return 1;
	}

	base = g_mapped_file_get_contents (map);

	/* Get the address of the logo */
	CHECK_BOUND(0x068);
	offset = GUINT32_FROM_LE(*((guint32 *) (base + 0x068)));
	CHECK_BOUND(offset);

	/* Check the version is version 1 */
	if (base[offset] != 0x1 || base[offset + 1] != 0x0) {
		g_warning ("Unsupported icon version, probably not an NDS file");
		return 1;
	}

	/* Get the tile and palette data for the logo */
	CHECK_BOUND((int) (offset + TILE_DATA_OFFSET + TILE_DATA_LENGTH));
	tile_data = g_memdup (base + offset + TILE_DATA_OFFSET, TILE_DATA_LENGTH);
	CHECK_BOUND((int) (offset + PALETTE_DATA_OFFSET + PALETTE_DATA_LENGTH));
	palette_data = g_memdup (base + offset + PALETTE_DATA_OFFSET, PALETTE_DATA_LENGTH);
	pixbuf = load_icon (tile_data, palette_data);
	g_free (palette_data);
	g_free (tile_data);

	g_mapped_file_free (map);

	scaled = gdk_pixbuf_scale_simple (pixbuf, output_size, output_size, 0);
	g_object_unref (pixbuf);
	if (gdk_pixbuf_save (scaled, output, "png", &error, NULL) == FALSE) {
		g_warning ("Couldn't save the thumbnail '%s' for file '%s': %s", output, input, error->message);
		g_error_free (error);
		return 1;
	}

	g_object_unref (scaled);

	return 0;
}

