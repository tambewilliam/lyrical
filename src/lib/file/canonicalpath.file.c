
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
string filecanonicalpath (u8* path) {
	
	string pathdup = stringduplicate2(path);
	
	uint i;
	
	u8* s = pathdup.ptr;
	// This loop replace all occurrence of "//"
	// by "/" in order to obtain a better result.
	while (i = (uint)stringsearchright4(s, "//")) {
		uint o = i - (uint)pathdup.ptr;
		stringremove(&pathdup, o, 1);
		s = pathdup.ptr + o;
	}
	
	s = pathdup.ptr;
	// This loop replace all occurrence of "/./"
	// by "/" in order to obtain a better result.
	while (i = (uint)stringsearchright4(s, "/./")) {
		uint o = i - (uint)pathdup.ptr;
		stringremove(&pathdup, o, 2);
		s = pathdup.ptr + o;
	}
	
	s = pathdup.ptr;
	// This loop remove all occurrence of "/../"
	// stripping the path level before it.
	while ((i = (uint)stringsearchright4(s, "/../")) ||
		//((i = (uint)stringsearchleft2(pathdup, "/..")) && !((u8*)i)[3])
		// ### The below do the same thing as above.
		(stringiseq3((u8*)(i = (uint)&pathdup.ptr[stringmmsz(pathdup)-3]), "/.."))
		) {
		
		if (i != (uint)pathdup.ptr) {
			
			u8* savedi = (u8*)i;
			
			//if (!(i = (uint)stringsearchleft6(pathdup.ptr, (i-(uint)pathdup.ptr), "/")))
			//	i = (uint)pathdup.ptr;
			// ### The below do the same thing as above.
			i = ((uint)stringsearchleft6(pathdup.ptr, (i-(uint)pathdup.ptr), "/") ?: (uint)pathdup.ptr);
			
			stringremove(&pathdup, (i-(uint)pathdup.ptr), ((uint)savedi+3)-(uint)i);
			
		} else break;
	}
	
	if (!pathdup.ptr[0]) stringappend4(&pathdup, '.');
	else if (pathdup.ptr[0] == '/' && !pathdup.ptr[1])
		pathdup.ptr[0] = '.';
	
	return pathdup;
}
