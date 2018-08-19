
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


#ifndef LIBLYRICALBACKENDX64
#define LIBLYRICALBACKENDX64

#include <lyrical.h>

// Structure used by
// lyricalbackendx64()
// to return the executable
// binary generated.
typedef struct {
	// Layout of the executable binary.
	// 
	// ----------------------------
	// Instructions to execute.
	// ----------------------------
	// Null terminated strings.
	// ----------------------------
	
	// The region for global variables
	// is not part of the generated
	// executable binary, and must be
	// allocated immediately after
	// the executable binary when
	// it is loaded in memory.
	
	// Executable binary.
	arrayu8 execbin;
	
	// Fields holding the size of
	// the executable instructions and
	// constant strings in the executable
	// binary; as well as the size of
	// global variables which is not
	// part of the executable binary.
	uint executableinstrsz;
	uint constantstringssz;
	uint globalvarregionsz;
	
	// Export information is a serie of
	// null terminated function signature string
	// followed by a u32 written in little endian
	// that is the offset within the executable
	// where the export is to be found.
	
	// Export information.
	arrayu8 exportinfo;
	
	// Import information is a serie of null
	// terminated function signature string that are
	// followed by a u32 written in little endian
	// which is the offset within the string region
	// from where the import offset is to be retrieved.
	// The import offset is an offset to the import
	// from where it is written in the string region.
	
	// Import information.
	arrayu8 importinfo;
	
	// Debug information are organized in sections:
	// Section1: Debug information allowing to
	// 	determine to which source code line
	// 	a specific binary instruction originated.
	// 	Each entry has the following fields:
	// 	u32 binoffset: Offset of the instruction in the binary.
	// 	u32 filepath: Offset of the source code absolute filepath string within section2 debug information.
	// 	u32 linenumber: Line number within the source code.
	// 	u32 lineoffset: Byte offset of the line within the source code.
	// 	The last entry has its field "linenumber" null,
	// 	while its field "binoffset" has the upper limit offset;
	// 	that last entry is not meant to be used, and
	// 	solely mark the end of section1 debug information.
	// Section2: Debug information holding
	// 	null-terminated strings used by
	// 	other debug information sections.
	// 
	// Layout of the debug information.
	// 
	// ------------------------------------------------
	// u32 size in bytes of section1 debug information.
	// ------------------------------------------------
	// Section1 debug information.
	// ------------------------------------------------
	// u32 size in bytes of section2 debug information.
	// ------------------------------------------------
	// Section2 debug information.
	// ------------------------------------------------
	
	// Debug information.
	arrayu8 dbginfo;
	
} lyricalbackendx64result;

// enum used with the argument flag
// of lyricalbackendx64().
typedef enum {
	// When used, the constant strings
	// and global variable regions are
	// aligned to 32 bits (4 bytes).
	LYRICALBACKENDX64COMPACT,
	
	// When used, the executable binary is
	// generated as if LYRICALBACKENDX64COMPACT
	// was used, but the instructions
	// to execute assume that the constant
	// strings and global variable regions
	// are aligned to a pagesize (4096 bytes);
	// hence when loading the executable
	// binary in memory, the constant strings
	// data must start at the next address,
	// after the instructions to execute,
	// that is aligned to a pagesize; the global
	// variable region must start at the next
	// address, after the constant strings
	// region, that is aligned to a pagesize.
	LYRICALBACKENDX64COMPACTPAGEALIGNED,
	
	// When used, the constant strings
	// and global variable regions are
	// aligned to a pagesize (4096 bytes).
	LYRICALBACKENDX64PAGEALIGNED,
	
} lyricalbackendx64flag;

// Backend which convert
// the result of the compilation
// to x64 instructions.
// The field rootfunc of the
// lyricalcompileresult used as
// argument should be non-null.
// Null is returned on faillure.
lyricalbackendx64result* lyricalbackendx64 (lyricalcompileresult compileresult, lyricalbackendx64flag flag);

// Free the memory used by the object
// returned by lyricalbackendx64().
void lyricalbackendx64resultfree (lyricalbackendx64result* o);

#endif
