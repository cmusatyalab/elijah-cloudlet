/*
 * dirtometer - Shows present and dirty chunks in a parcel's local cache
 *
 * Copyright (C) 2008-2011 Carnegie Mellon University
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.  A copy of the GNU General Public License
 * should have been distributed along with this program in the file
 * LICENSE.GPL.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>

struct pane {
	const char *config_key;
	const char *frame_label;
	const char *menu_label;
	gboolean initial;
	gboolean resizable;
	unsigned accel;
	GtkWidget *widget;
	GtkWidget *checkbox;
} panes[] = {
	{"show_stats",	"Statistics",	"Show statistics",	TRUE,	FALSE,	GDK_s},
	{"show_bitmap",	"Chunk bitmap",	"Show chunk bitmap",	TRUE,	TRUE,	GDK_c},
	{NULL}
};

char *states;
int numchunks;
gboolean mapped;

/** Utility ******************************************************************/

const char *statsdir;

#define max(a, b) ((a) > (b) ? (a) : (b))

void die(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "dirtometer: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(1);
}

char *get_attr(const char *attr)
{
	char *path;
	char *data;
	gboolean ok;

	path = g_strdup_printf("%s/%s", statsdir, attr);
	ok = g_file_get_contents(path, &data, NULL, NULL);
	g_free(path);
	if (!ok)
		return NULL;
	g_strchomp(data);
	return data;
}

void update_label(GtkLabel *lbl, const char *val)
{
	if (strcmp(gtk_label_get_label(lbl), val))
		gtk_label_set_label(lbl, val);
}

int set_signal_handler(int sig, void (*handler)(int sig))
{
	struct sigaction sa = {};
	sa.sa_handler = handler;
	sa.sa_flags = SA_RESTART;
	return sigaction(sig, &sa, NULL);
}

/** Config file **************************************************************/

#define CONFIG_GROUP "dirtometer"

const char *isrdir;
const char *confdir;
const char *conffile;
GKeyFile *config;

void read_config(void)
{
	GError *err = NULL;
	struct pane *pane;

	config = g_key_file_new();
	g_key_file_load_from_file(config, conffile, 0, NULL);

	g_key_file_get_integer(config, CONFIG_GROUP, "width", &err);
	if (err) {
		g_clear_error(&err);
		g_key_file_set_integer(config, CONFIG_GROUP, "width", 0);
	}

	g_key_file_get_integer(config, CONFIG_GROUP, "height", &err);
	if (err) {
		g_clear_error(&err);
		g_key_file_set_integer(config, CONFIG_GROUP, "height", 0);
	}

	g_key_file_get_boolean(config, CONFIG_GROUP, "keep_above", &err);
	if (err) {
		g_clear_error(&err);
		g_key_file_set_boolean(config, CONFIG_GROUP, "keep_above",
					TRUE);
	}

	g_key_file_get_boolean(config, CONFIG_GROUP, "show_image_cache", &err);
	if (err) {
		g_clear_error(&err);
		g_key_file_set_boolean(config, CONFIG_GROUP,
					"show_image_cache", FALSE);
	}

	for (pane = panes; pane->config_key != NULL; pane++) {
		g_key_file_get_boolean(config, CONFIG_GROUP, pane->config_key,
					&err);
		if (err) {
			g_clear_error(&err);
			g_key_file_set_boolean(config, CONFIG_GROUP,
					pane->config_key, pane->initial);
		}
	}
}

void write_config(void)
{
	GError *err = NULL;
	char *contents;
	gsize length;

	/* XXX: we should respect the isrdir config setting */
	if (!g_file_test(isrdir, G_FILE_TEST_IS_DIR) &&
				mkdir(isrdir, 0700)) {
		fprintf(stderr, "Couldn't create directory %s\n", isrdir);
		return;
	}
	if (!g_file_test(confdir, G_FILE_TEST_IS_DIR) &&
				mkdir(confdir, 0777)) {
		fprintf(stderr, "Couldn't create directory %s\n", confdir);
		return;
	}
	contents = g_key_file_to_data(config, &length, &err);
	if (err) {
		fprintf(stderr, "Couldn't write config file: %s\n",
					err->message);
		g_clear_error(&err);
		return;
	}
	if (!g_file_set_contents(conffile, contents, length, &err))
		fprintf(stderr, "Couldn't write config file: %s\n",
					err->message);
	g_clear_error(&err);
	g_free(contents);
}

/** Statistics pane **********************************************************/

#define NINPUTS 3
#define NOUTPUTS 2

struct stat_values {
	long long i[NINPUTS];
	double f;
};

struct stat_output {
	const char *tooltip;
	char *(*format)(struct stat_values *values, int which);
	gboolean (*changed)(struct stat_values *prev, struct stat_values *cur,
				int which);
	GtkWidget *ebox;
	GtkWidget *label;
};

struct stats {
	const char *heading;
	const char *attrs[NINPUTS];
	gboolean (*fetch)(struct stats *);
	struct stat_output output[NOUTPUTS];
	struct stat_values cur;
	struct stat_values prev;
};

int chunks_per_mb;

gboolean get_ints(struct stats *st)
{
	char *data;
	char *end;
	int i;
	gboolean success = TRUE;

	for (i = 0; i < NINPUTS; i++) {
		if (st->attrs[i] == NULL)
			continue;
		data = get_attr(st->attrs[i]);
		if (data == NULL)
			return FALSE;
		st->cur.i[i] = strtoll(data, &end, 10);
		if (data[0] == 0 || end[0] != 0)
			success = FALSE;
		g_free(data);
	}
	return success;
}

gboolean get_float(struct stats *st)
{
	char *data;
	char *end;
	gboolean success = TRUE;

	data = get_attr(st->attrs[0]);
	if (data == NULL)
		return FALSE;
	st->cur.f = strtod(data, &end);
	if (data[0] == 0 || end[0] != 0)
		success = FALSE;
	g_free(data);
	return success;
}

gboolean get_chunkstats(struct stats *st)
{
	int i;

	st->cur.i[0] = 0;
	st->cur.i[1] = 0;
	for (i = 0; i < numchunks; i++) {
		if (states[i] & 0x4) {
			/* Accessed this session */
			st->cur.i[0]++;
		}
		if (states[i] & 0x8) {
			/* Dirtied this session */
			st->cur.i[1]++;
		}
	}
	return TRUE;
}

char *format_bytes(struct stat_values *values, int which)
{
	return g_strdup_printf("%.1f", 1.0 * values->i[which] / (1 << 20));
}

char *format_chunks(struct stat_values *values, int which)
{
	return g_strdup_printf("%.1f", 1.0 * values->i[which] /
						chunks_per_mb);
}

char *format_compression(struct stat_values *values, int which)
{
	return g_strdup_printf("%.1f%%", 100 - values->f);
}

char *format_hit_rate(struct stat_values *values, int which)
{
	long hits = values->i[0];
	long misses = values->i[1];
	double result;

	if (hits + misses)
		result = 100.0 * hits / (hits + misses);
	else
		result = 0;
	return g_strdup_printf("%.1f%%", result);
}

char *format_dirty_evictions(struct stat_values *values, int which)
{
	long dirty_evictions = values->i[0];
	long evictions = values->i[1];
	double result;

	if (evictions)
		result = 100.0 * dirty_evictions / evictions;
	else
		result = 0;
	return g_strdup_printf("%.1f%%", result);
}

char *format_eviction_writebacks(struct stat_values *values, int which)
{
	long dirty_evictions = values->i[0];
	long writebacks = values->i[2];
	double result;

	if (writebacks)
		result = 100.0 * dirty_evictions / writebacks;
	else
		result = 0;
	return g_strdup_printf("%.1f%%", result);
}

gboolean int_changed(struct stat_values *prev, struct stat_values *cur,
				int which)
{
	return prev->i[which] != cur->i[which];
}

struct stats statistics[] = {
	{
		"Guest",
		{"bytes_read", "bytes_written"},
		get_ints,
		{{"Data read by guest OS this session (MB)",
			format_bytes, int_changed},
		{"Data written by guest OS this session (MB)",
			format_bytes, int_changed}}
	}, {
		"Chunks",
		{"chunk_reads", "chunk_writes"},
		get_ints,
		{{"Chunk data read by Parcelkeeper this session (MB)",
			format_chunks, int_changed},
		{"Chunk data written by Parcelkeeper this session (MB)",
			format_chunks, int_changed}}
	}, {
		"State",
		{NULL},
		get_chunkstats,
		{{"Distinct chunks accessed this session (MB)",
			format_chunks, int_changed},
		{"Distinct chunks dirtied this session (MB)",
			format_chunks, int_changed}}
	}, {
		"Pending",
		{NULL, "cache_dirty"},
		get_ints,
		{{NULL},
		{"Dirty chunk data pending writeback (MB)",
			format_chunks, int_changed}}
	}, {
		"Hit",
		{"cache_hits", "cache_misses"},
		get_ints,
		{{NULL},
		{"Parcelkeeper chunk cache hit rate",
			format_hit_rate, NULL}}
	}, {
		"DEvict",
		{"cache_evictions_dirty", "cache_evictions", "chunk_writes"},
		get_ints,
		{{"Cache evictions that initiate writebacks",
			format_dirty_evictions, NULL},
		{"Chunk writebacks initiated by evictions rather than timer",
			format_eviction_writebacks, NULL}}
	}, {
		"Savings",
		{"compression_ratio_pct"},
		get_float,
		{{NULL},
		{"Average compression savings from chunks written this session",
			format_compression, NULL}}
	}, {0}
};

void stat_output_set_changed(struct stat_output *output, gboolean changed)
{
	const GdkColor busy = {
		.red = 65535,
		.green = 16384,
		.blue = 16384
	};
	GtkStyle *style;
	gboolean prev;

	style = gtk_widget_get_style(output->ebox);
	prev = gdk_color_equal(&busy, &style->bg[GTK_STATE_NORMAL]);
	if (prev != changed)
		gtk_widget_modify_bg(output->ebox, GTK_STATE_NORMAL,
					changed ? &busy : NULL);
}

void update_stat_valid(struct stats *st)
{
	struct stat_output *output;
	int i;
	char *str;
	gboolean changed;

	for (i = 0; i < NOUTPUTS; i++) {
		output = &st->output[i];
		if (output->format == NULL)
			continue;
		str = output->format(&st->cur, i);
		update_label(GTK_LABEL(output->label), str);
		g_free(str);
		if (output->changed != NULL) {
			changed = output->changed(&st->prev, &st->cur, i);
			stat_output_set_changed(output, changed);
		}
	}
}

void update_stat_invalid(struct stats *st)
{
	struct stat_output *output;
	int i;

	for (i = 0; i < NOUTPUTS; i++) {
		output = &st->output[i];
		if (output->changed != NULL)
			stat_output_set_changed(output, FALSE);
	}
}

void update_stats(void)
{
	struct stats *st;
	gboolean visible;

	visible = mapped && g_key_file_get_boolean(config, CONFIG_GROUP,
				"show_stats", NULL);
	for (st = statistics; st->heading != NULL; st++) {
		if (st->fetch(st)) {
			if (visible)
				update_stat_valid(st);
			st->prev = st->cur;
		} else {
			if (visible)
				update_stat_invalid(st);
		}
	}
}

/** Chunk bitmap pane ********************************************************/

GtkWidget *img;
GtkWidget *img_viewport;

void free_pixels(unsigned char *pixels, void *data)
{
	g_free(pixels);
}

void update_img(void)
{
	static char *prev_states;
	static int last_width;
	static int last_height;
	static gboolean last_show_image_cache;
	uint32_t *pixels;
	int numpixels;
	GdkPixbuf *pixbuf;
	int i;
	int width;
	int height;
	int changed = 0;
	gboolean show_image_cache;

	if (!mapped || !g_key_file_get_boolean(config, CONFIG_GROUP,
				"show_bitmap", NULL))
		return;
	if (prev_states == NULL)
		prev_states = g_malloc(numchunks);
	width = img_viewport->allocation.width;
	height = (numchunks + width - 1) / width;
	numpixels = height * width;
	pixels = g_malloc(4 * numpixels);
	show_image_cache = g_key_file_get_boolean(config, CONFIG_GROUP,
				"show_image_cache", NULL);
	if (show_image_cache != last_show_image_cache)
		changed = 1;
	for (i = 0; i < numchunks; i++) {
		if ((states[i] ^ prev_states[i]) & (show_image_cache ?
					0xff : 0xcf)) {
			prev_states[i] = states[i];
			changed = 1;
		}
		if (show_image_cache && (states[i] & 0x20)) {
			/* Dirty in image cache */
			pixels[i] = htonl(0xffff00ff);
		} else if (show_image_cache && (states[i] & 0x10)) {
			/* Buffered in image cache */
			pixels[i] = htonl(0x00ff00ff);
		} else if (states[i] & 0x8) {
			/* Dirtied this session */
			pixels[i] = htonl(0xff0000ff);
		} else if (states[i] & 0x4) {
			/* Accessed this session */
			pixels[i] = htonl(0xffffffff);
		} else if (states[i] & 0x2) {
			/* Dirty */
			pixels[i] = htonl(0x800000ff);
		} else if (states[i] & 0x1) {
			/* Present */
			pixels[i] = htonl(0xa0a0a0ff);
		} else {
			/* Not present */
			pixels[i] = htonl(0x707070ff);
		}
	}
	for (i = numchunks; i < numpixels; i++)
		pixels[i] = 0;
	/* These calls are expensive for large buffers, so we only invoke them
	   if the image has changed */
	if (changed || width != last_width || height != last_height) {
		pixbuf = gdk_pixbuf_new_from_data((guchar *)pixels,
					GDK_COLORSPACE_RGB, TRUE, 8,
					width, height, width * 4,
					free_pixels, NULL);
		gtk_image_set_from_pixbuf(GTK_IMAGE(img), pixbuf);
		g_object_unref(pixbuf);
		last_width = width;
		last_height = height;
		last_show_image_cache = show_image_cache;
	} else {
		g_free(pixels);
	}
}

void img_size_allocate(GtkWidget *widget, GtkAllocation *alloc, void *data)
{
	static int current_width;
	static int prior_width;
	gboolean do_update;

	do_update = current_width != alloc->width;
	if (prior_width == alloc->width &&
				ABS(alloc->width - current_width) > 10) {
		/* We are cycling between two size allocations with
		   significantly different widths, probably indicating that
		   GtkScrolledWindow is oscillating adding and removing the
		   scroll bar.  This can happen when our size allocation, with
		   scroll bar, is just above the number of pixels we need for
		   the whole image.  Break the loop by refusing to update the
		   bitmap. */
		do_update = FALSE;
	}
	prior_width = current_width;
	current_width = alloc->width;
	if (do_update)
		update_img();
}

/** Window *******************************************************************/

GtkWidget *wd;
int state_fd;

void resize_window(void)
{
	struct pane *pane;
	gboolean resizable = FALSE;
	int width;
	int height;

	width = g_key_file_get_integer(config, CONFIG_GROUP, "width", NULL);
	height = g_key_file_get_integer(config, CONFIG_GROUP, "height", NULL);

	/* Calculate window resizability. */
	for (pane = panes; pane->config_key != NULL; pane++) {
		if (!g_key_file_get_boolean(config, CONFIG_GROUP,
					pane->config_key, NULL))
			continue;
		if (pane->resizable)
			resizable = TRUE;
	}

	/* If not resizable, use the minimum size. */
	if (!resizable)
		width = height = 1;

	/* To prevent gtk assertions, ensure that the config file does not
	   cause us to call gtk_window_resize() with an invalid height or
	   width. */
	height = MAX(height, 1);
	width = MAX(width, 1);

	g_key_file_set_integer(config, CONFIG_GROUP, "width", width);
	g_key_file_set_integer(config, CONFIG_GROUP, "height", height);
	/* Resize the window, respecting the minimum size. */
	gtk_window_resize(GTK_WINDOW(wd), width, height);
	gtk_window_set_resizable(GTK_WINDOW(wd), resizable);
}

void move_window(void)
{
	int x;
	int y;
	GError *err1 = NULL;
	GError *err2 = NULL;
	GdkScreen *screen;
	int monitor;
	GdkRectangle geom;

	x = g_key_file_get_integer(config, CONFIG_GROUP, "x", &err1);
	y = g_key_file_get_integer(config, CONFIG_GROUP, "y", &err2);
	if (err1 != NULL || err2 != NULL) {
		g_clear_error(&err1);
		g_clear_error(&err2);
		return;
	}

	/* Find out if this is a valid screen coordinate. */
	screen = gtk_window_get_screen(GTK_WINDOW(wd));
	monitor = gdk_screen_get_monitor_at_point(screen, x, y);
	gdk_screen_get_monitor_geometry(screen, monitor, &geom);
	if (x < geom.x || x >= geom.x + geom.width || y < geom.y ||
				y >= geom.y + geom.height)
		return;

	gtk_window_move(GTK_WINDOW(wd), x, y);
}

gboolean update_event(void *data)
{
	struct stat st;

	if (fstat(state_fd, &st))
		die("fstat failed");
	if (st.st_nlink == 0)
		gtk_main_quit();
	update_stats();
	update_img();
	return TRUE;
}

gboolean configure(GtkWidget *widget, GdkEventConfigure *event, void *data)
{
	g_key_file_set_integer(config, CONFIG_GROUP, "width", event->width);
	g_key_file_set_integer(config, CONFIG_GROUP, "height", event->height);
	g_key_file_set_integer(config, CONFIG_GROUP, "x", event->x);
	g_key_file_set_integer(config, CONFIG_GROUP, "y", event->y);
	return FALSE;
}

gboolean map(GtkWidget *widget, GdkEvent *event, void *data)
{
	mapped = TRUE;
	update_event(NULL);
	return FALSE;
}

gboolean unmap(GtkWidget *widget, GdkEvent *event, void *data)
{
	mapped = FALSE;
	return FALSE;
}

gboolean destroy(GtkWidget *widget, GdkEvent *event, void *data)
{
	gtk_main_quit();
	return TRUE;
}

/** Context menu ************************************************************/

GtkWidget *image_cache_item;
GtkWidget *always_on_top;

void update_pane_dimmers(void)
{
	struct pane *pane;
	int count = 0;

	for (pane = panes; pane->config_key != NULL; pane++)
		if (g_key_file_get_boolean(config, CONFIG_GROUP,
					pane->config_key, NULL))
			count++;
	for (pane = panes; pane->config_key != NULL; pane++) {
		if (count > 1) {
			gtk_widget_set_sensitive(pane->checkbox, TRUE);
		} else {
			if (g_key_file_get_boolean(config, CONFIG_GROUP,
						pane->config_key, NULL))
				gtk_widget_set_sensitive(pane->checkbox, FALSE);
			else
				gtk_widget_set_sensitive(pane->checkbox, TRUE);
		}
	}
}

gboolean menu_toggle_pane(GtkCheckMenuItem *item, void *data)
{
	struct pane *pane = data;
	gboolean newval;

	newval = gtk_check_menu_item_get_active(item);
	g_key_file_set_boolean(config, CONFIG_GROUP, pane->config_key, newval);
	if (newval)
		gtk_widget_show(pane->widget);
	else
		gtk_widget_hide(pane->widget);
	if (!strcmp(pane->config_key, "show_bitmap"))
		gtk_widget_set_sensitive(image_cache_item, newval);
	resize_window();
	update_pane_dimmers();
	return TRUE;
}

gboolean menu_set_option(GtkCheckMenuItem *item, void *which)
{
	gboolean newval;

	newval = gtk_check_menu_item_get_active(item);
	g_key_file_set_boolean(config, CONFIG_GROUP, which, newval);
	if (!strcmp(which, "keep_above"))
		gtk_window_set_keep_above(GTK_WINDOW(wd), newval);
	else if (!strcmp(which, "show_image_cache"))
		update_img();
	return TRUE;
}

gboolean menu_quit(GtkMenuItem *item, void *data)
{
	gtk_main_quit();
	return TRUE;
}

gboolean menu_popup(GtkWidget *widget, GdkEventButton *event, GtkWidget *menu)
{
	if (event->type != GDK_BUTTON_PRESS || event->button != 3)
		return FALSE;
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button,
				event->time);
	return TRUE;
}

gboolean keypress(GtkWidget *widget, GdkEventKey *event, void *data)
{
	switch (event->keyval) {
	case GDK_Tab:
		/* GTK won't let us install an accelerator for this, so we
		   have to do it by hand. */
		gtk_widget_activate(always_on_top);
		return TRUE;
	default:
		return FALSE;
	}
}

GtkWidget *init_menu(GtkAccelGroup *accels)
{
	GtkWidget *menu;
	GtkWidget *item;
	struct pane *pane;

	menu = gtk_menu_new();

	for (pane = panes; pane->config_key != NULL; pane++) {
		item = gtk_check_menu_item_new_with_label(pane->menu_label);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),
					g_key_file_get_boolean(config,
					CONFIG_GROUP, pane->config_key, NULL));
		g_signal_connect(item, "toggled",
					G_CALLBACK(menu_toggle_pane), pane);
		gtk_widget_add_accelerator(item, "activate", accels,
					pane->accel, 0, GTK_ACCEL_VISIBLE);
		pane->checkbox = item;
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	}
	update_pane_dimmers();

	item = gtk_check_menu_item_new_with_label("   Show image cache");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),
				g_key_file_get_boolean(config, CONFIG_GROUP,
				"show_image_cache", NULL));
	gtk_widget_set_sensitive(item, g_key_file_get_boolean(config,
				CONFIG_GROUP, "show_bitmap", NULL));
	g_signal_connect(item, "toggled", G_CALLBACK(menu_set_option),
				"show_image_cache");
	gtk_widget_add_accelerator(item, "activate", accels, GDK_M, 0,
				GTK_ACCEL_VISIBLE);
	image_cache_item = item;
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	item = gtk_check_menu_item_new_with_label("Keep window on top");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),
				g_key_file_get_boolean(config, CONFIG_GROUP,
				"keep_above", NULL));
	g_signal_connect(item, "toggled", G_CALLBACK(menu_set_option),
				"keep_above");
	gtk_widget_add_accelerator(item, "activate", accels, GDK_Tab, 0,
				GTK_ACCEL_VISIBLE);
	always_on_top = item;
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	item = gtk_menu_item_new_with_label("Quit");
	g_signal_connect(item, "activate", G_CALLBACK(menu_quit), NULL);
	gtk_widget_add_accelerator(item, "activate", accels, GDK_Escape, 0,
				GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(item, "activate", accels, GDK_q, 0, 0);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	gtk_widget_show_all(menu);
	return menu;
}

/** Initialization ***********************************************************/

const char *name;
const char *uuid;

void init_files(void)
{
	GError *err = NULL;
	char *path;
	char *data;
	char **lines;
	char **line;
	char **kv;
	char *val;
	char *end;

	path = g_strdup_printf("/dev/shm/openisr-chunkmap-%s", uuid);
	state_fd = open(path, O_RDONLY);
	if (state_fd == -1) {
		if (errno == ENOENT)
			die("Parcel %s is not currently running", uuid);
		else
			die("Couldn't open %s", path);
	}
	numchunks = lseek(state_fd, 0, SEEK_END);
	if (numchunks == -1)
		die("lseek failed");
	states = mmap(NULL, numchunks, PROT_READ, MAP_SHARED, state_fd, 0);
	if (states == MAP_FAILED)
		die("mmap failed");
	g_free(path);

	path = g_strdup_printf("%s/%s/parcel.cfg", isrdir, uuid);
	if (!g_file_get_contents(path, &data, NULL, &err))
		die("Couldn't read parcel.cfg: %s", err->message);
	g_free(path);
	lines = g_strsplit(data, "\n", 0);
	g_free(data);
	for (line = lines, val = NULL; *line != NULL; line++) {
		kv = g_strsplit(*line, "=", 2);
		g_strstrip(kv[0]);
		if (!strcmp(kv[0], "CHUNKSIZE")) {
			val = g_strdup(kv[1]);
			g_strfreev(kv);
			break;
		}
		g_strfreev(kv);
	}
	g_strfreev(lines);
	if (val == NULL)
		die("Couldn't get parcel chunk size from parcel.cfg");
	g_strstrip(val);
	chunks_per_mb = (1 << 20) / strtol(val, &end, 10);
	if (val[0] == 0 || end[0] != 0)
		die("Couldn't parse parcel chunk size");
	g_free(val);
}

GtkWidget *pane_widget(const char *config_key, GtkWidget *widget)
{
	struct pane *pane;
	GtkWidget *frame;

	for (pane = panes; pane->config_key != NULL; pane++) {
		if (!strcmp(config_key, pane->config_key)) {
			frame = gtk_frame_new(pane->frame_label);
			gtk_container_add(GTK_CONTAINER(frame), widget);
			pane->widget = frame;
			return frame;
		}
	}
	return NULL;
}

const char img_tooltip[] =
"Yellow: Dirty in image cache (M to toggle)\n"
"Green: Present in image cache (M to toggle)\n"
"Red: Dirtied this session\n"
"White: Accessed this session\n"
"Dark red: Dirtied in previous session\n"
"Light gray: Copied in previous session\n"
"Dark gray: Not present";

void init_window(void)
{
	GtkAccelGroup *accels;
	GtkWidget *vbox;
	GtkWidget *stats_table;
	GtkWidget *lbl;
	GtkWidget *menu;
	GtkWidget *img_scroller;
	GtkTooltips *tips;
	PangoAttrList *palist;
	struct stats *st;
	struct stat_output *output;
	struct pane *pane;
	char *title;
	int i;
	int j;

	title = g_strdup_printf("Dirtometer: %s", name);
	wd = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(wd), title);
	gtk_window_set_icon_from_file(GTK_WINDOW(wd),
				SHAREDIR "/logo-128.png", NULL);
	g_free(title);
	gtk_container_set_border_width(GTK_CONTAINER(wd), 2);
	gtk_window_set_gravity(GTK_WINDOW(wd), GDK_GRAVITY_STATIC);
	accels = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(wd), accels);
	menu = init_menu(accels);
	tips = gtk_tooltips_new();
	vbox = gtk_vbox_new(FALSE, 5);
	for (i = 0; statistics[i].heading != NULL; i++);
	stats_table = gtk_table_new(i, 3, TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(stats_table), 2);
	img = gtk_image_new();
	gtk_misc_set_alignment(GTK_MISC(img), 0, 0);
	img_scroller = gtk_scrolled_window_new(NULL, NULL);
	img_viewport = gtk_viewport_new(
				gtk_scrolled_window_get_hadjustment(
				GTK_SCROLLED_WINDOW(img_scroller)
				), gtk_scrolled_window_get_vadjustment(
				GTK_SCROLLED_WINDOW(img_scroller)));
	gtk_tooltips_set_tip(tips, img_viewport, img_tooltip, NULL);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(img_viewport),
				GTK_SHADOW_NONE);
	gtk_widget_set_size_request(img_viewport, 0, 150);
	gtk_container_add(GTK_CONTAINER(img_viewport), img);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(img_scroller),
				GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_set_border_width(GTK_CONTAINER(img_scroller), 2);
	gtk_container_add(GTK_CONTAINER(img_scroller), img_viewport);
	lbl = gtk_label_new(name);
	palist = pango_attr_list_new();
	pango_attr_list_insert(palist,
				pango_attr_weight_new(PANGO_WEIGHT_BOLD));
	gtk_label_set_attributes(GTK_LABEL(lbl), palist);
	pango_attr_list_unref(palist);
	gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0);
	gtk_misc_set_padding(GTK_MISC(lbl), 3, 2);
	gtk_box_pack_start(GTK_BOX(vbox), lbl, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), pane_widget("show_stats",
				stats_table), FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), pane_widget("show_bitmap",
				img_scroller), TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(wd), vbox);
	for (i = 0; statistics[i].heading != NULL; i++) {
		st = &statistics[i];
		lbl = gtk_label_new(st->heading);
		gtk_misc_set_alignment(GTK_MISC(lbl), 0, 0.5);
		gtk_table_attach(GTK_TABLE(stats_table), lbl, 0, 1, i, i + 1,
					GTK_FILL, 0, 0, 0);
		for (j = 0; j < NOUTPUTS; j++) {
			output = &st->output[j];
			if (output->format == NULL)
				continue;
			output->ebox = gtk_event_box_new();
			output->label = gtk_label_new("--");
			gtk_container_add(GTK_CONTAINER(output->ebox),
						output->label);
			gtk_misc_set_alignment(GTK_MISC(output->label), 1, 0.5);
			gtk_tooltips_set_tip(tips, output->ebox,
						output->tooltip, NULL);
			gtk_table_attach(GTK_TABLE(stats_table), output->ebox,
						j + 1, j + 2, i, i + 1,
						GTK_FILL, 0, 3, 2);
		}
	}
	gtk_widget_show_all(GTK_WIDGET(wd));
	/* Now re-hide the panes that are not enabled. */
	for (pane = panes; pane->config_key != NULL; pane++)
		if (!g_key_file_get_boolean(config, CONFIG_GROUP,
					pane->config_key, NULL))
			gtk_widget_hide(pane->widget);
	gtk_widget_add_events(wd, GDK_BUTTON_PRESS_MASK);
	g_signal_connect(wd, "configure-event", G_CALLBACK(configure), wd);
	g_signal_connect(wd, "delete-event", G_CALLBACK(destroy), NULL);
	g_signal_connect(wd, "key-press-event", G_CALLBACK(keypress), wd);
	g_signal_connect(wd, "button-press-event", G_CALLBACK(menu_popup),
				menu);
	g_signal_connect(wd, "map-event", G_CALLBACK(map), NULL);
	g_signal_connect(wd, "unmap-event", G_CALLBACK(unmap), NULL);
	g_signal_connect(img_viewport, "size-allocate",
				G_CALLBACK(img_size_allocate), NULL);

	move_window();
	gtk_window_set_keep_above(GTK_WINDOW(wd),
				g_key_file_get_boolean(config, CONFIG_GROUP,
				"keep_above", NULL));
	resize_window();
}

void signal_handler(int sig)
{
	gtk_main_quit();
}

GOptionEntry options[] = {
	{"name", 'n', 0, G_OPTION_ARG_STRING, &name, "Parcel name", "NAME"},
	{NULL, 0, 0, 0, NULL, NULL, NULL}
};

int main(int argc, char **argv)
{
	GError *err = NULL;

	if (!gtk_init_with_args(&argc, &argv, "<parcel-uuid>", options, NULL,
				&err))
		die("%s", err->message);
	if (argc != 2)
		die("Missing parcel UUID");
	uuid = argv[1];
	if (name == NULL)
		name = uuid;
	isrdir = g_strdup_printf("%s/.isr", getenv("HOME"));
	confdir = g_strdup_printf("%s/dirtometer", isrdir);
	conffile = g_strdup_printf("%s/%s", confdir, uuid);
	statsdir = g_strdup_printf("%s/%s/vfs/stats", isrdir, uuid);

	init_files();
	read_config();
	init_window();
	set_signal_handler(SIGINT, signal_handler);
	g_timeout_add(100, update_event, NULL);
	gtk_main();
	write_config();
	return 0;
}
