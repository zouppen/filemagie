# Kuh

Collection of command-line tools for taking advantage of modern Linux
file system CoW features.

## Tools

### Kleb

Kleb is a tool for concatenating multiple source files to a single
file without copying the contents on disk. It can benefit reflink
(range copy) support on the file system.

Usage: `kleb [OPTION?] FILE... TARGET | -t TARGET FILE...`

See full usage by running `kleb --help`.

The rationale behind this tool is to allow files (such as disk images)
to be stored in fragments during transfer but allowing space and time
efficient merge of the results.

**NB!** To benefit from reflink feature, on Btrfs the file "seams"
need to be at multiples of the fundamental block size of the
file system. Otherwise, warning is printed and a regular
copy is performed. To see the block size of current directory, you may
`stat -fc %S .`. Also, the files need to be on a same file system.

## Building

Requires GLib 2 (Debian package `libglib2.0-dev`) to build. The
process of building is a normal CMake one:

```sh
mkdir build
cd build
cmake ..
make
```

## License

These programs are free software: you can redistribute them and/or modify
them under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

[Full license text](LICENSE)
