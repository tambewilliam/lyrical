
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
uint filewrite1 (u8* path, void* buf, uint bufsz) {
	
	uint fd;
	
	// open() return -1 on error.
	if ((fd = open(path, O_WRONLY|O_CREAT, 0600)) != -1) {
		// If I get here I have successfully opened the file.
		
		uint noerror;
		
		// Null should be returned
		// if all the data could not
		// be written to the file.
		if (write(fd, buf, bufsz) != bufsz)
			noerror = 0;
		else noerror = 1;
		
		// Close the file.
		close(fd);
		
		return noerror;
		
	} else return 0;
}


// Same as filewrite (u8* path, void* buf, uint bufsz),
// but write the file from the offset given as argument.
// Appending is done if the argument offset is -1.
uint filewrite2 (u8* path, void* buf, uint bufsz, uint offset) {
	
	uint fd;
	
	// open() return -1 on error.
	if ((fd = open(path, O_WRONLY|O_CREAT, 0600)) != -1) {
		// If I get here I have successfully opened the file.
		
		uint noerror;
		
		uint append = (offset == -1);
		
		if (lseek(
				fd,
				append ? 0 : offset,
				append ? SEEK_END : SEEK_SET) == -1)
			noerror = 0;
		else noerror = 1;
		
		if (noerror) {
			// Null should be returned
			// if all the data could not
			// be written to the file.
			if (write(fd, buf, bufsz) != bufsz) noerror = 0;
		}
		
		// Close the file.
		close(fd);
		
		return noerror;
		
	} else return 0;
}


// Same as filewrite (u8* path, void* buf, uint bufsz), but
// also truncate the file to where the last byte was written.
uint filewritetruncate1 (u8* path, void* buf, uint bufsz) {
	
	uint fd;
	
	// open() return -1 on error.
	if ((fd = open(path, O_WRONLY|O_CREAT, 0600)) != -1) {
		// If I get here I have successfully opened the file.
		
		uint noerror;
		
		// Null should be returned
		// if all the data could not
		// be written to the file.
		if (write(fd, buf, bufsz) != bufsz) noerror = 0;
		else {
			// I truncate the file to where
			// the last byte was written.
			if (ftruncate(fd, bufsz) == -1)
				noerror = 0;
			else noerror = 1;
		}
		
		// Close the file.
		close(fd);
		
		return noerror;
		
	} else return 0;
}


// Same as filewrite (u8* path, void* buf, uint bufsz, uint offset),
// but also truncate the file to where the last byte was written.
uint filewritetruncate2 (u8* path, void* buf, uint bufsz, uint offset) {
	
	uint fd;
	
	// open() return -1 on error.
	if ((fd = open(path, O_WRONLY|O_CREAT, 0600)) != -1) {
		// If I get here I have successfully opened the file.
		
		uint noerror;
		
		uint append = (offset == -1);
		
		if (lseek(
				fd,
				append ? 0 : offset,
				append ? SEEK_END : SEEK_SET) == -1)
			noerror = 0;
		else noerror = 1;
		
		if (noerror) {
			// Null should be returned
			// if all the data could not
			// be written to the file.
			if (write(fd, buf, bufsz) != bufsz) noerror = 0;
			
			if (noerror && (offset != -1)) {
				// If I was not appending data in the file,
				// I truncate it to where the last byte was written.
				if (ftruncate(fd, offset+bufsz) == -1) noerror = 0;
			}
		}
		
		// Close the file.
		close(fd);
		
		return noerror;
		
	} else return 0;
}
