// Main module for Trenn
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
#include <inttypes.h>
#include <linux/fs.h> // Definition of FICLONE* constants
#include <sys/ioctl.h>
#include <glib.h>

// Seeks to the end of the dst and appends src to dest using a reflink
// copy.
static bool reflink_copy(int const src, int const dst);

// Count number of digits in a integer number of given base
static int digits(off_t const a, int base);

// Parse unit string (ki, Mi, ...) and return the multiplier.
static uint64_t parse_unit(char const *const s);

// Generates printf format string which can fit the given peak
// value. Returns freshly allocated buffer or NULL in case of an
// error.
char *new_format_string(off_t const peak_value);

static gchar *size_str = NULL;

static GOptionEntry entries[] =
{
	{ "size", 's', 0, G_OPTION_ARG_STRING, &size_str, "Target chunk size", "SIZE"},
	{ NULL }
};

int main(int argc, char **argv)
{
	g_autoptr(GError) error = NULL;
	g_autoptr(GOptionContext) context = g_option_context_new("FILE");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "Split a file to fragments on reflink capable file systems.");
	g_option_context_set_description(context,
					 "This tool can split a file to multiple equal-sized fragements. Source file is unaffected.\n"
					 "");

	if (!g_option_context_parse(context, &argc, &argv, &error))
	{
		errx(1, "Option parsing failed: %s", error->message);
	}

	if (argc != 2) {
		errx(1, "Only one file name is expected");
	}

	if (size_str == NULL) {
		errx(1, "Chunk size must be given with -s");
	}

	// Parse chunk size
	char *unit;
	long const chunk_n = strtol(size_str, &unit, 10);
	uint64_t const chunk_mult = parse_unit(unit);
	if (chunk_mult == -1) {
		errx(1, "Unknown size unit '%s'. Supported: k, ki, M, Mi, G, Gi, T, Ti, P, Pi", unit);
	}
	uint64_t chunk_size = chunk_n * chunk_mult;
	if (chunk_size <= 0) {
		errx(1, "Chunk size must be positive");
	}

	// Open input file
	char const *source = argv[1];
	int const fd_in = open(source, O_RDONLY);
	if (fd_in == -1) {
		err(2, "Unable to open '%s' for reading", source);
	}

	// Get input file length by seeking
	off_t const source_len = lseek(fd_in, 0, SEEK_END);
	if (source_len == -1) {
		err(3, "Unable to seek '%s', is it a regular file?", source);
	}

	uint64_t const chunks = (source_len-1)/chunk_size+1; // Round up

	// Prepares a format string which fits the given chunk count
	char const *const format = new_format_string(chunks);
	if (format == NULL) {
		errx(100, "Chunk file name formatting error");
	}

	printf("File size %ld, chunk size %ld, format string %s\n", source_len, chunk_size, format);

	for (uint64_t i=1; i<chunks; i++) {
		char *file;
		if (asprintf(&file, format, source, i) == -1) {
			errx(100, "Chunk file name formatting error");
		}
		// And use it somewhere
		puts(file);
	}

	return 0;
}

static int digits(off_t a, int base)
{
	bool pos = a > 0;
	int x = 0;
	while (a) {
		x++;
		a = a / base;
	}
	return x + (pos ? 0 : 1);
}

char *new_format_string(off_t const peak_value)
{
	char *s;
	if (asprintf(&s, "%%s-%%0%d" PRIu64, digits(peak_value, 10)) == -1) {
		return NULL;
	}
	return s;
}

static uint64_t parse_unit(char const *const s)
{
	if (*s == '\0') return 1;
	if (strcasecmp(s, "ki") == 0) return (uint64_t)1 << 10;
	if (strcasecmp(s, "Mi") == 0) return (uint64_t)1 << 20;
	if (strcasecmp(s, "Gi") == 0) return (uint64_t)1 << 30;
	if (strcasecmp(s, "Ti") == 0) return (uint64_t)1 << 40;
	if (strcasecmp(s, "Pi") == 0) return (uint64_t)1 << 50;
	if (strcasecmp(s, "k") == 0) return 1000;
	if (strcasecmp(s, "M") == 0) return 1000000;
	if (strcasecmp(s, "G") == 0) return 1000000000;
	if (strcasecmp(s, "T") == 0) return 1000000000000;
	if (strcasecmp(s, "P") == 0) return 1000000000000000;
	return -1;
}
