
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
uint filelsdir (string path, uint(*callback)(filelsdirtype type, string path)) {
	// This function will process the directory
	// for which the path is given as argument,
	// and will recursively call itself
	// to process subdirectories.
	void processdir (string path) {
		// In short, this is what is done:
		// - Open the directory given as argument.
		// - Read the list of files in the directory to an array of pointers to string.
		// - Close the directory; I should have one opened directory at a time.
		// - Go through the list of files in the array and pass each
		// entry to the callback function; if an entry is a directory,
		// after having passed it to the callback function and upon
		// returning from the callback function, recursively call this function
		// on the directory entry. The other files of the list will be processed
		// when I return from the recursion.
		
		arrayuint listoffiles = arrayuintnull;
		
		DIR* dirfd;
		
		if (dirfd = opendir(path.ptr)) {
			
			struct dirent* direntry;
			
			while (direntry = readdir(dirfd)) {
				
				u8* direntryname = direntry->d_name;
				
				if (direntryname[0] == '.' && ((direntryname[1] == '.' && !direntryname[2]) || !direntryname[1]))
					continue;
				
				*arrayuintappend1(&listoffiles) = (uint)stringduplicate2(direntryname).ptr;
			}
			
			closedir(dirfd);
		}
		
		uint listoffilescount = arrayuintsz(listoffiles);
		
		uint i = 0;
		
		while (i < listoffilescount) {
			
			string s = stringduplicate1(path);
			
			uint ssz = stringmmsz(s);
			
			if (ssz && s.ptr[ssz-1] != '/') stringappend4(&s, '/');
			
			stringappend2(&s, (u8*)listoffiles.ptr[i]);
			
			struct stat statbuffer;
			
			// Note that lstat() do not follow symlink.
			if (!lstat(s.ptr, &statbuffer)) {
				
				if (S_ISDIR(statbuffer.st_mode)) {
					
					isfilelsdirtypedirectory:;
					
					uint n = callback(FILELSDIRTYPEDIRECTORY, s);
					
					if (!n) {
						mmfree(s.ptr);
						break;
					}
					
					if ((sint)n > 0) processdir(s);
					
					mmfree(s.ptr);
					
				} else if (S_ISREG(statbuffer.st_mode)) {
					
					uint n = callback(FILELSDIRTYPEREGULAR, s);
					
					mmfree(s.ptr);
					
					if (!n) break;
					
				} else if (S_ISLNK(statbuffer.st_mode)) {
					
					uint n = callback(FILELSDIRTYPESYMLINK, s);
					
					if ((sint)n > 0) {
						// Note that stat() follow symlink.
						if (stat(s.ptr, &statbuffer) == 0) {
							// I check whether the symlink point to a directory.
							if (S_ISDIR(statbuffer.st_mode)) goto isfilelsdirtypedirectory;
						}
					}
					
					mmfree(s.ptr);
					
					if (!n) break;
					
				} else {
					
					uint n = callback(FILELSDIRTYPEOTHER, s);
					
					mmfree(s.ptr);
					
					if (!n) break;
				}
			}
			
			++i;
		}
		
		i = 0;
		
		while (i < listoffilescount) {
			
			mmfree((u8*)listoffiles.ptr[i]);
			
			++i;
		}
		
		if (listoffiles.ptr) mmfree(listoffiles.ptr);
	}
	
	struct stat statbuf;
	
	// Note that lstat() do not follow symlink.
	if (lstat(path.ptr, &statbuf)) return 0;
	else if (S_ISDIR(statbuf.st_mode)) {
		
		isfilelsdirtypedirectory:;
		
		if ((sint)callback(FILELSDIRTYPEDIRECTORY, path) > 0)
			processdir(path);
		
	} else if (S_ISREG(statbuf.st_mode)) callback(FILELSDIRTYPEREGULAR, path);
	else if (S_ISLNK(statbuf.st_mode)) {
		
		if ((sint)callback(FILELSDIRTYPESYMLINK, path) > 0) {
			// Note that stat() follow symlink.
			// I return 1 because at least
			// the symlink could be accessed.
			if (stat(path.ptr, &statbuf)) return 1;
			
			// I check whether the symlink point to a directory.
			if (S_ISDIR(statbuf.st_mode)) goto isfilelsdirtypedirectory;
		}
		
	} else callback(FILELSDIRTYPEOTHER, path);
	
	return 1;
}
