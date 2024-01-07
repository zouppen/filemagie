// Main module for Kleb
// SPDX-License-Identifier:   GPL-3.0-or-later
// Copyright (C) 2024 Joel Lehtonen
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <linux/fs.h> // Definition of FICLONE* constants
#include <sys/ioctl.h>
#include <glib.h>

static bool regular_copy(int const src, int const dst);

static gchar *target_file = NULL;
static gboolean force = false;
static gboolean append = false;
static gboolean strict = false;

static GOptionEntry entries[] =
{
	{ "target-file", 't', 0, G_OPTION_ARG_FILENAME, &target_file, "Glue all SOURCE arguments into TARGET", "TARGET"},
	{ "force", 'f', 0, G_OPTION_ARG_NONE, &force, "Force overwrite of target file", NULL},
	{ "append", 'a', 0, G_OPTION_ARG_NONE, &append, "Append to the target file (non-atomic)", NULL},
	{ "strict", 's', 0, G_OPTION_ARG_NONE, &strict, "Do not fallback to regular copy if reflinking fails", NULL},
	{ NULL }
};

int main(int argc, char **argv)
{
	g_autoptr(GError) error = NULL;
	g_autoptr(GOptionContext) context = g_option_context_new("FILE...");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "File surgery tool on reflink capable file systems.");
	g_option_context_set_description(context,
					 "This tool can glue files together or patch parts of a file with contents of another file. Source files are unaffected.\n"
					 "");

	if (!g_option_context_parse(context, &argc, &argv, &error))
	{
		errx(1, "Option parsing failed: %s", error->message);
	}

	int sources;
	if (target_file == NULL) {
		target_file = argv[argc-1];
		sources = argc-2;
	} else {
		sources = argc-1;
	}
	char **source = argv+1;

	// Command validation and selection
	if (sources < 1) {
		errx(1, "Missing file names. See %s --help", argv[0]);
	}

	// I miss pattern matching from Haskell
	int flags_out;
	if (force) {
		if (append) {
			flags_out = O_CREAT;
		} else {
			flags_out = O_CREAT | O_TRUNC;
		}
	} else {
		if (append) {
			flags_out = 0;
		} else {
			flags_out = O_CREAT | O_EXCL;
		}
	}

	int const fd_out = open(target_file, O_WRONLY | flags_out, 0666);
	if (fd_out == -1) {
		err(2, "Unable to open '%s' for writing", target_file);
	}

	for (int source_i=0; source_i<sources; source_i++) {
		int const fd_in = open(source[source_i], O_RDONLY);
		if (fd_in == -1) {
			err(2, "Unable to open '%s' for reading", source[source_i]);
		}

		off_t pos = lseek(fd_out, 0, SEEK_END);
		if (pos == -1) {
			err(3, "Unable to seek '%s', is it a regular file?", target_file);
		}

		struct file_clone_range range = {
			.src_fd = fd_in,
			.src_offset = 0,
			.src_length = 0,
			.dest_offset = pos,
		};

		if (ioctl(fd_out, FICLONERANGE, &range) == -1) {
			if (strict) {
				err(3, "Unable to reflink '%s'", source[source_i]);
			} else {
				warnx("Falling back to a regular copy on '%s'", source[source_i]);
				if (regular_copy(fd_in, fd_out) == false) {
					err(3, "Unable do regular copy on '%s'", source[source_i]);
				}
			}
		}

		if (close(fd_in) == -1) {
			err(2, "Unable to close file '%s'", source[source_i]);
		}
	}

	return 0;
}

static bool regular_copy(int const src, int const dst)
{
	// FIXME: This is way too small buffer
	uint8_t buf[4096];
	while (true) {
		ssize_t bytes_in = read(src, buf, sizeof(buf));
		switch (bytes_in) {
		case -1:
			return false;
		case 0:
			return true;
		default:
			ssize_t bytes_out = write(dst, buf, bytes_in);
			// Partial writes shouldn't happen. FIXME:
			// handle that case anyway instead of just failing?
			if (bytes_in != bytes_out) {
				return false;
			}
		}
	}
}
