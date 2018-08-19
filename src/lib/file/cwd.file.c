
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


// This function return the absolute pathname of
// the calling thread current working directory.
string filegetcwd () {
	
	enum {
		// Amount by which to increase
		// the buffer used with getcwd()
		// when it is too small.
		BUFFERINCREASEAMOUNT = 100
	};
	
	uint size = BUFFERINCREASEAMOUNT;
	
	u8* buffer = 0;
	
	while (1) {
		
		buffer = mmrealloc(buffer, size);
		
		if (getcwd(buffer, size)) {
			// If I get here, getcwd() could write
			// the current working directory path
			// in the buffer.
			// I now find the exact size of the string
			// for the current working directory path.
			
			size -= BUFFERINCREASEAMOUNT;
			
			u8* s = buffer+size;
			
			while (*s) {
				++s;
				++size;
			}
			
			// +1 account for the null terminating character.
			return ((string) {.ptr = mmrealloc(buffer, size+1)});
			
		} else size += BUFFERINCREASEAMOUNT;
	}
}


// This function set the calling thread
// current working directory to the path
// given as argument.
// On error, null is returned
// otherwise 1 is returned.
uint filesetcwd (u8* path) {
	return !chdir(path);
}
