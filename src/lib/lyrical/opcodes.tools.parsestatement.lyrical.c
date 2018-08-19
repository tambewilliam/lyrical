
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


// This function allocate memory for
// a new instruction structure and
// add it to the function given as argument.
// There is no check on whether
// the argument f is valid.
// Only the field next of the newly
// created instruction is set.
// A pointer to the instruction
// created is returned.
lyricalinstruction* newinstruction (lyricalfunction* f, lyricalop op) {
	// This function insure that there is at least
	// a count of unused registers as given as argument.
	// The argument count cannot be null.
	void insureenoughunusedregisters (uint count) {
		
		lyricalreg* r = f->gpr;
		
		// This loop check whether there are
		// already enough unused registers.
		do {
			if (!(r->returnaddr || r->funclevel || r->globalregionaddr || r->stringregionaddr ||
				r->thisaddr || r->retvaraddr || r->v || r->lock || r->reserved)) {
				
				if (!--count) return;
			}
			
		} while ((r = r->next) != f->gpr);
		
		// When I get here, r == f->gpr;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			newinstruction(f, LYRICALOPCOMMENT)->comment =
				stringduplicate2("begin: insure enough unused registers");
		}
		
		// I first start by flushing and discarding
		// registers that are assigned to variables, registers
		// which hold stackframe pointers, and the register which
		// hold the return address; then I discard registers that
		// are used for funclevel, globalregionaddr, stringregionaddr,
		// thisaddr and retvaraddr; that allow for re-using those registers
		// while I am flushing registers that are assigned to variables,
		// registers which hold stackframe pointers, and the register
		// which hold the return address.
		
		do {
			if (r->lock) continue;
			
			if (r->v) {
				
				if (r->dirty) flushreg(r);
				
				r->v = 0;
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					newinstruction(f, LYRICALOPCOMMENT)->comment =
						stringfmt("reg %%%d discarded", r->id);
				}
				
				if (!--count) goto done;
			}
			
		} while ((r = r->next) != f->gpr);
		
		// When I get here, r == f->gpr;
		
		do {
			if (r->lock) continue;
			
			if (r->dirty) flushreg(r);
			
			if (r->returnaddr) r->returnaddr = 0;
			else if (r->funclevel) r->funclevel = 0;
			else if (r->globalregionaddr) r->globalregionaddr = 0;
			else if (r->stringregionaddr) r->stringregionaddr = 0;
			else if (r->thisaddr) r->thisaddr = 0;
			else if (r->retvaraddr) r->retvaraddr = 0;
			else continue;
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
				newinstruction(f, LYRICALOPCOMMENT)->comment =
					stringfmt("reg %%%d discarded", r->id);
			}
			
			if (!--count) goto done;
			
		} while ((r = r->next) != f->gpr);
		
		// Function which return
		// the string equivalent
		// of a lyricalop.
		u8* opname (lyricalop op) {
			
			switch (op) {
				
				case LYRICALOPADD: return "ADD";
				case LYRICALOPADDI: return "ADDI";
				case LYRICALOPSUB: return "SUB";
				case LYRICALOPNEG: return "NEG";
				case LYRICALOPMUL: return "MUL";
				case LYRICALOPMULH: return "MULH";
				case LYRICALOPDIV: return "DIV";
				case LYRICALOPMOD: return "MOD";
				case LYRICALOPMULHU: return "MULHU";
				case LYRICALOPDIVU: return "DIVU";
				case LYRICALOPMODU: return "MODU";
				case LYRICALOPMULI: return "MULI";
				case LYRICALOPMULHI: return "MULHI";
				case LYRICALOPDIVI: return "DIVI";
				case LYRICALOPMODI: return "MODI";
				case LYRICALOPDIVI2: return "DIVI2";
				case LYRICALOPMODI2: return "MODI2";
				case LYRICALOPMULHUI: return "MULHUI";
				case LYRICALOPDIVUI: return "DIVUI";
				case LYRICALOPMODUI: return "MODUI";
				case LYRICALOPDIVUI2: return "DIVUI2";
				case LYRICALOPMODUI2: return "MODUI2";
				
				case LYRICALOPAND: return "AND";
				case LYRICALOPANDI: return "ANDI";
				case LYRICALOPOR: return "OR";
				case LYRICALOPORI: return "ORI";
				case LYRICALOPXOR: return "XOR";
				case LYRICALOPXORI: return "XORI";
				case LYRICALOPNOT: return "NOT";
				case LYRICALOPCPY: return "CPY";
				case LYRICALOPSLL: return "SLL";
				case LYRICALOPSLLI: return "SLLI";
				case LYRICALOPSLLI2: return "SLLI2";
				case LYRICALOPSRL: return "SRL";
				case LYRICALOPSRLI: return "SRLI";
				case LYRICALOPSRLI2: return "SRLI2";
				case LYRICALOPSRA: return "SRA";
				case LYRICALOPSRAI: return "SRAI";
				case LYRICALOPSRAI2: return "SRAI2";
				case LYRICALOPZXT: return "ZXT";
				case LYRICALOPSXT: return "SXT";
				
				case LYRICALOPSEQ: return "SEQ";
				case LYRICALOPSNE: return "SNE";
				case LYRICALOPSEQI: return "SEQI";
				case LYRICALOPSNEI: return "SNEI";
				case LYRICALOPSLT: return "SLT";
				case LYRICALOPSLTE: return "SLTE";
				case LYRICALOPSLTU: return "SLTU";
				case LYRICALOPSLTEU: return "SLTEU";
				case LYRICALOPSLTI: return "SLTI";
				case LYRICALOPSLTEI: return "SLTEI";
				case LYRICALOPSLTUI: return "SLTUI";
				case LYRICALOPSLTEUI: return "SLTEUI";
				case LYRICALOPSGTI: return "SGTI";
				case LYRICALOPSGTEI: return "SGTEI";
				case LYRICALOPSGTUI: return "SGTUI";
				case LYRICALOPSGTEUI: return "SGTEUI";
				case LYRICALOPSZ: return "SZ";
				case LYRICALOPSNZ: return "SNZ";
				
				case LYRICALOPJEQ: return "JEQ";
				case LYRICALOPJEQI: return "JEQI";
				case LYRICALOPJEQR: return "JEQR";
				case LYRICALOPJNE: return "JNE";
				case LYRICALOPJNEI: return "JNEI";
				case LYRICALOPJNER: return "JNER";
				case LYRICALOPJLT: return "JLT";
				case LYRICALOPJLTI: return "JLTI";
				case LYRICALOPJLTR: return "JLTR";
				case LYRICALOPJLTE: return "JLTE";
				case LYRICALOPJLTEI: return "JLTEI";
				case LYRICALOPJLTER: return "JLTER";
				case LYRICALOPJLTU: return "JLTU";
				case LYRICALOPJLTUI: return "JLTUI";
				case LYRICALOPJLTUR: return "JLTUR";
				case LYRICALOPJLTEU: return "JLTEU";
				case LYRICALOPJLTEUI: return "JLTEUI";
				case LYRICALOPJLTEUR: return "JLTEUR";
				case LYRICALOPJZ: return "JZ";
				case LYRICALOPJZI: return "JZI";
				case LYRICALOPJZR: return "JZR";
				case LYRICALOPJNZ: return "JNZ";
				case LYRICALOPJNZI: return "JNZI";
				case LYRICALOPJNZR: return "JNZR";
				case LYRICALOPJ: return "J";
				case LYRICALOPJI: return "JI";
				case LYRICALOPJR: return "JR";
				case LYRICALOPJL: return "JL";
				case LYRICALOPJLI: return "JLI";
				case LYRICALOPJLR: return "JLR";
				case LYRICALOPJPUSH: return "JPUSH";
				case LYRICALOPJPUSHI: return "JPUSHI";
				case LYRICALOPJPUSHR: return "JPUSHR";
				case LYRICALOPJPOP: return "JPOP";
				
				case LYRICALOPAFIP: return "AFIP";
				
				case LYRICALOPLI: return "LI";
				
				case LYRICALOPLD8: return "LD8";
				case LYRICALOPLD8R: return "LD8R";
				case LYRICALOPLD8I: return "LD8I";
				case LYRICALOPLD16: return "LD16";
				case LYRICALOPLD16R: return "LD16R";
				case LYRICALOPLD16I: return "LD16I";
				case LYRICALOPLD32: return "LD32";
				case LYRICALOPLD32R: return "LD32R";
				case LYRICALOPLD32I: return "LD32I";
				case LYRICALOPLD64: return "LD64";
				case LYRICALOPLD64R: return "LD64R";
				case LYRICALOPLD64I: return "LD64I";
				
				case LYRICALOPST8: return "ST8";
				case LYRICALOPST8R: return "ST8R";
				case LYRICALOPST8I: return "ST8I";
				case LYRICALOPST16: return "ST16";
				case LYRICALOPST16R: return "ST16R";
				case LYRICALOPST16I: return "ST16I";
				case LYRICALOPST32: return "ST32";
				case LYRICALOPST32R: return "ST32R";
				case LYRICALOPST32I: return "ST32I";
				case LYRICALOPST64: return "ST64";
				case LYRICALOPST64R: return "ST64R";
				case LYRICALOPST64I: return "ST64I";
				
				case LYRICALOPLDST8: return "LDST8";
				case LYRICALOPLDST8R: return "LDST8R";
				case LYRICALOPLDST8I: return "LDST8I";
				case LYRICALOPLDST16: return "LDST16";
				case LYRICALOPLDST16R: return "LDST16R";
				case LYRICALOPLDST16I: return "LDST16I";
				case LYRICALOPLDST32: return "LDST32";
				case LYRICALOPLDST32R: return "LDST32R";
				case LYRICALOPLDST32I: return "LDST32I";
				case LYRICALOPLDST64: return "LDST64";
				case LYRICALOPLDST64R: return "LDST64R";
				case LYRICALOPLDST64I: return "LDST64I";
				
				case LYRICALOPMEM8CPY: return "MEM8CPY";
				case LYRICALOPMEM8CPYI: return "MEM8CPYI";
				case LYRICALOPMEM8CPY2: return "MEM8CPY2";
				case LYRICALOPMEM8CPYI2: return "MEM8CPYI2";
				case LYRICALOPMEM16CPY: return "MEM16CPY";
				case LYRICALOPMEM16CPYI: return "MEM16CPYI";
				case LYRICALOPMEM16CPY2: return "MEM16CPY2";
				case LYRICALOPMEM16CPYI2: return "MEM16CPYI2";
				case LYRICALOPMEM32CPY: return "MEM32CPY";
				case LYRICALOPMEM32CPYI: return "MEM32CPYI";
				case LYRICALOPMEM32CPY2: return "MEM32CPY2";
				case LYRICALOPMEM32CPYI2: return "MEM32CPYI2";
				case LYRICALOPMEM64CPY: return "MEM64CPY";
				case LYRICALOPMEM64CPYI: return "MEM64CPYI";
				case LYRICALOPMEM64CPY2: return "MEM64CPY2";
				case LYRICALOPMEM64CPYI2: return "MEM64CPYI2";
				
				case LYRICALOPPAGEALLOC: return "PAGEALLOC";
				case LYRICALOPPAGEALLOCI: return "PAGEALLOCI";
				case LYRICALOPSTACKPAGEALLOC: return "STACKPAGEALLOC";
				case LYRICALOPPAGEFREE: return "PAGEFREE";
				case LYRICALOPPAGEFREEI: return "PAGEFREEI";
				case LYRICALOPSTACKPAGEFREE: return "STACKPAGEFREE";
				
				case LYRICALOPMACHINECODE: return "MACHINECODE";
				
				case LYRICALOPNOP: return "NOP";
				
				case LYRICALOPCOMMENT: return "COMMENT";
				
				default: return "UNKNOWN";
			}
		}
		
		throwerror(stringfmt("internal error: %s: could not find enough unused registers: op == %s: count == %d",
			__FUNCTION__, opname(op), count).ptr);
		
		done:
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			newinstruction(f, LYRICALOPCOMMENT)->comment =
				stringduplicate2("end: done");
		}
	}
	
	if ((f == currentfunc) && op < LYRICALOPNOP) {
		// I get here only if this function is called
		// for currentfunc, otherwise insureenoughunusedregisters()
		// can't properly use flushreg() which use currentfunc.
		
		uint minunusedregcount = compilearg->minunusedregcountforop[op];
		
		if (minunusedregcount) insureenoughunusedregisters(minunusedregcount);
	}
	
	// I allocate memory for
	// a new lyricalinstruction.
	lyricalinstruction* i = mmallocz(sizeof(lyricalinstruction));
	
	i->op = op;
	
	// Logic that find all currently unused
	// registers of the lyricalfunction f.
	{
		lyricalreg* r = f->gpr;
		
		uint n = 0;
		
		do {
			if (!(r->returnaddr || r->funclevel || r->globalregionaddr || r->stringregionaddr ||
				r->thisaddr || r->retvaraddr || r->v || r->lock || r->reserved)) {
				
				// +2 in the allocation size account
				// for the null terminating uint.
				i->unusedregs = mmrealloc(i->unusedregs, (n+2)*sizeof(uint));
				
				i->unusedregs[n] = r->id;
				
				++n;
			}
			
		} while ((r = r->next) != f->gpr);
		
		// Set the null terminating uint.
		if (n) i->unusedregs[n] = 0;
	}
	
	// I attach the newly
	// created lyricalinstruction
	// to the circular linkedlist.
	// Note that "next" is used with
	// the argument FIELDPREV of the macro
	// LINKEDLISTCIRCULARADDTOTOP()
	// because the ordering of the circular
	// linkedlist pointed by f->i is reversed,
	// whereby the linkedlist top is bottom,
	// and the linkedlist bottom is top.
	LINKEDLISTCIRCULARADDTOTOP(next, prev, i, f->i);
	
	if (!(compileargcompileflag&LYRICALCOMPILEGENERATEDEBUGINFO) || !curpos) return i;
	
	// I generate the debug information in i->dbginfo.
	
	// Variable used to compute
	// the position of the instruction
	// in the source.
	u8* ipos = curpos;
	
	// Compute the offset from
	// the "compileargsource" location,
	// of the line which contain
	// the "ipos" location.
	// Spaces at the beginning
	// of the line are skipped.
	
	// Find where the line begin.
	while (--ipos >= compileargsource && *ipos != '\n');
	
	// Skip spaces at the beginning of the line.
	while (*++ipos == '\x0d' || *ipos == ' ' || *ipos == '\t');
	
	uint o = (uint)ipos - (uint)compileargsource;
	
	chunk* c = chunks;
	
	// This loop find the chunk that
	// correspond to the address location
	// in the variable ipos and set its pointer
	// in the variable c; it will also set
	// the variable o with the offset within
	// the chunk content.
	while (1) {
		
		uint ccontentsz = c->contentsz;
		
		if (o < ccontentsz) break;
		
		o -= ccontentsz;
		
		c = c->next;
	}
	
	i->dbginfo.filepath = stringduplicate1(c->path);
	
	i->dbginfo.linenumber = countlines(ipos - o, ipos) + (c->linenumber -1);
	
	i->dbginfo.lineoffset = c->offset + o;
	
	return i;
}

// This function is used to create
// a comment instruction within the current function.
// The comment lyricalinstruction strings starting
// with "begin:" or "end:" are used by
// the text backend to make its result easily
// readeable by generating indentations.
lyricalinstruction* comment (string s) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPCOMMENT);
	
	i->comment = s;
	
	return i;
}

lyricalinstruction* add (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPADD);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// If the inputs are sign
	// or zero extended, the result
	// is sign extended if its size
	// is greater than both inputs;
	// note that the carryover bit
	// is being taken into account
	// by using ">" instead of ">="
	// when comparing the operands sizes.
	// The zero extension of the result
	// cannot be predicted.
	
	if ((r2->wassignextended | r2->waszeroextended) && (r3->wassignextended | r3->waszeroextended) &&
		r1->size > r2->size && r1->size > r3->size)
			r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	if (r2->waszeroextended && r3->waszeroextended && r1->size > r2->size && r1->size > r3->size)
		r1->waszeroextended = 1;
	else r1->waszeroextended = 0;
	
	return i;
}

lyricalinstruction* cpy (lyricalreg* r1, lyricalreg* r2) {
	
	if (r1 == r2) return 0;
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPCPY);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	// With the "cpy" instruction,
	// the sign and zero extension
	// status of the input is transfered
	// to the result if the size of
	// the input is less than or equal
	// to the size of the result,
	// otherwise the sign and zero
	// extension of the result
	// cannot be predicted.
	
	if (r1->size >= r2->size) {
		
		r1->waszeroextended = r2->waszeroextended;
		r1->wassignextended = r2->wassignextended;
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* addi (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return cpy(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPADDI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The immediate value is
	// assumed sign extended.
	// If the inputs are sign
	// or zero extended, the result
	// is sign extended if its size
	// is greater than both inputs;
	// note that the carryover bit
	// is being taken into account
	// by using ">" instead of ">="
	// when comparing the operands sizes.
	// The zero extension of the result
	// cannot be predicted.
	
	if ((r2->wassignextended | r2->waszeroextended) &&
		r1->size > r2->size && r1->size > countofsignextendedbytes(n))
			r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	// This compute the zero extended
	// size of the immediate value.
	uint immzxtsize = countofzeroextendedbytes(n);
	
	if (r2->waszeroextended && (immzxtsize < sizeofgpr) && r1->size > r2->size && r1->size > immzxtsize)
		r1->waszeroextended = 1;
	else r1->waszeroextended = 0;
	
	return i;
}

lyricalinstruction* li (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLI);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The result is set as sign extended
	// if the following is true:
	// r1->size >= countofsignextendedbytes(n);
	// The result is set as zero extended
	// if the following is true:
	// r1->size >= countofzeroextendedbytes(n);
	
	r1->waszeroextended = (r1->size >= countofzeroextendedbytes(n));
	
	r1->wassignextended = (r1->size >= countofsignextendedbytes(n));
	
	return i;
}

lyricalinstruction* sub (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	if (r2 == r3) return li(r1, 0);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSUB);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// If the inputs are sign
	// or zero extended, the result
	// is sign extended if its size
	// is greater than both inputs;
	// note that the carryover bit
	// is being taken into account
	// by using ">" instead of ">="
	// when comparing the operands sizes.
	// The zero extension of the result
	// cannot be predicted because
	// a negation occur.
	
	if ((r2->wassignextended | r2->waszeroextended) && (r3->wassignextended | r3->waszeroextended) &&
		r1->size > r2->size && r1->size > r3->size)
			r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	return i;
}

lyricalinstruction* neg (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPNEG);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	// If the input is sign or zero
	// extended, the result is sign
	// extended if its size is greater
	// than the size of the input.
	// The zero extension of the
	// result cannot be predicted.
	
	if (r2->wassignextended | r2->waszeroextended)
		r1->wassignextended = (r1->size > r2->size);
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	return i;
}

lyricalinstruction* mul (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMUL);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// The result is zero extended
	// if both inputs are zero extended,
	// and the size of the result
	// is greater than or equal to
	// the combined size of both inputs.
	// The result is sign extended
	// if its size is greater than
	// the combined size of both inputs
	// zero extended, or greater than
	// or equal to the combined size
	// of both inputs sign extended.
	
	if (r2->waszeroextended) {
		
		if (r3->waszeroextended) {
			
			uint size = (r2->size + r3->size);
			
			r1->waszeroextended = (r1->size >= size);
			r1->wassignextended = (r1->size > size);
			
		} else if (r3->wassignextended) {
			
			r1->waszeroextended = 0;
			r1->wassignextended = (r1->size > (r2->size + r3->size));
			
		} else {
			
			r1->waszeroextended = 0;
			r1->wassignextended = 0;
		}
		
	} else if (r2->wassignextended) {
		
		if (r3->wassignextended) {
			
			r1->waszeroextended = 0;
			r1->wassignextended = (r1->size >= (r2->size + r3->size));
			
		} else if (r3->waszeroextended) {
			
			r1->waszeroextended = 0;
			r1->wassignextended = (r1->size > (r2->size + r3->size));
			
		} else {
			
			r1->waszeroextended = 0;
			r1->wassignextended = 0;
		}
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* mulh (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMULH);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// The result is zero extended
	// if both inputs are zero extended,
	// and the size of the result
	// is greater than or equal to
	// the combined size of both inputs.
	// The result is sign extended
	// if its size is greater than
	// the combined size of both inputs
	// zero extended, or greater than
	// or equal to the combined size
	// of both inputs sign extended.
	
	// The result of a multiplication
	// has a size which is the combined
	// size of the inputs; hence the
	// substracting of sizeofgpr from
	// the combined size of the inputs.
	
	if (r2->waszeroextended) {
		
		if (r3->waszeroextended) {
			
			uint size = (r2->size + r3->size);
			
			r1->waszeroextended = ((size > sizeofgpr) ? (r1->size >= (size-sizeofgpr)) : 1);
			r1->wassignextended = ((size > sizeofgpr) ? (r1->size > (size-sizeofgpr)) : 1);
			
		} else if (r3->wassignextended) {
			
			uint size = (r2->size + r3->size);
			
			r1->waszeroextended = 0;
			r1->wassignextended = ((size > sizeofgpr) ? (r1->size > (size-sizeofgpr)) : 1);
			
		} else {
			
			r1->waszeroextended = 0;
			r1->wassignextended = 0;
		}
		
	} else if (r2->wassignextended) {
		
		if (r3->wassignextended) {
			
			uint size = (r2->size + r3->size);
			
			r1->waszeroextended = 0;
			r1->wassignextended = ((size > sizeofgpr) ? (r1->size >= (size-sizeofgpr)) : 1);
			
		} else if (r3->waszeroextended) {
			
			uint size = (r2->size + r3->size);
			
			r1->waszeroextended = 0;
			r1->wassignextended = ((size > sizeofgpr) ? (r1->size > (size-sizeofgpr)) : 1);
			
		} else {
			
			r1->waszeroextended = 0;
			r1->wassignextended = 0;
		}
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* div (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	if (r2 == r3) return li(r1, 1);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPDIV);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// The result is zero extended
	// if both inputs are zero extended,
	// and the size of the result
	// is greater than or equal to
	// the first input.
	// The result is sign extended
	// if its size is greater than
	// the first input zero extended,
	// or greater than or equal to
	// the first input sign extended.
	// Note that the absolute value
	// of the result is always smaller
	// than the absolute value of
	// the first input.
	
	if (r2->waszeroextended) {
		
		if (r3->waszeroextended) {
			
			r1->waszeroextended = (r1->size >= r2->size);
			r1->wassignextended = (r1->size > r2->size);
			
		} else {
			
			r1->waszeroextended = 0;
			r1->wassignextended = (r1->size > r2->size);
		}
		
	} else if (r2->wassignextended) {
		
		r1->waszeroextended = 0;
		r1->wassignextended = (r1->size >= r2->size);
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* mod (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	if (r2 == r3) return li(r1, 0);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMOD);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// The result is zero extended
	// if both inputs are zero extended,
	// and the size of the result
	// is greater than or equal to
	// the second input.
	// The result is sign extended
	// if its size is greater than
	// the second input zero extended,
	// or greater than or equal to
	// the second input sign extended.
	// Note that the absolute value
	// of the result is always smaller
	// than the absolute value of
	// the second input.
	
	if (r3->waszeroextended) {
		
		if (r2->waszeroextended) {
			
			r1->waszeroextended = (r1->size >= r3->size);
			r1->wassignextended = (r1->size > r3->size);
			
		} else {
			
			r1->waszeroextended = 0;
			r1->wassignextended = (r1->size > r3->size);
		}
		
	} else if (r3->wassignextended) {
		
		r1->waszeroextended = 0;
		r1->wassignextended = (r1->size >= r3->size);
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* mulhu (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMULHU);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// The result is zero extended
	// if its size is greater than
	// or equal to the combined size
	// of both inputs which must
	// be zero extended.
	// The result is sign extended
	// if its size is greater than
	// the combined size of both inputs
	// which must be zero extended.
	
	// The result of a multiplication
	// has a size which is the combined
	// size of the inputs; hence the
	// substracting of sizeofgpr from
	// the combined size of the inputs.
	
	if (r2->waszeroextended && r3->waszeroextended) {
		
		uint size = (r2->size + r3->size);
		
		r1->waszeroextended = ((size > sizeofgpr) ? (r1->size >= (size-sizeofgpr)) : 1);
		r1->wassignextended = ((size > sizeofgpr) ? (r1->size > (size-sizeofgpr)) : 1);
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* divu (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	if (r2 == r3) return li(r1, 1);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPDIVU);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// The result is zero extended
	// if its size is greater than
	// or equal to the first input
	// which must be zero extended.
	// The result is sign extended
	// if its size is greater than
	// the first input which must
	// be zero extended.
	// No need to check the second
	// input because the unsigned
	// value of the result is always
	// smaller than the unsigned
	// value of the first input.
	
	if (r2->waszeroextended) {
		
		r1->waszeroextended = (r1->size >= r2->size);
		r1->wassignextended = (r1->size > r2->size);
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* modu (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	if (r2 == r3) return li(r1, 0);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMODU);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// The result is zero extended
	// if its size is greater than
	// or equal to the second input
	// which must be zero extended.
	// The result is sign extended
	// if its size is greater than
	// the second input which must
	// be zero extended.
	// No need to check the first
	// input because the unsigned
	// value of the result is always
	// smaller than the unsigned
	// value of the second input.
	
	if (r3->waszeroextended) {
		
		r1->waszeroextended = (r1->size >= r3->size);
		r1->wassignextended = (r1->size > r3->size);
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* slli (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return cpy(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSLLI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The sign and zero extension
	// is preserved when the following
	// is true: r1->size >= (r2->size + ((n&0b11111)/8))
	// where 0b11111 represent the 5 bits
	// shift amount when (sizeofgpr == 4).
	
	if (r1->size >= (r2->size + ((n&targetshiftamountmask)/8))) {
		
		r1->waszeroextended = r2->waszeroextended;
		r1->wassignextended = r2->wassignextended;
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* muli (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return li(r1, 0);
	else if (n == 1) return cpy(r1, r2);
	else {
		// If the immediate value
		// is a powerof2, use
		// the shift instruction
		// which is faster.
		u64 x = ispowerof2(n);
		if (x) return slli(r1, r2, x);
	}
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMULI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The result is zero extended
	// if both inputs are zero extended,
	// and the size of the result
	// is greater than or equal to
	// the combined size of both inputs.
	// The result is sign extended
	// if its size is greater than
	// the combined size of both inputs
	// zero extended, or greater than
	// or equal to the combined size
	// of both inputs sign extended.
	
	// This compute the zero extended
	// size of the immediate value.
	uint immzxtsize = countofzeroextendedbytes(n);
	
	// This compute the sign extended
	// size of the immediate value.
	uint immsxtsize = countofsignextendedbytes(n);
	
	if (r2->waszeroextended) {
		
		if (immzxtsize < sizeofgpr) {
			// I get here if the immediate
			// value is zero extended.
			
			uint size = (r2->size + immzxtsize);
			
			r1->waszeroextended = (r1->size >= size);
			r1->wassignextended = (r1->size > size);
			
		} else {
			// Note that an immediate value
			// is assumed sign extended.
			
			r1->waszeroextended = 0;
			r1->wassignextended = (r1->size > (r2->size + immsxtsize));
		}
		
	} else if (r2->wassignextended) {
		// Note that an immediate value
		// is assumed sign extended.
		
		r1->waszeroextended = 0;
		r1->wassignextended = (r1->size >= (r2->size + immsxtsize));
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* mulhi (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return li(r1, 0);
	else if (n == 1) return cpy(r1, r2);
	else {
		// If the immediate value
		// is a powerof2, use
		// the shift instruction
		// which is faster.
		u64 x = ispowerof2(n);
		if (x) return slli(r1, r2, x);
	}
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMULHI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The result is zero extended
	// if both inputs are zero extended,
	// and the size of the result
	// is greater than or equal to
	// the combined size of both inputs.
	// The result is sign extended
	// if its size is greater than
	// the combined size of both inputs
	// zero extended, or greater than
	// or equal to the combined size
	// of both inputs sign extended.
	
	// This compute the zero extended
	// size of the immediate value.
	uint immzxtsize = countofzeroextendedbytes(n);
	
	// This compute the sign extended
	// size of the immediate value.
	uint immsxtsize = countofsignextendedbytes(n);
	
	// The result of a multiplication
	// has a size which is the combined
	// size of the inputs; hence the
	// substracting of sizeofgpr from
	// the combined size of the inputs.
	
	if (r2->waszeroextended) {
		
		if (immzxtsize < sizeofgpr) {
			// I get here if the immediate
			// value is zero extended.
			
			uint size = (r2->size + immzxtsize);
			
			r1->waszeroextended = ((size > sizeofgpr) ? (r1->size >= (size-sizeofgpr)) : 1);
			r1->wassignextended = ((size > sizeofgpr) ? (r1->size > (size-sizeofgpr)) : 1);
			
		} else {
			// Note that an immediate value
			// is assumed sign extended.
			
			uint size = (r2->size + immsxtsize);
			
			r1->waszeroextended = 0;
			r1->wassignextended = ((size > sizeofgpr) ? (r1->size > (size-sizeofgpr)) : 1);
		}
		
	} else if (r2->wassignextended) {
		// Note that an immediate value
		// is assumed sign extended.
		
		uint size = (r2->size + immsxtsize);
		
		r1->waszeroextended = 0;
		r1->wassignextended = ((size > sizeofgpr) ? (r1->size >= (size-sizeofgpr)) : 1);
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* divi (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (n == 1) return cpy(r1, r2);
	
	// Using srai() to perform
	// a signed division do not
	// produce the same result
	// as the signed division
	// operation itself, because
	// the quotient of the signed
	// division operation is
	// rounded toward zero, whereas
	// the result of srai() would
	// be rounded toward negative
	// infinity; so the difference
	// is apparent only for negative
	// numbers. ei: -9 / 4 using
	// the signed division yield -2;
	// if srai() is used to shift -9
	// right twice, the result is -3.
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPDIVI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The result is zero extended
	// if both inputs are zero extended,
	// and the size of the result
	// is greater than or equal to
	// the first input.
	// The result is sign extended
	// if its size is greater than
	// the first input zero extended,
	// or greater than or equal to
	// the first input sign extended.
	// Note that the absolute value
	// of the result is always smaller
	// than the absolute value of
	// the first input.
	
	if (r2->waszeroextended) {
		// If there is at least
		// one msb, the sign
		// of the result will
		// be positive.
		if (countofmsbzeros(n)) {
			
			r1->waszeroextended = (r1->size >= r2->size);
			r1->wassignextended = (r1->size > r2->size);
			
		} else {
			
			r1->waszeroextended = 0;
			r1->wassignextended = (r1->size > r2->size);
		}
		
	} else if (r2->wassignextended) {
		
		r1->waszeroextended = 0;
		r1->wassignextended = (r1->size >= r2->size);
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* modi (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (n == 1) return li(r1, 0);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMODI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The result is zero extended
	// if both inputs are zero extended,
	// and the size of the result
	// is greater than or equal to
	// the second input.
	// The result is sign extended
	// if its size is greater than
	// the second input zero extended,
	// or greater than or equal to
	// the second input sign extended.
	// Note that the absolute value
	// of the result is always smaller
	// than the absolute value of
	// the second input.
	
	// This compute the zero extended
	// size of the immediate value.
	uint immzxtsize = countofzeroextendedbytes(n);
	
	if (immzxtsize < sizeofgpr) {
		// I get here if the immediate
		// value is zero extended.
		
		if (r2->waszeroextended) {
			
			r1->waszeroextended = (r1->size >= immzxtsize);
			r1->wassignextended = (r1->size > immzxtsize);
			
		} else {
			
			r1->waszeroextended = 0;
			r1->wassignextended = (r1->size > immzxtsize);
		}
		
	} else {
		// This compute the sign extended
		// size of the immediate value.
		uint immsxtsize = countofsignextendedbytes(n);
		// Note that an immediate value
		// is assumed sign extended.
		
		r1->waszeroextended = 0;
		r1->wassignextended = (r1->size >= immsxtsize);
	}
	
	return i;
}

lyricalinstruction* divi2 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return li(r1, 0);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPDIVI2);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The result is zero extended
	// if both inputs are zero extended,
	// and the size of the result
	// is greater than or equal to
	// the second input.
	// The result is sign extended
	// if its size is greater than
	// the second input zero extended,
	// or greater than or equal to
	// the second input sign extended.
	// Note that the absolute value
	// of the result is always smaller
	// than the absolute value of
	// the second input.
	
	// This compute the zero extended
	// size of the immediate value.
	uint immzxtsize = countofzeroextendedbytes(n);
	
	if (immzxtsize < sizeofgpr) {
		// I get here if the immediate
		// value is zero extended.
		
		if (r2->waszeroextended) {
			
			r1->waszeroextended = (r1->size >= immzxtsize);
			r1->wassignextended = (r1->size > immzxtsize);
			
		} else {
			
			r1->waszeroextended = 0;
			r1->wassignextended = (r1->size > immzxtsize);
		}
		
	} else {
		// This compute the sign extended
		// size of the immediate value.
		uint immsxtsize = countofsignextendedbytes(n);
		// Note that an immediate value
		// is assumed sign extended.
		
		r1->waszeroextended = 0;
		r1->wassignextended = (r1->size >= immsxtsize);
	}
	
	return i;
}

lyricalinstruction* modi2 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return li(r1, 0);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMODI2);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The result is zero extended
	// if both inputs are zero extended,
	// and the size of the result
	// is greater than or equal to
	// the first input.
	// The result is sign extended
	// if its size is greater than
	// the first input zero extended,
	// or greater than or equal to
	// the first input sign extended.
	// Note that the absolute value
	// of the result is always smaller
	// than the absolute value of
	// the first input.
	
	if (r2->waszeroextended) {
		// If there is at least
		// one msb, the sign
		// of the result will
		// be positive.
		if (countofmsbzeros(n)) {
			
			r1->waszeroextended = (r1->size >= r2->size);
			r1->wassignextended = (r1->size > r2->size);
			
		} else {
			
			r1->waszeroextended = 0;
			r1->wassignextended = (r1->size > r2->size);
		}
		
	} else if (r2->wassignextended) {
		
		r1->waszeroextended = 0;
		r1->wassignextended = (r1->size >= r2->size);
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* mulhui (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return li(r1, 0);
	else if (n == 1) return cpy(r1, r2);
	else {
		// If the immediate value
		// is a powerof2, use
		// the shift instruction
		// which is faster.
		u64 x = ispowerof2(n);
		if (x) return slli(r1, r2, x);
	}
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMULHUI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The result is zero extended
	// if its size is greater than
	// or equal to the combined size
	// of both inputs which must
	// be zero extended.
	// The result is sign extended
	// if its size is greater than
	// the combined size of both inputs
	// which must be zero extended.
	
	// This compute the zero extended
	// size of the immediate value.
	uint immzxtsize = countofzeroextendedbytes(n);
	
	// The result of a multiplication
	// has a size which is the combined
	// size of the inputs; hence the
	// substracting of sizeofgpr from
	// the combined size of the inputs.
	
	if (r2->waszeroextended && (immzxtsize < sizeofgpr)) {
		// I get here if the immediate
		// value is also zero extended.
		
		uint size = (r2->size + immzxtsize);
		
		r1->waszeroextended = ((size > sizeofgpr) ? (r1->size >= (size-sizeofgpr)) : 1);
		r1->wassignextended = ((size > sizeofgpr) ? (r1->size > (size-sizeofgpr)) : 1);
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* srli (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return cpy(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSRLI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The zero extension is preserved
	// when the following is true:
	// r1->size >= (r2->size - ((n&0b11111)/8))
	// where 0b11111 represent the 5 bits
	// shift amount when (sizeofgpr == 4).
	// The sign extension of the result
	// register cannot be predicted.
	
	if (r1->size >= (r2->size - ((n&targetshiftamountmask)/8)))
		r1->waszeroextended = r2->waszeroextended;
	else r1->waszeroextended = 0;
	
	r1->wassignextended = 0;
	
	return i;
}

lyricalinstruction* divui (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (n == 1) return cpy(r1, r2);
	else if (n) {
		// If the immediate value
		// is a powerof2, use
		// the shift instruction
		// which is faster.
		u64 x = ispowerof2(n);
		if (x) return srli(r1, r2, x);
	}
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPDIVUI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The result is zero extended
	// if its size is greater than
	// or equal to the first input
	// which must be zero extended.
	// The result is sign extended
	// if its size is greater than
	// the first input which must
	// be zero extended.
	// No need to check the second
	// input because the unsigned
	// value of the result is always
	// smaller than the unsigned
	// value of the first input.
	
	if (r2->waszeroextended) {
		
		r1->waszeroextended = (r1->size >= r2->size);
		r1->wassignextended = (r1->size > r2->size);
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* andi (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return li(r1, 0);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPANDI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The result is zero extended
	// if its size is greater than
	// or equal to the size of any
	// input that is zero extended.
	// The result is sign extended
	// if its size is greater than
	// the size of any input that
	// is zero extended, or if both
	// inputs are sign extended
	// and the size of the result
	// is greater than or equal
	// to the size of the greater
	// sign extended input.
	
	// This compute the zero extended
	// size of the immediate value.
	uint immzxtsize = countofzeroextendedbytes(n);
	
	if (r2->waszeroextended) {
		
		if (r1->size >= r2->size) r1->waszeroextended = 1;
		else if (r1->size >= immzxtsize) r1->waszeroextended = 1;
		else r1->waszeroextended = 0;
		
	} else if (r1->size >= immzxtsize) r1->waszeroextended = 1;
	else r1->waszeroextended = 0;
	
	if (r2->waszeroextended) {
		
		if (r1->size > r2->size) r1->wassignextended = 1;
		else if (r1->size > immzxtsize) r1->wassignextended = 1;
		else r1->wassignextended = 0;
		
	} else if (r1->size > immzxtsize) r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	// Note that an immediate value
	// is assumed sign extended.
	if (!r1->wassignextended && r2->wassignextended) {
		// This compute the sign extended
		// size of the immediate value.
		uint immsxtsize = countofsignextendedbytes(n);
		
		if (r2->size > immsxtsize) r1->wassignextended = (r1->size >= r2->size);
		else r1->wassignextended = (r1->size >= immsxtsize);
	}
	
	return i;
}

lyricalinstruction* modui (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (n == 1) return li(r1, 0);
	else if (n) {
		// When doing an unsigned
		// modulo, if the divisor is
		// an immediate value that is
		// a powerof2, the modulo is:
		// dividend & (divisor - 1);
		// Using the bitwise instruction
		// is faster.
		if (ispowerof2(n)) return andi(r1, r2, n-1);
	}
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMODUI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The result is zero extended
	// if its size is greater than
	// or equal to the second input
	// which must be zero extended.
	// The result is sign extended
	// if its size is greater than
	// the second input which must
	// be zero extended.
	// No need to check the first
	// input because the unsigned
	// value of the result is always
	// smaller than the unsigned
	// value of the second input.
	
	// This compute the zero extended
	// size of the immediate value.
	uint immzxtsize = countofzeroextendedbytes(n);
	
	if (immzxtsize < sizeofgpr) {
		// I get here if the immediate
		// value is zero extended.
		
		r1->waszeroextended = (r1->size >= immzxtsize);
		r1->wassignextended = (r1->size > immzxtsize);
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* divui2 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return li(r1, 0);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPDIVUI2);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The result is zero extended
	// if its size is greater than
	// or equal to the second input
	// which must be zero extended.
	// The result is sign extended
	// if its size is greater than
	// the second input which must
	// be zero extended.
	// No need to check the first
	// input because the unsigned
	// value of the result is always
	// smaller than the unsigned
	// value of the second input.
	
	// This compute the zero extended
	// size of the immediate value.
	uint immzxtsize = countofzeroextendedbytes(n);
	
	if (immzxtsize < sizeofgpr) {
		
		r1->waszeroextended = (r1->size >= immzxtsize);
		r1->wassignextended = (r1->size > immzxtsize);
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* modui2 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return li(r1, 0);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMODUI2);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The result is zero extended
	// if its size is greater than
	// or equal to the first input
	// which must be zero extended.
	// The result is sign extended
	// if its size is greater than
	// the first input which must
	// be zero extended.
	// No need to check the second
	// input because the unsigned
	// value of the result is always
	// smaller than the unsigned
	// value of the first input.
	
	if (r2->waszeroextended) {
		
		r1->waszeroextended = (r1->size >= r2->size);
		r1->wassignextended = (r1->size > r2->size);
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* and (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	if (r2 == r3) return cpy(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPAND);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// The result is zero extended
	// if its size is greater than
	// or equal to the size of any
	// input that is zero extended.
	// The result is sign extended
	// if its size is greater than
	// the size of any input that
	// is zero extended, or if both
	// inputs are sign extended
	// and the size of the result
	// is greater than or equal
	// to the size of the greater
	// sign extended input.
	
	if (r2->waszeroextended) {
		
		if (r1->size >= r2->size) r1->waszeroextended = 1;
		else if (r1->size >= r3->size) r1->waszeroextended = r3->waszeroextended;
		else r1->waszeroextended = 0;
		
	} else if (r1->size >= r3->size) r1->waszeroextended = r3->waszeroextended;
	else r1->waszeroextended = 0;
	
	if (r2->waszeroextended) {
		
		if (r1->size > r2->size) r1->wassignextended = 1;
		else if (r1->size > r3->size) r1->wassignextended = r3->waszeroextended;
		else r1->wassignextended = 0;
		
	} else if (r1->size > r3->size) r1->wassignextended = r3->waszeroextended;
	else r1->wassignextended = 0;
	
	if (!r1->wassignextended && r2->wassignextended && r3->wassignextended) {
		
		if (r2->size > r3->size) r1->wassignextended = (r1->size >= r2->size);
		else r1->wassignextended = (r1->size >= r3->size);
	}
	
	return i;
}

lyricalinstruction* or (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	if (r2 == r3) return cpy(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPOR);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// The size of the result register
	// is compared against the larger
	// size between the inputs; and both
	// inputs need to be zero extended
	// for the result register to be
	// zero extended.
	// If both inputs are zero or sign
	// extended, the sign extension
	// is determined as following:
	// If the greater size input is sign
	// extended, the result become sign
	// extended if its size is greater
	// than or equal to the size
	// of the greater size input.
	// If the greater size input is zero
	// extended, the result become sign
	// extended if its size is greater
	// than the size of the greater
	// size input.
	
	if (r2->waszeroextended && r3->waszeroextended) {
		
		if (r2->size > r3->size) r1->waszeroextended = (r1->size >= r2->size);
		else r1->waszeroextended = (r1->size >= r3->size);
		
	} else r1->waszeroextended = 0;
	
	if (r2->size > r3->size) {
		
		if (r3->waszeroextended | r3->wassignextended) {
			
			if (r2->wassignextended)
				r1->wassignextended = (r1->size >= r2->size);
			else if (r2->waszeroextended)
				r1->wassignextended = (r1->size > r2->size);
			else r1->wassignextended = 0;
			
		} else r1->wassignextended = 0;
		
	} else {
		
		if (r2->waszeroextended | r2->wassignextended) {
			
			if (r3->wassignextended)
				r1->wassignextended = (r1->size >= r3->size);
			else if (r3->waszeroextended)
				r1->wassignextended = (r1->size > r3->size);
			else r1->wassignextended = 0;
			
		} else r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* ori (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return cpy(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPORI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The size of the result register
	// is compared against the larger
	// size between the inputs; and both
	// inputs need to be zero extended
	// for the result register to be
	// zero extended.
	// If both inputs are zero or sign
	// extended, the sign extension
	// is determined as following:
	// If the greater size input is sign
	// extended, the result become sign
	// extended if its size is greater
	// than or equal to the size
	// of the greater size input.
	// If the greater size input is zero
	// extended, the result become sign
	// extended if its size is greater
	// than the size of the greater
	// size input.
	
	if (r2->waszeroextended) {
		// This compute the zero extended
		// size of the immediate value.
		uint immzxtsize = countofzeroextendedbytes(n);
		
		if (r2->size > immzxtsize) r1->waszeroextended = (r1->size >= r2->size);
		else r1->waszeroextended = (r1->size >= immzxtsize);
		
	} else r1->waszeroextended = 0;
	
	// This compute the sign extended
	// size of the immediate value.
	uint immsxtsize = countofsignextendedbytes(n);
	// Note that an immediate value
	// is assumed sign extended.
	
	if (r2->size > immsxtsize) {
		
		if (r2->wassignextended)
			r1->wassignextended = (r1->size >= r2->size);
		else if (r2->waszeroextended)
			r1->wassignextended = (r1->size > r2->size);
		else r1->wassignextended = 0;
		
	} else {
		
		if (r2->waszeroextended | r2->wassignextended)
			r1->wassignextended = (r1->size >= immsxtsize);
		else r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* xor (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	if (r2 == r3) return li(r1, 0);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPXOR);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// The size of the result register
	// is compared against the larger
	// size between the inputs; and both
	// inputs need to be zero extended
	// for the result register to be
	// zero extended.
	// If both inputs are zero or sign
	// extended, the sign extension
	// is determined as following:
	// If the greater size input is sign
	// extended, the result become sign
	// extended if its size is greater
	// than or equal to the size
	// of the greater size input.
	// If the greater size input is zero
	// extended, the result become sign
	// extended if its size is greater
	// than the size of the greater
	// size input.
	
	if (r2->waszeroextended && r3->waszeroextended) {
		
		if (r2->size > r3->size) r1->waszeroextended = (r1->size >= r2->size);
		else r1->waszeroextended = (r1->size >= r3->size);
		
	} else r1->waszeroextended = 0;
	
	if (r2->size > r3->size) {
		
		if (r3->waszeroextended | r3->wassignextended) {
			
			if (r2->wassignextended)
				r1->wassignextended = (r1->size >= r2->size);
			else if (r2->waszeroextended)
				r1->wassignextended = (r1->size > r2->size);
			else r1->wassignextended = 0;
			
		} else r1->wassignextended = 0;
		
	} else {
		
		if (r2->waszeroextended | r2->wassignextended) {
			
			if (r3->wassignextended)
				r1->wassignextended = (r1->size >= r3->size);
			else if (r3->waszeroextended)
				r1->wassignextended = (r1->size > r3->size);
			else r1->wassignextended = 0;
			
		} else r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* xori (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return cpy(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPXORI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The size of the result register
	// is compared against the larger
	// size between the inputs; and both
	// inputs need to be zero extended
	// for the result register to be
	// zero extended.
	// If both inputs are zero or sign
	// extended, the sign extension
	// is determined as following:
	// If the greater size input is sign
	// extended, the result become sign
	// extended if its size is greater
	// than or equal to the size
	// of the greater size input.
	// If the greater size input is zero
	// extended, the result become sign
	// extended if its size is greater
	// than the size of the greater
	// size input.
	
	if (r2->waszeroextended) {
		// This compute the zero extended
		// size of the immediate value.
		uint immzxtsize = countofzeroextendedbytes(n);
		
		if (r2->size > immzxtsize) r1->waszeroextended = (r1->size >= r2->size);
		else r1->waszeroextended = (r1->size >= immzxtsize);
		
	} else r1->waszeroextended = 0;
	
	// This compute the sign extended
	// size of the immediate value.
	uint immsxtsize = countofsignextendedbytes(n);
	// Note that an immediate value
	// is assumed sign extended.
	
	if (r2->size > immsxtsize) {
		
		if (r2->wassignextended)
			r1->wassignextended = (r1->size >= r2->size);
		else if (r2->waszeroextended)
			r1->wassignextended = (r1->size > r2->size);
		else r1->wassignextended = 0;
		
	} else {
		
		if (r2->waszeroextended | r2->wassignextended)
			r1->wassignextended = (r1->size >= immsxtsize);
		else r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* not (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPNOT);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	// If the input is sign extended,
	// the result become sign extended
	// if its size is greater than or
	// equal to the size of the input.
	// If the input is zero extended,
	// the result become sign extended
	// if its size is greater than
	// the size of the input.
	// The zero extension of the
	// result cannot be predicted.
	
	if (r2->wassignextended)
		r1->wassignextended = (r1->size >= r2->size);
	else if (r2->waszeroextended)
		r1->wassignextended = (r1->size > r2->size);
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	return i;
}

lyricalinstruction* sll (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSLL);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// The sign and zero extension
	// of the result register
	// cannot be predicted.
	
	r1->waszeroextended = 0;
	r1->wassignextended = 0;
	
	return i;
}

lyricalinstruction* slli2 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return li(r1, 0);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSLLI2);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The sign and zero extension
	// of the result register
	// cannot be predicted.
	
	r1->waszeroextended = 0;
	r1->wassignextended = 0;
	
	return i;
}

lyricalinstruction* srl (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSRL);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// The zero extension is preserved
	// when the following is true:
	// r1->size >= r2->size;
	// The sign extension
	// of the result register
	// cannot be predicted.
	
	if (r1->size >= r2->size)
		r1->waszeroextended = r2->waszeroextended;
	else r1->waszeroextended = 0;
	
	r1->wassignextended = 0;
	
	return i;
}

lyricalinstruction* srli2 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return li(r1, 0);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSRLI2);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The zero extension is preserved
	// when the following is true:
	// r1->size >= countofzeroextendedbytes(n);
	// The sign extension
	// of the result register
	// cannot be predicted.
	
	r1->waszeroextended = (r1->size >= countofzeroextendedbytes(n));
	
	r1->wassignextended = 0;
	
	return i;
}

lyricalinstruction* sra (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSRA);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// The sign and zero extension
	// are preserved when the
	// following is true:
	// r1->size >= r2->size;
	
	if (r1->size >= r2->size) {
		
		r1->waszeroextended = r2->waszeroextended;
		r1->wassignextended = r2->wassignextended;
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* srai (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return cpy(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSRAI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The sign and zero extension
	// is preserved when the following
	// is true: r1->size >= (r2->size - ((n&0b11111)/8))
	// where 0b11111 represent the 5 bits
	// shift amount when (sizeofgpr == 4).
	
	if (r1->size >= (r2->size - ((n&targetshiftamountmask)/8))) {
		
		r1->waszeroextended = r2->waszeroextended;
		r1->wassignextended = r2->wassignextended;
		
	} else {
		
		r1->waszeroextended = 0;
		r1->wassignextended = 0;
	}
	
	return i;
}

lyricalinstruction* srai2 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return li(r1, 0);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSRAI2);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The result is set as sign extended
	// if the following is true:
	// r1->size >= countofsignextendedbytes(n);
	// The result is set as zero extended
	// if the following is true:
	// r1->size >= countofzeroextendedbytes(n);
	
	r1->waszeroextended = (r1->size >= countofzeroextendedbytes(n));
	
	r1->wassignextended = (r1->size >= countofsignextendedbytes(n));
	
	return i;
}

lyricalinstruction* zxt (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!(n%(sizeofgpr*8))) return 0;
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPZXT);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The result is sign extended
	// if the amount of zero extension
	// is smaller than the size of
	// the result, or if the input
	// to zero extend is already
	// zero extended and its size
	// is less than the size
	// of the result.
	// The result is zero extended
	// if the amount of zero extension
	// is smaller than or equal to
	// the size of the result, or
	// if the input to zero extend
	// is already zero extended and
	// its size is less than or equal
	// to the size of the result.
	
	if (r1->size >= (n>>3)) r1->waszeroextended = 1;
	else if (r2->waszeroextended) r1->waszeroextended = (r1->size >= r2->size);
	else r1->waszeroextended = 0;
	
	if (r1->size > (n>>3)) r1->wassignextended = 1;
	else if (r2->waszeroextended) r1->wassignextended = (r1->size > r2->size);
	else r1->wassignextended = 0;
	
	return i;
}

lyricalinstruction* sxt (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!(n%(sizeofgpr*8))) return 0;
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSXT);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The result is sign extended
	// if the amount of sign extension
	// is smaller than or equal to
	// the size of the result, or
	// if the input to sign extend
	// is already sign extended and
	// its size is less than or equal
	// to the size of the result.
	// The result is zero extended
	// if the size of the input to
	// sign extend is less than the
	// amount of sign extension, if
	// that input is zero extended,
	// and if the size of the result
	// is less than or equal to the
	// size of the input to sign extend.
	
	if (r1->size >= (n>>3)) r1->wassignextended = 1;
	else if (r2->wassignextended) r1->wassignextended = (r1->size >= r2->size);
	else r1->wassignextended = 0;
	
	if (r2->waszeroextended && r2->size < (n>>3))
		r1->waszeroextended = (r1->size >= r2->size);
	else r1->waszeroextended = 0;
	
	return i;
}

lyricalinstruction* seq (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSEQ);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

lyricalinstruction* sne (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSNE);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

lyricalinstruction* seqi (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSEQI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

lyricalinstruction* snei (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSNEI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

lyricalinstruction* slt (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSLT);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

lyricalinstruction* slte (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSLTE);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

lyricalinstruction* sltu (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSLTU);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

lyricalinstruction* slteu (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSLTEU);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

lyricalinstruction* slti (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSLTI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

lyricalinstruction* sltei (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSLTEI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

lyricalinstruction* sltui (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSLTUI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

lyricalinstruction* slteui (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSLTEUI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

lyricalinstruction* sgti (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSGTI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

lyricalinstruction* sgtei (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSGTEI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

lyricalinstruction* sgtui (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSGTUI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

lyricalinstruction* sgteui (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSGTEUI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

lyricalinstruction* sz (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSZ);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

lyricalinstruction* snz (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSNZ);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	// Regardless of the inputs size,
	// the result is both sign and
	// zero extended because only
	// the lsb is set.
	r1->waszeroextended = 1;
	r1->wassignextended = 1;
	
	return i;
}

// Keep in mind that the string label
// passed as argument is duplicated.
lyricalinstruction* jeq (lyricalreg* r1, lyricalreg* r2, string label) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJEQ);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	// The address where to
	// jump-to is resolved later.
	resolvelabellater(label, &i->imm->i);
	
	return i;
}

lyricalinstruction* jeqi (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJEQI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* jeqr (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJEQR);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	return i;
}

// Keep in mind that the string label
// passed as argument is duplicated.
lyricalinstruction* jne (lyricalreg* r1, lyricalreg* r2, string label) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJNE);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	// The address where to
	// jump-to is resolved later.
	resolvelabellater(label, &i->imm->i);
	
	return i;
}

lyricalinstruction* jnei (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJNEI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* jner (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJNER);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	return i;
}

// Keep in mind that the string label
// passed as argument is duplicated.
lyricalinstruction* jlt (lyricalreg* r1, lyricalreg* r2, string label) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJLT);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	// The address where to
	// jump-to is resolved later.
	resolvelabellater(label, &i->imm->i);
	
	return i;
}

lyricalinstruction* jlti (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJLTI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* jltr (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJLTR);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	return i;
}

// Keep in mind that the string label
// passed as argument is duplicated.
lyricalinstruction* jlte (lyricalreg* r1, lyricalreg* r2, string label) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJLTE);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	// The address where to
	// jump-to is resolved later.
	resolvelabellater(label, &i->imm->i);
	
	return i;
}

lyricalinstruction* jltei (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJLTEI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* jlter (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJLTER);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	return i;
}

// Keep in mind that the string label
// passed as argument is duplicated.
lyricalinstruction* jltu (lyricalreg* r1, lyricalreg* r2, string label) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJLTU);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	// The address where to
	// jump-to is resolved later.
	resolvelabellater(label, &i->imm->i);
	
	return i;
}

lyricalinstruction* jltui (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJLTUI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* jltur (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJLTUR);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	return i;
}

// Keep in mind that the string label
// passed as argument is duplicated.
lyricalinstruction* jlteu (lyricalreg* r1, lyricalreg* r2, string label) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJLTEU);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	// The address where to
	// jump-to is resolved later.
	resolvelabellater(label, &i->imm->i);
	
	return i;
}

lyricalinstruction* jlteui (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJLTEUI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* jlteur (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJLTEUR);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	return i;
}

// Keep in mind that the string label
// passed as argument is duplicated.
lyricalinstruction* jz (lyricalreg* r1, string label) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJZ);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	// The address where to
	// jump-to is resolved later.
	resolvelabellater(label, &i->imm->i);
	
	return i;
}

lyricalinstruction* jzi (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJZI);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* jzr (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJZR);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	return i;
}

// Keep in mind that the string label
// passed as argument is duplicated.
lyricalinstruction* jnz (lyricalreg* r1, string label) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJNZ);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	// The address where to
	// jump-to is resolved later.
	resolvelabellater(label, &i->imm->i);
	
	return i;
}

lyricalinstruction* jnzi (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJNZI);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* jnzr (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJNZR);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	return i;
}

lyricalinstruction* j (string label) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJ);
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	// The address where to
	// jump-to is resolved later.
	resolvelabellater(label, &i->imm->i);
	
	return i;
}

lyricalinstruction* ji (u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJI);
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* jr (lyricalreg* r1) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJR);
	
	i->r1 = r1->id;
	
	return i;
}

// Keep in mind that the string label
// passed as argument is duplicated.
lyricalinstruction* jl (lyricalreg* r1, string label) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJL);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	// The address where to
	// jump-to is resolved later.
	resolvelabellater(label, &i->imm->i);
	
	// The sign and zero extension
	// of the result register
	// cannot be predicted.
	
	r1->waszeroextended = 0;
	r1->wassignextended = 0;
	
	return i;
}

lyricalinstruction* jli (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJLI);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The sign and zero extension
	// of the result register
	// cannot be predicted.
	
	r1->waszeroextended = 0;
	r1->wassignextended = 0;
	
	return i;
}

lyricalinstruction* jlr (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJLR);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	// The sign and zero extension
	// of the result register
	// cannot be predicted.
	
	r1->waszeroextended = 0;
	r1->wassignextended = 0;
	
	return i;
}

// Keep in mind that the string label
// passed as argument is duplicated.
lyricalinstruction* jpush (string label) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJPUSH);
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	// The address where to
	// jump-to is resolved later.
	resolvelabellater(label, &i->imm->i);
	
	return i;
}

lyricalinstruction* jpushi (u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJPUSHI);
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* jpushr (lyricalreg* r1) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJPUSHR);
	
	i->r1 = r1->id;
	
	return i;
}

lyricalinstruction* jpop () {
	return newinstruction(currentfunc, LYRICALOPJPOP);
}

// Keep in mind that the string label
// passed as argument is duplicated.
lyricalinstruction* afip (lyricalreg* r1, string label) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPAFIP);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	// The address where to
	// jump-to is resolved later.
	resolvelabellater(label, &i->imm->i);
	
	// The sign and zero extension
	// of the result register
	// cannot be predicted.
	
	r1->waszeroextended = 0;
	r1->wassignextended = 0;
	
	return i;
}

lyricalinstruction* ld8r (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLD8R);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 1);
	r1->wassignextended = (r1->size > 1);
	
	return i;
}

lyricalinstruction* ld8 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return ld8r(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLD8);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 1);
	r1->wassignextended = (r1->size > 1);
	
	return i;
}

lyricalinstruction* ld8i (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLD8I);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 1);
	r1->wassignextended = (r1->size > 1);
	
	return i;
}

lyricalinstruction* ld16r (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLD16R);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 2);
	r1->wassignextended = (r1->size > 2);
	
	return i;
}

lyricalinstruction* ld16 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return ld16r(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLD16);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 2);
	r1->wassignextended = (r1->size > 2);
	
	return i;
}

lyricalinstruction* ld16i (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLD16I);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 2);
	r1->wassignextended = (r1->size > 2);
	
	return i;
}

lyricalinstruction* ld32r (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLD32R);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 4);
	r1->wassignextended = (r1->size > 4);
	
	return i;
}

lyricalinstruction* ld32 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return ld32r(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLD32);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 4);
	r1->wassignextended = (r1->size > 4);
	
	return i;
}

lyricalinstruction* ld32i (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLD32I);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 4);
	r1->wassignextended = (r1->size > 4);
	
	return i;
}

lyricalinstruction* ld64r (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLD64R);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 8);
	r1->wassignextended = (r1->size > 8);
	
	return i;
}

lyricalinstruction* ld64 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return ld64r(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLD64);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 8);
	r1->wassignextended = (r1->size > 8);
	
	return i;
}

lyricalinstruction* ld64i (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLD64I);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 8);
	r1->wassignextended = (r1->size > 8);
	
	return i;
}

lyricalinstruction* ldr (lyricalreg* r1, lyricalreg* r2) {
	
	if (sizeofgpr == 1) return ld8r(r1, r2);
	else if (sizeofgpr == 2) return ld16r(r1, r2);
	else if (sizeofgpr == 4) return ld32r(r1, r2);
	else if (sizeofgpr == 8) return ld64r(r1, r2);
}

lyricalinstruction* ld (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return ldr(r1, r2);
	
	if (sizeofgpr == 1) return ld8(r1, r2, n);
	else if (sizeofgpr == 2) return ld16(r1, r2, n);
	else if (sizeofgpr == 4) return ld32(r1, r2, n);
	else if (sizeofgpr == 8) return ld64(r1, r2, n);
}

lyricalinstruction* ldi (lyricalreg* r1, u64 n) {
	
	if (sizeofgpr == 1) return ld8i(r1, n);
	else if (sizeofgpr == 2) return ld16i(r1, n);
	else if (sizeofgpr == 4) return ld32i(r1, n);
	else if (sizeofgpr == 8) return ld64i(r1, n);
}

lyricalop lyricalopldr () {
	
	if (sizeofgpr == 1) return LYRICALOPLD8R;
	else if (sizeofgpr == 2) return LYRICALOPLD16R;
	else if (sizeofgpr == 4) return LYRICALOPLD32R;
	else if (sizeofgpr == 8) return LYRICALOPLD64R;
}

lyricalinstruction* st8r (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPST8R);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	return i;
}

lyricalinstruction* st8 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return st8r(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPST8);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* st8i (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPST8I);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* st16r (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPST16R);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	return i;
}

lyricalinstruction* st16 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return st16r(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPST16);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* st16i (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPST16I);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* st32r (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPST32R);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	return i;
}

lyricalinstruction* st32 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return st32r(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPST32);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* st32i (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPST32I);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* st64r (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPST64R);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	return i;
}

lyricalinstruction* st64 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return st64r(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPST64);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* st64i (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPST64I);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* str (lyricalreg* r1, lyricalreg* r2) {
	
	if (sizeofgpr == 1) return st8r(r1, r2);
	else if (sizeofgpr == 2) return st16r(r1, r2);
	else if (sizeofgpr == 4) return st32r(r1, r2);
	else if (sizeofgpr == 8) return st64r(r1, r2);
}

lyricalinstruction* st (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return str(r1, r2);
	
	if (sizeofgpr == 1) return st8(r1, r2, n);
	else if (sizeofgpr == 2) return st16(r1, r2, n);
	else if (sizeofgpr == 4) return st32(r1, r2, n);
	else if (sizeofgpr == 8) return st64(r1, r2, n);
}

lyricalinstruction* sti (lyricalreg* r1, u64 n) {
	
	if (sizeofgpr == 1) return st8i(r1, n);
	else if (sizeofgpr == 2) return st16i(r1, n);
	else if (sizeofgpr == 4) return st32i(r1, n);
	else if (sizeofgpr == 8) return st64i(r1, n);
}

lyricalop lyricalopstr () {
	
	if (sizeofgpr == 1) return LYRICALOPST8R;
	else if (sizeofgpr == 2) return LYRICALOPST16R;
	else if (sizeofgpr == 4) return LYRICALOPST32R;
	else if (sizeofgpr == 8) return LYRICALOPST64R;
}

lyricalop lyricalopst () {
	
	if (sizeofgpr == 1) return LYRICALOPST8;
	else if (sizeofgpr == 2) return LYRICALOPST16;
	else if (sizeofgpr == 4) return LYRICALOPST32;
	else if (sizeofgpr == 8) return LYRICALOPST64;
}

lyricalinstruction* ldst8r (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLDST8R);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 1);
	r1->wassignextended = (r1->size > 1);
	
	return i;
}

lyricalinstruction* ldst8 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return ldst8r(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLDST8);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 1);
	r1->wassignextended = (r1->size > 1);
	
	return i;
}

lyricalinstruction* ldst8i (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLDST8I);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 1);
	r1->wassignextended = (r1->size > 1);
	
	return i;
}

lyricalinstruction* ldst16r (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLDST16R);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 2);
	r1->wassignextended = (r1->size > 2);
	
	return i;
}

lyricalinstruction* ldst16 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return ldst16r(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLDST16);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 2);
	r1->wassignextended = (r1->size > 2);
	
	return i;
}

lyricalinstruction* ldst16i (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLDST16I);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 2);
	r1->wassignextended = (r1->size > 2);
	
	return i;
}

lyricalinstruction* ldst32r (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLDST32R);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 4);
	r1->wassignextended = (r1->size > 4);
	
	return i;
}

lyricalinstruction* ldst32 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return ldst32r(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLDST32);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 4);
	r1->wassignextended = (r1->size > 4);
	
	return i;
}

lyricalinstruction* ldst32i (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLDST32I);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 4);
	r1->wassignextended = (r1->size > 4);
	
	return i;
}

lyricalinstruction* ldst64r (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLDST64R);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 8);
	r1->wassignextended = (r1->size > 8);
	
	return i;
}

lyricalinstruction* ldst64 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return ldst64r(r1, r2);
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLDST64);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 8);
	r1->wassignextended = (r1->size > 8);
	
	return i;
}

lyricalinstruction* ldst64i (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPLDST64I);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// Loading a value from memory:
	// - zero extend it if the size
	// of the result is greater than
	// or equal to the size loaded
	// from memory.
	// - sign extend it if the size
	// of the result is greater than
	// the size loaded from memory.
	
	r1->waszeroextended = (r1->size >= 8);
	r1->wassignextended = (r1->size > 8);
	
	return i;
}

lyricalinstruction* ldstr (lyricalreg* r1, lyricalreg* r2) {
	
	if (sizeofgpr == 1) return ldst8r(r1, r2);
	else if (sizeofgpr == 2) return ldst16r(r1, r2);
	else if (sizeofgpr == 4) return ldst32r(r1, r2);
	else if (sizeofgpr == 8) return ldst64r(r1, r2);
}

lyricalinstruction* ldst (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (!n) return ldstr(r1, r2);
	
	if (sizeofgpr == 1) return ldst8(r1, r2, n);
	else if (sizeofgpr == 2) return ldst16(r1, r2, n);
	else if (sizeofgpr == 4) return ldst32(r1, r2, n);
	else if (sizeofgpr == 8) return ldst64(r1, r2, n);
}

lyricalinstruction* ldsti (lyricalreg* r1, u64 n) {
	
	if (sizeofgpr == 1) return ldst8i(r1, n);
	else if (sizeofgpr == 2) return ldst16i(r1, n);
	else if (sizeofgpr == 4) return ldst32i(r1, n);
	else if (sizeofgpr == 8) return ldst64i(r1, n);
}

lyricalinstruction* mem8cpy (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	// All operands are input-output.
	
	if (r1->id == r3->id) throwerror("the first and third operands cannot be the same");
	else if (r1->id == r2->id) throwerror("the first and second operands cannot be the same");
	else if (r2->id == r3->id) throwerror("the second and third operands cannot be the same");
	
	// I first generate a check
	// to insure that the value
	// of r3 will never be null.
	// The check simplify the
	// lyricalinstruction conversion
	// by the backend.
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJZ);
	
	i->r1 = r3->id;
	
	lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
	imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	i->imm = imm;
	
	// I now generate the mem8cpy instruction.
	
	i = newinstruction(currentfunc, LYRICALOPMEM8CPY);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// I set the destination
	// of the LYRICALOPJZ.
	
	i = newinstruction(currentfunc, LYRICALOPNOP);
	
	imm->i = i;
	
	// r1 is set sign extended if r3 is sign extended,
	// and if r1->size >= r3->size.
	// r2 is set sign extended if r3 is sign extended,
	// and if r2->size >= r3->size.
	// The value of r3 become zero, hence
	// it is set both zero and sign extended.
	
	if (r1->wassignextended && r3->wassignextended && r1->size >= r3->size)
		r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	if (r2->wassignextended && r3->wassignextended && r2->size >= r3->size)
		r2->wassignextended = 1;
	else r2->wassignextended = 0;
	
	r2->waszeroextended = 0;
	
	r3->waszeroextended = 1;
	r3->wassignextended = 1;
	
	return i;
}

lyricalinstruction* mem8cpy2 (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	// All operands are input-output.
	
	if (r1->id == r3->id) throwerror("the first and third operands cannot be the same");
	else if (r1->id == r2->id) throwerror("the first and second operands cannot be the same");
	else if (r2->id == r3->id) throwerror("the second and third operands cannot be the same");
	
	// I first generate a check
	// to insure that the value
	// of r3 will never be null.
	// The check simplify the
	// lyricalinstruction conversion
	// by the backend.
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJZ);
	
	i->r1 = r3->id;
	
	lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
	imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	i->imm = imm;
	
	// I now generate the mem8cpy2 instruction.
	
	i = newinstruction(currentfunc, LYRICALOPMEM8CPY2);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// I set the destination
	// of the LYRICALOPJZ.
	
	i = newinstruction(currentfunc, LYRICALOPNOP);
	
	imm->i = i;
	
	// r1 is set sign extended if r3 is sign extended,
	// and if r1->size >= r3->size.
	// r2 is set sign extended if r3 is sign extended,
	// and if r2->size >= r3->size.
	// The value of r3 become zero, hence
	// it is set both zero and sign extended.
	
	if (r1->wassignextended && r3->wassignextended && r1->size >= r3->size)
		r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	if (r2->wassignextended && r3->wassignextended && r2->size >= r3->size)
		r2->wassignextended = 1;
	else r2->wassignextended = 0;
	
	r2->waszeroextended = 0;
	
	r3->waszeroextended = 1;
	r3->wassignextended = 1;
	
	return i;
}

lyricalinstruction* mem8cpyi (lyricalreg* r1, lyricalreg* r2, u64 n) {
	// All operands are input-output,
	// except the immediate value.
	
	if (r1->id == r2->id) throwerror("the first and second operands cannot be the same");
	
	if (!n) return 0;
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMEM8CPYI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The immediate value is assumed sign extended.
	// r1 is set sign extended if r1->size >= countofsignextendedbytes(n).
	// r2 is set sign extended if r2->size >= countofsignextendedbytes(n).
	
	if (r1->wassignextended && r1->size >= countofsignextendedbytes(n))
		r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	if (r2->wassignextended && r2->size >= countofsignextendedbytes(n))
		r2->wassignextended = 1;
	else r2->wassignextended = 0;
	
	r2->waszeroextended = 0;
	
	return i;
}

lyricalinstruction* mem8cpyi2 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	// All operands are input-output,
	// except the immediate value.
	
	if (r1->id == r2->id) throwerror("the first and second operands cannot be the same");
	
	if (!n) return 0;
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMEM8CPYI2);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The immediate value is assumed sign extended.
	// r1 is set sign extended if r1->size >= countofsignextendedbytes(n).
	// r2 is set sign extended if r2->size >= countofsignextendedbytes(n).
	
	if (r1->wassignextended && r1->size >= countofsignextendedbytes(n))
		r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	if (r2->wassignextended && r2->size >= countofsignextendedbytes(n))
		r2->wassignextended = 1;
	else r2->wassignextended = 0;
	
	r2->waszeroextended = 0;
	
	return i;
}

lyricalinstruction* mem16cpy (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	// All operands are input-output.
	
	if (r1->id == r3->id) throwerror("the first and third operands cannot be the same");
	else if (r1->id == r2->id) throwerror("the first and second operands cannot be the same");
	else if (r2->id == r3->id) throwerror("the second and third operands cannot be the same");
	
	// I first generate a check
	// to insure that the value
	// of r3 will never be null.
	// The check simplify the
	// lyricalinstruction conversion
	// by the backend.
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJZ);
	
	i->r1 = r3->id;
	
	lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
	imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	i->imm = imm;
	
	// I now generate the mem16cpy instruction.
	
	i = newinstruction(currentfunc, LYRICALOPMEM16CPY);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// I set the destination
	// of the LYRICALOPJZ.
	
	i = newinstruction(currentfunc, LYRICALOPNOP);
	
	imm->i = i;
	
	// r1 is set sign extended if r3 is sign extended,
	// and if r1->size >= r3->size.
	// r2 is set sign extended if r3 is sign extended,
	// and if r2->size >= r3->size.
	// The value of r3 become zero, hence
	// it is set both zero and sign extended.
	
	if (r1->wassignextended && r3->wassignextended && r1->size >= r3->size)
		r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	if (r2->wassignextended && r3->wassignextended && r2->size >= r3->size)
		r2->wassignextended = 1;
	else r2->wassignextended = 0;
	
	r2->waszeroextended = 0;
	
	r3->waszeroextended = 1;
	r3->wassignextended = 1;
	
	return i;
}

lyricalinstruction* mem16cpy2 (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	// All operands are input-output.
	
	if (r1->id == r3->id) throwerror("the first and third operands cannot be the same");
	else if (r1->id == r2->id) throwerror("the first and second operands cannot be the same");
	else if (r2->id == r3->id) throwerror("the second and third operands cannot be the same");
	
	// I first generate a check
	// to insure that the value
	// of r3 will never be null.
	// The check simplify the
	// lyricalinstruction conversion
	// by the backend.
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJZ);
	
	i->r1 = r3->id;
	
	lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
	imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	i->imm = imm;
	
	// I now generate the mem16cpy2 instruction.
	
	i = newinstruction(currentfunc, LYRICALOPMEM16CPY2);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// I set the destination
	// of the LYRICALOPJZ.
	
	i = newinstruction(currentfunc, LYRICALOPNOP);
	
	imm->i = i;
	
	// r1 is set sign extended if r3 is sign extended,
	// and if r1->size >= r3->size.
	// r2 is set sign extended if r3 is sign extended,
	// and if r2->size >= r3->size.
	// The value of r3 become zero, hence
	// it is set both zero and sign extended.
	
	if (r1->wassignextended && r3->wassignextended && r1->size >= r3->size)
		r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	if (r2->wassignextended && r3->wassignextended && r2->size >= r3->size)
		r2->wassignextended = 1;
	else r2->wassignextended = 0;
	
	r2->waszeroextended = 0;
	
	r3->waszeroextended = 1;
	r3->wassignextended = 1;
	
	return i;
}

lyricalinstruction* mem16cpyi (lyricalreg* r1, lyricalreg* r2, u64 n) {
	// All operands are input-output,
	// except the immediate value.
	
	if (r1->id == r2->id) throwerror("the first and second operands cannot be the same");
	
	if (!n) return 0;
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMEM16CPYI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The immediate value is assumed sign extended.
	// r1 is set sign extended if r1->size >= countofsignextendedbytes(n).
	// r2 is set sign extended if r2->size >= countofsignextendedbytes(n).
	
	if (r1->wassignextended && r1->size >= countofsignextendedbytes(n))
		r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	if (r2->wassignextended && r2->size >= countofsignextendedbytes(n))
		r2->wassignextended = 1;
	else r2->wassignextended = 0;
	
	r2->waszeroextended = 0;
	
	return i;
}

lyricalinstruction* mem16cpyi2 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	// All operands are input-output,
	// except the immediate value.
	
	if (r1->id == r2->id) throwerror("the first and second operands cannot be the same");
	
	if (!n) return 0;
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMEM16CPYI2);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The immediate value is assumed sign extended.
	// r1 is set sign extended if r1->size >= countofsignextendedbytes(n).
	// r2 is set sign extended if r2->size >= countofsignextendedbytes(n).
	
	if (r1->wassignextended && r1->size >= countofsignextendedbytes(n))
		r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	if (r2->wassignextended && r2->size >= countofsignextendedbytes(n))
		r2->wassignextended = 1;
	else r2->wassignextended = 0;
	
	r2->waszeroextended = 0;
	
	return i;
}

lyricalinstruction* mem32cpy (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	// All operands are input-output.
	
	if (r1->id == r3->id) throwerror("the first and third operands cannot be the same");
	else if (r1->id == r2->id) throwerror("the first and second operands cannot be the same");
	else if (r2->id == r3->id) throwerror("the second and third operands cannot be the same");
	
	// I first generate a check
	// to insure that the value
	// of r3 will never be null.
	// The check simplify the
	// lyricalinstruction conversion
	// by the backend.
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJZ);
	
	i->r1 = r3->id;
	
	lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
	imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	i->imm = imm;
	
	// I now generate the mem32cpy instruction.
	
	i = newinstruction(currentfunc, LYRICALOPMEM32CPY);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// I set the destination
	// of the LYRICALOPJZ.
	
	i = newinstruction(currentfunc, LYRICALOPNOP);
	
	imm->i = i;
	
	// r1 is set sign extended if r3 is sign extended,
	// and if r1->size >= r3->size.
	// r2 is set sign extended if r3 is sign extended,
	// and if r2->size >= r3->size.
	// The value of r3 become zero, hence
	// it is set both zero and sign extended.
	
	if (r1->wassignextended && r3->wassignextended && r1->size >= r3->size)
		r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	if (r2->wassignextended && r3->wassignextended && r2->size >= r3->size)
		r2->wassignextended = 1;
	else r2->wassignextended = 0;
	
	r2->waszeroextended = 0;
	
	r3->waszeroextended = 1;
	r3->wassignextended = 1;
	
	return i;
}

lyricalinstruction* mem32cpy2 (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	// All operands are input-output.
	
	if (r1->id == r3->id) throwerror("the first and third operands cannot be the same");
	else if (r1->id == r2->id) throwerror("the first and second operands cannot be the same");
	else if (r2->id == r3->id) throwerror("the second and third operands cannot be the same");
	
	// I first generate a check
	// to insure that the value
	// of r3 will never be null.
	// The check simplify the
	// lyricalinstruction conversion
	// by the backend.
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJZ);
	
	i->r1 = r3->id;
	
	lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
	imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	i->imm = imm;
	
	// I now generate the mem32cpy2 instruction.
	
	i = newinstruction(currentfunc, LYRICALOPMEM32CPY2);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// I set the destination
	// of the LYRICALOPJZ.
	
	i = newinstruction(currentfunc, LYRICALOPNOP);
	
	imm->i = i;
	
	// r1 is set sign extended if r3 is sign extended,
	// and if r1->size >= r3->size.
	// r2 is set sign extended if r3 is sign extended,
	// and if r2->size >= r3->size.
	// The value of r3 become zero, hence
	// it is set both zero and sign extended.
	
	if (r1->wassignextended && r3->wassignextended && r1->size >= r3->size)
		r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	if (r2->wassignextended && r3->wassignextended && r2->size >= r3->size)
		r2->wassignextended = 1;
	else r2->wassignextended = 0;
	
	r2->waszeroextended = 0;
	
	r3->waszeroextended = 1;
	r3->wassignextended = 1;
	
	return i;
}

lyricalinstruction* mem32cpyi (lyricalreg* r1, lyricalreg* r2, u64 n) {
	// All operands are input-output,
	// except the immediate value.
	
	if (r1->id == r2->id) throwerror("the first and second operands cannot be the same");
	
	if (!n) return 0;
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMEM32CPYI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The immediate value is assumed sign extended.
	// r1 is set sign extended if r1->size >= countofsignextendedbytes(n).
	// r2 is set sign extended if r2->size >= countofsignextendedbytes(n).
	
	if (r1->wassignextended && r1->size >= countofsignextendedbytes(n))
		r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	if (r2->wassignextended && r2->size >= countofsignextendedbytes(n))
		r2->wassignextended = 1;
	else r2->wassignextended = 0;
	
	r2->waszeroextended = 0;
	
	return i;
}

lyricalinstruction* mem32cpyi2 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	// All operands are input-output,
	// except the immediate value.
	
	if (r1->id == r2->id) throwerror("the first and second operands cannot be the same");
	
	if (!n) return 0;
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMEM32CPYI2);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The immediate value is assumed sign extended.
	// r1 is set sign extended if r1->size >= countofsignextendedbytes(n).
	// r2 is set sign extended if r2->size >= countofsignextendedbytes(n).
	
	if (r1->wassignextended && r1->size >= countofsignextendedbytes(n))
		r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	if (r2->wassignextended && r2->size >= countofsignextendedbytes(n))
		r2->wassignextended = 1;
	else r2->wassignextended = 0;
	
	r2->waszeroextended = 0;
	
	return i;
}

lyricalinstruction* mem64cpy (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	// All operands are input-output.
	
	if (r1->id == r3->id) throwerror("the first and third operands cannot be the same");
	else if (r1->id == r2->id) throwerror("the first and second operands cannot be the same");
	else if (r2->id == r3->id) throwerror("the second and third operands cannot be the same");
	
	// I first generate a check
	// to insure that the value
	// of r3 will never be null.
	// The check simplify the
	// lyricalinstruction conversion
	// by the backend.
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJZ);
	
	i->r1 = r3->id;
	
	lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
	imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	i->imm = imm;
	
	// I now generate the mem64cpy instruction.
	
	i = newinstruction(currentfunc, LYRICALOPMEM64CPY);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// I set the destination
	// of the LYRICALOPJZ.
	
	i = newinstruction(currentfunc, LYRICALOPNOP);
	
	imm->i = i;
	
	// r1 is set sign extended if r3 is sign extended,
	// and if r1->size >= r3->size.
	// r2 is set sign extended if r3 is sign extended,
	// and if r2->size >= r3->size.
	// The value of r3 become zero, hence
	// it is set both zero and sign extended.
	
	if (r1->wassignextended && r3->wassignextended && r1->size >= r3->size)
		r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	if (r2->wassignextended && r3->wassignextended && r2->size >= r3->size)
		r2->wassignextended = 1;
	else r2->wassignextended = 0;
	
	r2->waszeroextended = 0;
	
	r3->waszeroextended = 1;
	r3->wassignextended = 1;
	
	return i;
}

lyricalinstruction* mem64cpy2 (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	// All operands are input-output.
	
	if (r1->id == r3->id) throwerror("the first and third operands cannot be the same");
	else if (r1->id == r2->id) throwerror("the first and second operands cannot be the same");
	else if (r2->id == r3->id) throwerror("the second and third operands cannot be the same");
	
	// I first generate a check
	// to insure that the value
	// of r3 will never be null.
	// The check simplify the
	// lyricalinstruction conversion
	// by the backend.
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJZ);
	
	i->r1 = r3->id;
	
	lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
	imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
	
	i->imm = imm;
	
	// I now generate the mem64cpy2 instruction.
	
	i = newinstruction(currentfunc, LYRICALOPMEM64CPY2);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	i->r3 = r3->id;
	
	// I set the destination
	// of the LYRICALOPJZ.
	
	i = newinstruction(currentfunc, LYRICALOPNOP);
	
	imm->i = i;
	
	// r1 is set sign extended if r3 is sign extended,
	// and if r1->size >= r3->size.
	// r2 is set sign extended if r3 is sign extended,
	// and if r2->size >= r3->size.
	// The value of r3 become zero, hence
	// it is set both zero and sign extended.
	
	if (r1->wassignextended && r3->wassignextended && r1->size >= r3->size)
		r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	if (r2->wassignextended && r3->wassignextended && r2->size >= r3->size)
		r2->wassignextended = 1;
	else r2->wassignextended = 0;
	
	r2->waszeroextended = 0;
	
	r3->waszeroextended = 1;
	r3->wassignextended = 1;
	
	return i;
}

lyricalinstruction* mem64cpyi (lyricalreg* r1, lyricalreg* r2, u64 n) {
	// All operands are input-output,
	// except the immediate value.
	
	if (r1->id == r2->id) throwerror("the first and second operands cannot be the same");
	
	if (!n) return 0;
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMEM64CPYI);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The immediate value is assumed sign extended.
	// r1 is set sign extended if r1->size >= countofsignextendedbytes(n).
	// r2 is set sign extended if r2->size >= countofsignextendedbytes(n).
	
	if (r1->wassignextended && r1->size >= countofsignextendedbytes(n))
		r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	if (r2->wassignextended && r2->size >= countofsignextendedbytes(n))
		r2->wassignextended = 1;
	else r2->wassignextended = 0;
	
	r2->waszeroextended = 0;
	
	return i;
}

lyricalinstruction* mem64cpyi2 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	// All operands are input-output,
	// except the immediate value.
	
	if (r1->id == r2->id) throwerror("the first and second operands cannot be the same");
	
	if (!n) return 0;
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMEM64CPYI2);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The immediate value is assumed sign extended.
	// r1 is set sign extended if r1->size >= countofsignextendedbytes(n).
	// r2 is set sign extended if r2->size >= countofsignextendedbytes(n).
	
	if (r1->wassignextended && r1->size >= countofsignextendedbytes(n))
		r1->wassignextended = 1;
	else r1->wassignextended = 0;
	
	r1->waszeroextended = 0;
	
	if (r2->wassignextended && r2->size >= countofsignextendedbytes(n))
		r2->wassignextended = 1;
	else r2->wassignextended = 0;
	
	r2->waszeroextended = 0;
	
	return i;
}

lyricalinstruction* memcpy (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	if (sizeofgpr == 1) return mem8cpy(r1, r2, r3);
	else if (sizeofgpr == 2) return mem16cpy(r1, r2, r3);
	else if (sizeofgpr == 4) return mem32cpy(r1, r2, r3);
	else if (sizeofgpr == 8) return mem64cpy(r1, r2, r3);
}

lyricalinstruction* memcpy2 (lyricalreg* r1, lyricalreg* r2, lyricalreg* r3) {
	
	if (sizeofgpr == 1) return mem8cpy2(r1, r2, r3);
	else if (sizeofgpr == 2) return mem16cpy2(r1, r2, r3);
	else if (sizeofgpr == 4) return mem32cpy2(r1, r2, r3);
	else if (sizeofgpr == 8) return mem64cpy2(r1, r2, r3);
}

lyricalinstruction* memcpyi (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (sizeofgpr == 1) return mem8cpyi(r1, r2, n);
	else if (sizeofgpr == 2) return mem16cpyi(r1, r2, n);
	else if (sizeofgpr == 4) return mem32cpyi(r1, r2, n);
	else if (sizeofgpr == 8) return mem64cpyi(r1, r2, n);
}

lyricalinstruction* memcpyi2 (lyricalreg* r1, lyricalreg* r2, u64 n) {
	
	if (sizeofgpr == 1) return mem8cpyi2(r1, r2, n);
	else if (sizeofgpr == 2) return mem16cpyi2(r1, r2, n);
	else if (sizeofgpr == 4) return mem32cpyi2(r1, r2, n);
	else if (sizeofgpr == 8) return mem64cpyi2(r1, r2, n);
}

lyricalinstruction* pagealloc (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPPAGEALLOC);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	// The sign and zero extension
	// of the result register
	// cannot be predicted.
	
	r1->waszeroextended = 0;
	r1->wassignextended = 0;
	
	return i;
}

lyricalinstruction* pagealloci (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPPAGEALLOCI);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	// The sign and zero extension
	// of the result register
	// cannot be predicted.
	
	r1->waszeroextended = 0;
	r1->wassignextended = 0;
	
	return i;
}

lyricalinstruction* stackpagealloc (lyricalreg* r1) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSTACKPAGEALLOC);
	
	i->r1 = r1->id;
	
	// The sign and zero extension
	// of the result register
	// cannot be predicted.
	
	r1->waszeroextended = 0;
	r1->wassignextended = 0;
	
	return i;
}

lyricalinstruction* pagefree (lyricalreg* r1, lyricalreg* r2) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPPAGEFREE);
	
	i->r1 = r1->id;
	i->r2 = r2->id;
	
	return i;
}

lyricalinstruction* pagefreei (lyricalreg* r1, u64 n) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPPAGEFREEI);
	
	i->r1 = r1->id;
	
	i->imm = mmallocz(sizeof(lyricalimmval));
	i->imm->type = LYRICALIMMVALUE;
	i->imm->n = n;
	
	return i;
}

lyricalinstruction* stackpagefree (lyricalreg* r1) {
	
	lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSTACKPAGEFREE);
	
	i->r1 = r1->id;
	
	return i;
}
