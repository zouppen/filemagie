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
#include <glib.h>

static gchar *target_file = NULL;

static GOptionEntry entries[] =
{
	{ "target-file", 't', 0, G_OPTION_ARG_FILENAME, &target_file, "Glue all SOURCE arguments into TARGET", "TARGET"},
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

	int const sources = target_file == NULL ? argc-2 : argc-1;
	char **source = argv+1;

	// Command validation and selection
	if (sources < 1) {
		errx(1, "Missing file names. See %s --help", argv[0]);
	}

	return 0;
}
