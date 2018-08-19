
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


// These functions find the first occurence
// of the string tosearch in the string s, searching
// from left to right, and return the starting address
// within s of the string found, otherwise null is returned.
// The functions stringsearchright() take into account
// the characters' case while the functions
// stringsearchrightignorecase() do not.

u8* stringsearchright1 (string s, string tosearch) {
	
	uint ssz = s.ptr ? (mmsz(s.ptr)-1) : 0;
	
	if (!ssz) return 0;
	
	uint tosearchsz = tosearch.ptr ? (mmsz(tosearch.ptr)-1) : 0;
	
	if (!tosearchsz) return 0;
	
	while (ssz >= tosearchsz) {
		if (stringiseq4(s.ptr, tosearch.ptr, tosearchsz)) return s.ptr;
		++s.ptr; --ssz;
	}
	
	return 0;
}

u8* stringsearchright2 (string s, u8* tosearch) {
	
	uint ssz = s.ptr ? (mmsz(s.ptr)-1) : 0;
	
	if (!ssz) return 0;
	
	uint tosearchsz = stringsz(tosearch);
	
	if (!tosearchsz) return 0;
	
	while (ssz >= tosearchsz) {
		if (stringiseq4(s.ptr, tosearch, tosearchsz)) return s.ptr;
		++s.ptr; --ssz;
	}
	
	return 0;
}

u8* stringsearchright3 (u8* s, string tosearch) {
	
	uint ssz = stringsz(s);
	
	if (!ssz) return 0;
	
	uint tosearchsz = tosearch.ptr ? (mmsz(tosearch.ptr)-1) : 0;
	
	if (!tosearchsz) return 0;
	
	while (ssz >= tosearchsz) {
		if (stringiseq4(s, tosearch.ptr, tosearchsz)) return s;
		++s; --ssz;
	}
	
	return 0;
}

u8* stringsearchright4 (u8* s, u8* tosearch) {
	
	uint ssz = stringsz(s);
	
	if (!ssz) return 0;
	
	uint tosearchsz = stringsz(tosearch);
	
	if (!tosearchsz) return 0;
	
	while (ssz >= tosearchsz) {
		if (stringiseq4(s, tosearch, tosearchsz)) return s;
		++s; --ssz;
	}
	
	return 0;
}

u8* stringsearchright5 (u8* s, uint ssz, string tosearch) {
	
	if (!ssz) return 0;
	
	uint tosearchsz = tosearch.ptr ? (mmsz(tosearch.ptr)-1) : 0;
	
	if (!tosearchsz) return 0;
	
	while (ssz >= tosearchsz) {
		if (stringiseq4(s, tosearch.ptr, tosearchsz)) return s;
		++s; --ssz;
	}
	
	return 0;
}

u8* stringsearchright6 (u8* s, uint ssz, u8* tosearch) {
	
	if (!ssz) return 0;
	
	uint tosearchsz = stringsz(tosearch);
	
	if (!tosearchsz) return 0;
	
	while (ssz >= tosearchsz) {
		if (stringiseq4(s, tosearch, tosearchsz)) return s;
		++s; --ssz;
	}
	
	return 0;
}

u8* stringsearchright7 (u8* s, uint ssz, u8* tosearch, uint tosearchsz) {
	
	if (!ssz) return 0;
	
	if (!tosearchsz) return 0;
	
	while (ssz >= tosearchsz) {
		if (stringiseq4(s, tosearch, tosearchsz)) return s;
		++s; --ssz;
	}
	
	return 0;
}

u8* stringsearchrightignorecase1 (string s, string tosearch) {
	
	uint ssz = s.ptr ? (mmsz(s.ptr)-1) : 0;
	
	if (!ssz) return 0;
	
	uint tosearchsz = tosearch.ptr ? (mmsz(tosearch.ptr)-1) : 0;
	
	if (!tosearchsz) return 0;
	
	while (ssz >= tosearchsz) {
		if (stringiseqnocase4(s.ptr, tosearch.ptr, tosearchsz)) return s.ptr;
		++s.ptr; --ssz;
	}
	
	return 0;
}

u8* stringsearchrightignorecase2 (string s, u8* tosearch) {
	
	uint ssz = s.ptr ? (mmsz(s.ptr)-1) : 0;
	
	if (!ssz) return 0;
	
	uint tosearchsz = stringsz(tosearch);
	
	if (!tosearchsz) return 0;
	
	while (ssz >= tosearchsz) {
		if (stringiseqnocase4(s.ptr, tosearch, tosearchsz)) return s.ptr;
		++s.ptr; --ssz;
	}
	
	return 0;
}

u8* stringsearchrightignorecase3 (u8* s, string tosearch) {
	
	uint ssz = stringsz(s);
	
	if (!ssz) return 0;
	
	uint tosearchsz = tosearch.ptr ? (mmsz(tosearch.ptr)-1) : 0;
	
	if (!tosearchsz) return 0;
	
	while (ssz >= tosearchsz) {
		if (stringiseqnocase4(s, tosearch.ptr, tosearchsz)) return s;
		++s; --ssz;
	}
	
	return 0;
}

u8* stringsearchrightignorecase4 (u8* s, u8* tosearch) {
	
	uint ssz = stringsz(s);
	
	if (!ssz) return 0;
	
	uint tosearchsz = stringsz(tosearch);
	
	if (!tosearchsz) return 0;
	
	while (ssz >= tosearchsz) {
		if (stringiseqnocase4(s, tosearch, tosearchsz)) return s;
		++s; --ssz;
	}
	
	return 0;
}

u8* stringsearchrightignorecase5 (u8* s, uint ssz, string tosearch) {
	
	if (!ssz) return 0;
	
	uint tosearchsz = tosearch.ptr ? (mmsz(tosearch.ptr)-1) : 0;
	
	if (!tosearchsz) return 0;
	
	while (ssz >= tosearchsz) {
		if (stringiseqnocase4(s, tosearch.ptr, tosearchsz)) return s;
		++s; --ssz;
	}
	
	return 0;
}

u8* stringsearchrightignorecase6 (u8* s, uint ssz, u8* tosearch) {
	
	if (!ssz) return 0;
	
	uint tosearchsz = stringsz(tosearch);
	
	if (!tosearchsz) return 0;
	
	while (ssz >= tosearchsz) {
		if (stringiseqnocase4(s, tosearch, tosearchsz)) return s;
		++s; --ssz;
	}
	
	return 0;
}

u8* stringsearchrightignorecase7 (u8* s, uint ssz, u8* tosearch, uint tosearchsz) {
	
	if (!ssz) return 0;
	
	if (!tosearchsz) return 0;
	
	while (ssz >= tosearchsz) {
		if (stringiseqnocase4(s, tosearch, tosearchsz)) return s;
		++s; --ssz;
	}
	
	return 0;
}


// These functions find the first occurence
// of the string tosearch in the string s, searching
// from right to left, and return the starting address
// within s of the string found, otherwise null is returned.
// The functions stringsearchright() take into account
// the characters' case while the functions
// stringsearchrightignorecase() do not.

u8* stringsearchleft1 (string s, string tosearch) {
	
	uint ssz = s.ptr ? (mmsz(s.ptr)-1) : 0;
	
	if (!ssz) return 0;
	
	uint tosearchsz = tosearch.ptr ? (mmsz(tosearch.ptr)-1) : 0;
	
	if (!tosearchsz) return 0;
	
	if (ssz < tosearchsz) return 0;
	
	u8* sptr = s.ptr + (ssz - tosearchsz);
	
	do {
		if (stringiseq4(sptr, tosearch.ptr, tosearchsz)) return sptr;
		
	} while (--sptr >= s.ptr);
	
	return 0;
}

u8* stringsearchleft2 (string s, u8* tosearch) {
	
	uint ssz = s.ptr ? (mmsz(s.ptr)-1) : 0;
	
	if (!ssz) return 0;
	
	uint tosearchsz = stringsz(tosearch);
	
	if (!tosearchsz) return 0;
	
	if (ssz < tosearchsz) return 0;
	
	u8* sptr = s.ptr + (ssz - tosearchsz);
	
	do {
		if (stringiseq4(sptr, tosearch, tosearchsz)) return sptr;
		
	} while (--sptr >= s.ptr);
	
	return 0;
}

u8* stringsearchleft3 (u8* s, string tosearch) {
	
	uint ssz = stringsz(s);
	
	if (!ssz) return 0;
	
	uint tosearchsz = tosearch.ptr ? (mmsz(tosearch.ptr)-1) : 0;
	
	if (!tosearchsz) return 0;
	
	if (ssz < tosearchsz) return 0;
	
	u8* sptr = s + (ssz - tosearchsz);
	
	do {
		if (stringiseq4(sptr, tosearch.ptr, tosearchsz)) return sptr;
		
	} while (--sptr >= s);
	
	return 0;
}

u8* stringsearchleft4 (u8* s, u8* tosearch) {
	
	uint ssz = stringsz(s);
	
	if (!ssz) return 0;
	
	uint tosearchsz = stringsz(tosearch);
	
	if (!tosearchsz) return 0;
	
	if (ssz < tosearchsz) return 0;
	
	u8* sptr = s + (ssz - tosearchsz);
	
	do {
		if (stringiseq4(sptr, tosearch, tosearchsz)) return sptr;
		
	} while (--sptr >= s);
	
	return 0;
}

u8* stringsearchleft5 (u8* s, uint ssz, string tosearch) {
	
	if (!ssz) return 0;
	
	uint tosearchsz = tosearch.ptr ? (mmsz(tosearch.ptr)-1) : 0;
	
	if (!tosearchsz) return 0;
	
	if (ssz < tosearchsz) return 0;
	
	u8* sptr = s + (ssz - tosearchsz);
	
	do {
		if (stringiseq4(sptr, tosearch.ptr, tosearchsz)) return sptr;
		
	} while (--sptr >= s);
	
	return 0;
}

u8* stringsearchleft6 (u8* s, uint ssz, u8* tosearch) {
	
	if (!ssz) return 0;
	
	uint tosearchsz = stringsz(tosearch);
	
	if (!tosearchsz) return 0;
	
	if (ssz < tosearchsz) return 0;
	
	u8* sptr = s + (ssz - tosearchsz);
	
	do {
		if (stringiseq4(sptr, tosearch, tosearchsz)) return sptr;
		
	} while (--sptr >= s);
	
	return 0;
}

u8* stringsearchleft7 (u8* s, uint ssz, u8* tosearch, uint tosearchsz) {
	
	if (!ssz) return 0;
	
	if (!tosearchsz) return 0;
	
	if (ssz < tosearchsz) return 0;
	
	u8* sptr = s + (ssz - tosearchsz);
	
	do {
		if (stringiseq4(sptr, tosearch, tosearchsz)) return sptr;
		
	} while (--sptr >= s);
	
	return 0;
}

u8* stringsearchleftignorecase1 (string s, string tosearch) {
	
	uint ssz = s.ptr ? (mmsz(s.ptr)-1) : 0;
	
	if (!ssz) return 0;
	
	uint tosearchsz = tosearch.ptr ? (mmsz(tosearch.ptr)-1) : 0;
	
	if (!tosearchsz) return 0;
	
	if (ssz < tosearchsz) return 0;
	
	u8* sptr = s.ptr + (ssz - tosearchsz);
	
	do {
		if (stringiseqnocase4(sptr, tosearch.ptr, tosearchsz)) return sptr;
		
	} while (--sptr >= s.ptr);
	
	return 0;
}

u8* stringsearchleftignorecase2 (string s, u8* tosearch) {
	
	uint ssz = s.ptr ? (mmsz(s.ptr)-1) : 0;
	
	if (!ssz) return 0;
	
	uint tosearchsz = stringsz(tosearch);
	
	if (!tosearchsz) return 0;
	
	if (ssz < tosearchsz) return 0;
	
	u8* sptr = s.ptr + (ssz - tosearchsz);
	
	do {
		if (stringiseqnocase4(sptr, tosearch, tosearchsz)) return sptr;
		
	} while (--sptr >= s.ptr);
	
	return 0;
}

u8* stringsearchleftignorecase3 (u8* s, string tosearch) {
	
	uint ssz = stringsz(s);
	
	if (!ssz) return 0;
	
	uint tosearchsz = tosearch.ptr ? (mmsz(tosearch.ptr)-1) : 0;
	
	if (!tosearchsz) return 0;
	
	if (ssz < tosearchsz) return 0;
	
	u8* sptr = s + (ssz - tosearchsz);
	
	do {
		if (stringiseqnocase4(sptr, tosearch.ptr, tosearchsz)) return sptr;
		
	} while (--sptr >= s);
	
	return 0;
}

u8* stringsearchleftignorecase4 (u8* s, u8* tosearch) {
	
	uint ssz = stringsz(s);
	
	if (!ssz) return 0;
	
	uint tosearchsz = stringsz(tosearch);
	
	if (!tosearchsz) return 0;
	
	if (ssz < tosearchsz) return 0;
	
	u8* sptr = s + (ssz - tosearchsz);
	
	do {
		if (stringiseqnocase4(sptr, tosearch, tosearchsz)) return sptr;
		
	} while (--sptr >= s);
	
	return 0;
}

u8* stringsearchleftignorecase5 (u8* s, uint ssz, string tosearch) {
	
	if (!ssz) return 0;
	
	uint tosearchsz = tosearch.ptr ? (mmsz(tosearch.ptr)-1) : 0;
	
	if (!tosearchsz) return 0;
	
	if (ssz < tosearchsz) return 0;
	
	u8* sptr = s + (ssz - tosearchsz);
	
	do {
		if (stringiseqnocase4(sptr, tosearch.ptr, tosearchsz)) return sptr;
		
	} while (--sptr >= s);
	
	return 0;
}

u8* stringsearchleftignorecase6 (u8* s, uint ssz, u8* tosearch) {
	
	if (!ssz) return 0;
	
	uint tosearchsz = stringsz(tosearch);
	
	if (!tosearchsz) return 0;
	
	if (ssz < tosearchsz) return 0;
	
	u8* sptr = s + (ssz - tosearchsz);
	
	do {
		if (stringiseqnocase4(sptr, tosearch, tosearchsz)) return sptr;
		
	} while (--sptr >= s);
	
	return 0;
}

u8* stringsearchleftignorecase7 (u8* s, uint ssz, u8* tosearch, uint tosearchsz) {
	
	if (!ssz) return 0;
	
	if (!tosearchsz) return 0;
	
	if (ssz < tosearchsz) return 0;
	
	u8* sptr = s + (ssz - tosearchsz);
	
	do {
		if (stringiseqnocase4(sptr, tosearch, tosearchsz)) return sptr;
		
	} while (--sptr >= s);
	
	return 0;
}
