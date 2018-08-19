
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


// Return the full content of the file
// for which the path is given as argument.
// A null character get appended
// to the data returned.
// arrayu8null is returned on faillure.
arrayu8 fileread1 (u8* path) {
	
	uint fd;
	
	// open() return -1 on error.
	if ((fd = open(path, O_RDONLY)) != -1) {
		// If I get here I have successfully opened
		// the file. I need to get its size.
		
		arrayu8 retvar;
		
		// Used with the syscall fstat().
		struct stat statbuffer;
		
		// fstat() return -1 on error.
		if (fstat(fd, &statbuffer) != -1) {
			// +1 account for the null terminating byte.
			retvar.ptr = mmalloc(statbuffer.st_size +1);
			
			uint retvarsz = read(fd, retvar.ptr, statbuffer.st_size);
			
			if (retvarsz != statbuffer.st_size) {
				
				mmfree(retvar.ptr);
				
				retvar = arrayu8null;
				
			} else retvar.ptr[retvarsz] = 0;
			
		} else retvar = arrayu8null;
		
		// Close the file.
		close(fd);
		
		return retvar;
		
	} else return arrayu8null;
}


// Same as "fileread (u8* path)", but
// take an offset where to start reading.
arrayu8 fileread2 (u8* path, uint offset) {
	
	uint fd;
	
	// open() return -1 on error.
	if ((fd = open(path, O_RDONLY)) != -1) {
		// If I get here I have successfully opened
		// the file. I need to get its size.
		
		arrayu8 retvar;
		
		// Used with the syscall fstat().
		struct stat statbuffer;
		
		// fstat() return -1 on error.
		if (fstat(fd, &statbuffer) != -1) {
			
			if (offset < statbuffer.st_size &&
				lseek(fd, offset, SEEK_SET) != -1) {
				
				// +1 account for the null terminating byte.
				retvar.ptr = mmalloc(statbuffer.st_size +1);
				
				uint retvarsz = read(fd, retvar.ptr, (statbuffer.st_size - offset));
				
				if (retvarsz != statbuffer.st_size) {
					
					mmfree(retvar.ptr);
					
					retvar = arrayu8null;
					
				} else retvar.ptr[retvarsz] = 0;
				
			} else retvar = arrayu8null;
			
		} else retvar = arrayu8null;
		
		// Close the file.
		close(fd);
		
		return retvar;
		
	} else return arrayu8null;
}


// Same as "fileread (u8* path, uint offset)",
// but take the count of bytes to read.
arrayu8 fileread3 (u8* path, uint offset, uint sz) {
	
	uint fd;
	
	// open() return -1 on error.
	if ((fd = open(path, O_RDONLY)) != -1) {
		// If I get here I have successfully opened
		// the file. I need to get its size.
		
		arrayu8 retvar;
		
		// Used with the syscall fstat().
		struct stat statbuffer;
		
		// fstat() return -1 on error.
		if (fstat(fd, &statbuffer) != -1) {
			
			if ((offset+sz) <= statbuffer.st_size &&
				lseek(fd, offset, SEEK_SET) != -1) {
				
				// +1 account for the null terminating byte.
				retvar.ptr = mmalloc(sz +1);
				
				uint retvarsz = read(fd, retvar.ptr, sz);
				
				if (retvarsz != sz) {
					
					mmfree(retvar.ptr);
					
					retvar = arrayu8null;
					
				} else retvar.ptr[retvarsz] = 0;
				
			} else retvar = arrayu8null;
			
		} else retvar = arrayu8null;
		
		// Close the file.
		close(fd);
		
		return retvar;
		
	} else return arrayu8null;
}


// Write in the buffer given by arguments
// buf and bufsz the content of the file
// for which the path is given as argument
// starting at the offset given as argument.
// At most bufsz bytes are read.
// Null is returned if nothing
// could be read from the file,
// otherwise the number of bytes
// read is returned.
uint fileread4 (u8* path, uint offset, void* buf, uint bufsz) {
	
	uint fd;
	
	// open() return -1 on error.
	if ((fd = open(path, O_RDONLY)) != -1) {
		// If I get here I have successfully opened
		// the file. I need to get its size.
		
		uint retvar;
		
		// Used with the syscall fstat().
		struct stat statbuffer;
		
		// fstat() return -1 on error.
		if (fstat(fd, &statbuffer) != -1) {
			
			if (offset < statbuffer.st_size &&
				lseek(fd, offset, SEEK_SET) != -1) {
				
				retvar = read(fd, buf, bufsz);
				
				if (retvar == -1) retvar = 0;
				
			} else retvar = 0;
			
		} else retvar = 0;
		
		// Close the file.
		close(fd);
		
		return retvar;
		
	} else return 0;
}
