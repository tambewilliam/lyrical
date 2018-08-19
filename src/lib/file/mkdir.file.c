
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


// This function create a directory.
// It is not an error if a directory already exist.
// Null is returned on failure.
uint filemkdir (u8* path) {
	return !mkdir(path, 0700);
}


// This function create a directory.
// Parent directories are created as needed.
// It is not an error if a directory already exist.
// Null is returned on failure.
uint filemkdirparents (u8* path) {
	
	__label__ done;
	
	void recurse (u8* path) {
		
		switch (fileis1(path)) {
			
			case FILEISNOTEXIST:;
				
				string pathdirname = filedirname(path);
				
				recurse(pathdirname.ptr);
				
				mmfree(pathdirname.ptr);
				
				mkdir(path, 0700);
				
				return;
				
			case FILEISFOLDER:
				
				return;
				
			default:
				
				goto done;
		}
	}
	
	recurse(path);
	
	done:
	
	return (fileis1(path) == FILEISFOLDER);
}
