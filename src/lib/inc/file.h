
// ---------------------------------------------------------------------
// Copyright (c) William Fonkou Tambe
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
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ---------------------------------------------------------------------


#ifndef LIBFILE
#define LIBFILE

#include <string.h>
#include <arrayu8.h>

// This function return the absolute pathname of
// the calling thread current working directory.
string filegetcwd ();

// This function set the calling thread
// current working directory to the path
// given as argument.
// On error, null is returned
// otherwise 1 is returned.
uint filesetcwd (u8* path);

// This function create a directory.
// It is not an error if a directory already exist.
// Null is returned on failure.
uint filemkdir (u8* path);
// This function create a directory.
// Parent directories are created as needed.
// It is not an error if a directory already exist.
// Null is returned on failure.
uint filemkdirparents (u8* path);

// Enum returned by fileis1().
typedef enum {
	
	FILEISNOTEXIST,	// Returned when the file do not exist.
	FILEISFOLDER,	// Returned when the file is folder.
	FILEISREGULAR,	// Returned when the file is regular.
	FILEISSYMLINK,	// Returned when the file is symlink.
	FILEISOTHER	// Returned when the file is other.
	
} fileisresult;

// This function return whether
// the file do not exist, is a folder,
// regular file, symlink, or other file.
fileisresult fileis1 (u8* path);

// This function return whether
// the file do not exist, is a folder,
// regular file, or other file.
fileisresult fileis2 (u8* path);

// The functions filedirname() and filebasename() break
// a null-terminated pathname string into directory and
// filename components respectively. ei:
// path             dirname            basename
// "/usr/lib"       "/usr"             "lib"
// "/usr/"          "/"                "usr"
// "usr"            "."                "usr"
// "/"              "/"                "/"
// "."              "."                "."
// ".."             "."                ".."
// The argument path should always be
// a valid pointer to a null terminated string.
string filedirname (u8* path);
string filebasename (u8* path);

// This function return
// the timestamp of a file,
// which is the last time
// it was modified.
// Null is returned on failure.
uint filetimestamp (u8* path);

// The function filerelativepath() convert
// an absolute path to a relative path, while
// the function fileabsolutepath() convert
// a relative path to an absolute path.
// Their argument cwd should be an absolute
// path to the current working directory.
// The argument target of filerelativepath()
// should be an absolute path to the target file.
// The argument target of fileabsolutepath()
// should be a relative path to the target file.
// 
// filerelativepath() example:
// cwd:    "/usr/include/kernel/sound"
// target: "/usr/shared/data"
// result: "../../../shared/data"
// 
// fileabsolutepath() example:
// cwd:    "/usr/include/kernel/sound"
// target: "../../../shared/data"
// result: "/usr/shared/data"
string filerelativepath (u8* cwd, u8* target);
string fileabsolutepath (u8* cwd, u8* target);

// This function return the canonical path
// from the path given as argument which
// can be an absolute or relative path. ie:
// both "../../here/bar/x" and "./test/../../bar/x"
// may refer to the same location, but
// textual comparison cannot be done on
// the two paths; however, if turned into
// their canonical representation, they both
// become "../bar/x" , and it can be seen that
// they actually refer to the same location.
string filecanonicalpath (u8* path);

// Enumerated type to be used with the argument type
// of the callback function of filelsdir().
typedef enum {
	
	FILELSDIRTYPEREGULAR,
	FILELSDIRTYPEDIRECTORY,
	FILELSDIRTYPESYMLINK,
	FILELSDIRTYPEOTHER
	
} filelsdirtype;

// This function list the path given
// as argument, followed by all the files
// that it contain if it is a directory,
// descending to its subdirectories.
// If the argument path is not a directory,
// it become the only file listed.
// 
// The argument callback is a pointer
// to a function taking as argument
// a filelsdirtype and a string .
// the filelsdirtype argument tell the type
// of the file for which the path is given
// by the string argument. The callback
// function should use the string argument
// path without attempting to tamper it.
// For every file, filelsdir() will
// call the callback function, which
// must return 1 to continue processing.
// If the callback function was called
// for a directory, and it return -1,
// there will be no descent into the directory;
// similarly if the callback function was
// called for a symlink, and it return -1,
// the symlink will not be followed
// if it was a directory.
// Processing immediately stop if
// the callback function return 0.
// 
// The entries "." and ".." of directories
// are never passed to the callback function.
// 
// This function return 0 if the path
// given as argument could not be accessed,
// otherwise it return 1.
uint filelsdir (string path, uint(*callback)(filelsdirtype type, string path));

// Delete the file for which
// the path is given as argument.
// If the file to delete is a folder,
// it must be empty.
// Null is returned on failure.
uint fileremove (u8* path);
// Delete the file for which
// the path is given as argument.
// If the file to delete is a folder,
// its content is deleted.
// Null is returned on failure.
uint fileremoverecursive (u8* path);

// Return the full content of the file
// for which the path is given as argument.
// A null character get appended
// to the data returned.
// arrayu8null is returned on faillure.
arrayu8 fileread1 (u8* path);
// Same as "fileread (u8* path)", but
// take an offset where to start reading.
arrayu8 fileread2 (u8* path, uint offset);
// Same as "fileread (u8* path, uint offset)",
// but take the count of bytes to read.
arrayu8 fileread3 (u8* path, uint offset, uint sz);
// Write in the buffer given by arguments
// buf and bufsz the content of the file
// for which the path is given as argument
// starting at the offset given as argument.
// At most bufsz bytes are read.
// Null is returned if nothing
// could be read from the file,
// otherwise the number of bytes
// read is returned.
uint fileread4 (u8* path, uint offset, void* buf, uint bufsz);

// Write the file for which the path
// is given as argument, with the data
// given through the arguments buf and bufsz.
// The file is created if it do not exist.
// Null is returned if a failure
// occured, otherwise 1 is returned.
// Note that a failure can occur even
// when data has been written to the file;
// in such a case, the failure can be
// due to all the data not having been
// written out to the file or to an error
// while setting the new size of the file
// after having overwritten its content.
uint filewrite1 (u8* path, void* buf, uint bufsz);
// Same as filewrite (u8* path, void* buf, uint bufsz),
// but write the file from the offset given as argument.
// Appending is done if the argument offset is -1.
uint filewrite2 (u8* path, void* buf, uint bufsz, uint offset);
// Same as filewrite (u8* path, void* buf, uint bufsz), but
// also truncate the file to where the last byte was written.
uint filewritetruncate1 (u8* path, void* buf, uint bufsz);
// Same as filewrite (u8* path, void* buf, uint bufsz, uint offset),
// but also truncate the file to where the last byte was written.
uint filewritetruncate2 (u8* path, void* buf, uint bufsz, uint offset);

// Return a line from the file for which
// the file descriptor is given as argument,
// starting at the offset given as argument.
// stringnull is returned on faillure.
string filegetline1 (uint fd, uint offset);

// Return a line from the file for which
// the path is given as argument, starting
// at the offset given as argument.
// stringnull is returned on faillure.
string filegetline2 (u8* path, uint offset);

#endif
