
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


// Transform a pattern to a set of tokens.
// The argument pattern should be valid and non-null.
// Null is returned if the pattern is incorrect.
pamsyntokenized pamsyntokenize (u8* pattern) {
	// TODO: Insure and comment that the field token.next
	// always point to a token that come later
	// in the tokenlist, because pamsynsearch()
	// has a function dup() which always
	// assume that token.next always point
	// to a token that come later.
	
	// Label to return to
	// when an error is thrown.
	__label__ error;
	
	// Used to throw an error.
	// Using a function-call instead of
	// a mere goto allow for backtracing
	// where the error occured.
	void throwerror () {
		goto error;
	}
	
	// Current position in the pattern string
	// initialized to the beginning of the string.
	u8* curpos = pattern;
	
	// Variables used to parse and create
	// the repeat sequences of tokens.
	uint* seq = 0;
	uint seqsz;
	
	// Variable used to save the parsed
	// id of a tokencompare block.
	uint tokcmpid = 0;
	
	// Used to save flags accross braces and brackets.
	
	// Flags actually applied to tokens.
	struct tokenflags flags = {
		.isnot = 0,
		.ishidden = 0,
		.isignorecharcase = 0,
		.isnochar = 0,
		.isfirst = 0,
		.islast = 0,
		.isany = 0,
		.ischarset = 0
	};
	
	// Used to temporarily set flags which will be set
	// later in the variable flags. Here I initialize it
	// to flags which was already initialized above.
	struct tokenflags pendingflags = flags;
	
	// This function return (pendingflags != flags).
	uint ispendingflags () {
		
		if (	pendingflags.isnot != flags.isnot ||
			pendingflags.ishidden != flags.ishidden ||
			pendingflags.isignorecharcase != flags.isignorecharcase ||
			pendingflags.isnochar != flags.isnochar ||
			pendingflags.isfirst != flags.isfirst ||
			pendingflags.islast != flags.islast ||
			pendingflags.isany != flags.isany ||
			pendingflags.ischarset != flags.ischarset)
			return 1;
		else 	return 0;
	}
	
	// Array of pointers to token.
	token** tokenlist = (token**)0;
	
	// Number of pointers to token in tokenlist.
	uint tokenlistsz = 0;
	
	// Array of pointers to token used as a stack.
	// It is only meant to hold the same pointers
	// found in tokenlist.
	token** tokenstack = 0;
	
	// Number of pointers to token in tokenstack.
	uint tokenstacksz = 0;
	
	// push() is called only if there is
	// at least one token in tokenlist.
	// It push the last token created
	// in tokenlist into the tokenstack.
	void push () {
		
		uint tokenstacksz1 = tokenstacksz+1;
		
		// Make space for the address of a new token in the stack.
		tokenstack = mmrealloc(tokenstack, tokenstacksz1*sizeof(token*));
		
		// Save the address in the stack.
		tokenstack[tokenstacksz] = tokenlist[tokenlistsz-1];
		
		tokenstacksz = tokenstacksz1;
	}
	
	// pop() must be called only if there is at least
	// the count of tokens given as argument in tokenstack.
	// It remove the last n tokens pushed in the stack.
	void pop (uint n) {
		
		tokenstacksz -= n;
		
		tokenstack = mmrealloc(tokenstack, tokenstacksz*sizeof(token*));
	}
	
	// Create a new token and save address in tokenlist.
	void new () {
		
		uint tokenlistsz1 = tokenlistsz+1;
		
		// Increase the size of the tokenlist array of pointers.
		tokenlist = mmrealloc(tokenlist, tokenlistsz1*sizeof(token*));
		
		// Allocate a new token.
		token* t = mmalloc(sizeof(token));
		
		// Add the new token to tokenlist.
		tokenlist[tokenlistsz] = t;
		
		// It is not necessary to set the field holder because
		// that address can change every time when tokenlist
		// is re-allocated. It will be set later.
		// t->holder = (token**)0;
		
		t->str.ptr = 0;
		t->str.sz = 0;
		
		// I set the field flags to the variable flags
		// which contain the current flags in effect.
		t->flags = flags;
		
		t->alt = 0;
		
		// It is safe not to set to zero the prev
		// and next field in order to gain speed;
		// they will be set when needed.
		// t->prev = 0;
		// t->next = 0;
		
		t->head = 0;
		
		t->repeat.seq = 0;
		
		t->tokcmpid = 0;
		
		// For speed, it is safe not to set to zero
		// the fields seqsz, sel and count.
		// t->repeat.seqsz = 0;
		// t->repeat.sel = 0;
		// t->repeat.count = 0;
		
		tokenlistsz = tokenlistsz1;
	}
	
	// If the address of the given token is found in tokenlist,
	// it is moved to the end of tokenlist.
	void mklast (token* t) {
		
		uint i = 0;
		
		uint j = tokenlistsz-1;
		
		while (i < j) {
			
			if (tokenlist[i] == t) {
				// This loop below will shift the other addresses up.
				do tokenlist[i] = tokenlist[i+1]; while (++i < j);
				
				// Save the address of the token found
				// as the last token in tokenlist.
				tokenlist[j] = t;
				
				return;
			}
			
			++i;
		}
	}
	
	// Add the character at curpos to the last token
	// created in tokenlist. curpos is modified.
	void addchar () {
		
		u8 c;
		
		if ((c = *curpos) == '\\') {
			// If I get here, I parse an escaped character.
			
			++curpos;
			
			u8 c0 = curpos[0];
			u8 c1 = curpos[1];
			
			// Make the hexadecimal digits case-insensitive.
			if (c0 >= 'A' && c0 <= 'Z') c0 += ('a' - 'A');
			if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
			
			// Check if the escape was being done using a two digits hexadecimal number.
			if (((c0 >= '0' && c0 <= '9') || (c0 >= 'a' && c0 <= 'f')) &&
				((c1 >= '0' && c1 <= '9') || (c1 >= 'a' && c1 <= 'f'))) {
				
				// Convert the first digit.
				if (c0 >= '0' && c0 <= '9')
					c = (c0 - '0');
				else c = ((c0 - 'a') + 10);
				
				// Convert the second digit.
				if (c1 >= '0' && c1 <= '9')
					c = ((c<<4) + (c1 - '0'));
				else c = ((c<<4) + ((c1 - 'a') + 10));
				
				curpos += 2; // Set curpos after the two hexadecimal digits of the escape.
				
			} else if ((c = *curpos) == 'n') {
				
				c = '\n';
				
				++curpos; // Set curpos after the character read.
				
			} else if (c == 't') {
				
				c = '\t';
				
				++curpos; // Set curpos after the character read.
				
			} else if (c == '\\' || ismetachar(c)) {
				
				++curpos; // Set curpos after the character read.
				
			} else throwerror();
			
		} else if (ismetachar(c)) throwerror();
		else ++curpos;
		
		// The character read above
		// is added to the token.str .
		
		token* t = tokenlist[tokenlistsz-1];
		
		uint tstrsz = t->str.sz;
		uint tstrsz1 = tstrsz+1;
		
		u8* tstrptr = mmrealloc(t->str.ptr, tstrsz1);
		tstrptr[tstrsz] = c;
		t->str.sz = tstrsz1;
		t->str.ptr = tstrptr;
	}
	
	// Read a string representing an
	// unsigned decimal integer value.
	uint readnum () {
		
		u8 c = *curpos;
		
		uint i = 0;
		
		while (c >= '0' && c <= '9') {
			// There is no overflow check.
			// In fact the user should be responsible
			// for insuring that numbers are not huge
			// to the point that they overflow.
			i = (i*10) + (c-'0');
			
			c = *++curpos;
		}
		
		return i;
	}
	
	// I create a new session which will help regaining
	// any allocated memory block if an error is thrown.
	mmsession tokenizesession = mmsessionnew();
	
	// It is important to create a first token in the array
	// which is not really used for any metacharater but will
	// receive the first characters read.
	// It is mostly used by subsequent created tokens, since
	// they need to know what was the previously created token.
	new();
	
	// Variables that will be used to respectively cache
	// tokenlist[tokenlistsz-1], tokenlist[tokenlistsz-2].
	token* tl1;
	//token* tl2;
	
	// Variables that will be used to respectively cache
	// tokenstack[tokenstacksz-1], tokenstack[tokenstacksz-2],
	// tokenstack[tokenstacksz-3] .
	token* ts1;
	token* ts2;
	token* ts3;
	
	// Variable that will be used to cache tl1->str.ptr .
	u8* tl1strptr;
	
	// Variable that will be used to cache tl1->str.sz .
	uint tl1strsz;
	
	keepparsingpattern:;
	
	// Variable that will be used to cache *curpos.
	u8 c;
	
	// Variable that will be used to cache *(curpos-1).
	u8 cm1;
	
	if (!(c = *curpos)) goto doneparsingpattern;
	
	switch (c) {
		
		case '?':
			
			if (ispendingflags()) throwerror();
			
			// Create a new token and set the field next of the token
			// that was previously created, to the newly created token.
			new(); tl1 = tokenlist[tokenlistsz-1];
			tokenlist[tokenlistsz-2]->next = tl1;
			
			tl1strsz = tl1->str.sz;
			
			// Look for any additional metachar '?' and increase the size of the string associated
			// with the token accordingly. The field ptr of the string stay null.
			do ++tl1strsz; while (*++curpos == '?');
			
			tl1->str.sz = tl1strsz;
			
			// Create a new token and point the field next of the token that
			// was previously created, to the token that I just created.
			// It is important to do this in order for pamsynmatch1() to work correctly.
			// Because if I don't do so and a regular char is parsed, it will be
			// added to the string associated to this token as opposed to a new token,
			// and pamsynmatch1() will give a wrong result.
			new();
			tokenlist[tokenlistsz-2]->next = tokenlist[tokenlistsz-1];
			
			goto keepparsingpattern;
			
		case '*':
			
			if (ispendingflags()) throwerror();
			
			// Create a new token and set the field next of the token that
			// was previously created, to the token that I just created.
			new(); tl1 = tokenlist[tokenlistsz-1];
			tokenlist[tokenlistsz-2]->next = tl1;
			
			// Mark it as a variadic metachar '?' .
			tl1->flags.isany = 1;
			
			// Eat up any extras '*' found, since **** is equivalent to a single *
			while (*++curpos == '*');
			
			// Create a new token and point the field next of the token that
			// was previously created, to the token that I just created.
			// It is important to do this in order for pamsynmatch1() to work correctly.
			// Because if I don't do so and a regular char is parsed, it will be
			// added to the string associated to this token as opposed to a new token,
			// and pamsynmatch1() will give a wrong result.
			new();
			tokenlist[tokenlistsz-2]->next = tokenlist[tokenlistsz-1];
			
			goto keepparsingpattern;
			
		case '!':
		case '#':
		case '_':
			// I set the bit which will be later set in
			// the variable flag by the openbrace case.
			if (c == '!') pendingflags.isnot = 1;
			else if (c == '#') pendingflags.ishidden = 1;
			else pendingflags.isignorecharcase = 1;
			
			++curpos;
			
			goto keepparsingpattern;
			
		case '%':
			
			if (ispendingflags()) throwerror();
			
			if ((c = *++curpos) == '[' || c == '{') tokcmpid = -1;
			else {
				// readnum() modify curpos.
				tokcmpid = readnum();
				
				// The tokencompare id cannot be null
				// because (tokcmpid == 0) mean
				// there was no tokencompare parsed.
				if (!tokcmpid || ((c = *curpos) != '[' && c != '{'))
					throwerror();
			}
			
			// If I am creating a tokencompare block within
			// another tokencompare block having the same id,
			// I thrown an error because it is a syntax error
			// to nest tokencompare blocks that have the same id.
			
			uint i = 0;
			
			while (i < tokenstacksz) {
				
				if (tokenstack[i]->tokcmpid == tokcmpid)
					throwerror();
				
				++i;
			}
			
			if (c == '[') goto caseopenbracket;
			else goto caseopenbrace;
			
		case '+':;
			
			if (ispendingflags()) throwerror();
			
			seqsz = 0; // Keep track of the number of integers allocated and saved in seq.
			
			// It is not necessary to set seq to 0, because
			// it will be already null when I get here.
			
			seqparsing:
			
			++curpos;
			
			seq = mmrealloc(seq, (seqsz+2)*sizeof(uint));
			seq[seqsz] = 0; seq[seqsz+1] = 0; // initialise to zero.
			
			// readnum() modify curpos.
			uint n = readnum();
			
			if ((c = *curpos) == '-' || (c == '*' && *(curpos+1) == '-') ) {
				// This test below make a difference
				// between reading zero and not reading
				// anything at all. Since readnum
				// return zero for both cases.
				if (n == 0 && *(curpos-1) != '0') {
					// If I get here then I must expect '*' at
					// the location pointed by curpos, otherwise
					// it is a syntax error.
					if (c != '*') throwerror();
					
					seq[seqsz] = -1;
					
					// Advance to '-' .
					++curpos;
					
				} else seq[seqsz] = n;
				
				++curpos; // Advance after '-' .
				
				n = readnum();
				
				// This test below make a difference
				// between reading zero and not reading
				// anything at all. Since readnum
				// return zero for both cases.
				if (n == 0 && *(curpos-1) != '0') {
					// If I get here then I must expect '*' at
					// the location pointed by curpos, otherwise
					// it is a syntax error.
					if (*curpos != '*') throwerror();
					
					seq[seqsz+1] = -1;
					
					++curpos;
					
				} else seq[seqsz+1] = n;
				
			// The case below should come before if (*curpos == '*')
			// This way, I can correctly treat cases such as +10*{a} which is a wrong syntax.
			} else if (n || (cm1 = *(curpos-1)) == '0') {
				// If I get here it mean that it was a number without '-' following it.
				seq[seqsz] = n;
				seq[seqsz+1] = n;
				
			} else if (c == '*') {
				// If I get here it mean that it was an asterix.
				seq[seqsz] = -1;
				seq[seqsz+1] = -1;
				
				++curpos;
				
			} else if ((c == '[' || c == '{') && cm1 == '+') {
				// This will take care of cases such as +{a,b} .
				seq[seqsz] = 1;
				seq[seqsz+1] = -1;
				
				// Since I have an opening brace
				// I can directly jump to caseopenbrace.
				// I update seqsz with the number of element
				// allocated before jumping to caseopenbrace.
				seqsz += 2;
				
				if (c == '[') goto caseopenbracket;
				else goto caseopenbrace;
				
			} else throwerror();
			
			seqsz += 2; // I update seqsz with the number of element allocated and saved in seq.
			
			// If I get here, I should have ',' or '{' .
			
			if ((c = *curpos) == ',') goto seqparsing;
			
			if (c == '[') goto caseopenbracket;
			
			if (c == '{') goto caseopenbrace;
			
			// If I get here, I have a syntax error.
			throwerror();
			
		case '[':
			
			caseopenbracket:
			
			if (!*++curpos) throwerror();
			
			if (*curpos == ']') {
				
				++curpos;
				
				goto keepparsingpattern;
			}
			
			// create and push the token that will be after the closing bracket.
			// This token will save the flag that I had before the opening bracket
			// in case I had something like ![ or #[ or _[ . This way I can recover
			// the flag that I had before something like ![ or #[ or _[ .
			new(); push();
			
			// Set the flags with which the next tokens will
			// be created and keep pendingflags unchanged.
			flags = pendingflags;
			
			// Create first token which is inside
			// the brackets and push it in the stack.
			// It will hold informations for a repetition
			// or tokencompare block if any.
			new(); push(); tl1 = tokenlist[tokenlistsz-1];
			
			if (seq) { // Check if there is any repetition to attach.
				
				tl1->repeat.seq = seq;
				tl1->repeat.seqsz = seqsz;
				
				// Reset seq so it do not get re-used.
				seq = 0;
				
				// It is safe not to reset seqsz because
				// it is used only when seq is non-null.
				
			} else if (tokcmpid) { // Check if there is a tokencompare id to set.
				
				tl1->tokcmpid = tokcmpid;
				
				// Reset tokcmpid so it do not get re-used.
				tokcmpid = 0;
			}
			
			// I set the field next of the token which terminated before
			// the left bracket, so that it point to the first token of
			// the bracket that I just created.
			tokenlist[tokenlistsz-3]->next = tl1;
			
			// It is important for the token holding repeating or tokencompare
			// informations not to have its field ischarset set because
			// pamsynmatch1() assume that a token holding such informations
			// do not have its field ischarset set.
			new(); tl1 = tokenlist[tokenlistsz-1];
			
			// Set the field next of the token that was
			// previously created, to the token that
			// I just created.
			tokenlist[tokenlistsz-2]->next = tl1;
			
			// When the field ischarset of a token is set, I should have at least a single
			// set of 2 characters pointed by the field str.ptr of the token.
			// When I get here, I am certain that I will have a character within the bracket.
			tl1->flags.ischarset = 1;
			
			// Parse the bracket until I encounter
			// the closing bracket or a null char.
			do {
				// If '-' is found, then it is
				// being incorrectly used.
				if (*curpos == '-') throwerror();
				
				// Add the current character at curpos in the field str.
				// I have already tested that the current character was not null.
				addchar();
				
				// Implement range when
				// using '-' inside brackets
				if (*curpos == '-') {
					
					++curpos;
					
					// If a null char or ']' is found,
					// then the metacharacter '-' is
					// being incorrectly used.
					if (!(c = *curpos) || c == ']') throwerror();
					
					addchar();
					
					tl1strptr = tl1->str.ptr;
					
					tl1strsz = tl1->str.sz;
					
					uint j = tl1strsz-2;
					uint k = tl1strsz-1;
					
					u8 tl1strptrj = tl1strptr[j];
					u8 tl1strptrk = tl1strptr[k];
					
					// I need to have a lower char come first in the string
					if (tl1strptrj > tl1strptrk) {
						tl1strptr[j] = tl1strptrk;
						tl1strptr[k] = tl1strptrj;
					}
					
				} else {
					// If I get here I duplicate the
					// last character added to the token.
					
					tl1strptr = tl1->str.ptr;
					
					tl1strsz = tl1->str.sz;
					
					uint tl1strsz1 = tl1strsz+1;
					
					tl1strptr = mmrealloc(tl1strptr, tl1strsz1);
					
					tl1strptr[tl1strsz] = tl1strptr[tl1strsz-1];
					
					tl1->str.sz = tl1strsz1;
					
					tl1->str.ptr = tl1strptr;
				}
				
			// I keep processing only if there is
			// no closing bracket or null char.
			} while ((c = *curpos) != ']' && c);
			
			// The string of set of 2 characters
			// is terminated by a null char;
			// and the field sz set to 1.
			tl1->str.ptr[tl1->str.sz] = 0;
			tl1->str.sz = 1;
			
			if (!*curpos) throwerror();
			
			++curpos; // Advance after ']'.
			
			// Create a new token and point the field next of the token that
			// was previously created, to the token that I just created.
			// It is important to do this in order for pamsynmatch1() to work correctly.
			// Because if I don't do so and a regular char is parsed, it will be
			// added to the string associated to this token as opposed to a new token,
			// and pamsynmatch1() will give a wrong result.
			new(); tl1 = tokenlist[tokenlistsz-1];
			tokenlist[tokenlistsz-2]->next = tl1;
			
			// Update the field next of the last created token in tokenlist
			// so that it point to the token that is supposed to be seen
			// after the closing bracket and which address was previously
			// pushed in the stack.
			tl1->next = (ts2 = tokenstack[tokenstacksz-2]);
			
			ts1 = tokenstack[tokenstacksz-1];
			
			// I update the field head if necessary.
			if (ts1->repeat.seq || ts1->tokcmpid)
				tl1->head = ts1;
			
			// I now move the token that is supposed to be after the closing
			// bracket and make it the last created token of tokenlist.
			mklast(ts2);
			
			// Set the flags back to what
			// it was before the opening bracket.
			flags = ts2->flags;
			pendingflags = flags;
			
			// Pop the token previously pushed.
			pop(2);
			
			goto keepparsingpattern;
			
		case '-':
		case ']':
			// If I parse the hyphen or
			// closing bracket, then I am outside
			// of any opened brackets; I throw
			// an error because the hyphen or
			// closing bracket are metacharacters
			// to be used after the opening bracket
			// has been parsed.
			throwerror();
			
		case '{':
			
			caseopenbrace:
			
			if (!*++curpos) throwerror();
			
			// create and push the token that will be after the closing brace
			// This token will save the flag that I had before the opening brace
			// in case I had something like !{ or #{ or _{ . This way I can recover
			// the flag that I had before something like !{ or #{ or _{ .
			new(); push();
			
			// Set the flags with which the next tokens will
			// be created and keep pendingflags unchanged.
			flags = pendingflags;
			
			// Create first token which is inside
			// the braces and push it in the stack.
			// It will hold informations for a repetition
			// or tokencompare block if any.
			new(); push(); tl1 = tokenlist[tokenlistsz-1];
			
			if (seq) { // Check if there is any repetition to attach.
				
				tl1->repeat.seq = seq;
				tl1->repeat.seqsz = seqsz;
				
				// Reset seq so it do not get re-used.
				seq = 0;
				
				// It is safe not to reset seqsz because
				// it is used only when seq is non-null.
				
			} else if (tokcmpid) { // Check if there is a tokencompare id to set.
				
				tl1->tokcmpid = tokcmpid;
				
				// Reset tokcmpid so it do not get re-used.
				tokcmpid = 0;
			}
			
			// I set the field next of the token which terminated before
			// the left brace, so that it point to the first token of
			// the brace that I just created.
			tokenlist[tokenlistsz-3]->next = tl1;
			
			// It is important for the token holding repeating or tokencompare
			// informations not to have an alternate token because pamsynmatch1()
			// assume that a token holding such informations never has
			// an alternative token.
			// So here I create another token and push it in the stack;
			// it will be used to hold the address to an alternate token.
			new(); push();
			
			// Set the field next of the token that was
			// previously created, to the token that
			// I just created.
			tokenlist[tokenlistsz-2]->next = tokenlist[tokenlistsz-1];
			
			goto keepparsingpattern;
			
		case ',':
		case '}':
			
			if (ispendingflags()) throwerror();
			
			// If tokenstacksz is non-null, then there is something
			// in the stack and it mean that at least one brace was open.
			// If tokenstacksz is null, then I am outside of any opened
			// braces and an error should be thrown.
			if (!tokenstacksz) throwerror();
			
			if (*curpos == ',') {
				// Update the field next of the last created token in tokenlist
				// so that it point to the token that is supposed to be seen
				// after the closing brace and which address was previously
				// pushed in the stack.
				tokenlist[tokenlistsz-1]->next = tokenstack[tokenstacksz-3];
				
				// Create new token and update the field alt of the token
				// previously pushed in the stack to the newly created token.
				new(); tl1 = tokenlist[tokenlistsz-1];
				tokenstack[tokenstacksz-1]->alt = tl1;
				// Update the address of the token used to link alternate
				// tokens in the stack so it point to the newly created token
				// since it will be used for other alternate tokens.
				tokenstack[tokenstacksz-1] = tl1;
				
			} else if (*curpos == '}') {
				// Create a new token and point the field next of the token
				// that was previously created, to the token that I just created.
				// It is important to do this because the token for which
				// the field head is to be set should not have had any string
				// associated with it, because the string will end up be
				// ignored by pamsynmatch1().
				new(); tl1 = tokenlist[tokenlistsz-1];
				tokenlist[tokenlistsz-2]->next = tl1;
				
				// Update the field next of the last created token in tokenlist
				// so that it point to the token that is supposed to be seen
				// after the closing brace and which address was previously
				// pushed in the stack.
				tl1->next = (ts3 = tokenstack[tokenstacksz-3]);
				
				ts2 = tokenstack[tokenstacksz-2];
				
				// I update the field head if necessary.
				if (ts2->repeat.seq || ts2->tokcmpid)
					tl1->head = ts2;
				
				// I now move the token that is supposed to be after the closing
				// brace and make it the last created token of tokenlist.
				mklast(ts3);
				
				// Set the flags back to what it was before the opening brace.
				flags = ts3->flags;
				pendingflags = flags;
				
				// Pop the three tokens previously pushed.
				pop(3);
			}
			
			++curpos;
			
			goto keepparsingpattern;
			
		case '<':
		case '>':
		case '.':
			
			if (ispendingflags()) throwerror();
			
			// Create a new token and set the field next of the token that
			// was previously created, to the token that I just created.
			new(); tl1 = tokenlist[tokenlistsz-1];
			tokenlist[tokenlistsz-2]->next = tl1;
			
			// Mark it as representing '<' or '>' or '.' .
			if (*curpos == '<') tl1->flags.isfirst = 1;
			else if (*curpos == '>') tl1->flags.islast = 1;
			else tl1->flags.isnochar = 1;
			
			// Eat up any similar metachar. It is important to do this
			// for the metachar '.' otherwise the function send()
			// within pamsynmatch1() will not work correctly.
			u8 metarchar = *curpos;
			while (*++curpos == metarchar);
			
			// Create a new token and point the field next of the token
			// that was previously created, to the token that I just created.
			// It is important to do this for cmp() in pamsynmatch1()
			// to work correctly.
			// If I don't do so and a regular char come along in the partern,
			// it will be added to field str of this token and pamsynmatch1()
			// will give a wrong result.
			new();
			tokenlist[tokenlistsz-2]->next = tokenlist[tokenlistsz-1];
			
			goto keepparsingpattern;
			
		default: // Case for non-metacharacters.
			
			if (ispendingflags()) throwerror();
			
			// Add the character at the position in curpos
			// to the string of the last created token.
			addchar();
			
			goto keepparsingpattern;
	}
	
	doneparsingpattern:
	
	// In case of a sudden end of pattern, an error is thrown.
	if (tokenstacksz) throwerror();
	
	// Note that even when tokenstacksz is null,
	// tokenstack can point to a valid memory block
	// if pop() was called at least once, because
	// mmrealloc() used within pop() return a valid
	// memory block even when its argument size is null.
	if (tokenstack) mmfree(tokenstack);
	
	// This need to come very last, before tokenlist
	// is returned; the field holder of every token
	// in tokenlist need to be set to tokenlist.
	
	// The field prev of the first token in the array
	// and the field next of the last token in the array
	// need to be set to null.
	// It is important since the function new() do not
	// initialize the fields prev and next; and they need
	// to be set for pamsynmatch1() to work correctly.
	tokenlist[0]->prev = 0;
	tokenlist[tokenlistsz-1]->next = 0;
	
	// I can use tokenlistsz as the counter of this loop
	// since I am not going to use it again.
	do tokenlist[--tokenlistsz]->holder = tokenlist;
		while (tokenlistsz);
	
	// I cancel the session that I created for regaining
	// any allocated memory in the event of a thrown error.
	mmsessioncancel(tokenizesession, MMDOSUBSESSIONS);
	
	return ((pamsyntokenized){.tokenlist = tokenlist});
	
	// I jump here in the event of an error.
	error:
	
	// Since an error was thrown, I free any memory block
	// that may have been allocated within the memory session
	// that I created before making any allocation.
	mmsessionfree(tokenizesession, MMDOSUBSESSIONS);
	
	return ((pamsyntokenized){.tokenlist = (token**)0});
}
