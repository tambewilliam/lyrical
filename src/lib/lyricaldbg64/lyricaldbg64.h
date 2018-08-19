
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


#ifndef LIBLYRICALDBG64
#define LIBLYRICALDBG64

// Structure used by lyricaldbg64getline()
// to return its result.
typedef struct {
	// Point to a null-terminated string
	// containing the file path from which
	// the line of text was extracted.
	// It must be freed using mmfree().
	u8* path;
	
	// Line number within the file.
	u64 lnum;
	
	// Point to a null-terminated string
	// containing the line of text
	// from the source code file.
	// An empty line of text is returned if
	// the source code file could not be opened.
	// It must be freed using mmfree().
	u8* ltxt;
	
} lyricaldbg64getlineresult;

// Function which retrieve
// the line of source code
// from which the binary,
// at the offset given by
// the argument boff,
// was generated.
// The argument dbgfilepath
// is the path to the debugging
// information file to use.
// Null is returned if an error
// occured, or the binary offset
// given as argument is out of range.
lyricaldbg64getlineresult lyricaldbg64getline (u8* dbgfilepath, u64 boff);

#endif
