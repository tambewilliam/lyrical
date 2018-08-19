
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


// This function return whether
// the file do not exist, is a folder,
// regular file, symlink, or other file.
fileisresult fileis1 (u8* path) {
	
	struct stat buf;
	
	if (lstat(path, &buf)) return FILEISNOTEXIST;
	else return (
		S_ISDIR(buf.st_mode) ? FILEISFOLDER :
		S_ISREG(buf.st_mode) ? FILEISREGULAR :
		S_ISLNK(buf.st_mode) ? FILEISSYMLINK :
		FILEISOTHER);
}


// This function return whether
// the file do not exist, is a folder,
// regular file, or other file.
fileisresult fileis2 (u8* path) {
	
	struct stat buf;
	
	if (stat(path, &buf)) return FILEISNOTEXIST;
	else return (
		S_ISDIR(buf.st_mode) ? FILEISFOLDER :
		S_ISREG(buf.st_mode) ? FILEISREGULAR :
		// S_ISLNK(buf.st_mode) ? FILEISSYMLINK : // This case would never be returned since stat() instead of lstat() was used.
		FILEISOTHER);
}
