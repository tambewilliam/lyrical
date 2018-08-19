
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


// The stringconverttoint() functions convert
// a string having an unsigned decimal, hexadecimal,
// octale or binary notation, to an integer.
// They stop at the first invalid character, or
// at the limit set by the argument sz when used.
// When u8** s for the string to convert is used,
// the address where the conversion stopped
// get updated in *s.
// A decimal notation number cannot start with 0.
// Overflow is ignored.
// Here below are examples of strings accepted:
// Decimal: 234
// Hexadecimal: 0xaa13
// Octale: 0o1234
// Binary: 0b101110

uint stringconverttoint1 (u8* s) {
	
	uint retvar = 0;
	
	u8 c = *s;
	
	if (c == '0') {
		
		++s;
		
		c = *s;
		
		if (c == 'x' || c == 'X') {
			// Hexadecimal number.
			
			while (1) {
				
				++s;
				
				c = *s;
				
				if (c >= '0' && c <= '9') {
					
					retvar = (retvar<<4) + (c - '0'); // <<4 does a multiplication by 16.
					
				} else if (c >= 'a' && c <= 'f') {
					
					retvar = (retvar<<4) + ((c - 'a') + 10); // <<4 does a multiplication by 16.
					
				} else if (c >= 'A' && c <= 'F') {
					
					retvar = (retvar<<4) + ((c - 'A') + 10); // <<4 does a multiplication by 16.
					
				} else return retvar;
			}
			
		} else if (c == 'b' || c == 'B') {
			// Binary number.
			
			while (1) {
				
				++s;
				
				c = *s;
				
				if (c == '0' || c == '1') {
					
					retvar = (retvar<<1) + (c - '0'); // <<1 does a multiplication by 2.
					
				} else return retvar;
			}
			
		} else if (c == 'o' || c == 'O') {
			// Octale number.
			
			while (1) {
				
				++s;
				
				c = *s;
				
				if (c >= '0' && c <= '7') {
					
					retvar = (retvar<<3) + (c - '0'); // <<3 does a multiplication by 8.
					
				} else return retvar;
			}
		}
	}
	
	// Decimal number.
	
	while (1) {
		
		if (c >= '0' && c <= '9') {
			
			retvar = (retvar*10) + (c - '0');
			
		} else return retvar;
		
		++s;
		
		c = *s;
	}
}

uint stringconverttoint2 (u8* s, uint sz) {
	
	u8* e = s+sz;
	
	uint retvar = 0;
	
	u8 c = *s;
	
	if (c == '0') {
		
		if (++s >= e) return retvar;
		
		c = *s;
		
		if (c == 'x' || c == 'X') {
			// Hexadecimal number.
			
			while (1) {
				
				if (++s >= e) return retvar;
				
				c = *s;
				
				if (c >= '0' && c <= '9') {
					
					retvar = (retvar<<4) + (c - '0'); // <<4 does a multiplication by 16.
					
				} else if (c >= 'a' && c <= 'f') {
					
					retvar = (retvar<<4) + ((c - 'a') + 10); // <<4 does a multiplication by 16.
					
				} else if (c >= 'A' && c <= 'F') {
					
					retvar = (retvar<<4) + ((c - 'A') + 10); // <<4 does a multiplication by 16.
					
				} else return retvar;
			}
			
		} else if (c == 'b' || c == 'B') {
			// Binary number.
			
			while (1) {
				
				if (++s >= e) return retvar;
				
				c = *s;
				
				if (c == '0' || c == '1') {
					
					retvar = (retvar<<1) + (c - '0'); // <<1 does a multiplication by 2.
					
				} else return retvar;
			}
			
		} else if (c == 'o' || c == 'O') {
			// Octale number.
			
			while (1) {
				
				if (++s >= e) return retvar;
				
				c = *s;
				
				if (c >= '0' && c <= '7') {
					
					retvar = (retvar<<3) + (c - '0'); // <<3 does a multiplication by 8.
					
				} else return retvar;
			}
		}
	}
	
	// Decimal number.
	
	while (1) {
		
		if (c >= '0' && c <= '9') {
			
			retvar = (retvar*10) + (c - '0');
			
		} else return retvar;
		
		if (++s >= e) return retvar;
		
		c = *s;
	}
}

uint stringconverttoint3 (u8** s) {
	
	u8* ss = *s;
	
	uint retvar = 0;
	
	u8 c = *ss;
	
	if (c == '0') {
		
		++ss;
		
		c = *ss;
		
		if (c == 'x' || c == 'X') {
			// Hexadecimal number.
			
			while (1) {
				
				++ss;
				
				c = *ss;
				
				if (c >= '0' && c <= '9') {
					
					retvar = (retvar<<4) + (c - '0'); // <<4 does a multiplication by 16.
					
				} else if (c >= 'a' && c <= 'f') {
					
					retvar = (retvar<<4) + ((c - 'a') + 10); // <<4 does a multiplication by 16.
					
				} else if (c >= 'A' && c <= 'F') {
					
					retvar = (retvar<<4) + ((c - 'A') + 10); // <<4 does a multiplication by 16.
					
				} else break;
			}
			
		} else if (c == 'b' || c == 'B') {
			// Binary number.
			
			while (1) {
				
				++ss;
				
				c = *ss;
				
				if (c == '0' || c == '1') {
					
					retvar = (retvar<<1) + (c - '0'); // <<1 does a multiplication by 2.
					
				} else break;
			}
			
		} else if (c == 'o' || c == 'O') {
			// Octale number.
			
			while (1) {
				
				++ss;
				
				c = *ss;
				
				if (c >= '0' && c <= '7') {
					
					retvar = (retvar<<3) + (c - '0'); // <<3 does a multiplication by 8.
					
				} else break;
			}
		}
	}
	
	// Decimal number.
	
	while (1) {
		
		if (c >= '0' && c <= '9') {
			
			retvar = (retvar*10) + (c - '0');
			
		} else break;
		
		++ss;
		
		c = *ss;
	}
	
	*s = ss;
	
	return retvar;
}

uint stringconverttoint4 (u8** s, uint sz) {
	
	u8* ss = *s;
	
	u8* e = ss+sz;
	
	uint retvar = 0;
	
	u8 c = *ss;
	
	if (c == '0') {
		
		if (++ss >= e) return 0;
		
		c = *ss;
		
		if (c == 'x' || c == 'X') {
			// Hexadecimal number.
			
			while (1) {
				
				if (++ss >= e) break;
				
				c = *ss;
				
				if (c >= '0' && c <= '9') {
					
					retvar = (retvar<<4) + (c - '0'); // <<4 does a multiplication by 16.
					
				} else if (c >= 'a' && c <= 'f') {
					
					retvar = (retvar<<4) + ((c - 'a') + 10); // <<4 does a multiplication by 16.
					
				} else if (c >= 'A' && c <= 'F') {
					
					retvar = (retvar<<4) + ((c - 'A') + 10); // <<4 does a multiplication by 16.
					
				} else break;
			}
			
		} else if (c == 'b' || c == 'B') {
			// Binary number.
			
			while (1) {
				
				if (++ss >= e) break;
				
				c = *ss;
				
				if (c == '0' || c == '1') {
					
					retvar = (retvar<<1) + (c - '0'); // <<1 does a multiplication by 2.
					
				} else break;
			}
			
		} else if (c == 'o' || c == 'O') {
			// Octale number.
			
			while (1) {
				
				if (++ss >= e) break;
				
				c = *ss;
				
				if (c >= '0' && c <= '7') {
					
					retvar = (retvar<<3) + (c - '0'); // <<3 does a multiplication by 8.
					
				} else break;
			}
		}
		
	}
	
	// Decimal number.
	
	while (1) {
		
		if (c >= '0' && c <= '9') {
			
			retvar = (retvar*10) + (c - '0');
			
		} else break;
		
		if (++ss >= e) break;
		
		c = *ss;
	}
	
	*s = ss;
	
	return retvar;
}


// Functions converting their uint argument
// to their corresponding encoding. ei:
// respectively decimal, hexadecimal,
// octale and binary.
// Their argument buf must point to a buffer
// large enough for the targetted encoding.
// They return the number of digits
// that resulted from the conversion.

// The buffer size must be (STRINGMAXDECDIGITS+1).
uint stringconverttodec_ (uint n, u8* buf) {
	
	u8 s[STRINGMAXDECDIGITS];
	
	uint i = STRINGMAXDECDIGITS-1;
	
	while (n >= 10) {
		
		s[i] = (n%10) + '0';
		
		--i;
		
		n /= 10;
	}
	
	*buf = n + '0';
	
	// Compute the number of digits
	// that resulted from the conversion.
	uint retvar = (STRINGMAXDECDIGITS-i);
	
	while (++i < STRINGMAXDECDIGITS)
		*++buf = s[i];
	
	*++buf = 0;
	
	return retvar;
}

// The buffer size must be (STRINGMAXHEXDIGITS+1).
uint stringconverttohex_ (uint n, u8* buf) {
	
	u8 s[STRINGMAXHEXDIGITS+1];
	
	uint i = STRINGMAXHEXDIGITS-1;
	
	while (n >= 16) {
		
		uint nn = n%16;
		
		if (nn < 10) s[i] = nn + '0';
		else s[i] = (nn - 10) + 'a';
		
		--i;
		
		n >>= 4; // Do a division by 16.
	}
	
	if (n < 10) *buf = n + '0';
	else *buf = (n - 10) + 'a';
	
	// Compute the number of digits
	// that resulted from the conversion.
	uint retvar = (STRINGMAXHEXDIGITS-i);
	
	while (++i < STRINGMAXHEXDIGITS)
		*++buf = s[i];
	
	*++buf = 0;
	
	return retvar;
}

// The buffer size must be (STRINGMAXOCTDIGITS+1).
uint stringconverttooct_ (uint n, u8* buf) {
	
	u8 s[STRINGMAXOCTDIGITS+1];
	
	uint i = STRINGMAXOCTDIGITS-1;
	
	while (n >= 8) {
		
		s[i] = (n%8) + '0';
		
		--i;
		
		n >>= 3; // Do a division by 8.
	}
	
	*buf = n + '0';
	
	// Compute the number of digits
	// that resulted from the conversion.
	uint retvar = (STRINGMAXOCTDIGITS-i);
	
	while (++i < STRINGMAXOCTDIGITS)
		*++buf = s[i];
	
	*++buf = 0;
	
	return retvar;
}

// The buffer size must be (STRINGMAXBINDIGITS+1).
uint stringconverttobin_ (uint n, u8* buf) {
	
	u8 s[STRINGMAXBINDIGITS+1];
	
	uint i = STRINGMAXBINDIGITS-1;
	
	while (n >= 2) {
		
		s[i] = (n%2) + '0';
		
		--i;
		
		n >>= 1; // Do a division by 2.
	}
	
	*buf = n + '0';
	
	// Compute the number of digits
	// that resulted from the conversion.
	uint retvar = (STRINGMAXBINDIGITS-i);
	
	while (++i < STRINGMAXBINDIGITS)
		*++buf = s[i];
	
	*++buf = 0;
	
	return retvar;
}


// Functions converting their BIGINTTYPE argument
// to their corresponding encoding. ei:
// respectively decimal, hexadecimal,
// octale and binary.

string stringconverttodec (uint n) {
	
	u8 s[STRINGMAXDECDIGITS];
	
	uint i = STRINGMAXDECDIGITS-1;
	
	while (n >= 10) {
		
		s[i] = (n%10) + '0';
		
		--i;
		
		n /= 10;
	}
	
	// +1 account for the null terminating character.
	string retvar = {.ptr = mmalloc((STRINGMAXDECDIGITS-i)+1)};
	
	u8* ptr = retvar.ptr;
	
	*ptr = n + '0';
	
	while (++i < STRINGMAXDECDIGITS)
		*++ptr = s[i];
	
	*++ptr = 0;
	
	return retvar;
}

string stringconverttohex (uint n) {
	
	u8 s[STRINGMAXHEXDIGITS+1];
	
	uint i = STRINGMAXHEXDIGITS-1;
	
	while (n >= 16) {
		
		uint nn = n%16;
		
		if (nn < 10) s[i] = nn + '0';
		else s[i] = (nn - 10) + 'a';
		
		--i;
		
		n >>= 4; // Do a division by 16.
	}
	
	// +1 account for the null terminating character.
	string retvar = {.ptr = mmalloc((STRINGMAXHEXDIGITS-i)+1)};
	
	u8* ptr = retvar.ptr;
	
	if (n < 10) *ptr = n + '0';
	else *ptr = (n - 10) + 'a';
	
	while (++i < STRINGMAXHEXDIGITS)
		*++ptr = s[i];
	
	*++ptr = 0;
	
	return retvar;
}

string stringconverttooct (uint n) {
	
	u8 s[STRINGMAXOCTDIGITS+1];
	
	uint i = STRINGMAXOCTDIGITS-1;
	
	while (n >= 8) {
		
		s[i] = (n%8) + '0';
		
		--i;
		
		n >>= 3; // Do a division by 8.
	}
	
	// +1 account for the null terminating character.
	string retvar = {.ptr = mmalloc((STRINGMAXOCTDIGITS-i)+1)};
	
	u8* ptr = retvar.ptr;
	
	*ptr = n + '0';
	
	while (++i < STRINGMAXOCTDIGITS)
		*++ptr = s[i];
	
	*++ptr = 0;
	
	return retvar;
}

string stringconverttobin (uint n) {
	
	u8 s[STRINGMAXBINDIGITS+1];
	
	uint i = STRINGMAXBINDIGITS-1;
	
	while (n >= 2) {
		
		s[i] = (n%2) + '0';
		
		--i;
		
		n >>= 1; // Do a division by 2.
	}
	
	// +1 account for the null terminating character.
	string retvar = {.ptr = mmalloc((STRINGMAXBINDIGITS-i)+1)};
	
	u8* ptr = retvar.ptr;
	
	*ptr = n + '0';
	
	while (++i < STRINGMAXBINDIGITS)
		*++ptr = s[i];
	
	*++ptr = 0;
	
	return retvar;
}
