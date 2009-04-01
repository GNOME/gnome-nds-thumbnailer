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
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/* From specs at http://www.bottledlight.com/ds/index.php/FileFormats/NDSFormat
 * and code at http://www.kde-apps.org/content/show.php?content=39247 */

#define BOUND_ERROR(x) {											\
	if (error != NULL) {											\
		g_warning ("Couldn't access file data at 0x%x, probably not a NDS ROM: %s", x, error->message);	\
		g_error_free (error);										\
	} else													\
		g_warning ("Couldn't access file data at 0x%x, probably not a NDS ROM", x);			\
	if (stream != NULL)											\
		g_object_unref (stream);									\
	return 1;												\
}

#define LOGO_OFFSET_OFFSET 0x068
#define BANNER_LENGTH 2112
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
	GFile *input;
	const char *output;
	GFileInputStream *stream;

	guint32 logo_offset[4];
	char *banner_data;

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

	input = g_file_new_for_commandline_arg (filenames[0]);
	output = filenames[1];

	/* Open the file for reading */
	stream = g_file_read (input, NULL, &error);
	g_object_unref (input);

	if (stream == NULL) {
		g_warning ("Couldn't open '%s': %s", filenames[0], error->message);
		g_error_free (error);
		return 1;
	}

	/* Get the address of the logo */
	if (g_input_stream_skip (G_INPUT_STREAM (stream), LOGO_OFFSET_OFFSET, NULL, &error) == FALSE)
		BOUND_ERROR(LOGO_OFFSET_OFFSET);
	if (g_input_stream_read (G_INPUT_STREAM (stream), &logo_offset, sizeof(guint32), NULL, &error) == FALSE)
		BOUND_ERROR(LOGO_OFFSET_OFFSET);
	offset = GUINT32_FROM_LE(*logo_offset) - g_seekable_tell (G_SEEKABLE (stream));

	/* Get the icon data */
	if (g_input_stream_skip (G_INPUT_STREAM (stream), offset, NULL, &error) != offset)
		BOUND_ERROR(offset);
	banner_data = g_malloc0(BANNER_LENGTH);
	if (g_input_stream_read (G_INPUT_STREAM (stream), banner_data, BANNER_LENGTH, NULL, &error) != BANNER_LENGTH)
		BOUND_ERROR(LOGO_OFFSET_OFFSET);

	g_input_stream_close (G_INPUT_STREAM (stream), NULL, NULL);
	g_object_unref (stream);

	/* Check the version is version 1 or 3 */
	if ((banner_data[0] != 0x1 || banner_data[1] != 0x0) &&
	    (banner_data[0] != 0x3 || banner_data[1] != 0x0)) {
		g_free (banner_data);
		g_warning ("Unsupported icon version, probably not an NDS file");
		return 1;
	}

	/* Get the tile and palette data for the logo */
	tile_data = g_memdup (banner_data + TILE_DATA_OFFSET, TILE_DATA_LENGTH);
	palette_data = g_memdup (banner_data + PALETTE_DATA_OFFSET, PALETTE_DATA_LENGTH);
	g_free (banner_data);
	pixbuf = load_icon (tile_data, palette_data);
	g_free (palette_data);
	g_free (tile_data);

	scaled = gdk_pixbuf_scale_simple (pixbuf, output_size, output_size, GDK_INTERP_BILINEAR);
	g_object_unref (pixbuf);
	if (gdk_pixbuf_save (scaled, output, "png", &error, NULL) == FALSE) {
		g_warning ("Couldn't save the thumbnail '%s' for file '%s': %s", output, filenames[0], error->message);
		g_error_free (error);
		return 1;
	}

	g_object_unref (scaled);

	return 0;
}

