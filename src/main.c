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
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <glib.h>

static gchar *target_file = NULL;
static gboolean force = false;
static gboolean append = false;

static GOptionEntry entries[] =
{
	{ "target-file", 't', 0, G_OPTION_ARG_FILENAME, &target_file, "Glue all SOURCE arguments into TARGET", "TARGET"},
	{ "force", 'f', 0, G_OPTION_ARG_NONE, &force, "Force overwrite of target file", NULL},
	{ "append", 'a', 0, G_OPTION_ARG_NONE, &append, "Append to the target file (non-atomic)", NULL},
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

	int flags_out = 0;
	if (force) {
		flags_out = O_CREAT;
	} else if (append) {
		flags_out = 0;
	} else {
		flags_out = O_CREAT | O_EXCL;
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

		if (close(fd_in) == -1) {
			err(2, "Unable to close file '%s'", source[source_i]);
		}
	}

	return 0;
}
