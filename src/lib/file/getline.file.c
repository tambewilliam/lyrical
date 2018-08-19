
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


// Return a line from the file for which
// the file descriptor is given as argument,
// starting at the offset given as argument.
// stringnull is returned on faillure.
string filegetline1 (uint fd, uint offset) {
	
	if (lseek(fd, offset, SEEK_SET) == -1) return stringnull;
	
	string retvar = stringduplicate2("");
	
	u8 c;
	
	while (read(fd, &c, 1) == 1 && c && c != '\n')
		stringappend4(&retvar, c);
	
	return retvar;
}

// Return a line from the file for which
// the path is given as argument, starting
// at the offset given as argument.
// stringnull is returned on faillure.
string filegetline2 (u8* path, uint offset) {
	
	uint fd = open(path, O_RDONLY);
	
	if (fd == -1) return stringnull;
	
	if (lseek(fd, offset, SEEK_SET) == -1) return stringnull;
	
	string retvar = stringduplicate2("");
	
	u8 c;
	
	while (read(fd, &c, 1) == 1 && c && c != '\n')
		stringappend4(&retvar, c);
	
	close(fd);
	
	return retvar;
}
