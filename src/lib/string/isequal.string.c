
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


// These functions return 0 if the strings given
// as argument are not equal, otherwise 1 is returned.
// stringiseq() take into account the characters' case
// while stringiseqnocase() do not.
// Functions for which the last argument is sz compare
// the amount of characters given by the argument sz
// ignoring their null-terminating character.

uint stringiseq1 (string s1, string s2) {
	
	uint s1sz = s1.ptr ? (mmsz(s1.ptr)-1) : 0;
	
	uint s2sz = s2.ptr ? (mmsz(s2.ptr)-1) : 0;
	
	// The size of both strings has to be equal to each other
	// to be the same string even if the sizes are null.
	if (s1sz != s2sz) return 0;
	
	// If s1sz is null then s2sz is also null since
	// I tested above s1sz != s2sz; and I return 1.
	if (!s1sz) return 1;
	
	while (1) {
		
		u8 c = *s1.ptr;
		
		if (c != *s2.ptr) return 0;
		
		// This might look confusing.
		// In fact it is enough to only check
		// whether *s1.ptr == 0 which also imply that
		// *s2.ptr == 0 because right above I tested
		// for *s1.ptr != *s2.ptr .
		if (!c) return 1;
		
		++s1.ptr; ++s2.ptr;
	}
}

uint stringiseq2 (string s1, u8* s2) {
	
	uint s1sz = s1.ptr ? (mmsz(s1.ptr)-1) : 0;
	
	u8 c = *s2;
	
	if (!s1sz) {
		if (c) return 0;
		else return 1;
	} else if (!c) return 0;
	
	while (1) {
		
		if (c != *s1.ptr) return 0;
		
		// This might look confusing.
		// In fact it is enough to only check
		// whether *s2 == 0 which also imply that
		// *s1.ptr == 0 because right above I tested
		// for *s2 != *s1.ptr .
		if (!c) return 1;
		
		++s1.ptr; ++s2;
		
		c = *s2;
	}
}

uint stringiseq3 (u8* s1, u8* s2) {
	
	while (1) {
		
		u8 c = *s1;
		
		if (c != *s2) return 0;
		
		// This might look confusing.
		// In fact it is enough to only check
		// whether *s1 == 0 which also imply that
		// *s2 == 0 because right above I tested
		// for *s1 != *s2
		if (!c) return 1;
		
		++s1; ++s2;
	}
}

uint stringiseq4 (u8* s1, u8* s2, uint sz) {
	// If sz is null I assume both
	// string equal and I return 1.
	if (!sz) return 1;
	
	while (1) {
		
		if (*s1 != *s2) return 0;
		
		if (!--sz) return 1;
		
		++s1; ++s2;
	}
}

uint stringiseq5 (u8* s1, uint s1sz, string s2) {
	
	uint s2sz = s2.ptr ? (mmsz(s2.ptr)-1) : 0;
	
	// The size of both strings has to be equal to each other
	// to be the same string even if the sizes are null.
	if (s1sz != s2sz) return 0;
	
	// If s1sz is null then s2sz is also null since
	// I tested above s1sz != s2sz; and I return 1.
	if (!s1sz) return 1;
	
	while (1) {
		
		u8 c = *s1;
		
		if (c != *s2.ptr) return 0;
		
		if (!--s1sz) return 1;
		
		++s1; ++s2.ptr;
	}
}

uint stringiseq6 (u8* s1, uint s1sz, u8* s2) {
	
	u8 c = *s2;
	
	if (!s1sz) {
		if (c) return 0;
		else return 1;
	} else if (!c) return 0;
	
	while (1) {
		
		if (c != *s1) return 0;
		
		++s1; ++s2;
		
		c = *s2;
		
		if (--s1sz) {
			if (!c) return 0;
		} else if (!c) return 1;
	}
}

uint stringiseq7 (u8* s1, uint s1sz, u8* s2, uint s2sz) {
	// The size of both strings has to be equal to each other
	// to be the same string even if the sizes are null.
	if (s1sz != s2sz) return 0;
	
	// If s1sz is null then s2sz is also null since
	// I tested above s1sz != s2sz; and I return 1.
	if (!s1sz) return 1;
	
	while (1) {
		
		u8 c = *s1;
		
		if (c != *s2) return 0;
		
		if (!--s1sz) return 1;
		
		++s1; ++s2;
	}
}

uint stringiseqnocase1 (string s1, string s2) {
	
	uint s1sz = s1.ptr ? (mmsz(s1.ptr)-1) : 0;
	
	uint s2sz = s2.ptr ? (mmsz(s2.ptr)-1) : 0;
	
	// The size of both strings has to be equal to each other
	// to be the same string even if the sizes are null.
	if (s1sz != s2sz) return 0;
	
	// If s1sz is null then s2sz is also null since
	// I tested above s1sz != s2sz; and I return 1.
	if (!s1sz) return 1;
	
	while (1) {
		
		u8 c1 = *s1.ptr;
		if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
		
		u8 c2 = *s2.ptr;
		if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
		
		if (c1 != c2) return 0;
		
		// This might look confusing.
		// In fact it is enough to only check
		// whether c1 == 0 which also imply that
		// c2 == 0 because right above I tested
		// for c1 != c2 .
		if (!c1) return 1;
		
		++s1.ptr; ++s2.ptr;
	}
}

uint stringiseqnocase2 (string s1, u8* s2) {
	
	uint s1sz = s1.ptr ? (mmsz(s1.ptr)-1) : 0;
	
	u8 c2 = *s2;
	
	if (!s1sz) {
		if (c2) return 0;
		else return 1;
	} else if (!c2) return 0;
	
	while (1) {
		
		if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
		
		u8 c1 = *s1.ptr;
		if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
		
		if (c2 != c1) return 0;
		
		// This might look confusing.
		// In fact it is enough to only check
		// whether c2 == 0 which also imply that
		// c1 == 0 because right above I tested
		// for c2 != c1 .
		if (!c2) return 1;
		
		++s1.ptr; ++s2;
		
		c2 = *s2;
	}
}

uint stringiseqnocase3 (u8* s1, u8* s2) {
	
	while (1) {
		
		u8 c1 = *s1;
		if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
		
		u8 c2 = *s2;
		if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
		
		if (c1 != c2) return 0;
		
		// This might look confusing.
		// In fact it is enough to only check
		// whether c1 == 0 which also imply that
		// c2 == 0 because right above I tested
		// for c1 != c2
		if (!c1) return 1;
		
		++s1; ++s2;
	}
}

uint stringiseqnocase4 (u8* s1, u8* s2, uint sz) {
	// If sz is null I assume both
	// string equal and I return 1.
	if (!sz) return 1;
	
	while (1) {
		
		u8 c1 = *s1;
		if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
		
		u8 c2 = *s2;
		if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
		
		if (c1 != c2) return 0;
		
		if (!--sz) return 1;
		
		++s1; ++s2;
	}
}

uint stringiseqnocase5 (u8* s1, uint s1sz, string s2) {
	
	uint s2sz = s2.ptr ? (mmsz(s2.ptr)-1) : 0;
	
	// The size of both strings has to be equal to each other
	// to be the same string even if the sizes are null.
	if (s1sz != s2sz) return 0;
	
	// If s1sz is null then s2sz is also null since
	// I tested above s1sz != s2sz; and I return 1.
	if (!s1sz) return 1;
	
	while (1) {
		
		u8 c1 = *s1;
		if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
		
		u8 c2 = *s2.ptr;
		if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
		
		if (c1 != c2) return 0;
		
		if (!--s1sz) return 1;
		
		++s1; ++s2.ptr;
	}
}

uint stringiseqnocase6 (u8* s1, uint s1sz, u8* s2) {
	
	u8 c2 = *s2;
	
	if (!s1sz) {
		if (c2) return 0;
		else return 1;
	} else if (!c2) return 0;
	
	while (1) {
		
		if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
		
		u8 c1 = *s1;
		if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
		
		if (c2 != c1) return 0;
		
		++s1; ++s2;
		
		c2 = *s2;
		
		if (--s1sz) {
			if (!c2) return 0;
		} else if (!c2) return 1;
	}
}

uint stringiseqnocase7 (u8* s1, uint s1sz, u8* s2, uint s2sz) {
	// The size of both strings has to be equal to each other
	// to be the same string even if the sizes are null.
	if (s1sz != s2sz) return 0;
	
	// If s1sz is null then s2sz is also null since
	// I tested above s1sz != s2sz; and I return 1.
	if (!s1sz) return 1;
	
	while (1) {
		
		u8 c1 = *s1;
		if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
		
		u8 c2 = *s2;
		if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
		
		if (c1 != c2) return 0;
		
		if (!--s1sz) return 1;
		
		++s1; ++s2;
	}
}
