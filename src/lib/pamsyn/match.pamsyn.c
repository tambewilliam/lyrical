
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


// This function search for a match within the memory
// region given by the arguments start and size, and pass
// a match found in the form of a pamsynmatchfound to the
// specified callback function everytime a match is found.
// Note that it can happen that a null pointer is passed
// to the callback function, and that happen when a match
// occured but there was no non-hidden character in the match;
// so it is important for the callback function to make sure
// that its argument is not a null pointer.
// - When the callback function return 0,
// this function stop looking for matches.
// - When the callback function returns 1, this function
// look for the next match starting after the previous match.
// If the match found was a nochar, this function will ignore
// the return value of the callback function and perform
// a slow lookup as described below; this is because
// nochar match have no size.
// - when the callback function returns >1, this function
// look for the next match as if it didn't find a match, meaning
// without positioning itself after the last character of
// the previous match.
// The callback function pointer must be non-null and valid.
// The argument size cannot be null.
// The callback function must not attempt to free
// the pamsynmatchfound object that it receive as it will be
// done by pamsynmatch1() upon the callback function returning.
void pamsynmatch1 (pamsyntokenized t, u8* start, uint size, uint (*callback)(pamsynmatchfound*)) {
	
	if (!t.tokenlist) return;
	
	// Variable to keep starting and current position
	// within the area I am matching against.
	u8* startpos;
	u8* curpos;
	
	// Keep the address of the current token being processed.
	token* curtoken;
	
	// Keep the addresses of created duplicates.
	// Those duplicates are pointers to array of pointers
	// to tokens. It is necessary to initialize it to zero.
	token*** duplicates = (token***)0;
	uint duplicatessz = 0;
	
	// Variable that will be used to cache *curpos.
	u8 c;
	
	// Compare a token and a string.
	uint cmp () {
		
		uint true, false;
		
		// Reverse the meaning of true and false
		// if the token was within !{} .
		if (curtoken->flags.isnot) {
			
			true = 0;
			false = 1;
			
		} else {
			
			true = 1;
			false = 0;
		}
		
		// Check first if the token was representing
		// any of the metachars '<' or '>'
		
		// Check if the token represent the metacharacter '<' .
		if (curtoken->flags.isfirst) {
			if (curpos == start) return true; else return false;
		}
		
		// Check if the token represent the metacharacter '>' .
		if (curtoken->flags.islast) {
			if (curpos == start+size) return true; else return false;
		}
		
		u8* curtokenstrptr = curtoken->str.ptr;
		
		// I compare the string only if a string was attached
		// to the field str of the token.
		// I return 1 so I can skip when I had used a metachar '?'
		// or the token get ignored if its string size was null.
		if (!curtokenstrptr) return 1;
		
		// Check if the token represent a character set
		// within brackets. ei: [af-j];
		if (curtoken->flags.ischarset) {
			
			c = *curpos;
			
			uint i = 0;
			
			u8 curtokenstrptri = curtokenstrptr[i];
			
			// When the field ischar of a token is set,
			// I should have at least a single set of 2 characters,
			// that's why I can use a do while() loop here.
			do {
				u8 curtokenstrptri1 = curtokenstrptr[i+1];
				
				if (c >= curtokenstrptri && c <= curtokenstrptri1)
					return true;
				
				// If the character's case is to be ignored,
				// compare also against its other case.
				if (curtoken->flags.isignorecharcase) {
					
					if 	(c >= 'A' && c <= 'Z') c += ('a'-'A');
					else if (c >= 'a' && c <= 'z') c -= ('a'-'A');
					
					if (c >= curtokenstrptri && c <= curtokenstrptri1)
					return true;
				}
				
				i += 2;
				
				curtokenstrptri = curtokenstrptr[i];
				
			} while (curtokenstrptri);
			
			return false;
		}
		
		// If I get here the token has a string in its field str
		// that I compare against the text at curpos.
		if (curtoken->flags.isignorecharcase)
			return stringiseqnocase4(curtokenstrptr, curpos, curtoken->str.sz) ? true : false;
		else 	return stringiseq4(curtokenstrptr, curpos, curtoken->str.sz) ? true : false;
	}
	
	// Create a duplicate of the set of tokens to repeat and
	// set curtoken to the first token of the duplicated set.
	// curtoken need to hold valid repeat informations.
	// 1 is returned if duplication took place otherwise
	// 0 is returned if I completely worked out a duplicate.
	// This is the most complicate part of the entire library.
	uint dup () {
		
		uint spaceleft;
		
		uint* curtokenrepeatseq = curtoken->repeat.seq;
		uint curtokenrepeatseqsz = curtoken->repeat.seqsz;
		uint curtokenrepeatsel = curtoken->repeat.sel;
		uint curtokenrepeatcount = curtoken->repeat.count;
		
		// Update the fields sel and count to what will be the next
		// repeat to do, or set the field count to an appropriate value.
		void update () {
			
			uint curtokenrepeatsel1 = curtokenrepeatsel+1;
			
			uint curtokenrepeatseqcurtokenrepeatsel1 = curtokenrepeatseq[curtokenrepeatsel1];
			
			// keep in mind that I can have something like +1,8{a} or +8,1{a} and
			// that is the reason why I evaluate the field count less than and greater than.
			if (curtokenrepeatcount < curtokenrepeatseqcurtokenrepeatsel1) {
				
				++curtokenrepeatcount;
				
				// If the field repeat.count is bigger than spaceleft I can go ahead
				// and select another repeat sequence. It is useless to continue with the same sequence.
				if (curtokenrepeatcount > spaceleft) goto updatesel;
				
			} else if (curtokenrepeatcount > curtokenrepeatseqcurtokenrepeatsel1) {
				
				--curtokenrepeatcount;
				
				// If the field repeat.count is bigger than spaceleft, set count to the space left
				// only if is it bigger than the second limit of the sequence.
				if (curtokenrepeatcount > spaceleft) {
					if (spaceleft > curtokenrepeatseqcurtokenrepeatsel1) {
						curtokenrepeatcount = spaceleft;
					} else goto updatesel;
				}
				
			} else {
				
				updatesel:
				
				// If I get here,
				// curtoken->repeat.count == curtoken->repeat.seq[curtoken->repeat.sel+1]
				// I need to update the field sel to the next repeat sequence.
				curtokenrepeatsel1 = (curtokenrepeatsel += 2)+1;
				
				// I set the field count to the lower value of the
				// new repeat sequence only if the field sel < seqsz
				if (curtokenrepeatsel < curtokenrepeatseqsz) {
					
					curtokenrepeatcount = curtokenrepeatseq[curtokenrepeatsel];
					
					if (curtokenrepeatcount > spaceleft) {
						
						curtokenrepeatseqcurtokenrepeatsel1 = curtokenrepeatseq[curtokenrepeatsel1];
						
						if (curtokenrepeatcount > curtokenrepeatseqcurtokenrepeatsel1
							&& spaceleft > curtokenrepeatseqcurtokenrepeatsel1) {
							
							curtokenrepeatcount = spaceleft;
							
						} else goto updatesel;
					}
					
				} // The else case is tested outside of this function.
			}
			
			curtoken->repeat.sel = curtokenrepeatsel;
			curtoken->repeat.count = curtokenrepeatcount;
		}
		
		// Before exiting dup() after a successful duplication
		// I call update(); so here I need to make sure that after
		// the previous call to update() the field repeat.sel was not
		// equal to the field repeat.seqsz, because when it is the case
		// it mean that the entire repeat sequence had been completely
		// worked out and I should reset the field repeat.sel
		// and return null.
		
		// If the field sel == seqsz then I cannot duplicate
		// the token, because it has been worked out.
		if (curtokenrepeatsel == curtokenrepeatseqsz) {
			// I need to reinitialize the fields
			// sel and count so that the repeat information
			// of curtoken can still be processed
			// if I ever come back.
			curtoken->repeat.sel = 0;
			curtoken->repeat.count = curtokenrepeatseq[0];
			
			return 0;
		}
		
		// "+3{+0-9{a}}" is similar to "+0-9{a}+0-9{a}+0-9{a}" but
		// if applied to a text such as "a", the pattern "+3{+0-9{a}}" will
		// fail while the pattern "+0-9{a}+0-9{a}+0-9{a}" will succeed.
		// This is because a repetition number cannot be larger than
		// the space left to match, even if within the repetition brace
		// they could be a pattern which can match a space lesser than
		// the repetition size as it is the case for "+3{+0-9{a}}",
		// but it is hard to write a code which can detect that.
		spaceleft = size - ((uint)curpos - (uint)start);
		
		// If the field repeat.count is greater than spaceleft,
		// I should call update() to set it to a reasoneable value,
		// because I may have a very large value in the field repeat.count
		// whereas spaceleft is small and it can take much more processing
		// to create all the duplicate until I finish processing
		// the repeat sequence.
		if (curtokenrepeatcount > spaceleft) update();
		
		// Because I called update() right above, I need to check and
		// make sure that the field repeat.sel is not equal to the field
		// repeat.seqsz, because when it is the case the field repeat.count
		// is not set and what should be done is to reset
		// the field repeat.sel and return null.
		if (curtokenrepeatsel == curtokenrepeatseqsz) {
			// I need to reinitialize the field sel and count
			// so that the repeat information of curtoken
			// can still be processed if I ever come back.
			curtoken->repeat.sel = 0;
			curtoken->repeat.count = curtokenrepeatseq[0];
			
			return 0;
		}
		
		// When I get here the field repeat.count has a correct
		// value and I can proceed to peform duplication.
		
		// I find out where the repeat start.
		uint m1 = 0; while (curtoken->holder[m1] != curtoken) ++m1;
		// I find out where the repeat end, looking at the field head.
		uint m2 = m1; while (curtoken->holder[m2]->head != curtoken) ++m2;
		
		token** duptokenlist;
		
		// Variable that will be used to cache duptokenlist[0].
		token* duptokenlist0;
		
		// I assume two cases depending on the value of count.
		if (curtokenrepeatcount) {
			
			uint n = (m2-m1)+1;
			
			// Create enough space to hold the duplicate set.
			uint j = n*curtokenrepeatcount;
			
			duptokenlist = mmalloc(j*sizeof(token*));
			
			do duptokenlist[--j] = mmalloc(sizeof(token));
				while (j);
			
			// j is zero when I get here.
			
			// This loop control go through the entire array of duplicates
			// created in the loop above starting from the last duplicate.
			while (j < curtokenrepeatcount) {
				
				uint i = 0;
				
				// This loop go through a single duplicate in the array
				// starting from the last token of a duplicate.
				while (i < n) {
					// x will correspond to a token in the array
					// containing the duplicates.
					uint x = ((n*(curtokenrepeatcount-j))-1)-i;
					
					// y will correspond to a token
					// from the set being duplicated.
					uint y = m2-i;
					
					token* duptokenlistx = duptokenlist[x];
					token* curtokenholdery = curtoken->holder[y];
					
					duptokenlistx->holder = duptokenlist;
					duptokenlistx->str.ptr = curtokenholdery->str.ptr;
					duptokenlistx->str.sz = curtokenholdery->str.sz;
					duptokenlistx->flags = curtokenholdery->flags;
					// duptokenlistx->prev = 0; // I don't need to set this field for speed.
					duptokenlistx->tokcmpid = curtokenholdery->tokcmpid;
					duptokenlistx->repeat.seq = curtokenholdery->repeat.seq;
					duptokenlistx->repeat.seqsz = curtokenholdery->repeat.seqsz;
					duptokenlistx->repeat.sel = curtokenholdery->repeat.sel;
					duptokenlistx->repeat.count = curtokenholdery->repeat.count;
					
					uint k;
					
					// Return the position of a token in curtoken->holder .
					// A valid address that exist in the array of pointers
					// to tokens pointed by curtoken->holder should be passed
					// to this function, otherwise a pagefault is likely to occur.
					uint find (token* t) {
						
						uint i = 0;
						
						while (curtoken->holder[i] != t) ++i;
						
						return i;
					}
					
					// I need to rewrite the address saved in the fields next.
					uint z = find(curtokenholdery->next);
					
					// If (z > m2) then the field next point probably to what
					// was supposed to be after a single duplicate set; else
					// the field "next" point to a token within the duptokenlist set.
					if (z > m2) {
						// This will detect the last duplicate set in duptokenlist.
						if (j) {
							// I could have used the expression:
							// k = (n*((curtoken->repeat.count-j)-1))+(z-m1);
							// which is used in the else case of the block if (z > m2),
							// but the expression below is faster.
							k = n*(curtokenrepeatcount-j);
							
							token* duptokenlistk = duptokenlist[k];
							
							duptokenlistx->next = duptokenlistk;
							
							// I need to set the repeat field of the first token
							// of a repeated set otherwise while processing it,
							// a duplicate of what I am duplicating will be done again;
							// but it is not supposed to happen.
							// Keep in mind that a token that has its field seq set
							// always has its field head set to null; I mention that
							// because it is important for the field head to be null
							// when not in use.
							duptokenlistk->repeat.seq = 0;
							
							// For speed, it not necessary to set the fields below.
							// duptokenlistk->head = 0;
							// duptokenlistk->repeat.seqsz = 0;
							// duptokenlistk->repeat.sel = 0;
							// duptokenlistk->repeat.count = 0;
							
						} else duptokenlistx->next = curtoken->holder[m2]->next;
						
					} else {
						
						k = (n*((curtokenrepeatcount-j)-1))+(z-m1);
						
						duptokenlistx->next = duptokenlist[k];
					}
					
					// I rewrite the field alt.
					if (curtokenholdery->alt) {
						
						z = find(curtokenholdery->alt);
						
						// k is calculated using z calculated above. I use the same formular as
						// the one used above to calculate the field next when z is not > m2.
						k = (n*((curtokenrepeatcount-j)-1))+(z-m1);
						
						duptokenlistx->alt = duptokenlist[k];
						
					} else duptokenlistx->alt = 0;
					
					// I rewrite the field head.
					if (curtokenholdery->head) {
						
						z = find(curtokenholdery->head);
						
						// k is calculated using z calculated above. I use the same formular as
						// the one used above to calculate the field next when z is not > m2.
						k = (n*((curtokenrepeatcount-j)-1))+(z-m1);
						
						duptokenlistx->head = duptokenlist[k];
						
					} else duptokenlistx->head = 0;
					
					++i;
				}
				
				++j;
			}
			
		} else {
			// I create a duplicate which will do nothing.
			duptokenlist = mmalloc(sizeof(token*));
			
			// I allocate memory for the only token in the duplicate.
			duptokenlist[0] = (duptokenlist0 = mmalloc(sizeof(token)));
			
			duptokenlist0->holder = duptokenlist;
			
			duptokenlist0->str.ptr = 0;
			duptokenlist0->str.sz = 0;
			
			duptokenlist0->flags.isnot = 0;
			duptokenlist0->flags.ishidden = 0;
			duptokenlist0->flags.isfirst = 0;
			duptokenlist0->flags.islast = 0;
			duptokenlist0->flags.isnochar = 0;
			duptokenlist0->flags.isany = 0;
			duptokenlist0->flags.ischarset = 0;
			
			duptokenlist0->tokcmpid = 0;
			
			duptokenlist0->alt = 0;
			
			duptokenlist0->next = curtoken->holder[m2]->next;
			
			// It is not necessary to set the field seq to zero because it will not
			// be checked once curtoken is set as the first token of the duplicate.
		}
		
		// I need to save in the field head of the first token
		// of the duplicate, the address of the token holding
		// repeating informations. freedup() will use it
		// to set curtoken so that the next duplicate can be done.
		// The token holding repeating informations never has
		// an alternate token attached to it.
		
		// I set the field head of the first token in the duplicate.
		// I don't need to worry about any alternate token attached to
		// that first token because there will never be one.
		(duptokenlist0 = duptokenlist[0])->head = curtoken;
		
		// It is not necessary to set to zero the field seq of the first
		// token in the duplicate; in fact when dup() is called, curtoken
		// is set to the first token of the duplicate and that token will
		// not see its field seq checked; so it is safe
		// to comment out this line.
		// duptokenlist0->repeat.seq = 0;
		
		// For speed, I don't need to update this below.
		// duptokenlist0->repeat.seqsz = 0;
		// duptokenlist0->repeat.sel = 0;
		// duptokenlist0->repeat.count = 0;
		
		update();
		
		// I set the field prev of the first token in the duplicate
		// to the prev field of the token pointed by curtoken.
		// In fact at this point duptokenlist[0] is a duplicate
		// of what is in curtoken and can be seen as one token
		// instead of two token attached to each other.
		duptokenlist0->prev = curtoken->prev;
		
		// I set curtoken to the first token in duptokenlist.
		curtoken = duptokenlist0;
		
		// I save the address of the duplicate.
		uint duplicatessz1 = duplicatessz+1;
		duplicates = mmrealloc(duplicates, duplicatessz1*sizeof(token**));
		duplicates[duplicatessz] = duptokenlist;
		duplicatessz = duplicatessz1;
		
		return 1;
	}
	
	// Set curtoken appropriatly and free a duplicate array.
	// This function is called only when curtoken point
	// to the first token of the duplicate array to free.
	void freedup () {
		
		token** curtokenholder = curtoken->holder;
		
		curtoken = curtoken->head;
		
		uint i = (mmsz(curtokenholder)/sizeof(token*));
		
		do mmfree(curtokenholder[--i]);
			while (i);
		
		mmfree(curtokenholder);
		
		duplicates = mmrealloc(duplicates, (--duplicatessz)*sizeof(token**));
	}
	
	// Free all created duplicates.
	void freealldup () {
		
		while (duplicatessz) {
			
			token** tokenholder = duplicates[--duplicatessz];
			
			uint i = (mmsz(tokenholder)/sizeof(token*));
			
			do mmfree(tokenholder[--i]);
				while (i);
			
			mmfree(tokenholder);
		}
		
		mmfree(duplicates);
		
		duplicates = (token***)0;
		// duplicatessz has already been
		// set null in the above loop.
	}
	
	// This function reset the fields count, sel and
	// str.sz when the token is used for variadic '?' .
	void init () {
		
		uint i = (mmsz(t.tokenlist)/sizeof(token*));
		
		do {
			--i;
			
			token* ttokenlisti = t.tokenlist[i];
			
			uint* ttokenlistirepeatseq = ttokenlisti->repeat.seq;
			
			if (ttokenlistirepeatseq) {
				
				ttokenlisti->repeat.sel = 0;
				ttokenlisti->repeat.count = ttokenlistirepeatseq[0];
				
			} else if (ttokenlisti->flags.isany) ttokenlisti->str.sz = 0;
			
		} while (i);
	}
	
	// This function gather all the matches found
	// and pass it to the callback function.
	uint send () {
		
		u8* p = curpos;
		
		token* t = curtoken;
		
		// Keep the address of the match currently being worked out.
		pamsynmatchfound* m = 0;
		
		// Keep the address of a match previously completed
		// and is used to set the field next of the match in m.
		pamsynmatchfound* n = 0;
		
		// This structure is used to gather
		// the match result from a tokencompare block.
		// A tokencompare block is the portion of
		// the patern found between "%{" and "}".
		typedef struct tokcmpobj {
			// Id that was associated with
			// the tokencompare block.
			uint id;
			
			// string used to store the match
			// result from the tokencompare block.
			string str;
			
			uint isignorecharcase; // When 1, characters' case are ignored.
			
			// The last element of the linkedlist
			// has this field null.
			struct tokcmpobj* next;
			
		} tokcmpobj;
		
		// This variable point to a linkedlist
		// of tokcmpobj for which the field str
		// contain the complete match result from
		// the tokencompare block for which
		// it was created.
		tokcmpobj* complete = 0;
		
		// This variable point to a linkedlist
		// of tokcmpobj for which the field str
		// contain an incomplete match result from
		// the tokencompare block for which
		// it was created.
		tokcmpobj* incomplete = 0;
		
		// I Create the memory session that will be used
		// to regain any memory allocated by this function.
		mmsession sendmmsession = mmsessionnew();
		
		gathermatch:
		
		if (t->head) {
			
			if (t->head->tokcmpid) {
				// If I get here, I create a new object
				// for the tokencompare block and add it
				// to the linkedlist pointed by incomplete.
				
				tokcmpobj* obj = mmalloc(sizeof(tokcmpobj));
				
				obj->id = t->head->tokcmpid;
				
				obj->str = stringnull;
				
				obj->isignorecharcase = t->flags.isignorecharcase;
				
				obj->next = incomplete;
				
				incomplete = obj;
			}
			
		} else if (t->tokcmpid) {
			// When I get here, the following is always true:
			// t->tokcmpid == incomplete->id;
			
			tokcmpobj* obj = complete;
			
			while (obj) {
				
				if (obj->id == incomplete->id) {
					
					if ((obj->isignorecharcase | incomplete->isignorecharcase) ?
						// Ignore characters' case if either
						// tokcmpobj->isignorecharcase are true.
						stringiseqnocase1(obj->str, incomplete->str) :
						stringiseq1(obj->str, incomplete->str)) {
						
						// If I get here a tokencompare block
						// with the same id was already processed and
						// the result of its match in obj->str is
						// the same as the match result in incomplete->str
						// for the tokencompare block just processed.
						
						// I can free the tokcmpobj pointed by incomplete.
						
						u8* str = incomplete->str.ptr;
						
						if (str) mmfree(str);
						
						obj = incomplete->next;
						
						mmfree(incomplete);
						
						incomplete = obj;
						
						goto keeptokenprocessing;
						
					} else {
						// If I get here, a tokencompare block with
						// the same id was already processed but the result
						// of its match in obj->str is not the same as
						// the match result in incomplete->str for
						// the tokencompare block just processed.
						
						// Since I have a tokencompare block mismatch,
						// I free any memory allocated within this function
						// and I return 2 to continue looking for matches.
						
						mmsessionfree(sendmmsession, MMIGNORESUBSESSIONS);
						
						return 2;
					}
				}
				
				obj = obj->next;
			}
			
			// If I get here, a tokencompare block with
			// the same id had not been previously processed.
			// I move the tokcmpobj pointed by incomplete
			// to the linkedlist pointed by complete.
			
			obj = incomplete->next;
			
			incomplete->next = complete;
			
			complete = incomplete;
			
			incomplete = obj;
			
		// Here I check whether I have a nochar or
		// a token with a non-null size.
		// Note that, nochar tokens are the only tokens with
		// a null string size that are allowed to be used
		// to create a pamsynmatchfound. A token such as the one
		// representing the metachar '<' will never be used
		// to create a pamsynmatchfound.
		} else if (t->flags.isnochar || t->str.sz) {
			// I only add to m a token which is not hidden.
			if (t->flags.ishidden) {
				// If the token is hidden I set m to null
				// in order to create a new pamsynmatchfound.
				if (m) {
					// I adjust the field nochar if I did not have
					// characters gathered in the pamsynmatchfound.
					// So even if I had encountered a nochar,
					// it will be cancelled because it was
					// among non-hidden characters.
					if (m->start <= m->end) m->nochar = 0;
					else {
						// If I get here I have a nochar.
						// If (m->start == start) then (m->end == start-1) which
						// would be incorrect; so I must set m->end to m->start
						// and set the field nochar to 1 instead.
						if (m->start == start) {
							
							m->end = m->start;
							m->nochar = 1;
							
						} else {
							
							m->start = m->end;
							m->nochar = 2;
						}
					}
					
					n = m; // Save in n what was the last completed pamsynmatchfound.
					m = 0;
				}
				
			} else {
				
				if (m) m->start -= t->str.sz;
				else {
					m = mmalloc(sizeof(pamsynmatchfound));
					
					m->end = p - 1;
					m->start = p - t->str.sz;
					m->next = n;
					
					// I get here only if I had a non-null string or nochar.
					// I set the the field nochar of the pamsynmatchfound to 3
					// and it will be adjusted later to the correct value.
					m->nochar = 3;
				}
				
				if (incomplete && t->str.sz) {
					// Prepend the string which has
					// the size t->str.sz and which
					// is at the addres (p - t->str.sz),
					// to the string in incomplete->str.
					stringinsert3(&incomplete->str, p - t->str.sz, t->str.sz, 0);
				}
			}
			
			p -= t->str.sz;
		}
		
		keeptokenprocessing:
		
		if (t->prev) {
			
			t = t->prev;
			
			goto gathermatch;
		}
		
		// If m is null, I set it to a previously created
		// pamsynmatchfound that was successfully completed.
		if (!m) m = n;
		
		// I adjust the field nochar of the pamsynmatchfound only
		// if there was any and if it was not set before.
		if (m && m->nochar == 3) {
			// I adjust the field nochar if I did not have characters
			// gathered in the pamsynmatchfound. So even if I had encountered
			// a nochar it will be cancelled because it was among
			// non-hidden characters.
			if (m->start <= m->end) m->nochar = 0;
			else {
				// If I get here I have a nochar.
				// If (m->start == start) then (m->end == start-1) which
				// would be incorrect; So I must set m->end to m->start
				// and set the field nochar to 1 instead.
				if (m->start == start) {
					
					m->end = m->start;
					m->nochar = 1;
					
				} else {
					
					m->start = m->end;
					m->nochar = 2;
				}
			}
		}
		
		// When I get here, all tokens making up
		// the match have been processed.
		
		// I free all tokcmpobj created in
		// the linkedlist pointed by complete.
		while (complete) {
			
			tokcmpobj* obj = complete;
			
			complete = complete->next;
			
			u8* str = obj->str.ptr;
			
			if (str) mmfree(str);
			
			mmfree(obj);
		}
		
		// I free all tokcmpobj created in
		// the linkedlist pointed by incomplete.
		while (incomplete) {
			
			tokcmpobj* obj = incomplete;
			
			incomplete = incomplete->next;
			
			u8* str = obj->str.ptr;
			
			if (str) mmfree(str);
			
			mmfree(obj);
		}
		
		// I create the memory session that will be used to preserve
		// any memory allocated within the callback function.
		mmsession callbackmmsession = mmsessionnew();
		
		// When 0 is returned by the callback function,
		// I should stop looking for a match.
		// When 1 is returned by the callback function,
		// I should continue matching from curpos starting over
		// from the first token in the tokenlist; I also
		// call it regular look up.
		// When >1 is returned by the callback function, I should
		// continue as if I didn't find a match and I should look
		// for alternate tokens; I also call it slow look up.
		uint i = callback(m);
		
		// I free any memory allocated within this function.
		mmsessionfree(sendmmsession, MMIGNORESUBSESSIONS);
		
		// I can now discard the memory session that was used to preserve
		// any memory allocated within the callback function.
		mmsessioncancel(callbackmmsession, MMIGNORESUBSESSIONS);
		
		if (!i || i > 1) return i;
		
		// If I get here i == 1 and I return 1 unless p != curpos
		// otherwise I should do a slow lookup.
		// I will enter an infinite loop if it is not taken in consideration,
		// because curpos will not see its value changed due to
		// the fact that the fields sz of the strings associated
		// to tokens will remain null.
		
		if (p == curpos) return 2;
		
		return 1;
	}
	
	// The argument size cannot be null.
	if (!size) return;
	
	// Initialize with the address of
	// the beginning of the memory area.
	startpos = (u8*)start;
	
	setcurpos:
	
	curpos = startpos;
	
	resetcurtoken:
	
	// I save in curtoken the address of
	// the first token of the tokenlist.
	curtoken = t.tokenlist[0];
	
	// I initialize all state variables within tokens
	// which are necessary for repeatitions.
	init();
	
	checkrepeat:
	
	// If the seq field is not null,
	// duplicate the set of tokens enclosed.
	if (curtoken->repeat.seq) {
		
		if (!dup()) {
			// I jump to findprev. Note that the first token
			// of a repeat set do not have alternate tokens
			// attached to it.
			goto findprev;
		}
	}
	
	// Make sure that I will not create a page fault and
	// compare the token against what is in memory.
	// Notice that I used <= because if a match is at the end
	// of the text region where I am looking for a match,
	// adding curtoken->str.sz to curpos will obviously put curpos
	// at start+size which is a byte after the last character of
	// the region where I am looking for a match.
	// If I do not use <= , I would not be able to use those matches
	// which are the end of the region where I am looking for a match.
	compare:
	if (((curpos + curtoken->str.sz) <= start+size) && cmp()) {
		// Update curpos.
		curpos += curtoken->str.sz;
		
		// Set the field prev of the next token
		// so that I can go back up.
		if (curtoken->next) {
			
			curtoken->next->prev = curtoken;
			
			// Select the next token.
			curtoken = curtoken->next;
			
			goto checkrepeat;
		}
		
		// If I get here, then I found a match.
		// I call send() to process the match.
		uint i = send();
		
		// if i == 0 it mean that I should stop searching for matches.
		// if i == 1 then continue with regular lookup.
		// if i > 1 then continue with slow lookup.
		
		if (i < 2) {
			// I free created duplicates if any,
			// since I will not go back up;
			// so they are not needed anymore.
			if (duplicates) freealldup();
			
			if (!i) return;
			
			// If I get here then i == 1 .
			
			// If curpos was at the end of the text area
			// that I am processing, I should stop.
			if (curpos == start+size) return;
			
			startpos = curpos;
			
			goto resetcurtoken;
		}
		
		// If I get here then i > 1 .
		
		curpos -= curtoken->str.sz;
		
		goto findalt;
	}
	
	// Look for an alternative token, but first
	// look at all variadic '?' possibilities.
	findalt:
	
	if (curtoken->flags.isany) {
		
		if (((curpos + curtoken->str.sz) > start+size) || curtoken->str.sz == -1) {
			// Set the field sz of the string back to zero
			// so that if I come back I can re-use it.
			curtoken->str.sz = 0;
			
			// I jump to findprev, because I am sure that
			// there is no alternative case, because for
			// a metacharacter '*', a single token is created for it
			// and there is no alternate cases attached to it.
			goto findprev;
		}
		
		++curtoken->str.sz;
		
		goto compare;
	}
	
	if (curtoken->alt) {
		// Set the field prev of the alternate token
		// so that I can go back up.
		(*curtoken->alt).prev = curtoken->prev;
		
		// Select the alternate token.
		curtoken = curtoken->alt;
		
		goto checkrepeat;
	}
	
	findprev:
	// Check if I am about to leave a duplicate branch.
	if (curtoken->head) {
		
		if (curtoken->head->holder != curtoken->holder) {
			// If I get here then I have reached
			// the first token of a duplucate.
			// I free the duplicate. curtoken will
			// be set appropriatly by freedup().
			// freedup() is only called when curtoken
			// point to the first token of a duplicate set.
			freedup();
			
			goto checkrepeat;
		}
	}
	
	// Go back up using the field prev until
	// I find a non null field alt.
	if (curtoken->prev) {
		// It is important for this below
		// to come before updating curpos.
		curtoken = curtoken->prev;
		
		// Update curpos
		curpos -= curtoken->str.sz;
		
		goto findalt;
	}
	
	// If I get here then I went all the way back
	// to the first token of tokenlist. If after
	// incrementing startpos, I am going beyond
	// boundaries, I should stop.
	++startpos;
	
	if (startpos == start+size) {
		// When I get here, I should not have any created
		// duplicates, but the array of duplicates pointed by
		// the variable duplicates may still contain an allocated
		// memory block because while going up the linkedlist of
		// tokens through their field prev, freedup() is called
		// to free a duplicate set and use mmrealloc() to resize
		// the array of duplicates pointed by the variable duplicates.
		// So even when duplicatessz is null, the variable duplicates
		// may still point to an allocated memory block because
		// mmrealloc() used with a size of zero do not free
		// the memory block but simply internally mark it as having
		// a size of zero and return a pointer to it. mmfree()
		// is the only function which can completely free
		// an allocated or re-allocated block of memory.
		if (duplicates) mmfree(duplicates);
		
		return;
	}
	
	goto setcurpos;
}


// This function will return the address of the first
// and last non-hidden character of what was matched,
// using the pamsyntokenized on the memory region
// given through the arguments start and size.
// A null pamsynmatched is returned.
pamsynmatched pamsynmatch2 (pamsyntokenized t, u8* start, uint size) {
	// This variable will contain what is to be returned.
	pamsynmatched retvar = {.start = 0, .end = 0};
	
	if (!t.tokenlist) return retvar;
	
	// Callback function used with pamsynmatch1().
	uint processmatch (pamsynmatchfound* m) {
		// I check if a pamsynmatchfound was given to us;
		// if not I return 2 to tell pamsynmatch1() to do a slow lookup.
		if (!m) return 2;
		
		// This skip pamsynmatchfound that are nochar.
		while (m->nochar) {
			
			if (!(m = m->next)) {
				// If the pamsynmatchfound linkedlist was only made out of nochar,
				// I return 2 to tell pamsynmatch1() to do a slow lookup.
				return 2;
			}
		}
		
		// Here I set the address of
		// the first non-hidden character.
		retvar.start = m->start;
		
		// Here I find the last non-hidden character.
		
		retvar.end = m->end;
		
		while (m = m->next) {
			// I skip pamsynmatchfound that are nochar.
			if (!m->nochar) retvar.end = m->end;
		}
		
		// I return 0 to tell pamsynmatch1()
		// to stop looking for matches.
		return 0;
	}
	
	pamsynmatch1(t, start, size, processmatch);
	
	return retvar;
}
