
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


#include <stdtypes.h>
#include <mm.h>
#include <string.h>
#include <file.h>
#include <lyrical.h>


// Backend which convert the result of
// the compilation to a human readable text.
// The field rootfunc of the lyricalcompileresult
// used as argument should be non-null.
string lyricalbackendtext (lyricalcompileresult compileresult) {
	
	lyricalfunction* f = compileresult.rootfunc;
	
	do {
		// Note that f->i is always non-null,
		// so there is no need to check it.
		lyricalinstruction* i = f->i;
		
		// This loop allocate in each lyricalinstruction,
		// a string which will be used to store
		// the textual equivalement of the instruction.
		do {
			i->backenddata = (string*)mmallocz(sizeof(string));
			
			i = i->next;
			
		} while (i != f->i);
		
	} while ((f = f->next) != compileresult.rootfunc);
	
	// When I get here, f == compileresult.rootfunc;
	
	// Variables which will be used to store
	// a textual list of exports and imports.
	string exports = stringnull;
	string imports = stringnull;
	
	do {
		// This variable is used for indenting
		// instructions string found between
		// an instruction comment starting with "begin:"
		// and an instruction comment starting with "end:" .
		string indent = stringnull;
		
		// Note that f->i is always non-null,
		// so there is no need to check it.
		lyricalinstruction* i = f->i;
		
		// i == f->i and f->i point to the last
		// lyricalintruction that was generated,
		// while f->i->next point to the first
		// lyricalinstruction generated.
		i = i->next;
		
		// Variable which get set
		// to the string* set
		// in i->backenddata.
		string* b = (string*)i->backenddata;
		
		if (f == compileresult.rootfunc) {
			
			string s = stringfmt("# function_%08x\n", f);
			stringappend1(b, s);
			mmfree(s.ptr);
			
		} else {
			
			lyricalfunction* ff = f;
			
			// I create the label name representing
			// the start of the function.
			// I use the address of the struct function
			// in order to generate a unique label name,
			// followed by the signature of the function.
			// If the function is not a child of the
			// root function, I append the labeling of
			// its parent funtions to differentiate it
			// from other functions with the same name.
			do {
				string s = stringfmt("# function_%08x:%s\n", ff, ff->linkingsignature);
				stringappend1(b, s);
				mmfree(s.ptr);
				
			} while ((ff = ff->parent) != compileresult.rootfunc);
			
			// If function is an export or import,
			// list it in the corresponding variable.
			if (f->toexport) {
				
				string s = stringfmt("# %s\n", f->linkingsignature);
				stringappend1(b, s);
				mmfree(s.ptr);
				
			} else if (f->toimport) {
				
				string s = stringfmt("# %s:%d\n", f->linkingsignature, f->toimport-1);
				stringappend1(&imports, s);
				mmfree(s.ptr);
			}
		}
		
		// I indent the instructions following.
		stringappend4(&indent, '\t');
		stringappend4(b, '\t');
		
		// This function append
		// the opcode single register argument.
		void appendarg1reg () {
			string s = stringfmt("\t%%%d", i->r1);
			stringappend1(b, s);
			mmfree(s.ptr);
		}
		
		// This function append the opcode arguments
		// where the arguments are 2 registers.
		void appendarg2reg () {
			string s = stringfmt("\t%%%d, %%%d", i->r1, i->r2);
			stringappend1(b, s);
			mmfree(s.ptr);
		}
		
		// This function append the opcode arguments
		// where the arguments are 3 registers.
		void appendarg3reg () {
			string s = stringfmt("\t%%%d, %%%d, %%%d", i->r1, i->r2, i->r3);
			stringappend1(b, s);
			mmfree(s.ptr);
		}
		
		// Enum used with appendargimm().
		typedef enum {
			
			BEGINWITHCOMMA,
			DONOTBEGINWITHCOMMA
			
		} appendargimmflag;
		
		// This function append
		// the immediate argument.
		void appendargimm (appendargimmflag flag) {
			
			string s;
			
			if (flag == BEGINWITHCOMMA) stringappend2(b, ", ");
			else stringappend4(b, '\t');
			
			lyricalimmval* imm = i->imm;
			
			keepprocessing:
			
			switch(imm->type) {
				
				case LYRICALIMMVALUE:
					
					if ((s64)imm->n < 0) {
						// If the integer value was negative
						// I include the sign of the value.
						s = stringfmt("-%d", -imm->n);
					} else s = stringfmt("%d", imm->n);
					
					stringappend1(b, s);
					
					mmfree(s.ptr);
					
					break;
					
				case LYRICALIMMOFFSETTOINSTRUCTION:
					// I create a label name representing
					// the instruction address.
					// I use the address of the struct lyricalinstruction
					// in order to generate a unique label name.
					s = stringfmt("label_%08x", imm->i);
					
					// Here I insert the label string at
					// the location for the instruction only
					// if it had not been previously inserted.
					if (!(*(string*)imm->i->backenddata).ptr || !stringiseq4((*(string*)imm->i->backenddata).ptr, "label_", 6)) {
						stringinsert2((string*)imm->i->backenddata, ":\n", 0);
						stringinsert1((string*)imm->i->backenddata, s, 0);
					}
					
					stringappend1(b, s);
					
					mmfree(s.ptr);
					
					break;
					
				case LYRICALIMMOFFSETTOFUNCTION:
					// I create the label name representing
					// the start of the function.
					// I use the address of the struct lyricalfunction
					// in order to generate a unique label name.
					;u8* linkingsignatureptr = imm->f->linkingsignature.ptr;
					if (linkingsignatureptr)
						s = stringfmt("function_%08x:%s", imm->f, linkingsignatureptr);
					else s = stringfmt("function_%08x", imm->f);
					
					stringappend1(b, s);
					
					mmfree(s.ptr);
					
					break;
					
				case LYRICALIMMOFFSETTOGLOBALREGION:
					
					stringappend2(b, "OFFSET_TO_GLOBAL_REGION");
					
					break;
					
				case LYRICALIMMOFFSETTOSTRINGREGION:
					
					stringappend2(b, "OFFSET_TO_STRING_REGION");
					
					break;
			}
			
			if (imm->next) {
				imm = imm->next;
				stringappend2(b, " + ");
				goto keepprocessing;
			}
		}
		
		while (1) {
			// Used to detect whether filepath is different.
			static string savedfilepath = stringnull;
			
			// Used to detect whether linenumber is different.
			static uint savedlinenumber = 0;
			
			// Print debug information location if any.
			// TODO: Temporary ... Instead there will
			// be an lyricalop which will be generated
			// when parsing a new file or new line.
			if (i->dbginfo.linenumber) {
				
				if (!stringiseq1(i->dbginfo.filepath, savedfilepath)) {
					
					stringappend2(b, "# ");
					stringappend1(b, i->dbginfo.filepath);
					stringappend4(b, '\n');
					stringappend1(b, indent);
					
					savedfilepath = i->dbginfo.filepath;
					
					savedlinenumber = 0;
				}
				
				if (i->dbginfo.linenumber != savedlinenumber) {
					
					string s = stringfmt("# %d: ", i->dbginfo.linenumber);
					stringappend1(b, s);
					mmfree(s.ptr);
					s = filegetline2(i->dbginfo.filepath.ptr, i->dbginfo.lineoffset);
					u8* sptr = s.ptr;
					if (sptr) {
						while (*sptr == ' ' || *sptr == '\t') ++sptr;
						stringappend2(b, sptr);
						mmfree(s.ptr);
					}
					stringappend4(b, '\n');
					stringappend1(b, indent);
					
					savedlinenumber = i->dbginfo.linenumber;
				}
				
			} else if (savedlinenumber) {
				
				savedfilepath = stringnull;
				savedlinenumber = 0;
			}
			
			if (i->op != LYRICALOPCOMMENT) {
				string s = stringfmt("0x%x: ", i->dbginfo.binoffset);
				stringappend1(b, s);
				mmfree(s.ptr);
			}
			
			switch(i->op) {
				
				case LYRICALOPADD:
					stringappend2(b, "add");
					appendarg3reg();
					break;
					
				case LYRICALOPADDI:
					stringappend2(b, "addi");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSUB:
					stringappend2(b, "sub");
					appendarg3reg();
					break;
					
				case LYRICALOPNEG:
					stringappend2(b, "neg");
					appendarg2reg();
					break;
					
				case LYRICALOPMUL:
					stringappend2(b, "mul");
					appendarg3reg();
					break;
					
				case LYRICALOPMULH:
					stringappend2(b, "mulh");
					appendarg3reg();
					break;
					
				case LYRICALOPDIV:
					stringappend2(b, "div");
					appendarg3reg();
					break;
					
				case LYRICALOPMOD:
					stringappend2(b, "mod");
					appendarg3reg();
					break;
					
				case LYRICALOPMULHU:
					stringappend2(b, "mulhu");
					appendarg3reg();
					break;
					
				case LYRICALOPDIVU:
					stringappend2(b, "divu");
					appendarg3reg();
					break;
					
				case LYRICALOPMODU:
					stringappend2(b, "modu");
					appendarg3reg();
					break;
					
				case LYRICALOPMULI:
					stringappend2(b, "muli");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPMULHI:
					stringappend2(b, "mulhi");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPDIVI:
					stringappend2(b, "divi");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPMODI:
					stringappend2(b, "modi");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPDIVI2:
					stringappend2(b, "divi2");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPMODI2:
					stringappend2(b, "modi2");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPMULHUI:
					stringappend2(b, "mulhui");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPDIVUI:
					stringappend2(b, "divui");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPMODUI:
					stringappend2(b, "modui");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPDIVUI2:
					stringappend2(b, "divui2");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPMODUI2:
					stringappend2(b, "modui2");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPAND:
					stringappend2(b, "and");
					appendarg3reg();
					break;
					
				case LYRICALOPANDI:
					stringappend2(b, "andi");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPOR:
					stringappend2(b, "or");
					appendarg3reg();
					break;
					
				case LYRICALOPORI:
					stringappend2(b, "ori");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPXOR:
					stringappend2(b, "xor");
					appendarg3reg();
					break;
					
				case LYRICALOPXORI:
					stringappend2(b, "xori");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPNOT:
					stringappend2(b, "not");
					appendarg2reg();
					break;
					
				case LYRICALOPCPY:
					stringappend2(b, "cpy");
					appendarg2reg();
					break;
					
				case LYRICALOPSLL:
					stringappend2(b, "sll");
					appendarg3reg();
					break;
					
				case LYRICALOPSLLI:
					stringappend2(b, "slli");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSLLI2:
					stringappend2(b, "slli2");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSRL:
					stringappend2(b, "srl");
					appendarg3reg();
					break;
					
				case LYRICALOPSRLI:
					stringappend2(b, "srli");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSRLI2:
					stringappend2(b, "srli2");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSRA:
					stringappend2(b, "sra");
					appendarg3reg();
					break;
					
				case LYRICALOPSRAI:
					stringappend2(b, "srai");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSRAI2:
					stringappend2(b, "srai2");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPZXT:
					stringappend2(b, "zxt");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSXT:
					stringappend2(b, "sxt");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSEQ:
					stringappend2(b, "seq");
					appendarg3reg();
					break;
					
				case LYRICALOPSNE:
					stringappend2(b, "sne");
					appendarg3reg();
					break;
					
				case LYRICALOPSEQI:
					stringappend2(b, "seqi");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSNEI:
					stringappend2(b, "snei");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSLT:
					stringappend2(b, "slt");
					appendarg3reg();
					break;
					
				case LYRICALOPSLTE:
					stringappend2(b, "slte");
					appendarg3reg();
					break;
					
				case LYRICALOPSLTU:
					stringappend2(b, "sltu");
					appendarg3reg();
					break;
					
				case LYRICALOPSLTEU:
					stringappend2(b, "slteu");
					appendarg3reg();
					break;
					
				case LYRICALOPSLTI:
					stringappend2(b, "slti");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSLTEI:
					stringappend2(b, "sltei");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSLTUI:
					stringappend2(b, "sltui");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSLTEUI:
					stringappend2(b, "slteui");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSGTI:
					stringappend2(b, "sgti");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSGTEI:
					stringappend2(b, "sgtei");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSGTUI:
					stringappend2(b, "sgtui");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSGTEUI:
					stringappend2(b, "sgteui");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSZ:
					stringappend2(b, "sz");
					appendarg2reg();
					break;
					
				case LYRICALOPSNZ:
					stringappend2(b, "snz");
					appendarg2reg();
					break;
					
				case LYRICALOPJEQ:
					stringappend2(b, "jeq");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJEQI:
					stringappend2(b, "jeqi");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJEQR:
					stringappend2(b, "jeqr");
					appendarg3reg();
					break;
					
				case LYRICALOPJNE:
					stringappend2(b, "jne");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJNEI:
					stringappend2(b, "jnei");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJNER:
					stringappend2(b, "jner");
					appendarg3reg();
					break;
					
				case LYRICALOPJLT:
					stringappend2(b, "jlt");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJLTI:
					stringappend2(b, "jlti");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJLTR:
					stringappend2(b, "jltr");
					appendarg3reg();
					break;
					
				case LYRICALOPJLTE:
					stringappend2(b, "jlte");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJLTEI:
					stringappend2(b, "jltei");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJLTER:
					stringappend2(b, "jlter");
					appendarg3reg();
					break;
					
				case LYRICALOPJLTU:
					stringappend2(b, "jltu");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJLTUI:
					stringappend2(b, "jltui");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJLTUR:
					stringappend2(b, "jltur");
					appendarg3reg();
					break;
					
				case LYRICALOPJLTEU:
					stringappend2(b, "jlteu");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJLTEUI:
					stringappend2(b, "jlteui");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJLTEUR:
					stringappend2(b, "jlteur");
					appendarg3reg();
					break;
					
				case LYRICALOPJZ:
					stringappend2(b, "jz");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJZI:
					stringappend2(b, "jzi");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJZR:
					stringappend2(b, "jzr");
					appendarg2reg();
					break;
					
				case LYRICALOPJNZ:
					stringappend2(b, "jnz");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJNZI:
					stringappend2(b, "jnzi");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJNZR:
					stringappend2(b, "jnzr");
					appendarg2reg();
					break;
					
				case LYRICALOPJ:
					stringappend2(b, "j");
					appendargimm(DONOTBEGINWITHCOMMA);
					break;
					
				case LYRICALOPJI:
					stringappend2(b, "ji");
					appendargimm(DONOTBEGINWITHCOMMA);
					break;
					
				case LYRICALOPJR:
					stringappend2(b, "jr");
					appendarg1reg();
					break;
					
				case LYRICALOPJL:
					stringappend2(b, "jl");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJLI:
					stringappend2(b, "jli");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPJLR:
					stringappend2(b, "jlr");
					appendarg2reg();
					break;
					
				case LYRICALOPJPUSH:
					stringappend2(b, "jpush");
					appendargimm(DONOTBEGINWITHCOMMA);
					break;
					
				case LYRICALOPJPUSHI:
					stringappend2(b, "jpushi");
					appendargimm(DONOTBEGINWITHCOMMA);
					break;
					
				case LYRICALOPJPUSHR:
					stringappend2(b, "jpushr");
					appendarg1reg();
					break;
					
				case LYRICALOPJPOP:
					stringappend2(b, "jpop");
					break;
					
				case LYRICALOPAFIP:
					stringappend2(b, "afip");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPLI:
					stringappend2(b, "li");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPLD8:
					stringappend2(b, "ld8");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPLD8R:
					stringappend2(b, "ld8r");
					appendarg2reg();
					break;
					
				case LYRICALOPLD8I:
					stringappend2(b, "ld8i");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPLD16:
					stringappend2(b, "ld16");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPLD16R:
					stringappend2(b, "ld16r");
					appendarg2reg();
					break;
					
				case LYRICALOPLD16I:
					stringappend2(b, "ld16i");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPLD32:
					stringappend2(b, "ld32");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPLD32R:
					stringappend2(b, "ld32r");
					appendarg2reg();
					break;
					
				case LYRICALOPLD32I:
					stringappend2(b, "ld32i");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPLD64:
					stringappend2(b, "ld64");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPLD64R:
					stringappend2(b, "ld64r");
					appendarg2reg();
					break;
					
				case LYRICALOPLD64I:
					stringappend2(b, "ld64i");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPST8:
					stringappend2(b, "st8");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPST8R:
					stringappend2(b, "st8r");
					appendarg2reg();
					break;
					
				case LYRICALOPST8I:
					stringappend2(b, "st8i");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPST16:
					stringappend2(b, "st16");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPST16R:
					stringappend2(b, "st16r");
					appendarg2reg();
					break;
					
				case LYRICALOPST16I:
					stringappend2(b, "st16i");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPST32:
					stringappend2(b, "st32");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPST32R:
					stringappend2(b, "st32r");
					appendarg2reg();
					break;
					
				case LYRICALOPST32I:
					stringappend2(b, "st32i");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPST64:
					stringappend2(b, "st64");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPST64R:
					stringappend2(b, "st64r");
					appendarg2reg();
					break;
					
				case LYRICALOPST64I:
					stringappend2(b, "st64i");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPLDST8:
					stringappend2(b, "ldst8");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPLDST8R:
					stringappend2(b, "ldst8r");
					appendarg2reg();
					break;
					
				case LYRICALOPLDST8I:
					stringappend2(b, "ldst8i");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPLDST16:
					stringappend2(b, "ldst16");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPLDST16R:
					stringappend2(b, "ldst16r");
					appendarg2reg();
					break;
					
				case LYRICALOPLDST16I:
					stringappend2(b, "ldst16i");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPLDST32:
					stringappend2(b, "ldst32");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPLDST32R:
					stringappend2(b, "ldst32r");
					appendarg2reg();
					break;
					
				case LYRICALOPLDST32I:
					stringappend2(b, "ldst32i");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPLDST64:
					stringappend2(b, "ldst64");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPLDST64R:
					stringappend2(b, "ldst64r");
					appendarg2reg();
					break;
					
				case LYRICALOPLDST64I:
					stringappend2(b, "ldst64i");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPMEM8CPY:
					stringappend2(b, "mem8cpy");
					appendarg3reg();
					break;
					
				case LYRICALOPMEM8CPYI:
					stringappend2(b, "mem8cpyi");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPMEM8CPY2:
					stringappend2(b, "mem8cpy2");
					appendarg3reg();
					break;
					
				case LYRICALOPMEM8CPYI2:
					stringappend2(b, "mem8cpyi2");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPMEM16CPY:
					stringappend2(b, "mem16cpy");
					appendarg3reg();
					break;
					
				case LYRICALOPMEM16CPYI:
					stringappend2(b, "mem16cpyi");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPMEM16CPY2:
					stringappend2(b, "mem16cpy2");
					appendarg3reg();
					break;
					
				case LYRICALOPMEM16CPYI2:
					stringappend2(b, "mem16cpyi2");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPMEM32CPY:
					stringappend2(b, "mem32cpy");
					appendarg3reg();
					break;
					
				case LYRICALOPMEM32CPYI:
					stringappend2(b, "mem32cpyi");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPMEM32CPY2:
					stringappend2(b, "mem32cpy2");
					appendarg3reg();
					break;
					
				case LYRICALOPMEM32CPYI2:
					stringappend2(b, "mem32cpyi2");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPMEM64CPY:
					stringappend2(b, "mem64cpy");
					appendarg3reg();
					break;
					
				case LYRICALOPMEM64CPYI:
					stringappend2(b, "mem64cpyi");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPMEM64CPY2:
					stringappend2(b, "mem64cpy2");
					appendarg3reg();
					break;
					
				case LYRICALOPMEM64CPYI2:
					stringappend2(b, "mem64cpyi2");
					appendarg2reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPPAGEALLOC:
					stringappend2(b, "pagealloc");
					appendarg2reg();
					break;
					
				case LYRICALOPPAGEALLOCI:
					stringappend2(b, "pagealloci");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSTACKPAGEALLOC:
					stringappend2(b, "stackpagealloc");
					appendarg1reg();
					break;
					
				case LYRICALOPPAGEFREE:
					stringappend2(b, "pagefree");
					appendarg2reg();
					break;
					
				case LYRICALOPPAGEFREEI:
					stringappend2(b, "pagefreei");
					appendarg1reg();
					appendargimm(BEGINWITHCOMMA);
					break;
					
				case LYRICALOPSTACKPAGEFREE:
					stringappend2(b, "stackpagefree");
					appendarg1reg();
					break;
					
				case LYRICALOPMACHINECODE:
					stringappend2(b, "machinecode");
					break;
					
				case LYRICALOPNOP:
					// Do not make it to
					// the final result;
					// It is handled here
					// so as to be easily
					// ignored.
					stringappend2(b, "nop");
					break;
					
				case LYRICALOPCOMMENT:
					// Instructions between the comment instruction
					// starting with "begin:" and the comment instruction
					// starting with "end:" get indented.
					if (stringiseq4(i->comment.ptr, "begin:", 6))
						stringappend4(&indent, '\t');
					else if (stringiseq4(i->comment.ptr, "end:", 4)) {
						// I remove the tab that
						// was appended to *b
						// and decrease indent
						// by one tab as well.
						stringchop(b, 1);
						stringchop(&indent, 1);
					}
					stringappend2(b, "# ");
					stringappend1(b, i->comment);
					break;
					
				default:
					stringappend2(b, "# unsupported instruction");
					break;
			}
			
			stringappend4(b, '\n');
			
			if ((i = i->next) == f->i->next) break;
			
			b = (string*)i->backenddata;
			
			stringappend1(b, indent);
		}
		
		doneprocessingfunc:
		stringappend4(b, '\n');
		if (indent.ptr) mmfree(indent.ptr);
		
	} while ((f = f->next) != compileresult.rootfunc);
	
	// When I get here f == compileresult.rootfunc;
	
	// Will contain the final string to return.
	string result = stringnull;
	
	if (compileresult.stringregion.ptr) {
		// I write the size of the string region.
		string s = stringfmt("# string region size: %d\n", arrayu8sz(compileresult.stringregion));
		stringappend1(&result, s);
		mmfree(s.ptr);
	}
	
	if (compileresult.globalregionsz) {
		// I write the size of the global region.
		string s = stringfmt("# global region size: %d\n\n", compileresult.globalregionsz);
		stringappend1(&result, s);
		mmfree(s.ptr);
	}
	
	if (exports.ptr) {
		// I write the list of exported functions.
		stringappend2(&result, "# Exports: \n");
		stringappend1(&result, exports);
		mmfree(exports.ptr);
		stringappend4(&result, '\n');
	}
	
	if (imports.ptr) {
		// I write the list of imported functions.
		stringappend2(&result, "# Imports: \n");
		stringappend1(&result, imports);
		mmfree(imports.ptr);
		stringappend4(&result, '\n');
	}
	
	do {
		// Note that f->i is always non-null,
		// so there is no need to check it.
		
		// Note that f->i point to the last lyricalinstruction
		// of the circular linkedlist and f->i->next point to
		// the first lyricalinstruction of the linkedlist.
		lyricalinstruction* i = f->i->next;
		
		lyricalinstruction* firstlyricalinstruction = i;
		
		do {
			string* b = (string*)i->backenddata;
			
			// Ignore LYRICALOPNOP.
			if (i->op != LYRICALOPNOP) stringappend1(&result, *b);
			
			// There is no need to check whether
			// b->ptr is non-null, since for every
			// lyricalinstruction an equivalent
			// text string was generated.
			mmfree(b->ptr);
			
			mmfree(b);
			
			i = i->next;
			
		} while (i != firstlyricalinstruction);
		
	} while ((f = f->next) != compileresult.rootfunc);
	
	return result;
}
