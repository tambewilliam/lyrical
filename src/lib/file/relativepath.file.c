
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


// The function filerelativepath() convert
// an absolute path to a relative path, while
// the function fileabsolutepath() convert
// a relative path to an absolute path.
// Their argument cwd should be an absolute
// path to the current working directory.
// The argument target of filerelativepath()
// should be an absolute path to the target file.
// The argument target of fileabsolutepath()
// should be a relative path to the target file.
// 
// filerelativepath() example:
// cwd:    "/usr/include/kernel/sound"
// target: "/usr/shared/data"
// result: "../../../shared/data"
// 
// fileabsolutepath() example:
// cwd:    "/usr/include/kernel/sound"
// target: "../../../shared/data"
// result: "/usr/shared/data"

string filerelativepath (u8* cwd, u8* target) {
	
	string cwddup = stringduplicate2(cwd);
	
	string targetdup = stringduplicate2(target);
	
	if (cwddup.ptr[stringmmsz(cwddup)-1] != '/') stringappend4(&cwddup, '/');
	
	if (targetdup.ptr[stringmmsz(targetdup)-1] != '/') stringappend4(&targetdup, '/');
	
	uint i;
	
	u8* s = cwddup.ptr;
	// This loop replace all occurrence of "//"
	// by "/" in order to obtain a better result.
	while (i = (uint)stringsearchright4(s, "//")) {
		uint o = i - (uint)cwddup.ptr;
		stringremove(&cwddup, o, 1);
		s = cwddup.ptr + o;
	}
	
	s = targetdup.ptr;
	// This loop replace all occurrence of "//"
	// by "/" in order to obtain a better result.
	while (i = (uint)stringsearchright4(s, "//")) {
		uint o = i - (uint)targetdup.ptr;
		stringremove(&targetdup, o, 1);
		s = targetdup.ptr + o;
	}
	
	s = cwddup.ptr;
	// This loop replace all occurrence of "/./"
	// by "/" in order to obtain a better result.
	while (i = (uint)stringsearchright4(s, "/./")) {
		uint o = i - (uint)cwddup.ptr;
		stringremove(&cwddup, o, 2);
		s = cwddup.ptr + o;
	}
	
	s = targetdup.ptr;
	// This loop replace all occurrence of "/./"
	// by "/" in order to obtain a better result.
	while (i = (uint)stringsearchright4(s, "/./")) {
		uint o = i - (uint)targetdup.ptr;
		stringremove(&targetdup, o, 2);
		s = targetdup.ptr + o;
	}
	
	// String in which will be accumulated
	// the "../" for the relative path and
	// in which will be computed the relative
	// path to return.
	string retvar = stringnull;
	
	// This loop will gradually strip the end
	// of the path in cwddup until the path that
	// is common between cwd and target is found.
	// This loop will also accumulate the "../"
	// for the relative path to the target.
	while (!stringiseq4(cwddup.ptr, targetdup.ptr, stringmmsz(cwddup))) {
		
		string dirname = filedirname(cwddup.ptr);
		
		mmfree(cwddup.ptr);
		
		cwddup = dirname;
		
		// The result of filedirname() is
		// never terminated by '/'; I append '/'
		// to insure that the end of the path
		// in cwddup is well determined when
		// comparing it against the start of
		// the path in targetdup.
		stringappend4(&cwddup, '/');
		
		stringappend2(&retvar, "../");
	}
	
	if (retvar.ptr) stringappend2(&retvar, targetdup.ptr + stringmmsz(cwddup));
	else stringappend4(&retvar, '.');
	
	mmfree(targetdup.ptr);
	
	mmfree(cwddup.ptr);
	
	// If found, I remove "/" from the end of the result.
	if (retvar.ptr[stringmmsz(retvar) -1] == '/')
		stringchop(&retvar, 1);
	
	return retvar;
}
