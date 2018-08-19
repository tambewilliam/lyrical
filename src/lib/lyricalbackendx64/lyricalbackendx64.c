
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


#include <sys/mman.h>
#ifndef MAP_STACK
#define MAP_STACK 0x20000
#endif
#ifndef MAP_UNINITIALIZED
#define MAP_UNINITIALIZED 0x4000000
#endif

#include <stdtypes.h>
#include <macros.h>
#include <byt.h>
#include <mm.h>
#include <string.h>
#include <arrayu8.h>
#include <arrayu64.h>
#include <bintree.h>
#include <lyrical.h>


// # On Linux, to disassemble a raw binary file, use the following command:
// objdump --start-address=0x0 -D -b binary -mi386:x86-64 -Mintel,addr64,data64 <filename>
// # Modify the option --start-address to point
// # to where disassembling should start.


#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "little endian required"
#endif


// Structure used by
// lyricalbackendx64()
// to return the executable
// binary generated.
typedef struct {
	// Layout of the executable binary.
	// 
	// ----------------------------
	// Instructions to execute.
	// ----------------------------
	// Null terminated strings.
	// ----------------------------
	
	// The region for global variables
	// is not part of the generated
	// executable binary, and must be
	// allocated immediately after
	// the executable binary when
	// it is loaded in memory.
	
	// Executable binary.
	arrayu8 execbin;
	
	// Fields holding the size of
	// the executable instructions and
	// constant strings in the executable
	// binary; as well as the size of
	// global variables which is not
	// part of the executable binary.
	uint executableinstrsz;
	uint constantstringssz;
	uint globalvarregionsz;
	
	// Export information is a serie of
	// null terminated function signature string
	// followed by a u64 written in little endian
	// that is the offset within the executable
	// where the export is to be found.
	
	// Export information.
	arrayu8 exportinfo;
	
	// Import information is a serie of null
	// terminated function signature string that are
	// followed by a u64 written in little endian
	// which is the offset within the string region
	// from where the import offset is to be retrieved.
	// The import offset is an offset to the import
	// from where it is written in the string region.
	
	// Import information.
	arrayu8 importinfo;
	
	// Debug information are organized in sections:
	// Section1: Debug information allowing to
	// 	determine to which source code line
	// 	a specific binary instruction originated.
	// 	Each entry has the following fields:
	// 	u64 binoffset: Offset of the instruction in the binary.
	// 	u64 filepath: Offset of the source code absolute filepath string within section2 debug information.
	// 	u64 linenumber: Line number within the source code.
	// 	u64 lineoffset: Byte offset of the line within the source code.
	// 	The last entry has its field "linenumber" null,
	// 	while its field "binoffset" has the upper limit offset;
	// 	that last entry is not meant to be used, and
	// 	solely mark the end of section1 debug information.
	// Section2: Debug information holding
	// 	null-terminated strings used by
	// 	other debug information sections.
	// 
	// Layout of the debug information.
	// 
	// ------------------------------------------------
	// u64 size in bytes of section1 debug information.
	// ------------------------------------------------
	// Section1 debug information.
	// ------------------------------------------------
	// u64 size in bytes of section2 debug information.
	// ------------------------------------------------
	// Section2 debug information.
	// ------------------------------------------------
	
	// Debug information.
	arrayu8 dbginfo;
	
} lyricalbackendx64result;

// enum used with the argument flag
// of lyricalbackendx64().
typedef enum {
	// When used, the constant strings
	// and global variable regions are
	// aligned to 64 bits (8 bytes).
	LYRICALBACKENDX64COMPACT,
	
	// When used, the executable binary is
	// generated as if LYRICALBACKENDX64COMPACT
	// was used, but the instructions
	// to execute assume that the constant
	// strings and global variable regions
	// are aligned to a pagesize (4096 bytes);
	// hence when loading the executable
	// binary in memory, the constant strings
	// data must start at the next address,
	// after the instructions to execute,
	// that is aligned to a pagesize; the global
	// variable region must start at the next
	// address, after the constant strings
	// region, that is aligned to a pagesize.
	LYRICALBACKENDX64COMPACTPAGEALIGNED,
	
	// When used, the constant strings
	// and global variable regions are
	// aligned to a pagesize (4096 bytes).
	LYRICALBACKENDX64PAGEALIGNED,
	
} lyricalbackendx64flag;

enum {
	IMM64 = 1,	// 64bits immediate.
	IMM32,		// 32bits immediate.
	IMM8,		// 8bits immediate.
};

// Backend which convert
// the result of the compilation
// to x64 instructions.
// The field rootfunc of the
// lyricalcompileresult used as
// argument should be non-null.
// Null is returned on faillure.
lyricalbackendx64result* lyricalbackendx64 (lyricalcompileresult compileresult, lyricalbackendx64flag flag) {
	// In generating instructions,
	// this function assume that
	// the L-bit of the Code Segment
	// is set so that the default operand
	// and address sizes are 64bits.
	
	// The overall format of x64
	// instructions is as follow:
	// | Prefix | REX Prefix | Opcode | ModR/M | SIB | Disp | Imm |
	// Where each field other than Opcode
	// may not exist in the instruction.
	
	// Label to return to
	// when an error happen.
	// An error happen only
	// when an unrecognized
	// instruction is found.
	__label__ error;
	
	// Used to throw an error.
	// Using a function-call instead of
	// a mere goto allow for backtracing
	// where the error occured.
	void throwerror () {
		goto error;
	}
	
	// This check insure that the compiler
	// used immediate values with size
	// greater than or equal to sizeof(u64).
	if (sizeof(u64) > sizeof(((lyricalimmval){}).n))
		throwerror();
	
	// Structure describing
	// the fields of the data
	// attached to each instruction
	// to convert to its binary
	// executable equivalent.
	typedef struct {
		// Executable binary
		// of the instruction.
		arrayu8 binary;
		
		// This field hold the
		// byte offset location,
		// among all generated
		// instructions, of the
		// data in the field binary.
		uint binaryoffset;
		
		// This field is non-null
		// if the instruction used
		// an immediate value.
		// When non-null, it provide
		// additional information
		// about the immediate value:
		// When 1, the immediate
		// value is 64 bits.
		// When 2, the immediate
		// value is 32 bits.
		// When 3, the immediate
		// value is 8 bits.
		uint isimmused;
		
		// Similar to the field isimmused,
		// but used for the secondary immediate.
		uint isimm2used;
		
		// This field hold a byte offset,
		// within the data generated in
		// the field binary, where the
		// immediate value is to be written
		// during the last stage of the
		// conversion when every relative
		// position offsets can be
		// determined and calculated.
		uint immfieldoffset;
		
		// Similar to the field field immfieldoffset,
		// but used for the secondary immediate.
		uint imm2fieldoffset;
		
		// This field hold a value
		// that is added to the
		// immediate value
		// of the instruction.
		u64 immmiscvalue;
		
		// Similar to the field field immmiscvalue,
		// but used for the secondary immediate.
		u64 imm2miscvalue;
		
	} backenddata;
	
	lyricalfunction* f = compileresult.rootfunc;
	
	// I create a new session
	// which will allow to regain
	// any allocated memory block
	// if an error is thrown.
	mmsession convertsession = mmsessionnew();
	
	// Count of registers.
	enum {REGCOUNT = 16};
	
	// The compiler assume that a pagesize
	// is 4096 bytes when generating
	// instructions that manipulate the stack.
	enum {PAGESIZE = 4096};
	
	// Array that will be used to track
	// which register was used to save
	// the value of the register given
	// as argument to savereg().
	// When the value of an element
	// of the array is -1, it mean
	// that the corresponding register
	// was saved in the stack.
	uint savedregs[REGCOUNT] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	
	// Array that will be used
	// to temporarily mark an
	// unused register used.
	uint regsinusetmp[REGCOUNT] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	
	// Binary tree that will be used
	// to store all lyricalinstruction*
	// to redo.
	bintree toredo = bintreenull;
	
	// Label I jump to in order to
	// begin redoing lyricalinstruction.
	redo:;
	
	// Array which will hold
	// section1 debug information.
	arrayu64 dbginfosection1;
	
	// Array which will hold
	// pointers to strings for
	// section2 debug information.
	arrayu64 dbginfosection2;
	// Variable which store the total length
	// of all strings for which the pointers
	// are stored in dbginfosection2.
	uint dbginfosection2len = 0;
	
	dbginfosection1 = arrayu64null;
	
	// Reserve space at the top
	// for storing the byte usage
	// of section1 debug information.
	*arrayu64append1(&dbginfosection1) = 0;
	
	dbginfosection2 = arrayu64null;
	
	// Variable used to keep track
	// of the size of instructions
	// to execute.
	uint executableinstrsz = 0;
	
	lyricalinstruction* i;
	
	// Variable which get set
	// to the backenddata* set
	// in i->backenddata.
	backenddata* b;
	
	do {
		// Note that f->i is always non-null,
		// so there is no need to check it.
		
		// Note that f->i point to the last
		// intruction that was generated and
		// f->i->next point to the first
		// instruction generated.
		i = f->i->next;
		
		// Enum used to map x64 register names
		// to Lyrical register numbering.
		enum {
			RSP = 0,
			RAX = 1,
			RBX = 2,
			RCX = 3,
			RDX = 4,
			RBP = 5,
			RDI = 6,
			RSI = 7,
			R8  = 8,
			R9  = 9,
			R10 = 10,
			R11 = 11,
			R12 = 12,
			R13 = 13,
			R14 = 14,
			R15 = 15,
		};
		
		// Convert the value of the field id
		// of an lyricalreg to its equivalent
		// value useable in the field REG or
		// R/M of the ModR/M byte, or the field
		// BASE or INDEX of the SIB byte.
		u8 lookupreg (uint id) {
			
			switch(id) {
				
				case RSP: return 4;
				
				case RAX: return 0;
				
				case RBX: return 3;
				
				case RCX: return 1;
				
				case RDX: return 2;
				
				case RBP: return 5;
				
				case RDI: return 7;
				
				case RSI: return 6;
				
				case R8 : return 8;
				
				case R9 : return 9;
				
				case R10: return 10;
				
				case R11: return 11;
				
				case R12: return 12;
				
				case R13: return 13;
				
				case R14: return 14;
				
				case R15: return 15;
				
				default: throwerror();
			}
		}
		
		#define LOOKUPREGRSP 4
		#define LOOKUPREGRBP 5
		
		// This function set the fields
		// isimmused, immfieldoffset and
		// immmiscvalue (increment it).
		// of the instruction pointed by i;
		// those fields are used later when
		// computing the actual immediate value.
		// This function also create space for
		// the 8bits immediate value to be set later.
		void append8bitsimm (uint n) {
			// Set the attributes
			// of the immediate value.
			b->isimmused = IMM8;
			b->immfieldoffset = arrayu8sz(b->binary);
			b->immmiscvalue += n;
			
			// Here I simply create space
			// for a 8bits immediate value
			// to be resolved later.
			arrayu8append1(&b->binary);
		}
		
		// This function set the fields
		// isimm2used, imm2fieldoffset and
		// imm2miscvalue (increment it).
		// of the instruction pointed by i;
		// those fields are used later when
		// computing the actual immediate2 value.
		// This function also create space for
		// the 8bits immediate2 value to be set later.
		void append8bitsimm2 (uint n) {
			// Set the attributes
			// of the immediate2 value.
			b->isimm2used = IMM8;
			b->imm2fieldoffset = arrayu8sz(b->binary);
			b->imm2miscvalue += n;
			
			// Here I simply create space
			// for a 8bits immediate2 value
			// to be resolved later.
			arrayu8append1(&b->binary);
		}
		
		// This function set the fields
		// isimmused, immfieldoffset and
		// immmiscvalue (increment it)
		// of the instruction pointed by i;
		// those fields are used later when
		// computing the actual immediate value.
		// This function also create space for
		// the 32bits immediate value to be set later.
		void append32bitsimm (u32 n) {
			// Set the attributes
			// of the immediate value.
			b->isimmused = IMM32;
			b->immfieldoffset = arrayu8sz(b->binary);
			b->immmiscvalue += n;
			
			// Here I simply create space
			// for a 32bits immediate value
			// to be resolved later.
			arrayu8append2(&b->binary, sizeof(u32));
		}
		
		// This function set the fields
		// isimm2used, imm2fieldoffset and
		// imm2miscvalue (increment it)
		// of the instruction pointed by i;
		// those fields are used later when
		// computing the actual immediate2 value.
		// This function also create space for
		// the 32bits immediate2 value to be set later.
		void append32bitsimm2 (u32 n) {
			// Set the attributes
			// of the immediate2 value.
			b->isimm2used = IMM32;
			b->imm2fieldoffset = arrayu8sz(b->binary);
			b->imm2miscvalue += n;
			
			// Here I simply create space
			// for a 32bits immediate2 value
			// to be resolved later.
			arrayu8append2(&b->binary, sizeof(u32));
		}
		
		// This function set the fields
		// isimmused, immfieldoffset and
		// immmiscvalue (increment it)
		// of the instruction pointed by i;
		// those fields are used later when
		// computing the actual immediate value.
		// This function also create space for
		// the 64bits immediate value to be set later.
		void append64bitsimm (u64 n) {
			// Set the attributes
			// of the immediate value.
			b->isimmused = IMM64;
			b->immfieldoffset = arrayu8sz(b->binary);
			b->immmiscvalue += n;
			
			// Here I simply create space
			// for a 64bits immediate value
			// to be resolved later.
			arrayu8append2(&b->binary, sizeof(u64));
		}
		
		// This function set the fields
		// isimm2used, imm2fieldoffset and
		// imm2miscvalue (increment it).
		// of the instruction pointed by i;
		// those fields are used later when
		// computing the actual immediate2 value.
		// This function also create space for
		// the 64bits immediate2 value to be set later.
		void append64bitsimm2 (u64 n) {
			// Set the attributes
			// of the immediate2 value.
			b->isimm2used = IMM64;
			b->imm2fieldoffset = arrayu8sz(b->binary);
			b->imm2miscvalue += n;
			
			// Here I simply create space
			// for a 64bits immediate2 value
			// to be resolved later.
			arrayu8append2(&b->binary, sizeof(u64));
		}
		
		void nop () {
			// Specification from the Intel manual.
			// 90		NOP		#One byte no-operation instruction.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x90;
		}
		
		// Instruction size in bytes
		// of the NOP instruction.
		enum {NOPINSTRSZ = 1};
		
		void cpy (uint r1, uint r2) {
			// Specification from the Intel manual.
			// REX.W 89 /r	MOV r/m64,r64	#Move r64 to r/m64.
			
			u8 lookupregrm = lookupreg(r1);
			u8 lookupregreg = lookupreg(r2);
			
			// REX-Prefix == 0100WR0B
			u8 rex = (0x48|((lookupregreg>7)<<2)|(lookupregrm>7));
			
			lookupregrm %= 8;
			lookupregreg %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == reg[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
			u8 modrm = ((0b11<<6)|(lookupregreg<<3)|lookupregrm);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0x89;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
		}
		
		void push (uint r) {
			// Specification from the Intel manual.
			// 50+rd	PUSH r64	#Push r64; default operand size is 64-bits.
			
			u8 lookupregr = lookupreg(r);
			
			if (lookupregr > 7) {
				// REX-Prefix == 0100000B
				*arrayu8append1(&b->binary) = 0x41;
				
				lookupregr %= 8;
			}
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = (0x50+lookupregr);
		}
		
		void pop (uint r) {
			// The behavior is erroneous
			// when this instruction
			// is used with RSP; and r
			// will never be RSP since
			// the compiler do not use it
			// when allocating registers
			// to variables.
			
			// Specification from the Intel manual.
			// 58+rd	POP r64		#Pop r64; default operand size is 64-bits.
			
			u8 lookupregr = lookupreg(r);
			
			if (lookupregr > 7) {
				// REX-Prefix == 0100000B
				*arrayu8append1(&b->binary) = 0x41;
				
				lookupregr %= 8;
			}
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = (0x58+lookupregr);
		}
		
		// This function return the id
		// of an unused register; null is
		// returned if there was no unused
		// register available; note that
		// register id 0 is the stack pointer
		// register which is always in use.
		// When the argument r is non-null,
		// this function first attempt to return
		// that register if it was unused.
		uint findunusedreg (uint r) {
			
			searchagain:;
			
			uint* unusedregs = i->unusedregs;
			
			if (unusedregs) {
				
				uint s;
				
				while (s = *unusedregs) {
					// Check that the unused register
					// was not marked as temporarily used.
					if (regsinusetmp[s]) {
						++unusedregs;
						continue;
					}
					
					// If I get here, I found
					// an unused register.
					
					if (!r || s == r) return s;
					else ++unusedregs;
				}
			}
			
			// If I get here, I could not
			// find an unused register to use.
			
			if (r) {
				
				r = 0;
				
				// I reattempt the search
				// with r set to null.
				goto searchagain;
			}
			
			return 0;
		}
		
		// This function return the id
		// of an unused register that
		// can be encoded in the field reg
		// of an instruction ModR/M when
		// it is used as an 8bits operand;
		// either RAX, RBX, RCX, RDX
		// is returned when unused,
		// otherwise null is returned.
		// When the argument r is non-null,
		// this function first attempt to return
		// that register if it was unused.
		uint findunusedregfor8bitsoperand (uint r) {
			
			searchagain:;
			
			uint* unusedregs = i->unusedregs;
			
			if (unusedregs) {
				
				uint s;
				
				while (s = *unusedregs) {
					// Check that the unused register
					// is either RAX, RBX, RCX, RDX and
					// was not marked as temporarily used.
					if ((s < RAX || s > RDX) || regsinusetmp[s]) {
						++unusedregs;
						continue;
					}
					
					// If I get here, I found
					// an unused register.
					
					if (!r || s == r) return s;
				}
			}
			
			// If I get here, I could not
			// find an unused register to use.
			
			if (r) {
				
				r = 0;
				
				// I reattempt the search
				// with r set to null.
				goto searchagain;
			}
			
			return 0;
		}
		
		// Function used to save
		// the value of a register.
		// The register is saved
		// in an unused register,
		// or the stack if there were
		// no unused registers left.
		void savereg (uint r) {
			// Throw an error if called
			// for a register which
			// was already saved.
			if (savedregs[r]) throwerror();
			
			uint s;
			
			if ((s = findunusedreg(0))) {
				// Mark as temporarily used.
				regsinusetmp[s] = 1;
				
				cpy(s, r);
				
				savedregs[r] = s;
				
			} else {
				
				push(r);
				
				savedregs[r] = -1;
			}
		}
		
		// Function used to restore
		// the value of a register.
		// This function must be called
		// in the reverse order that
		// savereg() was called.
		void restorereg (uint r) {
			// Get from where to restore the register.
			uint s = savedregs[r];
			
			if (s == -1) pop(r);
			else if (s) {
				
				cpy(r, s);
				
				// Unmark as temporarily used.
				regsinusetmp[s] = 0;
				
			// Throw an error if called
			// for a register which
			// was not saved.
			} else throwerror();
			
			savedregs[r] = 0;
		}
		
		// Function which return 1
		// if the register given
		// as argument is not
		// among unused registers, or
		// is temporarily marked used.
		uint isreginuse (uint r) {
			
			uint* unusedregs = i->unusedregs;
			
			if (unusedregs) {
				
				uint s;
				
				while (s = *unusedregs) {
					
					if (s == r) return regsinusetmp[r];
					
					++unusedregs;
				}
			}
			
			return 1;
		}
		
		void add (uint r1, uint r2) {
			// Specification from the Intel manual.
			// REX.W 01 /r	ADD r/m64, r64		#Add r64 to r/m64.
			
			u8 lookupregrm = lookupreg(r1);
			u8 lookupregreg = lookupreg(r2);
			
			// REX-Prefix == 0100WR0B
			u8 rex = (0x48|((lookupregreg>7)<<2)|(lookupregrm>7));
			
			lookupregrm %= 8;
			lookupregreg %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == reg[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
			u8 modrm = ((0b11<<6)|(lookupregreg<<3)|lookupregrm);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0x01;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
		}
		
		void xor (uint r1, uint r2) {
			// Specification from the Intel manual.
			// REX.W 31 /r	XOR r/m64, r64		#r/m64 XOR r64.
			
			u8 lookupregrm = lookupreg(r1);
			u8 lookupregreg = lookupreg(r2);
			
			// REX-Prefix == 0100WR0B
			u8 rex = (0x48|((lookupregreg>7)<<2)|(lookupregrm>7));
			
			lookupregrm %= 8;
			lookupregreg %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == reg[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
			u8 modrm = ((0b11<<6)|(lookupregreg<<3)|lookupregrm);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0x31;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
		}
		
		// Function used to load the
		// immediate value associated
		// with the lyricalinstruction
		// pointed by i; the value of
		// the argument n get added
		// to the lyricalinstruction
		// immediate value.
		void ldi (uint r, u64 n) {
			
			lyricalimmval* imm = i->imm;
			
			do {
				// If the immediate value is null,
				// use xor() to set the register null.
				if (imm->type != LYRICALIMMVALUE || *(u64*)&imm->n || n) {
					// If b->isimmused is set, then
					// I am redoing the lyricalinstruction.
					if (((((sint)n < 0) ? -n : n) >= (1 << 31)) ||
						b->isimmused == IMM64) { // I also get here, if n cannot fit in a 32bits immediate.
						
						// Specification from the Intel manual.
						// REX.W B8+rd io		MOV r64, imm64		#Move imm64 to r64.
						
						u8 lookupregr = lookupreg(r);
						
						// REX-Prefix == 0100W00B
						u8 rex = (0x48|(lookupregr>7));
						
						lookupregr %= 8;
						
						// Append the opcode byte.
						*arrayu8append1(&b->binary) = rex;
						*arrayu8append1(&b->binary) = (0xb8+lookupregr);
						
						// Append the immediate value.
						append64bitsimm(n);
						
					} else {
						// Specification from the Intel manual.
						// REX.W C7 /0 id		MOV r/m64, imm32	#Move imm32 (sign-extended to 64bits) to r/m64.
						
						u8 lookupregr = lookupreg(r);
						
						// REX-Prefix == 0100W00B
						u8 rex = (0x48|(lookupregr>7));
						
						lookupregr %= 8;
						
						// From the most significant bit to the least significant bit.
						// MOD == 0b11;
						// REG == op[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						u8 modrm = ((0b11<<6)|lookupregr);
						
						// Append the opcode byte.
						*arrayu8append1(&b->binary) = rex;
						*arrayu8append1(&b->binary) = 0xc7;
						
						// Append the ModR/M byte.
						*arrayu8append1(&b->binary) = modrm;
						
						// Append the immediate value.
						append32bitsimm(n);
					}
					
					return;
				}
				
			} while (imm = imm->next);
			
			xor(r, r);
		}
		
		// Function used to load
		// an immediate2 value.
		// The value of the argument n get
		// added to the immediate2 value.
		void ldi2 (uint r, u64 n) {
			// If b->isimm2used is set, then
			// I am redoing the lyricalinstruction.
			if (((((sint)n < 0) ? -n : n) >= (1 << 31)) ||
				b->isimm2used == IMM64) { // I also get here, if n cannot fit in a 32bits immediate2.
				
				// Specification from the Intel manual.
				// REX.W B8+rd io		MOV r64, imm64		#Move imm64 to r64.
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = (0xb8+lookupregr);
				
				// Append the immediate2 value.
				append64bitsimm2(n);
				
			} else {
				// Specification from the Intel manual.
				// REX.W C7 /0 id		MOV r/m64, imm32	#Move imm32 (sign-extended to 64bits) to r/m64.
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0xc7;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate2 value.
				append32bitsimm2(n);
			}
		}
		
		void addi (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				add(r, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// REX.W 81 /0 id	ADD r/m64, imm32	#Add imm32 (sign-extended to 64bits) to r/m64.
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x81;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// REX.W 83 /0 ib	ADD r/m64, imm8	#Add imm8 (sign-extended to 64bits) to r/m64.
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x83;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void addi2 (uint r) {
			// If b->isimm2used is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimm2used == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi2(unusedreg, 0);
				add(r, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else if (b->isimm2used == IMM32) {
				// Specification from the Intel manual.
				// REX.W 81 /0 id	ADD r/m64, imm32	#Add imm32 (sign-extended to 64bits) to r/m64.
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x81;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate2 value.
				append32bitsimm2(0);
				
			} else {
				// Specification from the Intel manual.
				// REX.W 83 /0 ib	ADD r/m64, imm8	#Add imm8 (sign-extended to 64bits) to r/m64.
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x83;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate2 value.
				append8bitsimm2(0);
			}
		}
		
		// Commented out; kept for reference.
		#if 0
		void inc (uint r) {
			// Specification from the Intel manual.
			// REX.W FF /0	INC r/m64	#Increment r/m64 by 1.
			
			u8 lookupregr = lookupreg(r);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregr>7));
			
			lookupregr %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregr);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xff;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
		}
		
		// Increment register by 8bits immediate
		// value given as argument.
		void inc8 (uint r, u8 n) {
			// Specification from the Intel manual.
			// REX.W 83 /0 ib	ADD r/m64, imm8	#Add imm8 (sign-extended to 64bits) to r/m64.
			
			u8 lookupregr = lookupreg(r);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregr>7));
			
			lookupregr %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregr);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0x83;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// Append 8bits immediate value.
			*arrayu8append1(&b->binary) = n;
		}
		#endif
		
		void sub (uint r1, uint r2) {
			// Specification from the Intel manual.
			// REX.W 29 /r	SUB r/m64, r64		#Subtract r64 from r/m64.
			
			u8 lookupregrm = lookupreg(r1);
			u8 lookupregreg = lookupreg(r2);
			
			// REX-Prefix == 0100WR0B
			u8 rex = (0x48|((lookupregreg>7)<<2)|(lookupregrm>7));
			
			lookupregrm %= 8;
			lookupregreg %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == reg[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
			u8 modrm = ((0b11<<6)|(lookupregreg<<3)|lookupregrm);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0x29;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
		}
		
		// Not used; kept for reference.
		#if 0
		void subi (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				sub(r, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// REX.W 81 /5 id	SUB r/m64, imm32	#Substract imm32 (sign-extended to 64bits) from r/m64.
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|(5<<3)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x81;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// REX.W 83 /5 ib	SUB r/m64, imm8	#Substract imm8 (sign-extended to 64bits) from r/m64.
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|(5<<3)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x83;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		#endif
		
		void neg (uint r) {
			// Specification from the Intel manual.
			// REX.W F7 /3	NEG r/m64	#Two's complement negate r/m64.
			
			u8 lookupregr = lookupreg(r);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregr>7));
			
			lookupregr %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(3<<3)|lookupregr);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
		}
		
		// Function used to atomically
		// xchg the value of two registers.
		void xchg (uint r1, uint r2) {
			// Specification from the Intel manual.
			// REX.W 87 /r	XCHG r/m64, r64		#Exchange r64 with quadword from r/m64.
			
			u8 lookupregrm = lookupreg(r1);
			u8 lookupregreg = lookupreg(r2);
			
			// REX-Prefix == 0100WR0B
			u8 rex = (0x48|((lookupregreg>7)<<2)|(lookupregrm>7));
			
			lookupregrm %= 8;
			lookupregreg %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == reg[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
			u8 modrm = ((0b11<<6)|(lookupregreg<<3)|lookupregrm);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0x87;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
		}
		
		void mul (uint r1, uint r2) {
			// Specification from the Intel manual.
			// REX.W 0F AF /r	IMUL r64, r/m64		#reg = reg * r/m64.
			
			u8 lookupregreg = lookupreg(r1);
			u8 lookupregrm = lookupreg(r2);
			
			// REX-Prefix == 0100WR0B
			u8 rex = (0x48|((lookupregreg>7)<<2)|(lookupregrm>7));
			
			lookupregrm %= 8;
			lookupregreg %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == reg[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
			u8 modrm = ((0b11<<6)|(lookupregreg<<3)|lookupregrm);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0xaf;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
		}
		
		void mulh (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r1 != RAX && r2 == RAX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the multiplicator
				// in RAX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint x;
			
			if (r2 == RAX) {
				// Look for an unused register.
				x = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!x) throwerror();
				
				regsinusetmp[x] = 1; // Mark unused register as temporarily used.
				
				cpy(x, r2);
				
			} else x = r2;
			
			uint raxwassaved = 0;
			uint rdxwassaved = 0;
			
			if (r1 != RAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RAX)) {
					savereg(RAX); raxwassaved = 1;
				} else regsinusetmp[RAX] = 1; // Mark unused register as temporarily used.
				
				cpy(RAX, r1);
			}
			
			if (r1 != RDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDX)) {
					savereg(RDX); rdxwassaved = 1;
				} else regsinusetmp[RDX] = 1; // Mark unused register as temporarily used.
			}
			
			// Specification from the Intel manual.
			// REX.W F7 /5	IMUL r/m64	#Signed multiply RDX:RAX = RAX * r/m64,
			// with the low result stored in RAX and high result stored in RDX.
			
			u8 lookupregx = lookupreg(x);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregx>7));
			
			lookupregx %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(5<<3)|lookupregx);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			if (r1 != RDX) {
				
				cpy(r1, RDX);
				
				if (rdxwassaved) {
					// Restore register value.
					restorereg(RDX);
				} else regsinusetmp[RDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1 != RAX) {
				
				if (raxwassaved) {
					// Restore register value.
					restorereg(RAX);
				} else regsinusetmp[RAX] = 0; // Unmark register as temporarily used.
			}
			
			if (x != r2) regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void div (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r1 != RAX && r2 == RAX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the dividend
				// in RDX:RAX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint x;
			
			if (r2 == RAX || r2 == RDX) {
				// Look for an unused register.
				x = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!x) throwerror();
				
				regsinusetmp[x] = 1; // Mark unused register as temporarily used.
				
				cpy(x, r2);
				
			} else x = r2;
			
			uint raxwassaved = 0;
			uint rdxwassaved = 0;
			
			if (r1 != RAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RAX)) {
					savereg(RAX); raxwassaved = 1;
				} else regsinusetmp[RAX] = 1; // Mark unused register as temporarily used.
				
				cpy(RAX, r1);
			}
			
			if (r1 != RDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDX)) {
					savereg(RDX); rdxwassaved = 1;
				} else regsinusetmp[RDX] = 1; // Mark unused register as temporarily used.
			}
			
			xor(RDX, RDX);
			
			// Specification from the Intel manual.
			// REX.W F7 /7	IDIV r/m64	#Signed divide RDX:RAX by r/m64,
			// with quotient stored in RAX and remainder stored in RDX.
			
			u8 lookupregx = lookupreg(x);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregx>7));
			
			lookupregx %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(7<<3)|lookupregx);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			if (r1 != RDX) {
				
				if (rdxwassaved) {
					// Restore register value.
					restorereg(RDX);
				} else regsinusetmp[RDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1 != RAX) {
				
				cpy(r1, RAX);
				
				if (raxwassaved) {
					// Restore register value.
					restorereg(RAX);
				} else regsinusetmp[RAX] = 0; // Unmark register as temporarily used.
			}
			
			if (x != r2) regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void mod (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r1 != RAX && r2 == RAX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the dividend
				// in RDX:RAX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint x;
			
			if (r2 == RAX || r2 == RDX) {
				// Look for an unused register.
				x = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!x) throwerror();
				
				regsinusetmp[x] = 1; // Mark unused register as temporarily used.
				
				cpy(x, r2);
				
			} else x = r2;
			
			uint raxwassaved = 0;
			uint rdxwassaved = 0;
			
			if (r1 != RAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RAX)) {
					savereg(RAX); raxwassaved = 1;
				} else regsinusetmp[RAX] = 1; // Mark unused register as temporarily used.
				
				cpy(RAX, r1);
			}
			
			if (r1 != RDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDX)) {
					savereg(RDX); rdxwassaved = 1;
				} else regsinusetmp[RDX] = 1; // Mark unused register as temporarily used.
			}
			
			xor(RDX, RDX);
			
			// Specification from the Intel manual.
			// REX.W F7 /7	IDIV r/m64	#Signed divide RDX:RAX by r/m64,
			// with quotient stored in RAX and remainder stored in RDX.
			
			u8 lookupregx = lookupreg(x);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregx>7));
			
			lookupregx %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(7<<3)|lookupregx);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			if (r1 != RDX) {
				
				cpy(r1, RDX);
				
				if (rdxwassaved) {
					// Restore register value.
					restorereg(RDX);
				} else regsinusetmp[RDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1 != RAX) {
				
				if (raxwassaved) {
					// Restore register value.
					restorereg(RAX);
				} else regsinusetmp[RAX] = 0; // Unmark register as temporarily used.
			}
			
			if (x != r2) regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void mulhu (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r1 != RAX && r2 == RAX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the multiplicator
				// in RAX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint x;
			
			if (r2 == RAX) {
				// Look for an unused register.
				x = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!x) throwerror();
				
				regsinusetmp[x] = 1; // Mark unused register as temporarily used.
				
				cpy(x, r2);
				
			} else x = r2;
			
			uint raxwassaved = 0;
			uint rdxwassaved = 0;
			
			if (r1 != RAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RAX)) {
					savereg(RAX); raxwassaved = 1;
				} else regsinusetmp[RAX] = 1; // Mark unused register as temporarily used.
				
				cpy(RAX, r1);
			}
			
			if (r1 != RDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDX)) {
					savereg(RDX); rdxwassaved = 1;
				} else regsinusetmp[RDX] = 1; // Mark unused register as temporarily used.
			}
			
			// Specification from the Intel manual.
			// REX.W F7 /4	MUL r/m64	#Unsigned multiply RDX:RAX = RAX * r/m64,
			// with the low result stored in RAX and high result stored in RDX.
			
			u8 lookupregx = lookupreg(x);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregx>7));
			
			lookupregx %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(4<<3)|lookupregx);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			if (r1 != RDX) {
				
				cpy(r1, RDX);
				
				if (rdxwassaved) {
					// Restore register value.
					restorereg(RDX);
				} else regsinusetmp[RDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1 != RAX) {
				
				if (raxwassaved) {
					// Restore register value.
					restorereg(RAX);
				} else regsinusetmp[RAX] = 0; // Unmark register as temporarily used.
			}
			
			if (x != r2) regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void divu (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r1 != RAX && r2 == RAX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the dividend
				// in RDX:RAX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint x;
			
			if (r2 == RAX || r2 == RDX) {
				// Look for an unused register.
				x = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!x) throwerror();
				
				regsinusetmp[x] = 1; // Mark unused register as temporarily used.
				
				cpy(x, r2);
				
			} else x = r2;
			
			uint raxwassaved = 0;
			uint rdxwassaved = 0;
			
			if (r1 != RAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RAX)) {
					savereg(RAX); raxwassaved = 1;
				} else regsinusetmp[RAX] = 1; // Mark unused register as temporarily used.
				
				cpy(RAX, r1);
			}
			
			if (r1 != RDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDX)) {
					savereg(RDX); rdxwassaved = 1;
				} else regsinusetmp[RDX] = 1; // Mark unused register as temporarily used.
			}
			
			xor(RDX, RDX);
			
			// Specification from the Intel manual.
			// REX.W F7 /6	DIV r/m64	#Unsigned divide RDX:RAX by r/m64,
			// with quotient stored in RAX and remainder stored in RDX.
			
			u8 lookupregx = lookupreg(x);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregx>7));
			
			lookupregx %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(6<<3)|lookupregx);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			if (r1 != RDX) {
				
				if (rdxwassaved) {
					// Restore register value.
					restorereg(RDX);
				} else regsinusetmp[RDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1 != RAX) {
				
				cpy(r1, RAX);
				
				if (raxwassaved) {
					// Restore register value.
					restorereg(RAX);
				} else regsinusetmp[RAX] = 0; // Unmark register as temporarily used.
			}
			
			if (x != r2) regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void modu (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r1 != RAX && r2 == RAX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the dividend
				// in RDX:RAX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint x;
			
			if (r2 == RAX || r2 == RDX) {
				// Look for an unused register.
				x = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!x) throwerror();
				
				regsinusetmp[x] = 1; // Mark unused register as temporarily used.
				
				cpy(x, r2);
				
			} else x = r2;
			
			uint raxwassaved = 0;
			uint rdxwassaved = 0;
			
			if (r1 != RAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RAX)) {
					savereg(RAX); raxwassaved = 1;
				} else regsinusetmp[RAX] = 1; // Mark unused register as temporarily used.
				
				cpy(RAX, r1);
			}
			
			if (r1 != RDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDX)) {
					savereg(RDX); rdxwassaved = 1;
				} else regsinusetmp[RDX] = 1; // Mark unused register as temporarily used.
			}
			
			xor(RDX, RDX);
			
			// Specification from the Intel manual.
			// REX.W F7 /6	DIV r/m64	#Unsigned divide RDX:RAX by r/m64,
			// with quotient stored in RAX and remainder stored in RDX.
			
			u8 lookupregx = lookupreg(x);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregx>7));
			
			lookupregx %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(6<<3)|lookupregx);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			if (r1 != RDX) {
				
				cpy(r1, RDX);
				
				if (rdxwassaved) {
					// Restore register value.
					restorereg(RDX);
				} else regsinusetmp[RDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1 != RAX) {
				
				if (raxwassaved) {
					// Restore register value.
					restorereg(RAX);
				} else regsinusetmp[RAX] = 0; // Unmark register as temporarily used.
			}
			
			if (x != r2) regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void muli (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				mul(r, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// REX.W 69 /r id	IMUL r64, r/m64, imm32		#reg = r/m64 * imm32.
				
				u8 lookupregr = lookupreg(r);
				
				u8 lookupregrgt7 = (lookupregr>7);
				
				// REX-Prefix == 0100WR0B
				u8 rex = (0x48|(lookupregrgt7<<2)|lookupregrgt7);
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
				u8 modrm = ((0b11<<6)|(lookupregr<<3)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x69;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// REX.W 6b /r ib	IMUL r64, r/m64, imm8		#reg = r/m64 * imm8.
				
				u8 lookupregr = lookupreg(r);
				
				u8 lookupregrgt7 = (lookupregr>7);
				
				// REX-Prefix == 0100WR0B
				u8 rex = (0x48|(lookupregrgt7<<2)|lookupregrgt7);
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
				u8 modrm = ((0b11<<6)|(lookupregr<<3)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x6b;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		// Load immediate value given as argument.
		void loadimm (uint r, u64 n) {
			
			if (!n) xor(r, r);
			else if ((((sint)n < 0) ? -n : n) >= (1 << 31)) {
				// Specification from the Intel manual.
				// REX.W B8+rd io		MOV r64, imm64		#Move imm64 to r64.
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = (0xb8+lookupregr);
				
				// Append 64bits immediate value.
				*(u64*)arrayu8append2(&b->binary, sizeof(u64)) = n;
				
			} else {
				// Specification from the Intel manual.
				// REX.W C7 /0 id		MOV r/m64, imm32	#Move imm32 (sign-extended to 64bits) to r/m64.
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0xc7;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append 32bits immediate value.
				*(u32*)arrayu8append2(&b->binary, sizeof(u32)) = n;
			}
		}
		
		void mulhi (uint r) {
			
			uint raxwassaved = 0;
			uint rdxwassaved = 0;
			
			if (r != RAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RAX)) {
					savereg(RAX); raxwassaved = 1;
				} else regsinusetmp[RAX] = 1; // Mark unused register as temporarily used.
				
				cpy(RAX, r);
			}
			
			if (r != RDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDX)) {
					savereg(RDX); rdxwassaved = 1;
				} else regsinusetmp[RDX] = 1; // Mark unused register as temporarily used.
			}
			
			ldi(RDX, 0);
			
			// Specification from the Intel manual.
			// REX.W F7 /5	IMUL r/m64	#Signed multiply RDX:RAX = RAX * r/m64,
			// with the low result stored in RAX and high result stored in RDX.
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(5<<3)|RDX);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x48; // REX-Prefix == 0100W000
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			if (r != RDX) {
				
				cpy(r, RDX);
				
				if (rdxwassaved) {
					// Restore register value.
					restorereg(RDX);
				} else regsinusetmp[RDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r != RAX) {
				
				if (raxwassaved) {
					// Restore register value.
					restorereg(RAX);
				} else regsinusetmp[RAX] = 0; // Unmark register as temporarily used.
			}
		}
		
		void divi (uint r) {
			
			uint raxwassaved = 0;
			uint rdxwassaved = 0;
			
			if (r != RAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RAX)) {
					savereg(RAX); raxwassaved = 1;
				} else regsinusetmp[RAX] = 1; // Mark unused register as temporarily used.
				
				cpy(RAX, r);
			}
			
			if (r != RDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDX)) {
					savereg(RDX); rdxwassaved = 1;
				} else regsinusetmp[RDX] = 1; // Mark unused register as temporarily used.
			}
			
			// Look for an unused register.
			uint x = findunusedreg(0);
			
			// The compiler should have insured
			// that there are enough unused
			// registers available.
			if (!x) throwerror();
			
			regsinusetmp[x] = 1; // Mark unused register as temporarily used.
			
			ldi(x, 0);
			
			xor(RDX, RDX);
			
			// Specification from the Intel manual.
			// REX.W F7 /7	IDIV r/m64	#Signed divide RDX:RAX by r/m64,
			// with quotient stored in RAX and remainder stored in RDX.
			
			u8 lookupregx = lookupreg(x);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregx>7));
			
			lookupregx %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(7<<3)|lookupregx);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r != RDX) {
				
				if (rdxwassaved) {
					// Restore register value.
					restorereg(RDX);
				} else regsinusetmp[RDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r != RAX) {
				
				cpy(r, RAX);
				
				if (raxwassaved) {
					// Restore register value.
					restorereg(RAX);
				} else regsinusetmp[RAX] = 0; // Unmark register as temporarily used.
			}
		}
		
		void modi (uint r) {
			
			uint raxwassaved = 0;
			uint rdxwassaved = 0;
			
			if (r != RAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RAX)) {
					savereg(RAX); raxwassaved = 1;
				} else regsinusetmp[RAX] = 1; // Mark unused register as temporarily used.
				
				cpy(RAX, r);
			}
			
			if (r != RDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDX)) {
					savereg(RDX); rdxwassaved = 1;
				} else regsinusetmp[RDX] = 1; // Mark unused register as temporarily used.
			}
			
			// Look for an unused register.
			uint x = findunusedreg(0);
			
			// The compiler should have insured
			// that there are enough unused
			// registers available.
			if (!x) throwerror();
			
			regsinusetmp[x] = 1; // Mark unused register as temporarily used.
			
			ldi(x, 0);
			
			xor(RDX, RDX);
			
			// Specification from the Intel manual.
			// REX.W F7 /7	IDIV r/m64	#Signed divide RDX:RAX by r/m64,
			// with quotient stored in RAX and remainder stored in RDX.
			
			u8 lookupregx = lookupreg(x);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregx>7));
			
			lookupregx %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(7<<3)|lookupregx);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r != RDX) {
				
				cpy(r, RDX);
				
				if (rdxwassaved) {
					// Restore register value.
					restorereg(RDX);
				} else regsinusetmp[RDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r != RAX) {
				
				if (raxwassaved) {
					// Restore register value.
					restorereg(RAX);
				} else regsinusetmp[RAX] = 0; // Unmark register as temporarily used.
			}
		}
		
		void mulhui (uint r) {
			
			uint raxwassaved = 0;
			uint rdxwassaved = 0;
			
			if (r != RAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RAX)) {
					savereg(RAX); raxwassaved = 1;
				} else regsinusetmp[RAX] = 1; // Mark unused register as temporarily used.
				
				cpy(RAX, r);
			}
			
			if (r != RDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDX)) {
					savereg(RDX); rdxwassaved = 1;
				} else regsinusetmp[RDX] = 1; // Mark unused register as temporarily used.
			}
			
			ldi(RDX, 0);
			
			// Specification from the Intel manual.
			// REX.W F7 /4	MUL r/m64	#Unsigned multiply RDX:RAX = RAX * r/m64,
			// with the low result stored in RAX and high result stored in RDX.
			
			u8 lookupregrm = lookupreg(RDX);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregrm>7));
			
			lookupregrm %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(4<<3)|lookupregrm);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			if (r != RDX) {
				
				cpy(r, RDX);
				
				if (rdxwassaved) {
					// Restore register value.
					restorereg(RDX);
				} else regsinusetmp[RDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r != RAX) {
				
				if (raxwassaved) {
					// Restore register value.
					restorereg(RAX);
				} else regsinusetmp[RAX] = 0; // Unmark register as temporarily used.
			}
		}
		
		void divui (uint r) {
			
			uint raxwassaved = 0;
			uint rdxwassaved = 0;
			
			if (r != RAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RAX)) {
					savereg(RAX); raxwassaved = 1;
				} else regsinusetmp[RAX] = 1; // Mark unused register as temporarily used.
				
				cpy(RAX, r);
			}
			
			if (r != RDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDX)) {
					savereg(RDX); rdxwassaved = 1;
				} else regsinusetmp[RDX] = 1; // Mark unused register as temporarily used.
			}
			
			// Look for an unused register.
			uint x = findunusedreg(0);
			
			// The compiler should have insured
			// that there are enough unused
			// registers available.
			if (!x) throwerror();
			
			regsinusetmp[x] = 1; // Mark unused register as temporarily used.
			
			ldi(x, 0);
			
			xor(RDX, RDX);
			
			// Specification from the Intel manual.
			// REX.W F7 /6	DIV r/m64	#Unsigned divide RDX:RAX by r/m64,,
			// with quotient stored in RAX and remainder stored in RDX.
			
			u8 lookupregx = lookupreg(x);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregx>7));
			
			lookupregx %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(6<<3)|lookupregx);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r != RDX) {
				
				if (rdxwassaved) {
					// Restore register value.
					restorereg(RDX);
				} else regsinusetmp[RDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r != RAX) {
				
				cpy(r, RAX);
				
				if (raxwassaved) {
					// Restore register value.
					restorereg(RAX);
				} else regsinusetmp[RAX] = 0; // Unmark register as temporarily used.
			}
		}
		
		void modui (uint r) {
			
			uint raxwassaved = 0;
			uint rdxwassaved = 0;
			
			if (r != RAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RAX)) {
					savereg(RAX); raxwassaved = 1;
				} else regsinusetmp[RAX] = 1; // Mark unused register as temporarily used.
				
				cpy(RAX, r);
			}
			
			if (r != RDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDX)) {
					savereg(RDX); rdxwassaved = 1;
				} else regsinusetmp[RDX] = 1; // Mark unused register as temporarily used.
			}
			
			// Look for an unused register.
			uint x = findunusedreg(0);
			
			// The compiler should have insured
			// that there are enough unused
			// registers available.
			if (!x) throwerror();
			
			regsinusetmp[x] = 1; // Mark unused register as temporarily used.
			
			ldi(x, 0);
			
			xor(RDX, RDX);
			
			// Specification from the Intel manual.
			// REX.W F7 /6	DIV r/m64	#Unsigned divide RDX:RAX by r/m64,
			// with quotient stored in RAX and remainder stored in RDX.
			
			u8 lookupregx = lookupreg(x);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregx>7));
			
			lookupregx %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(6<<3)|lookupregx);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r != RDX) {
				
				cpy(r, RDX);
				
				if (rdxwassaved) {
					// Restore register value.
					restorereg(RDX);
				} else regsinusetmp[RDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r != RAX) {
				
				if (raxwassaved) {
					// Restore register value.
					restorereg(RAX);
				} else regsinusetmp[RAX] = 0; // Unmark register as temporarily used.
			}
		}
		
		void and (uint r1, uint r2) {
			// Specification from the Intel manual.
			// REX.W 21 /r	AND r/m64, r64		#r/m64 AND r64.
			
			u8 lookupregrm = lookupreg(r1);
			u8 lookupregreg = lookupreg(r2);
			
			// REX-Prefix == 0100WR0B
			u8 rex = (0x48|((lookupregreg>7)<<2)|(lookupregrm>7));
			
			lookupregrm %= 8;
			lookupregreg %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == reg[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
			u8 modrm = ((0b11<<6)|(lookupregreg<<3)|lookupregrm);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0x21;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
		}
		
		void andi (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				and(r, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// REX.W 81 /4 id	AND r/m64, imm32	#r/m64 AND imm32 (sign-extended to 64bits).
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|(4<<3)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x81;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// REX.W 83 /4 ib	AND r/m64, imm8	#r/m64 AND imm8 (sign-extended to 64bits).
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|(4<<3)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x83;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void andimm (uint r, u64 n) {
			
			if ((((sint)n < 0) ? -n : n) < (1 << 7)) {
				// Specification from the Intel manual.
				// REX.W 83 /4 ib	AND r/m64, imm8	#r/m64 AND imm8 (sign-extended to 64bits).
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|(4<<3)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x83;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate value.
				*arrayu8append1(&b->binary) = n;
				
			} else if ((((sint)n < 0) ? -n : n) < (1 << 31)) {
				// Specification from the Intel manual.
				// REX.W 81 /4 id	AND r/m64, imm32	#r/m64 AND imm32 (sign-extended to 64bits).
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|(4<<3)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x81;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append 32bits immediate value.
				*(u32*)arrayu8append2(&b->binary, sizeof(u32)) = n;
				
			} else {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				loadimm(unusedreg, n);
				and(r, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			}
		}
		
		void or (uint r1, uint r2) {
			// Specification from the Intel manual.
			// REX.W 09 /r	OR r/m64, r64		#r/m64 OR r64.
			
			u8 lookupregrm = lookupreg(r1);
			u8 lookupregreg = lookupreg(r2);
			
			// REX-Prefix == 0100WR0B
			u8 rex = (0x48|((lookupregreg>7)<<2)|(lookupregrm>7));
			
			lookupregrm %= 8;
			lookupregreg %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == reg[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
			u8 modrm = ((0b11<<6)|(lookupregreg<<3)|lookupregrm);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0x09;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
		}
		
		void ori (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				or(r, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// REX.W 81 /1 id	OR r/m64, imm32	#r/m64 OR imm32 (sign-extended to 64bits).
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|(1<<3)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x81;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// REX.W 83 /1 ib	OR r/m64, imm8	#r/m64 OR imm8 (sign-extended to 64bits).
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|(1<<3)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x83;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void xori (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				xor(r, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// REX.W 81 /6 id	XOR r/m64, imm32	#r/m64 XOR imm32 (sign-extended to 64bits).
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|(6<<3)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x81;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// REX.W 83 /6 ib	XOR r/m64, imm8	#r/m64 XOR imm8 (sign-extended to 64bits).
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|(6<<3)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x83;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void not (uint r) {
			// Specification from the Intel manual.
			// REX.W F7 /2	NOT r/m64	#Reverse each bit of r/m64.
			
			u8 lookupregr = lookupreg(r);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregr>7));
			
			lookupregr %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(2<<3)|lookupregr);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
		}
		
		void sll (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r2 != RCX && r1 == RCX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the shift amount
				// in RCX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint rcxwassaved = 0;
			
			if (r2 != RCX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RCX)) {
					savereg(RCX); rcxwassaved = 1;
				} else regsinusetmp[RCX] = 1; // Mark unused register as temporarily used.
				
				cpy(RCX, r2);
			}
			
			// Specification from the Intel manual.
			// REX.W D3 /4	SHL r/m64, CL		#Shift logically r/m64 left CL times.
			
			u8 lookupregrm = lookupreg(r1);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregrm>7));
			
			lookupregrm %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(4<<3)|lookupregrm);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xd3;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			if (r2 != RCX) {
				
				if (rcxwassaved) {
					// Restore register value.
					restorereg(RCX);
				} else regsinusetmp[RCX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void slli (uint r) {
			// Specification from the Intel manual.
			// REX.W C1 /4 ib	SHL r/m64, imm8		#Shift logically r/m64 left imm8 times.
			
			u8 lookupregr = lookupreg(r);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregr>7));
			
			lookupregr %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(4<<3)|lookupregr);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xc1;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// Append the immediate value.
			append8bitsimm(0);
		}
		
		void sllimm (uint r, u8 n) {
			// Specification from the Intel manual.
			// REX.W C1 /4 ib	SHL r/m64, imm8		#Shift logically r/m64 left imm8 times.
			
			u8 lookupregr = lookupreg(r);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregr>7));
			
			lookupregr %= 8;
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xc1;
			
			// Append the ModR/M byte.
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			*arrayu8append1(&b->binary) = ((0b11<<6)|(4<<3)|lookupregr);
			
			// Append the 8bits immediate value.
			*arrayu8append1(&b->binary) = n;
		}
		
		void srl (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r2 != RCX && r1 == RCX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the shift amount
				// in RCX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint rcxwassaved = 0;
			
			if (r2 != RCX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RCX)) {
					savereg(RCX); rcxwassaved = 1;
				} else regsinusetmp[RCX] = 1; // Mark unused register as temporarily used.
				
				cpy(RCX, r2);
			}
			
			// Specification from the Intel manual.
			// REX.W D3 /5	SHR r/m64, CL		#Shift logically r/m64 right CL times.
			
			u8 lookupregrm = lookupreg(r1);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregrm>7));
			
			lookupregrm %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(5<<3)|lookupregrm);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xd3;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			if (r2 != RCX) {
				
				if (rcxwassaved) {
					// Restore register value.
					restorereg(RCX);
				} else regsinusetmp[RCX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void srli (uint r) {
			// Specification from the Intel manual.
			// REX.W C1 /5 ib	SHR r/m64, imm8		#Shift logically r/m64 right imm8 times.
			
			u8 lookupregr = lookupreg(r);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregr>7));
			
			lookupregr %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(5<<3)|lookupregr);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xc1;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// Append the immediate value.
			append8bitsimm(0);
		}
		
		void sra (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r2 != RCX && r1 == RCX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the shift amount
				// in RCX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint rcxwassaved = 0;
			
			if (r2 != RCX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RCX)) {
					savereg(RCX); rcxwassaved = 1;
				} else regsinusetmp[RCX] = 1; // Mark unused register as temporarily used.
				
				cpy(RCX, r2);
			}
			
			// Specification from the Intel manual.
			// REX.W D3 /7	SAR r/m64, CL		#Shift arithmetically r/m64 right CL times.
			
			u8 lookupregrm = lookupreg(r1);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregrm>7));
			
			lookupregrm %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(7<<3)|lookupregrm);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xd3;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			if (r2 != RCX) {
				
				if (rcxwassaved) {
					// Restore register value.
					restorereg(RCX);
				} else regsinusetmp[RCX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void srai (uint r) {
			// Specification from the Intel manual.
			// REX.W C1 /7 ib	SAR r/m64, imm8		#Shift arithmetically r/m64 right imm8 times.
			
			u8 lookupregr = lookupreg(r);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregr>7));
			
			lookupregr %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(7<<3)|lookupregr);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xc1;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// Append the immediate value.
			append8bitsimm(0);
		}
		
		void sraimm (uint r, u8 n) {
			// Specification from the Intel manual.
			// REX.W C1 /7 ib	SAR r/m64, imm8		#Shift arithmetically r/m64 right imm8 times.
			
			u8 lookupregr = lookupreg(r);
			
			// REX-Prefix == 0100W00B
			u8 rex = (0x48|(lookupregr>7));
			
			lookupregr %= 8;
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0xc1;
			
			// Append the ModR/M byte.
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			*arrayu8append1(&b->binary) = ((0b11<<6)|(7<<3)|lookupregr);
			
			// Append the 8bits immediate value.
			*arrayu8append1(&b->binary) = n;
		}
		
		// Function which zero extend
		// the n least significant bits
		// of the register r.
		void zxt (uint r, u8 n) {
			
			if (n == 8) {
				// r must be a register that
				// can be encoded in the field reg
				// of an instruction ModR/M when
				// it is used as an 8bits operand;
				// either RAX, RBX, RCX, RDX;
				
				// Variable to be set to the register
				// which will be encoded in the field
				// reg of the instruction ModR/M.
				uint rvalid;
				
				// Variable set to 1 if
				// the register rvalid
				// was saved.
				uint rvalidwassaved = 0;
				
				// I find a valid unused register
				// to use if r is not either
				// of RAX, RBX, RCX, RDX.
				if (r < RAX || r > RDX) {
					
					if (!(rvalid = findunusedregfor8bitsoperand(0))) {
						// I get here if r was not
						// either of RAX, RBX, RCX, RDX
						// and I could not find a valid
						// unused register to use.
						
						rvalid = RAX;
						
						savereg(rvalid); rvalidwassaved = 1;
						
					} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
					
					cpy(rvalid, r);
					
				} else rvalid = r;
				
				// Specification from the Intel manual.
				// 0F B6 /r	MOVZX r32, r/m8		#Move r/m8 to r32, zero-extension.
				
				u8 lookupregrvalid = lookupreg(rvalid);
				
				u8 lookupregrvalidgt7 = (lookupregrvalid > 7);
				
				if (lookupregrvalidgt7) {
					// REX-Prefix == 01000R0B
					*arrayu8append1(&b->binary) = (0x40|(lookupregrvalidgt7<<2)|lookupregrvalidgt7);
					
					lookupregrvalid %= 8;
				}
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0xb6;
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
				*arrayu8append1(&b->binary) = ((0b11<<6)|(lookupregrvalid<<3)|lookupregrvalid);
				
				if (rvalid != r) {
					
					cpy(r, rvalid);
					
					if (rvalidwassaved) restorereg(rvalid);
					// Unmark register as temporarily used.
					else regsinusetmp[rvalid] = 0;
				}
				
			} else if (n == 16) {
				// Specification from the Intel manual.
				// 0F B7 /r	MOVZX r32, r/m16	#Move r/m16 to r32, zero-extension.
				
				u8 lookupregr = lookupreg(r);
				
				u8 lookupregrgt7 = (lookupregr > 7);
				
				if (lookupregrgt7) {
					// REX-Prefix == 01000R0B
					*arrayu8append1(&b->binary) = (0x40|(lookupregrgt7<<2)|lookupregrgt7);
					
					lookupregr %= 8;
				}
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0xb7;
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
				*arrayu8append1(&b->binary) = ((0b11<<6)|(lookupregr<<3)|lookupregr);
				
			} else if (n == 32) {
				// Specification from the Intel manual.
				// 8B /r	MOV r32,r/m32		#Move r/m32 to r32, zero-extension.
				
				u8 lookupregr = lookupreg(r);
				
				u8 lookupregrgt7 = (lookupregr > 7);
				
				if (lookupregrgt7) {
					// REX-Prefix == 01000R0B
					*arrayu8append1(&b->binary) = (0x40|(lookupregrgt7<<2)|lookupregrgt7);
					
					lookupregr %= 8;
				}
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x8b;
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
				*arrayu8append1(&b->binary) = ((0b11<<6)|(lookupregr<<3)|lookupregr);
				
			} else {
				// I get here if n is not 8, 16 or 32.
				// Zero extension is done using
				// the operation "and".
				
				// Compute the immediate value
				// to use with the operation "and"
				// in order to do zero extension.
				u64 bitselect = ((u64)1 << n) - 1;
				
				if (bitselect < (1<<7)) {
					// Specification from the Intel manual.
					// REX.W 83 /4 ib	AND r/m64, imm8		#r/m64 AND imm8 (sign-extended).
					
					u8 lookupregr = lookupreg(r);
					
					// REX-Prefix == 0100W00B
					u8 rex = (0x48|(lookupregr>7));
					
					lookupregr %= 8;
					
					// From the most significant bit to the least significant bit.
					// MOD == 0b11;
					// REG == op[2:0]; #Only the 3 least significant bits matters.
					// R/M == rm[2:0]; #Only the 3 least significant bits matters.
					u8 modrm = ((0b11<<6)|(4<<3)|lookupregr);
					
					// Append the opcode byte.
					*arrayu8append1(&b->binary) = rex;
					*arrayu8append1(&b->binary) = 0x83;
					
					// Append the ModR/M byte.
					*arrayu8append1(&b->binary) = modrm;
					
					// Append the 8bits immediate value.
					*arrayu8append1(&b->binary) = bitselect;
					
				} else if (bitselect < (1<<31)) {
					// Specification from the Intel manual.
					// REX.W 81 /4 id	AND r/m64, imm32		#r/m64 AND imm32 (sign-extended).
					
					u8 lookupregr = lookupreg(r);
					
					// REX-Prefix == 0100W00B
					u8 rex = (0x48|(lookupregr>7));
					
					lookupregr %= 8;
					
					// From the most significant bit to the least significant bit.
					// MOD == 0b11;
					// REG == op[2:0]; #Only the 3 least significant bits matters.
					// R/M == rm[2:0]; #Only the 3 least significant bits matters.
					u8 modrm = ((0b11<<6)|(4<<3)|lookupregr);
					
					// Append the opcode byte.
					*arrayu8append1(&b->binary) = rex;
					*arrayu8append1(&b->binary) = 0x81;
					
					// Append the ModR/M byte.
					*arrayu8append1(&b->binary) = modrm;
					
					// Append the 32bits immediate value.
					*(u32*)arrayu8append2(&b->binary, sizeof(u32)) = bitselect;
					
				} else {
					// Look for an unused register
					uint x = findunusedreg(0);
					
					// The compiler should have insured
					// that there are enough unused
					// registers available.
					if (!x) throwerror();
					
					regsinusetmp[x] = 1; // Mark unused register as temporarily used.
					
					loadimm(x, bitselect);
					
					and(r, x);
					
					regsinusetmp[x] = 0; // Unmark register as temporarily used.
				}
			}
		}
		
		// Function which sign extend
		// the n least significant bits
		// of the register r.
		void sxt (uint r, u8 n) {
			
			if (n == 8) {
				// r must be a register that
				// can be encoded in the field reg
				// of an instruction ModR/M when
				// it is used as an 8bits operand;
				// either RAX, RBX, RCX, RDX;
				
				// Variable to be set to the register
				// which will be encoded in the field
				// reg of the instruction ModR/M.
				uint rvalid;
				
				// Variable set to 1 if
				// the register rvalid
				// was saved.
				uint rvalidwassaved = 0;
				
				// I find a valid unused register
				// to use if r is not either
				// of RAX, RBX, RCX, RDX.
				if (r < RAX || r > RDX) {
					
					if (!(rvalid = findunusedregfor8bitsoperand(0))) {
						// I get here if r was not
						// either of RAX, RBX, RCX, RDX
						// and I could not find a valid
						// unused register to use.
						
						rvalid = RAX;
						
						savereg(rvalid); rvalidwassaved = 1;
						
					} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
					
					cpy(rvalid, r);
					
				} else rvalid = r;
				
				// Specification from the Intel manual.
				// REX.W 0F BE /r	MOVSX r64, r/m8		#Move r/m8 to r64, sign-extension.
				
				u8 lookupregrvalid = lookupreg(rvalid);
				
				//u8 lookupregrvalidgt7 = (lookupregrvalid > 7);
				
				// REX-Prefix == 0100WR0B
				//u8 rex = (0x48|((lookupregrvalidgt7)<<2)|(lookupregrvalidgt7));
				
				//lookupregrvalid %= 8; // rvalid is <= 7;
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x48; //rex;
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0xbe;
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
				*arrayu8append1(&b->binary) = ((0b11<<6)|(lookupregrvalid<<3)|lookupregrvalid);
				
				if (rvalid != r) {
					
					cpy(r, rvalid);
					
					if (rvalidwassaved) restorereg(rvalid);
					// Unmark register as temporarily used.
					else regsinusetmp[rvalid] = 0;
				}
				
			} else if (n == 16) {
				// Specification from the Intel manual.
				// REX.W 0F BF /r	MOVSX r64, r/m16	#Move r/m16 to r64, sign-extension.
				
				u8 lookupregr = lookupreg(r);
				
				u8 lookupregrgt7 = (lookupregr > 7);
				
				// REX-Prefix == 0100WR0B
				*arrayu8append1(&b->binary) = (0x48|(lookupregrgt7<<2)|lookupregrgt7);
				
				lookupregr %= 8;
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0xbf;
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
				*arrayu8append1(&b->binary) = ((0b11<<6)|(lookupregr<<3)|lookupregr);
				
			} else {
				// I get here if n is not 8 or 16.
				// Sign extension is done shifting left
				// then arithmetically shifting right.
				
				// Compute the amount
				// by which to shift
				// left then right in
				// order to sign extend.
				n = 64 - n;
				
				sllimm(r, n);
				sraimm(r, n);
			}
		}
		
		void cmp (uint r1, uint r2) {
			// Specification from the Intel manual.
			// REX.W 39 /r	CMP r/m64, r64		#Compare r64 with r/m64.
			
			u8 lookupregrm = lookupreg(r1);
			u8 lookupregreg = lookupreg(r2);
			
			// REX-Prefix == 0100WR0B
			u8 rex = (0x48|((lookupregreg>7)<<2)|(lookupregrm>7));
			
			lookupregrm %= 8;
			lookupregreg %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == reg[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
			u8 modrm = ((0b11<<6)|(lookupregreg<<3)|lookupregrm);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0x39;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
		}
		
		void seq (uint r1, uint r2) {
			// r1 must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r1 is not either
			// of RAX, RBX, RCX, RDX.
			if (r1 < RAX || r1 > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != RAX) rvalid = RAX;
					else rvalid = RBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// REX.W 0F 94	SETE r/m8	#Set byte if equal (ZF=1).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x94;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r1) {
				
				cpy(r1, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void sne (uint r1, uint r2) {
			// r1 must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r1 is not either
			// of RAX, RBX, RCX, RDX.
			if (r1 < RAX || r1 > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != RAX) rvalid = RAX;
					else rvalid = RBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// REX.W 0F 95	SETNE r/m8	#Set byte if not equal (ZF=0).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x95;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r1) {
				
				cpy(r1, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void cmpi (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				cmp(r, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// REX.W 81 /7 id	CMP r/m64, imm32	#Compare imm32 (sign-extended to 64bits) with r/m64.
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|(7<<3)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x81;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// REX.W 83 /7 ib	CMP r/m64, imm8	#Compare imm8 (sign-extended to 64bits) with r/m64.
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100W00B
				u8 rex = (0x48|(lookupregr>7));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b11;
				// REG == op[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				u8 modrm = ((0b11<<6)|(7<<3)|lookupregr);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x83;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void seqi (uint r) {
			// r must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r is not either
			// of RAX, RBX, RCX, RDX.
			if (r < RAX || r > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = RAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// REX.W 0F 94	SETE r/m8	#Set byte if equal (ZF=1).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x94;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r) {
				
				cpy(r, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void snei (uint r) {
			// r must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r is not either
			// of RAX, RBX, RCX, RDX.
			if (r < RAX || r > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = RAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// REX.W 0F 95	SETNE r/m8	#Set byte if not equal (ZF=0).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x95;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r) {
				
				cpy(r, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void slt (uint r1, uint r2) {
			// r1 must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r1 is not either
			// of RAX, RBX, RCX, RDX.
			if (r1 < RAX || r1 > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != RAX) rvalid = RAX;
					else rvalid = RBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// REX.W 0F 9C	SETL r/m8	#Set byte if less (SF≠OF).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x9c;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r1) {
				
				cpy(r1, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void slte (uint r1, uint r2) {
			// r1 must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r1 is not either
			// of RAX, RBX, RCX, RDX.
			if (r1 < RAX || r1 > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != RAX) rvalid = RAX;
					else rvalid = RBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// REX.W 0F 9E	SETLE r/m8	#Set byte if less or equal (ZF=1 or SF≠OF).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x9e;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r1) {
				
				cpy(r1, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void sltu (uint r1, uint r2) {
			// r1 must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r1 is not either
			// of RAX, RBX, RCX, RDX.
			if (r1 < RAX || r1 > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != RAX) rvalid = RAX;
					else rvalid = RBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// REX.W 0F 92	SETB r/m8	#Set byte if below (CF=1).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x92;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r1) {
				
				cpy(r1, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void slteu (uint r1, uint r2) {
			// r1 must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r1 is not either
			// of RAX, RBX, RCX, RDX.
			if (r1 < RAX || r1 > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != RAX) rvalid = RAX;
					else rvalid = RBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// REX.W 0F 96	SETBE r/m8	#Set byte if below or equal (CF=1 or ZF=1).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x96;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r1) {
				
				cpy(r1, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void sgt (uint r1, uint r2) {
			// r1 must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r1 is not either
			// of RAX, RBX, RCX, RDX.
			if (r1 < RAX || r1 > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != RAX) rvalid = RAX;
					else rvalid = RBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// REX.W 0F 9F	SETG r/m8	#Set byte if greater (ZF=0 and SF=OF).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x9f;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r1) {
				
				cpy(r1, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void sgte (uint r1, uint r2) {
			// r1 must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r1 is not either
			// of RAX, RBX, RCX, RDX.
			if (r1 < RAX || r1 > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != RAX) rvalid = RAX;
					else rvalid = RBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// REX.W 0F 9D	SETGE r/m8	#Set byte if greater or equal (SF=OF).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x9d;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r1) {
				
				cpy(r1, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void sgtu (uint r1, uint r2) {
			// r1 must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r1 is not either
			// of RAX, RBX, RCX, RDX.
			if (r1 < RAX || r1 > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != RAX) rvalid = RAX;
					else rvalid = RBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// REX.W 0F 97	SETA r/m8	#Set byte if above (CF=0 and ZF=0).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x97;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r1) {
				
				cpy(r1, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void sgteu (uint r1, uint r2) {
			// r1 must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r1 is not either
			// of RAX, RBX, RCX, RDX.
			if (r1 < RAX || r1 > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != RAX) rvalid = RAX;
					else rvalid = RBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// REX.W 0F 93	SETAE r/m8	#Set byte if above or equal (CF=0).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x93;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r1) {
				
				cpy(r1, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void slti (uint r) {
			// r must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r is not either
			// of RAX, RBX, RCX, RDX.
			if (r < RAX || r > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = RAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// REX.W 0F 9C	SETL r/m8	#Set byte if less (SF≠OF).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x9c;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r) {
				
				cpy(r, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void sltei (uint r) {
			// r must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r is not either
			// of RAX, RBX, RCX, RDX.
			if (r < RAX || r > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = RAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// REX.W 0F 9E	SETLE r/m8	#Set byte if less or equal (ZF=1 or SF≠OF).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x9e;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r) {
				
				cpy(r, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void sltui (uint r) {
			// r must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r is not either
			// of RAX, RBX, RCX, RDX.
			if (r < RAX || r > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = RAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// REX.W 0F 92	SETB r/m8	#Set byte if below (CF=1).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x92;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r) {
				
				cpy(r, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void slteui (uint r) {
			// r must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r is not either
			// of RAX, RBX, RCX, RDX.
			if (r < RAX || r > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = RAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// REX.W 0F 96	SETBE r/m8	#Set byte if below or equal (CF=1 or ZF=1).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x96;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r) {
				
				cpy(r, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void sgti (uint r) {
			// r must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r is not either
			// of RAX, RBX, RCX, RDX.
			if (r < RAX || r > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = RAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// REX.W 0F 9F	SETG r/m8	#Set byte if greater (ZF=0 and SF=OF).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x9f;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r) {
				
				cpy(r, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void sgtei (uint r) {
			// r must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r is not either
			// of RAX, RBX, RCX, RDX.
			if (r < RAX || r > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = RAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// REX.W 0F 9D	SETGE r/m8	#Set byte if greater or equal (SF=OF).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x9d;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r) {
				
				cpy(r, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void sgtui (uint r) {
			// r must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r is not either
			// of RAX, RBX, RCX, RDX.
			if (r < RAX || r > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = RAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// REX.W 0F 97	SETA r/m8	#Set byte if above (CF=0 and ZF=0).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x97;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r) {
				
				cpy(r, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void sgteui (uint r) {
			// r must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r is not either
			// of RAX, RBX, RCX, RDX.
			if (r < RAX || r > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = RAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// REX.W 0F 93	SETAE r/m8	#Set byte if above or equal (CF=0).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x93;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r) {
				
				cpy(r, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		// Function used to compare
		// a register value against zero.
		void cmptozero (uint r) {
			// Specification from the Intel manual.
			// REX.W 85 /r	TEST r/m64, r64		# AND r64 with r/m64; set SF, ZF, PF according to result.
			
			u8 lookupregr = lookupreg(r);
			
			u8 lookupregrgt7 = (lookupregr>7);
			
			// REX-Prefix == 0100WR0B
			u8 rex = (0x48|(lookupregrgt7<<2)|lookupregrgt7);
			
			lookupregr %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == reg[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
			u8 modrm = ((0b11<<6)|(lookupregr<<3)|lookupregr);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0x85;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
		}
		
		void sz (uint r) {
			// r must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r is not either
			// of RAX, RBX, RCX, RDX.
			if (r < RAX || r > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = RAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmptozero(rvalid);
			
			// Specification from the Intel manual.
			// REX.W 0F 94	SETZ r/m8	#Set byte if zero (ZF=1).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x94;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r) {
				
				cpy(r, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void snz (uint r) {
			// r must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r is not either
			// of RAX, RBX, RCX, RDX.
			if (r < RAX || r > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = RAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmptozero(rvalid);
			
			// Specification from the Intel manual.
			// REX.W 0F 95	SETNZ r/m8	#Set byte if not zero (ZF=0).
			
			u8 lookupregrm = lookupreg(rvalid);
			
			// REX-Prefix == 0100W00B
			//u8 rex = (0x48|(lookupregrm>7));
			
			//lookupregrm %= 8; // rvalid is <= 7;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|lookupregrm);
			
			// Append the opcode byte.
			//*arrayu8append1(&b->binary) = 0x48; //rex; // Not needed.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x95;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r) {
				
				cpy(r, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void afip (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			//if (b->isimmused == IMM64) { // ### Commented-out as RIP-relative addressing is failing.
				// Specification from the Intel manual.
				// E8 cd	CALL rel32	#Call relative.
				// ### Using the rel16 variant
				// ### (by prefixing 0x66) which
				// ### is shorter, would segfault.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0xe8;
				
				// Append a 32bits offset immediate value of zero.
				*(u32*)arrayu8append2(&b->binary, sizeof(u32)) = (u32)0;
				
				uint mmszbbinaryptr = mmsz(b->binary.ptr);
				
				// Pop in r the return address
				// which is the address of
				// the POP instruction itself.
				pop(r);
				
				// Note that the immediate field for which
				// the offset will be encoded in b->immfieldoffset
				// is expected to be in the last bytes of afip.
				addi(r);
				
				// Increment the immediate misc value
				// to account for instructions from
				// after the CALL instruction.
				b->immmiscvalue += (mmsz(b->binary.ptr) - mmszbbinaryptr);
				
			/*} else { // ### Commented-out as RIP-relative addressing is failing.
				// Specification from the Intel manual.
				// REX.W 8B /r	MOV r64,r/m64		#Move r/m64 to r64.
				// The ModR/M byte get set such that
				// a 32bits displacement is expected.
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100WR00
				u8 rex = (0x48|((lookupregr>7)<<2));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b101
				u8 modrm = ((lookupregr<<3)|0b101);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x8b;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate value.
				// Note that the immediate field for which
				// the offset will be encoded in b->immfieldoffset
				// is expected to be in the last bytes of afip.
				append32bitsimm(0);
			}*/
		}
		
		void afip2 (uint r) {
			// If b->isimm2used is set, then
			// I am redoing the lyricalinstruction.
			//if (b->isimm2used == IMM64) { // ### Commented-out as RIP-relative addressing is failing.
				// Specification from the Intel manual.
				// E8 cd	CALL rel32	#Call relative.
				// ### Using the rel16 variant
				// ### (by prefixing 0x66) which
				// ### is shorter, would segfault.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0xe8;
				
				// Append a 32bits offset immediate value of zero.
				*(u32*)arrayu8append2(&b->binary, sizeof(u32)) = (u32)0;
				
				uint mmszbbinaryptr = mmsz(b->binary.ptr);
				
				// Pop in r the return address
				// which is the address of
				// the POP instruction itself.
				pop(r);
				
				// Note that the immediate2 field for which
				// the offset will be encoded in b->imm2fieldoffset
				// is expected to be in the last bytes of afip2.
				addi2(r);
				
				// Increment the immediate2 misc value
				// to account for instructions from
				// after the CALL instruction.
				b->imm2miscvalue += (mmsz(b->binary.ptr) - mmszbbinaryptr);
				
			/*} else { // ### Commented-out as RIP-relative addressing is failing.
				// Specification from the Intel manual.
				// REX.W 8B /r	MOV r64,r/m64		#Move r/m64 to r64.
				// The ModR/M byte get set such that
				// a 32bits displacement is expected.
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100WR00
				u8 rex = (0x48|((lookupregr>7)<<2));
				
				lookupregr %= 8;
				
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b101
				u8 modrm = ((lookupregr<<3)|0b101);
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = rex;
				*arrayu8append1(&b->binary) = 0x8b;
				
				// Append the ModR/M byte.
				*arrayu8append1(&b->binary) = modrm;
				
				// Append the immediate2 value.
				// Note that the immediate2 field for which
				// the offset will be encoded in b->imm2fieldoffset
				// is expected to be in the last bytes of afip2.
				append32bitsimm2(0);
			}*/
		}
		
		void jr (uint r) {
			// Specification from the Intel manual.
			// FF /4	JMP r/m64	#Jump absolute.
			
			u8 lookupregr = lookupreg(r);
			
			if (lookupregr > 7) {
				// REX-Prefix == 0100000B
				*arrayu8append1(&b->binary) = 0x41;
				
				lookupregr %= 8;
			}
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xff;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			u8 modrm = ((0b11<<6)|(4<<3)|lookupregr);
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
		}
		
		void j () {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// There is always enough
				// unused registers before
				// a non-conditional branching,
				// because all registers
				// are flushed and discarded.
				// This function is also
				// used when generating
				// conditional branching;
				// for conditional branching,
				// the compiler insured that
				// there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				// Mark unused register as temporarily used.
				regsinusetmp[unusedreg] = 1;
				
				afip(unusedreg);
				jr(unusedreg);
				
				// Unmark register as temporarily used.
				regsinusetmp[unusedreg] = 0;
				
			} else if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// E9 cd	JMP rel32	#Jump relative, RIP = RIP + 32-bit.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0xe9;
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// EB cb	JMP rel8	#Jump relative, RIP = RIP + 8-bit.
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0xEB;
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void jeq (uint r1, uint r2) {
			
			cmp(r1, r2);
			
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Specification from the Intel manual.
				// 75 cb	JNE rel8	#Jump short if not equal (ZF=0).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x75;
				
				// Append the 8bits immediate
				// field which is the bytesize
				// of the following branching
				// instruction.
				u8* imm = arrayu8append1(&b->binary);
				
				u8* bbinaryptr = b->binary.ptr;
				u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
				
				j();
				
				bbinaryptr = b->binary.ptr;
				// Set the 8bits immediate field appended above.
				*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
				
			} else if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// 0F 84 cd	JE rel32	#Jump near if equal (ZF=1).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0x84;
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// 74 cb	JE rel8	#Jump short if equal (ZF=1).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x74;
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void jeqr (uint r1, uint r2, uint r3) {
			
			cmp(r1, r2);
			
			// Specification from the Intel manual.
			// 75 cb	JNE rel8	#Jump short if not equal (ZF=0).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x75;
			
			// Append the 8bits immediate
			// field which is the bytesize
			// of the following branching
			// instruction.
			u8* imm = arrayu8append1(&b->binary);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			jr(r3);
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void ji () {
			// The x64 instruction set
			// do not have a jump instruction
			// to an immediate absolute address,
			// so I use an unused register to hold
			// the absolute address to jump to.
			
			// Look for an unused register.
			uint unusedreg = findunusedreg(0);
			
			// There is always enough
			// unused registers before
			// a non-conditional branching,
			// because all registers
			// are flushed and discarded.
			// This function is also
			// used when generating
			// conditional branching;
			// for conditional branching,
			// the compiler insured that
			// there are enough unused
			// registers available.
			if (!unusedreg) throwerror();
			
			// Mark unused register as temporarily used.
			regsinusetmp[unusedreg] = 1;
			
			ldi(unusedreg, 0);
			
			jr(unusedreg);
			
			// Unmark register as temporarily used.
			regsinusetmp[unusedreg] = 0;
		}
		
		void jeqi (uint r1, uint r2) {
			
			cmp(r1, r2);
			
			// Specification from the Intel manual.
			// 75 cb	JNE rel8	#Jump short if not equal (ZF=0).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x75;
			
			// Append the 8bits immediate
			// field which is the bytesize
			// of the following branching
			// instruction.
			u8* imm = arrayu8append1(&b->binary);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			ji();
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jne (uint r1, uint r2) {
			
			cmp(r1, r2);
			
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Specification from the Intel manual.
				// 74 cb	JE rel8	#Jump short if equal (ZF=1).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x74;
				
				// Append the 8bits immediate
				// field which is the bytesize
				// of the following branching
				// instruction.
				u8* imm = arrayu8append1(&b->binary);
				
				u8* bbinaryptr = b->binary.ptr;
				u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
				
				j();
				
				bbinaryptr = b->binary.ptr;
				// Set the 8bits immediate field appended above.
				*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
				
			} else if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// 0F 85 cd	JNE rel32	#Jump near if not equal (ZF=0).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0x85;
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// 75 cb	JNE rel8	#Jump short if not equal (ZF=0).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x75;
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void jner (uint r1, uint r2, uint r3) {
			
			cmp(r1, r2);
			
			// Specification from the Intel manual.
			// 74 cb	JE rel8		#Jump short if equal (ZF=1).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x74;
			
			// Append the 8bits immediate
			// field which is the bytesize
			// of the following branching
			// instruction.
			u8* imm = arrayu8append1(&b->binary);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			jr(r3);
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jnei (uint r1, uint r2) {
			
			cmp(r1, r2);
			
			// Specification from the Intel manual.
			// 74 cb	JE rel8		#Jump short if equal (ZF=1).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x74;
			
			// Append the 8bits immediate
			// field which is the bytesize
			// of the following branching
			// instruction.
			u8* imm = arrayu8append1(&b->binary);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			ji();
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jlt (uint r1, uint r2) {
			
			cmp(r1, r2);
			
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Specification from the Intel manual.
				// 7D cb	JNL rel8	#Jump short if not less (SF=OF).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x7d;
				
				// Append the 8bits immediate
				// field which is the bytesize
				// of the following branching
				// instruction.
				u8* imm = arrayu8append1(&b->binary);
				
				u8* bbinaryptr = b->binary.ptr;
				u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
				
				j();
				
				bbinaryptr = b->binary.ptr;
				// Set the 8bits immediate field appended above.
				*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
				
			} else if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// 0F 8C cd	JL rel32	#Jump near if less (SF≠OF).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0x8c;
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// 7C cb	JL rel8	#Jump short if less (SF≠OF).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x7c;
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void jltr (uint r1, uint r2, uint r3) {
			
			cmp(r1, r2);
			
			// Specification from the Intel manual.
			// 7D cb	JNL rel8	#Jump short if not less (SF=OF).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x7d;
			
			// Append the 8bits immediate
			// field which is the bytesize
			// of the following branching
			// instruction.
			u8* imm = arrayu8append1(&b->binary);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			jr(r3);
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jlti (uint r1, uint r2) {
			
			cmp(r1, r2);
			
			// Specification from the Intel manual.
			// 7D cb	JNL rel8	#Jump short if not less (SF=OF).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x7d;
			
			// Append the 8bits immediate
			// field which is the bytesize
			// of the following branching
			// instruction.
			u8* imm = arrayu8append1(&b->binary);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			ji();
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jlte (uint r1, uint r2) {
			
			cmp(r1, r2);
			
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Specification from the Intel manual.
				// 7F cb	JNLE rel8	#Jump short if not less or equal (ZF=0 and SF=OF).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x7f;
				
				// Append the 8bits immediate
				// field which is the bytesize
				// of the following branching
				// instruction.
				u8* imm = arrayu8append1(&b->binary);
				
				u8* bbinaryptr = b->binary.ptr;
				u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
				
				j();
				
				bbinaryptr = b->binary.ptr;
				// Set the 8bits immediate field appended above.
				*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
				
			} else if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// 0F 8E cd	JLE rel32	#Jump near if less or equal (ZF=1 or SF≠OF).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0x8e;
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// 7E cb	JLE rel8	#Jump short if less or equal (ZF=1 or SF≠OF).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x7e;
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void jlter (uint r1, uint r2, uint r3) {
			
			cmp(r1, r2);
			
			// Specification from the Intel manual.
			// 7F cb	JNLE rel8	#Jump short if not less or equal (ZF=0 and SF=OF).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x7f;
			
			// Append the 8bits immediate
			// field which is the bytesize
			// of the following branching
			// instruction.
			u8* imm = arrayu8append1(&b->binary);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			jr(r3);
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jltei (uint r1, uint r2) {
			
			cmp(r1, r2);
			
			// Specification from the Intel manual.
			// 7F cb	JNLE rel8	#Jump short if not less or equal (ZF=0 and SF=OF).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x7f;
			
			// Append the 8bits immediate
			// field which is the bytesize
			// of the following branching
			// instruction.
			u8* imm = arrayu8append1(&b->binary);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			ji();
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jltu (uint r1, uint r2) {
			
			cmp(r1, r2);
			
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Specification from the Intel manual.
				// 73 cb	JNB rel8	#Jump short if not below (CF=0).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x73;
				
				// Append the 8bits immediate
				// field which is the bytesize
				// of the following branching
				// instruction.
				u8* imm = arrayu8append1(&b->binary);
				
				u8* bbinaryptr = b->binary.ptr;
				u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
				
				j();
				
				bbinaryptr = b->binary.ptr;
				// Set the 8bits immediate field appended above.
				*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
				
			} else if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// 0F 82 cd	JB rel32	#Jump near if below (CF=1).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0x82;
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// 72 cb	JB rel8	#Jump short if below (CF=1).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x72;
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void jltur (uint r1, uint r2, uint r3) {
			
			cmp(r1, r2);
			
			// Specification from the Intel manual.
			// 73 cb	JNB rel8	#Jump short if not below (CF=0).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x73;
			
			// Append the 8bits immediate
			// field which is the bytesize
			// of the following branching
			// instruction.
			u8* imm = arrayu8append1(&b->binary);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			jr(r3);
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jltui (uint r1, uint r2) {
			
			cmp(r1, r2);
			
			// Specification from the Intel manual.
			// 73 cb	JNB rel8	#Jump short if not below (CF=0).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x73;
			
			// Append the 8bits immediate
			// field which is the bytesize
			// of the following branching
			// instruction.
			u8* imm = arrayu8append1(&b->binary);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			ji();
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jlteu (uint r1, uint r2) {
			
			cmp(r1, r2);
			
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Specification from the Intel manual.
				// 77 cb	JNBE rel8	#Jump short if not below or equal (CF=0 and ZF=0).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x77;
				
				// Append the 8bits immediate
				// field which is the bytesize
				// of the following branching
				// instruction.
				u8* imm = arrayu8append1(&b->binary);
				
				u8* bbinaryptr = b->binary.ptr;
				u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
				
				j();
				
				bbinaryptr = b->binary.ptr;
				// Set the 8bits immediate field appended above.
				*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
				
			} else if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// 0F 86 cd	JBE rel32	#Jump near if below or equal (CF=1 or ZF=1).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0x86;
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// 76 cb	JBE rel8	#Jump short if below or equal (CF=1 or ZF=1).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x76;
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void jlteur (uint r1, uint r2, uint r3) {
			
			cmp(r1, r2);
			
			// Specification from the Intel manual.
			// 77 cb	JNBE rel8	#Jump short if not below or equal (CF=0 and ZF=0).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x77;
			
			// Append the 8bits immediate
			// field which is the bytesize
			// of the following branching
			// instruction.
			u8* imm = arrayu8append1(&b->binary);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			jr(r3);
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jlteui (uint r1, uint r2) {
			
			cmp(r1, r2);
			
			// Specification from the Intel manual.
			// 77 cb	JNBE rel8	#Jump short if not below or equal (CF=0 and ZF=0).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x77;
			
			// Append the 8bits immediate
			// field which is the bytesize
			// of the following branching
			// instruction.
			u8* imm = arrayu8append1(&b->binary);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			ji();
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jz (uint r) {
			
			cmptozero(r);
			
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Specification from the Intel manual.
				// 75 cb	JNZ rel8	#Jump short if not zero (ZF=0).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x75;
				
				// Append the 8bits immediate
				// field which is the bytesize
				// of the following branching
				// instruction.
				u8* imm = arrayu8append1(&b->binary);
				
				u8* bbinaryptr = b->binary.ptr;
				u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
				
				j();
				
				bbinaryptr = b->binary.ptr;
				// Set the 8bits immediate field appended above.
				*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
				
			} else if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// 0F 84 cd	JZ rel32	#Jump near if zero (ZF=1).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0x84;
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// 74 cb	JZ rel8	#Jump short if zero (ZF=1).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x74;
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void jzr (uint r1, uint r2) {
			
			cmptozero(r1);
			
			// Specification from the Intel manual.
			// 75 cb	JNZ rel8	#Jump short if not zero (ZF=0).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x75;
			
			// Append the 8bits immediate
			// field which is the bytesize
			// of the following branching
			// instruction.
			u8* imm = arrayu8append1(&b->binary);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			jr(r2);
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jzi (uint r) {
			
			cmptozero(r);
			
			// Specification from the Intel manual.
			// 75 cb	JNZ rel8	#Jump short if not zero (ZF=0).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x75;
			
			// Append the 8bits immediate
			// field which is the bytesize
			// of the following branching
			// instruction.
			u8* imm = arrayu8append1(&b->binary);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			ji();
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jnz (uint r) {
			
			cmptozero(r);
			
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Specification from the Intel manual.
				// 74 cb	JZ rel8		#Jump short if zero (ZF=1).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x74;
				
				// Append the 8bits immediate
				// field which is the bytesize
				// of the following branching
				// instruction.
				u8* imm = arrayu8append1(&b->binary);
				
				u8* bbinaryptr = b->binary.ptr;
				u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
				
				j();
				
				bbinaryptr = b->binary.ptr;
				// Set the 8bits immediate field appended above.
				*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
				
			} else if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// 0F 85 cd	JNZ rel32	#Jump near if not zero (ZF=0).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0x85;
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// 75 cb	JNZ rel8	#Jump short if not zero (ZF=0).
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x75;
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void jnzr (uint r1, uint r2) {
			
			cmptozero(r1);
			
			// Specification from the Intel manual.
			// 74 cb	JZ rel8		#Jump short if zero (ZF=1).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x74;
			
			// Append the 8bits immediate
			// field which is the bytesize
			// of the following branching
			// instruction.
			u8* imm = arrayu8append1(&b->binary);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			jr(r2);
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jnzi (uint r) {
			
			cmptozero(r);
			
			// Specification from the Intel manual.
			// 74 cb	JZ rel8		#Jump short if zero (ZF=1).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x74;
			
			// Append the 8bits immediate
			// field which is the bytesize
			// of the following branching
			// instruction.
			u8* imm = arrayu8append1(&b->binary);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			ji();
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		// Generate the instruction that compute
		// in r, an RIP-relative address, and return
		// the address of the 32bits immediate field
		// which is set to the displacement from RIP.
		/* // ### Commented-out as RIP-relative addressing is failing.
		u32* gip32 (uint r) {
			// Specification from the Intel manual.
			// REX.W 8B /r	MOV r64,r/m64		#Move r/m64 to r64.
			// The ModR/M byte get set such that
			// a 32bits displacement is expected.
			
			u8 lookupregr = lookupreg(r);
			
			// REX-Prefix == 0100WR00
			u8 rex = (0x48|((lookupregr>7)<<2));
			
			lookupregr %= 8;
			
			// From the most significant bit to the least significant bit.
			// MOD == 0b00;
			// REG == reg[2:0]; #Only the 3 least significant bits matters.
			// R/M == 0b101
			u8 modrm = ((lookupregr<<3)|0b101);
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = rex;
			*arrayu8append1(&b->binary) = 0x8b;
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = modrm;
			
			// Append a 32bits offset immediate field,
			// set it null and return its address.
			u32* imm = (u32*)arrayu8append2(&b->binary, sizeof(u32));
			*imm = 0;
			return imm;
		}*/
		
		// Generate the instruction that compute
		// in r, an RIP-relative address, and return
		// the address of the 8bits immediate field
		// which is set to the displacement from RIP.
		u8* gip8 (uint r) {
			// Specification from the Intel manual.
			// E8 cd	CALL rel32	#Call relative.
			// ### Using the rel16 variant
			// ### (by prefixing 0x66) which
			// ### is shorter, would segfault.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xe8;
			
			// Append a 32bits offset immediate value of zero.
			*(u32*)arrayu8append2(&b->binary, sizeof(u32)) = (u32)0;
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			// Pop in r the return address
			// which is the address of
			// the POP instruction itself.
			pop(r);
			
			// Specification from the Intel manual.
			// REX.W 83 /0 ib     ADD r/m64, imm8         #Add sign-extended imm8 to r/m64.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x48;
			*arrayu8append1(&b->binary) = 0x83;
			
			// Append the ModR/M byte.
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			*arrayu8append1(&b->binary) = ((0b11<<6)|lookupreg(r));
			
			// Append the 8bits immediate field to be later set.
			u8* imm = arrayu8append1(&b->binary);
			
			bbinaryptr = b->binary.ptr;
			// Set the immediate value
			// to account for instructions from
			// after the CALL instruction.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
			
			return imm;
		}
		
		void jl (uint r) {
			
			u8* imm = gip8(r);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			j();
			
			bbinaryptr = b->binary.ptr;
			// Increment the 8bits immediate field from gip8().
			*imm += (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jlr (uint r1, uint r2) {
			
			u8* imm = gip8(r1);
			
			u8* bbinaryptr = b->binary.ptr;
			u8* opstartptr = (bbinaryptr + mmsz(bbinaryptr));
			
			jr(r2);
			
			bbinaryptr = b->binary.ptr;
			// Increment the 8bits immediate field from gip8().
			*imm += (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jli (uint r) {
			// The x64 instruction set
			// do not have a jump instruction
			// to an immediate absolute address,
			// so I use an unused register to hold
			// the absolute address to jump to.
			
			// Look for an unused register.
			uint unusedreg = findunusedreg(0);
			
			// Since all registers are
			// flushed before a branching,
			// there should be unused
			// registers available.
			if (!unusedreg) throwerror();
			
			// Mark unused register as temporarily used.
			regsinusetmp[unusedreg] = 1;
			
			ldi(unusedreg, 0);
			
			jlr(r, unusedreg);
			
			// Unmark register as temporarily used.
			regsinusetmp[unusedreg] = 0;
		}
		
		void jpushr (uint r) {
			// Specification from the Intel manual.
			// FF /2	CALL r/m64	#Call absolute.
			
			u8 lookupregr = lookupreg(r);
			
			if (lookupregr > 7) {
				// REX-Prefix == 0100000B
				*arrayu8append1(&b->binary) = 0x41;
				
				lookupregr %= 8;
			}
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xff;
			
			// Append the ModR/M byte.
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			*arrayu8append1(&b->binary) = ((0b11<<6)|(2<<3)|lookupregr);
		}
		
		void jpush () {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// There is always enough
				// unused registers before
				// a non-conditional branching,
				// because all registers
				// are flushed and discarded.
				if (!unusedreg) throwerror();
				
				// Mark unused register as temporarily used.
				regsinusetmp[unusedreg] = 1;
				
				afip(unusedreg);
				jpushr(unusedreg);
				
				// Unmark register as temporarily used.
				regsinusetmp[unusedreg] = 0;
				
			} else {
				// Specification from the Intel manual.
				// E8 cd	CALL rel32	#Call relative.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0xe8;
				
				// Append the immediate value.
				append32bitsimm(0);
			}
		}
		
		void jpushi () {
			// The x64 instruction set
			// do not have a jump instruction
			// to an immediate absolute address,
			// so I use an unused register to hold
			// the absolute address to jump to.
			
			// Look for an unused register.
			uint unusedreg = findunusedreg(0);
			
			// Since all registers are
			// flushed before a branching,
			// there should be unused
			// registers available.
			if (!unusedreg) throwerror();
			
			// Mark unused register as temporarily used.
			regsinusetmp[unusedreg] = 1;
			
			ldi(unusedreg, 0);
			
			jpushr(unusedreg);
			
			// Unmark register as temporarily used.
			regsinusetmp[unusedreg] = 0;
		}
		
		void jpop () {
			// Specification from the Intel manual.
			// C3	RET	#return to calling procedure.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xc3;
		}
		
		void ld8r (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 0F B6 /r	MOVZX r32, r/m8	#Move r/m8 to r32, zero-extension.
			
			u8 lookupregrm = lookupreg(r2);
			u8 lookupregreg = lookupreg(r1);
			
			u8 lookupregrmgt7 = (lookupregrm > 7);
			u8 lookupregreggt7 = (lookupregreg > 7);
			
			if (lookupregreggt7 || lookupregrmgt7) {
				// REX-Prefix == 01000R0B
				*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
				
				lookupregrm %= 8;
				lookupregreg %= 8;
			}
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0xb6;
			
			if (lookupregrm != LOOKUPREGRSP && lookupregrm != LOOKUPREGRBP) {
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((lookupregreg<<3)|lookupregrm);
				
			} else {
				// When I get here, I use a SIB byte.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b01;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
		}
		
		void ld8 (uint r1, uint r2) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				
				if (r1 == r2) {
					// Look for an unused register.
					uint unusedreg = findunusedreg(0);
					
					// The compiler should have insured
					// that there are enough unused
					// registers available.
					if (!unusedreg) throwerror();
					
					regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
					
					ldi(unusedreg, 0);
					add(unusedreg, r2);
					ld8r(r1, unusedreg);
					
					regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
					
				} else {
					
					ldi(r1, 0);
					add(r1, r2);
					ld8r(r1, r1);
				}
				
			} else {
				// Specification from the Intel manual.
				// 0F B6 /r	MOVZX r32, r/m8	#Move r/m8 to r32, zero-extension.
				
				u8 lookupregrm = lookupreg(r2);
				u8 lookupregreg = lookupreg(r1);
				
				u8 lookupregrmgt7 = (lookupregrm > 7);
				u8 lookupregreggt7 = (lookupregreg > 7);
				
				if (lookupregreggt7 || lookupregrmgt7) {
					// REX-Prefix == 01000R0B
					*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
					
					lookupregrm %= 8;
					lookupregreg %= 8;
				}
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0xb6;
				
				if (b->isimmused == IMM32) {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append32bitsimm(0);
					
				} else {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append8bitsimm(0);
				}
			}
		}
		
		void ld8i (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				
				ldi(r, 0);
				ld8r(r, r);
				
			} else {
				// Specification from the Intel manual.
				// 0F B6 /r	MOVZX r32, r/m8		#Move r/m8 to r32, zero-extension.
				
				u8 lookupregr = lookupreg(r);
				
				u8 lookupregrgt7 = (lookupregr > 7);
				
				if (lookupregrgt7) {
					// REX-Prefix == 01000R00
					*arrayu8append1(&b->binary) = (0x40|(lookupregrgt7<<2));
					
					lookupregr %= 8;
				}
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0xb6;
				
				// Note that in x64, a ModR/M byte with
				// MOD == 0b00 and R/M == 0b101 correspond
				// to RIP-relative addressing; hence
				// the reason an alternate encoding
				// that use an extra SIB byte is used.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = (((lookupregr)<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: 0b101; #A register for base is not used and a 32bits immediate is expected.
				*arrayu8append1(&b->binary) = ((0b100<<3)|0b101);
				
				// Append the immediate value.
				append32bitsimm(0);
			}
		}
		
		void ld16r (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 0F B7 /r	MOVZX r32, r/m16	#Move r/m16 to r32, zero-extension.
			
			u8 lookupregrm = lookupreg(r2);
			u8 lookupregreg = lookupreg(r1);
			
			u8 lookupregrmgt7 = (lookupregrm > 7);
			u8 lookupregreggt7 = (lookupregreg > 7);
			
			if (lookupregreggt7 || lookupregrmgt7) {
				// REX-Prefix == 01000R0B
				*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
				
				lookupregrm %= 8;
				lookupregreg %= 8;
			}
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0xb7;
			
			if (lookupregrm != LOOKUPREGRSP && lookupregrm != LOOKUPREGRBP) {
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((lookupregreg<<3)|lookupregrm);
				
			} else {
				// When I get here, I use a SIB byte.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b01;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
		}
		
		void ld16 (uint r1, uint r2) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				
				if (r1 == r2) {
					// Look for an unused register.
					uint unusedreg = findunusedreg(0);
					
					// The compiler should have insured
					// that there are enough unused
					// registers available.
					if (!unusedreg) throwerror();
					
					regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
					
					ldi(unusedreg, 0);
					add(unusedreg, r2);
					ld16r(r1, unusedreg);
					
					regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
					
				} else {
					
					ldi(r1, 0);
					add(r1, r2);
					ld16r(r1, r1);
				}
				
			} else {
				// Specification from the Intel manual.
				// 0F B7 /r	MOVZX r32, r/m16	#Move r/m16 to r32, zero-extension.
				
				u8 lookupregrm = lookupreg(r2);
				u8 lookupregreg = lookupreg(r1);
				
				u8 lookupregrmgt7 = (lookupregrm > 7);
				u8 lookupregreggt7 = (lookupregreg > 7);
				
				if (lookupregreggt7 || lookupregrmgt7) {
					// REX-Prefix == 01000R0B
					*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
					
					lookupregrm %= 8;
					lookupregreg %= 8;
				}
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0xb7;
				
				if (b->isimmused == IMM32) {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append32bitsimm(0);
					
				} else {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append8bitsimm(0);
				}
			}
		}
		
		void ld16i (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				
				ldi(r, 0);
				ld16r(r, r);
				
			} else {
				// Specification from the Intel manual.
				// 0F B7 /r	MOVZX r32, r/m16	#Move r/m16 to r32, zero-extension.
				
				u8 lookupregr = lookupreg(r);
				
				u8 lookupregrgt7 = (lookupregr > 7);
				
				if (lookupregrgt7) {
					// REX-Prefix == 01000R00
					*arrayu8append1(&b->binary) = (0x40|(lookupregrgt7<<2));
					
					lookupregr %= 8;
				}
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0xb7;
				
				// Note that in x64, a ModR/M byte with
				// MOD == 0b00 and R/M == 0b101 correspond
				// to RIP-relative addressing; hence
				// the reason an alternate encoding
				// that use an extra SIB byte is used.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = (((lookupregr)<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: 0b101; #A register for base is not used and a 32bits immediate is expected.
				*arrayu8append1(&b->binary) = ((0b100<<3)|0b101);
				
				// Append the immediate value.
				append32bitsimm(0);
			}
		}
		
		void ld32r (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 8B /r	MOV r32,r/m32		#Move r/m32 to r32, zero-extension.
			
			u8 lookupregrm = lookupreg(r2);
			u8 lookupregreg = lookupreg(r1);
			
			u8 lookupregrmgt7 = (lookupregrm > 7);
			u8 lookupregreggt7 = (lookupregreg > 7);
			
			if (lookupregreggt7 || lookupregrmgt7) {
				// REX-Prefix == 01000R0B
				*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
				
				lookupregrm %= 8;
				lookupregreg %= 8;
			}
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x8b;
			
			if (lookupregrm != LOOKUPREGRSP && lookupregrm != LOOKUPREGRBP) {
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((lookupregreg<<3)|lookupregrm);
				
			} else {
				// When I get here, I use a SIB byte.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b01;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
		}
		
		void ld32 (uint r1, uint r2) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				
				if (r1 == r2) {
					// Look for an unused register.
					uint unusedreg = findunusedreg(0);
					
					// The compiler should have insured
					// that there are enough unused
					// registers available.
					if (!unusedreg) throwerror();
					
					regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
					
					ldi(unusedreg, 0);
					add(unusedreg, r2);
					ld32r(r1, unusedreg);
					
					regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
					
				} else {
					
					ldi(r1, 0);
					add(r1, r2);
					ld32r(r1, r1);
				}
				
			} else {
				// Specification from the Intel manual.
				// 8B /r	MOV r32,r/m32		#Move r/m32 to r32, zero-extension.
				
				u8 lookupregrm = lookupreg(r2);
				u8 lookupregreg = lookupreg(r1);
				
				u8 lookupregrmgt7 = (lookupregrm > 7);
				u8 lookupregreggt7 = (lookupregreg > 7);
				
				if (lookupregreggt7 || lookupregrmgt7) {
					// REX-Prefix == 01000R0B
					*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
					
					lookupregrm %= 8;
					lookupregreg %= 8;
				}
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x8b;
				
				if (b->isimmused == IMM32) {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append32bitsimm(0);
					
				} else {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append8bitsimm(0);
				}
			}
		}
		
		void ld32i (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				
				ldi(r, 0);
				ld32r(r, r);
				
			} else {
				// Specification from the Intel manual.
				// 8B /r	MOV r32,r/m32		#Move r/m32 to r32, zero-extension.
				
				u8 lookupregr = lookupreg(r);
				
				u8 lookupregrgt7 = (lookupregr > 7);
				
				if (lookupregrgt7) {
					// REX-Prefix == 01000R00
					*arrayu8append1(&b->binary) = (0x40|(lookupregrgt7<<2));
					
					lookupregr %= 8;
				}
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x8b;
				
				// Note that in x64, a ModR/M byte with
				// MOD == 0b00 and R/M == 0b101 correspond
				// to RIP-relative addressing; hence
				// the reason an alternate encoding
				// that use an extra SIB byte is used.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = (((lookupregr)<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: 0b101; #A register for base is not used and a 32bits immediate is expected.
				*arrayu8append1(&b->binary) = ((0b100<<3)|0b101);
				
				// Append the immediate value.
				append32bitsimm(0);
			}
		}
		
		void ld64r (uint r1, uint r2) {
			// Specification from the Intel manual.
			// REX.W 8B /r	MOV r64,r/m64		#Move r/m64 to r64.
			
			u8 lookupregrm = lookupreg(r2);
			u8 lookupregreg = lookupreg(r1);
			
			// REX-Prefix == 0100WR0B
			*arrayu8append1(&b->binary) = (0x48|((lookupregreg>7)<<2)|(lookupregrm>7));
			
			lookupregrm %= 8;
			lookupregreg %= 8;
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x8b;
			
			if (lookupregrm != LOOKUPREGRSP && lookupregrm != LOOKUPREGRBP) {
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((lookupregreg<<3)|lookupregrm);
				
			} else {
				// When I get here, I use a SIB byte.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b01;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
		}
		
		void ld64 (uint r1, uint r2) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				
				if (r1 == r2) {
					// Look for an unused register.
					uint unusedreg = findunusedreg(0);
					
					// The compiler should have insured
					// that there are enough unused
					// registers available.
					if (!unusedreg) throwerror();
					
					regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
					
					ldi(unusedreg, 0);
					add(unusedreg, r2);
					ld64r(r1, unusedreg);
					
					regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
					
				} else {
					
					ldi(r1, 0);
					add(r1, r2);
					ld64r(r1, r1);
				}
				
			} else {
				// Specification from the Intel manual.
				// REX.W 8B /r	MOV r64,r/m64		#Move r/m64 to r64.
				
				u8 lookupregrm = lookupreg(r2);
				u8 lookupregreg = lookupreg(r1);
				
				// REX-Prefix == 0100WR0B
				*arrayu8append1(&b->binary) = (0x48|((lookupregreg>7)<<2)|(lookupregrm>7));
				
				lookupregrm %= 8;
				lookupregreg %= 8;
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x8b;
				
				if (b->isimmused == IMM32) {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append32bitsimm(0);
					
				} else {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append8bitsimm(0);
				}
			}
		}
		
		void ld64i (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				
				ldi(r, 0);
				ld64r(r, r);
				
			} else {
				// Specification from the Intel manual.
				// REX.W 8B /r	MOV r64,r/m64		#Move r/m64 to r64.
				
				u8 lookupregr = lookupreg(r);
				
				// REX-Prefix == 0100WR00
				*arrayu8append1(&b->binary) = (0x48|((lookupregr>7)<<2));
				
				lookupregr %= 8;
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x8b;
				
				// Note that in x64, a ModR/M byte with
				// MOD == 0b00 and R/M == 0b101 correspond
				// to RIP-relative addressing; hence
				// the reason an alternate encoding
				// that use an extra SIB byte is used.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = (((lookupregr)<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: 0b101; #A register for base is not used and a 32bits immediate is expected.
				*arrayu8append1(&b->binary) = ((0b100<<3)|0b101);
				
				// Append the immediate value.
				append32bitsimm(0);
			}
		}
		
		void st8r (uint r1, uint r2) {
			// r1 must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r1 is not either
			// of RAX, RBX, RCX, RDX.
			if (r1 < RAX || r1 > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != RAX) rvalid = RAX;
					else rvalid = RBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			// Specification from the Intel manual.
			// 88 /r	MOV r/m8,r8		#Move r8 to r/m8.
			
			u8 lookupregrm = lookupreg(r2);
			u8 lookupregreg = lookupreg(rvalid);
			
			u8 lookupregrmgt7 = (lookupregrm > 7);
			u8 lookupregreggt7 = (lookupregreg > 7);
			
			if (lookupregreggt7 || lookupregrmgt7) {
				// REX-Prefix == 01000R0B
				*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
				
				lookupregrm %= 8;
				lookupregreg %= 8;
			}
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x88;
			
			if (lookupregrm != LOOKUPREGRSP && lookupregrm != LOOKUPREGRBP) {
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((lookupregreg<<3)|lookupregrm);
				
			} else {
				// When I get here, I use a SIB byte.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b01;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
			
			if (rvalid != r1) {
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void st8 (uint r1, uint r2) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				add(unusedreg, r2);
				st8r(r1, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else {
				// r1 must be a register that
				// can be encoded in the field reg
				// of an instruction ModR/M when
				// it is used as an 8bits operand;
				// either RAX, RBX, RCX, RDX;
				
				// Variable to be set to the register
				// which will be encoded in the field
				// reg of the instruction ModR/M.
				uint rvalid;
				
				// Variable set to 1 if
				// the register rvalid
				// was saved.
				uint rvalidwassaved = 0;
				
				// I find a valid unused register
				// to use if r1 is not either
				// of RAX, RBX, RCX, RDX.
				if (r1 < RAX || r1 > RDX) {
					
					if (!(rvalid = findunusedregfor8bitsoperand(0))) {
						// I get here if r1 was not
						// either of RAX, RBX, RCX, RDX
						// and I could not find a valid
						// unused register to use.
						
						if (r2 != RAX) rvalid = RAX;
						else rvalid = RBX;
						
						savereg(rvalid); rvalidwassaved = 1;
						
					} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
					
					cpy(rvalid, r1);
					
				} else rvalid = r1;
				
				// Specification from the Intel manual.
				// 88 /r	MOV r/m8,r8		#Move r8 to r/m8.
				
				u8 lookupregrm = lookupreg(r2);
				u8 lookupregreg = lookupreg(rvalid);
				
				u8 lookupregrmgt7 = (lookupregrm > 7);
				u8 lookupregreggt7 = (lookupregreg > 7);
				
				if (lookupregreggt7 || lookupregrmgt7) {
					// REX-Prefix == 01000R0B
					*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
					
					lookupregrm %= 8;
					lookupregreg %= 8;
				}
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x88;
				
				if (b->isimmused == IMM32) {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append32bitsimm(0);
					
				} else {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append8bitsimm(0);
				}
				
				if (rvalid != r1) {
					
					if (rvalidwassaved) restorereg(rvalid);
					// Unmark register as temporarily used.
					else regsinusetmp[rvalid] = 0;
				}
			}
		}
		
		void st8i (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				st8r(r, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else {
				// r must be a register that
				// can be encoded in the field reg
				// of an instruction ModR/M when
				// it is used as an 8bits operand;
				// either RAX, RBX, RCX, RDX;
				
				// Variable to be set to the register
				// which will be encoded in the field
				// reg of the instruction ModR/M.
				uint rvalid;
				
				// Variable set to 1 if
				// the register rvalid
				// was saved.
				uint rvalidwassaved = 0;
				
				// I find a valid unused register
				// to use if r is not either
				// of RAX, RBX, RCX, RDX.
				if (r < RAX || r > RDX) {
					
					if (!(rvalid = findunusedregfor8bitsoperand(0))) {
						// I get here if r was not
						// either of RAX, RBX, RCX, RDX
						// and I could not find a valid
						// unused register to use.
						
						rvalid = RAX;
						
						savereg(rvalid); rvalidwassaved = 1;
						
					} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
					
					cpy(rvalid, r);
					
				} else rvalid = r;
				
				// Specification from the Intel manual.
				// 88 /r	MOV r/m8,r8		#Move r8 to r/m8.
				
				u8 lookupregr = lookupreg(r);
				
				u8 lookupregrgt7 = (lookupregr > 7);
				
				if (lookupregrgt7) {
					// REX-Prefix == 01000R00
					*arrayu8append1(&b->binary) = (0x40|(lookupregrgt7<<2));
					
					lookupregr %= 8;
				}
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x88;
				
				// Note that in x64, a ModR/M byte with
				// MOD == 0b00 and R/M == 0b101 correspond
				// to RIP-relative addressing; hence
				// the reason an alternate encoding
				// that use an extra SIB byte is used.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = (((lookupregr)<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: 0b101; #A register for base is not used and a 32bits immediate is expected.
				*arrayu8append1(&b->binary) = ((0b100<<3)|0b101);
				
				// Append the immediate value.
				append32bitsimm(0);
				
				if (rvalid != r) {
					
					if (rvalidwassaved) restorereg(rvalid);
					// Unmark register as temporarily used.
					else regsinusetmp[rvalid] = 0;
				}
			}
		}
		
		void st16r (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 89 /r	MOV r/m16,r16		#Move r16 to r/m16.
			
			u8 lookupregrm = lookupreg(r2);
			u8 lookupregreg = lookupreg(r1);
			
			u8 lookupregrmgt7 = (lookupregrm > 7);
			u8 lookupregreggt7 = (lookupregreg > 7);
			
			if (lookupregreggt7 || lookupregrmgt7) {
				// REX-Prefix == 01000R0B
				*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
				
				lookupregrm %= 8;
				lookupregreg %= 8;
			}
			
			// Append the prefix byte 0x66
			// to consider operands as 16bits.
			*arrayu8append1(&b->binary) = 0x66;
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x89;
			
			if (lookupregrm != LOOKUPREGRSP && lookupregrm != LOOKUPREGRBP) {
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((lookupregreg<<3)|lookupregrm);
				
			} else {
				// When I get here, I use a SIB byte.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b01;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
		}
		
		void st16 (uint r1, uint r2) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				add(unusedreg, r2);
				st16r(r1, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else {
				// Specification from the Intel manual.
				// 89 /r	MOV r/m16,r16		#Move r16 to r/m16.
				
				u8 lookupregrm = lookupreg(r2);
				u8 lookupregreg = lookupreg(r1);
				
				u8 lookupregrmgt7 = (lookupregrm > 7);
				u8 lookupregreggt7 = (lookupregreg > 7);
				
				if (lookupregreggt7 || lookupregrmgt7) {
					// REX-Prefix == 01000R0B
					*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
					
					lookupregrm %= 8;
					lookupregreg %= 8;
				}
				
				// Append the prefix byte 0x66
				// to consider operands as 16bits.
				*arrayu8append1(&b->binary) = 0x66;
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x89;
				
				if (b->isimmused == IMM32) {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append32bitsimm(0);
					
				} else {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append8bitsimm(0);
				}
			}
		}
		
		void st16i (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				st16r(r, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else {
				// Specification from the Intel manual.
				// 89 /r	MOV r/m16,r16		#Move r16 to r/m16.
				
				u8 lookupregr = lookupreg(r);
				
				u8 lookupregrgt7 = (lookupregr > 7);
				
				if (lookupregrgt7) {
					// REX-Prefix == 01000R00
					*arrayu8append1(&b->binary) = (0x40|(lookupregrgt7<<2));
					
					lookupregr %= 8;
				}
				
				// Append the prefix byte 0x66
				// to consider operands as 16bits.
				*arrayu8append1(&b->binary) = 0x66;
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x89;
				
				// Note that in x64, a ModR/M byte with
				// MOD == 0b00 and R/M == 0b101 correspond
				// to RIP-relative addressing; hence
				// the reason an alternate encoding
				// that use an extra SIB byte is used.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = (((lookupregr)<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: 0b101; #A register for base is not used and a 32bits immediate is expected.
				*arrayu8append1(&b->binary) = ((0b100<<3)|0b101);
				
				// Append the immediate value.
				append32bitsimm(0);
			}
		}
		
		void st32r (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 89 /r	MOV r/m32,r32		#Move r32 to r/m32.
			
			u8 lookupregrm = lookupreg(r2);
			u8 lookupregreg = lookupreg(r1);
			
			u8 lookupregrmgt7 = (lookupregrm > 7);
			u8 lookupregreggt7 = (lookupregreg > 7);
			
			if (lookupregreggt7 || lookupregrmgt7) {
				// REX-Prefix == 01000R0B
				*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
				
				lookupregrm %= 8;
				lookupregreg %= 8;
			}
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x89;
			
			if (lookupregrm != LOOKUPREGRSP && lookupregrm != LOOKUPREGRBP) {
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((lookupregreg<<3)|lookupregrm);
				
			} else {
				// When I get here, I use a SIB byte.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b01;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
		}
		
		void st32 (uint r1, uint r2) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				add(unusedreg, r2);
				st32r(r1, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else {
				// Specification from the Intel manual.
				// 89 /r	MOV r/m32,r32		#Move r32 to r/m32.
				
				u8 lookupregrm = lookupreg(r2);
				u8 lookupregreg = lookupreg(r1);
				
				u8 lookupregrmgt7 = (lookupregrm > 7);
				u8 lookupregreggt7 = (lookupregreg > 7);
				
				if (lookupregreggt7 || lookupregrmgt7) {
					// REX-Prefix == 01000R0B
					*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
					
					lookupregrm %= 8;
					lookupregreg %= 8;
				}
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x89;
				
				if (b->isimmused == IMM32) {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append32bitsimm(0);
					
				} else {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append8bitsimm(0);
				}
			}
		}
		
		void st32i (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				st32r(r, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else {
				// Specification from the Intel manual.
				// 89 /r	MOV r/m32,r32		#Move r32 to r/m32.
				
				u8 lookupregr = lookupreg(r);
				
				u8 lookupregrgt7 = (lookupregr > 7);
				
				if (lookupregrgt7) {
					// REX-Prefix == 01000R00
					*arrayu8append1(&b->binary) = (0x40|(lookupregrgt7<<2));
					
					lookupregr %= 8;
				}
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x89;
				
				// Note that in x64, a ModR/M byte with
				// MOD == 0b00 and R/M == 0b101 correspond
				// to RIP-relative addressing; hence
				// the reason an alternate encoding
				// that use an extra SIB byte is used.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = (((lookupregr)<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: 0b101; #A register for base is not used and a 32bits immediate is expected.
				*arrayu8append1(&b->binary) = ((0b100<<3)|0b101);
				
				// Append the immediate value.
				append32bitsimm(0);
			}
		}
		
		void st64r (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 89 /r	MOV r/m64,r64		#Move r64 to r/m64.
			
			u8 lookupregrm = lookupreg(r2);
			u8 lookupregreg = lookupreg(r1);
			
			u8 lookupregrmgt7 = (lookupregrm > 7);
			u8 lookupregreggt7 = (lookupregreg > 7);
			
			// REX-Prefix == 0100WR0B
			*arrayu8append1(&b->binary) = (0x48|(lookupregreggt7<<2)|lookupregrmgt7);
			
			lookupregrm %= 8;
			lookupregreg %= 8;
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x89;
			
			if (lookupregrm != LOOKUPREGRSP && lookupregrm != LOOKUPREGRBP) {
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((lookupregreg<<3)|lookupregrm);
				
			} else {
				// When I get here, I use a SIB byte.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b01;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
		}
		
		void st64 (uint r1, uint r2) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				add(unusedreg, r2);
				st64r(r1, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else {
				// Specification from the Intel manual.
				// REX.W 89 /r	MOV r/m64,r64		#Move r64 to r/m64.
				
				u8 lookupregrm = lookupreg(r2);
				u8 lookupregreg = lookupreg(r1);
				
				u8 lookupregrmgt7 = (lookupregrm > 7);
				u8 lookupregreggt7 = (lookupregreg > 7);
				
				// REX-Prefix == 0100WR0B
				*arrayu8append1(&b->binary) = (0x48|(lookupregreggt7<<2)|lookupregrmgt7);
				
				lookupregrm %= 8;
				lookupregreg %= 8;
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x89;
				
				if (b->isimmused == IMM32) {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append32bitsimm(0);
					
				} else {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append8bitsimm(0);
				}
			}
		}
		
		void st64i (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				st64r(r, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else {
				// Specification from the Intel manual.
				// REX.W 89 /r	MOV r/m64,r64		#Move r64 to r/m64.
				
				u8 lookupregr = lookupreg(r);
				
				u8 lookupregrgt7 = (lookupregr > 7);
				
				// REX-Prefix == 0100WR00
				*arrayu8append1(&b->binary) = (0x48|(lookupregrgt7<<2));
				
				lookupregr %= 8;
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x89;
				
				// Note that in x64, a ModR/M byte with
				// MOD == 0b00 and R/M == 0b101 correspond
				// to RIP-relative addressing; hence
				// the reason an alternate encoding
				// that use an extra SIB byte is used.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = (((lookupregr)<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: 0b101; #A register for base is not used and a 32bits immediate is expected.
				*arrayu8append1(&b->binary) = ((0b100<<3)|0b101);
				
				// Append the immediate value.
				append32bitsimm(0);
			}
		}
		
		void ldst8r (uint r1, uint r2) {
			// r1 must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either RAX, RBX, RCX, RDX;
			
			// Variable to be set to the register
			// which will be encoded in the field
			// reg of the instruction ModR/M.
			uint rvalid;
			
			// Variable set to 1 if
			// the register rvalid
			// was saved.
			uint rvalidwassaved = 0;
			
			// I find a valid unused register
			// to use if r1 is not either
			// of RAX, RBX, RCX, RDX.
			if (r1 < RAX || r1 > RDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of RAX, RBX, RCX, RDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != RAX) rvalid = RAX;
					else rvalid = RBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			// Specification from the Intel manual.
			// 86 /r	XCHG r/m8, r8		#Exchange r8 with byte from r/m8.
			
			u8 lookupregrm = lookupreg(r2);
			u8 lookupregreg = lookupreg(rvalid);
			
			u8 lookupregrmgt7 = (lookupregrm > 7);
			u8 lookupregreggt7 = (lookupregreg > 7);
			
			if (lookupregreggt7 || lookupregrmgt7) {
				// REX-Prefix == 01000R0B
				*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
				
				lookupregrm %= 8;
				lookupregreg %= 8;
			}
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x86;
			
			if (lookupregrm != LOOKUPREGRSP && lookupregrm != LOOKUPREGRBP) {
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((lookupregreg<<3)|lookupregrm);
				
			} else {
				// When I get here, I use a SIB byte.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b01;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
			
			if (rvalid != r1) {
				
				cpy(r1, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void ldst8 (uint r1, uint r2) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				add(unusedreg, r2);
				ldst8r(r1, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else {
				// r1 must be a register that
				// can be encoded in the field reg
				// of an instruction ModR/M when
				// it is used as an 8bits operand;
				// either RAX, RBX, RCX, RDX;
				
				// Variable to be set to the register
				// which will be encoded in the field
				// reg of the instruction ModR/M.
				uint rvalid;
				
				// Variable set to 1 if
				// the register rvalid
				// was saved.
				uint rvalidwassaved = 0;
				
				// I find a valid unused register
				// to use if r1 is not either
				// of RAX, RBX, RCX, RDX.
				if (r1 < RAX || r1 > RDX) {
					
					if (!(rvalid = findunusedregfor8bitsoperand(0))) {
						// I get here if r1 was not
						// either of RAX, RBX, RCX, RDX
						// and I could not find a valid
						// unused register to use.
						
						if (r2 != RAX) rvalid = RAX;
						else rvalid = RBX;
						
						savereg(rvalid); rvalidwassaved = 1;
						
					} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
					
					cpy(rvalid, r1);
					
				} else rvalid = r1;
				
				// Specification from the Intel manual.
				// 86 /r	XCHG r/m8, r8		#Exchange r8 with byte from r/m8.
				
				u8 lookupregrm = lookupreg(r2);
				u8 lookupregreg = lookupreg(rvalid);
				
				u8 lookupregrmgt7 = (lookupregrm > 7);
				u8 lookupregreggt7 = (lookupregreg > 7);
				
				if (lookupregreggt7 || lookupregrmgt7) {
					// REX-Prefix == 01000R0B
					*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
					
					lookupregrm %= 8;
					lookupregreg %= 8;
				}
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x86;
				
				if (b->isimmused == IMM32) {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append32bitsimm(0);
					
				} else {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append8bitsimm(0);
				}
				
				if (rvalid != r1) {
					
					cpy(r1, rvalid);
					
					if (rvalidwassaved) restorereg(rvalid);
					// Unmark register as temporarily used.
					else regsinusetmp[rvalid] = 0;
				}
			}
		}
		
		void ldst8i (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				ldst8r(r, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else {
				// r must be a register that
				// can be encoded in the field reg
				// of an instruction ModR/M when
				// it is used as an 8bits operand;
				// either RAX, RBX, RCX, RDX;
				
				// Variable to be set to the register
				// which will be encoded in the field
				// reg of the instruction ModR/M.
				uint rvalid;
				
				// Variable set to 1 if
				// the register rvalid
				// was saved.
				uint rvalidwassaved = 0;
				
				// I find a valid unused register
				// to use if r is not either
				// of RAX, RBX, RCX, RDX.
				if (r < RAX || r > RDX) {
					
					if (!(rvalid = findunusedregfor8bitsoperand(0))) {
						// I get here if r was not
						// either of RAX, RBX, RCX, RDX
						// and I could not find a valid
						// unused register to use.
						
						rvalid = RAX;
						
						savereg(rvalid); rvalidwassaved = 1;
						
					} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
					
					cpy(rvalid, r);
					
				} else rvalid = r;
				
				// Specification from the Intel manual.
				// 86 /r	XCHG r/m8, r8		#Exchange r8 with byte from r/m8.
				
				u8 lookupregr = lookupreg(r);
				
				u8 lookupregrgt7 = (lookupregr > 7);
				
				if (lookupregrgt7) {
					// REX-Prefix == 01000R00
					*arrayu8append1(&b->binary) = (0x40|(lookupregrgt7<<2));
					
					lookupregr %= 8;
				}
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x86;
				
				// Note that in x64, a ModR/M byte with
				// MOD == 0b00 and R/M == 0b101 correspond
				// to RIP-relative addressing; hence
				// the reason an alternate encoding
				// that use an extra SIB byte is used.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = (((lookupregr)<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: 0b101; #A register for base is not used and a 32bits immediate is expected.
				*arrayu8append1(&b->binary) = ((0b100<<3)|0b101);
				
				// Append the immediate value.
				append32bitsimm(0);
				
				if (rvalid != r) {
					
					cpy(r, rvalid);
					
					if (rvalidwassaved) restorereg(rvalid);
					// Unmark register as temporarily used.
					else regsinusetmp[rvalid] = 0;
				}
			}
		}
		
		void ldst16r (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 87 /r	XCHG r/m16, r16		#Exchange r16 with doubleword from r/m16.
			
			u8 lookupregrm = lookupreg(r2);
			u8 lookupregreg = lookupreg(r1);
			
			u8 lookupregrmgt7 = (lookupregrm > 7);
			u8 lookupregreggt7 = (lookupregreg > 7);
			
			if (lookupregreggt7 || lookupregrmgt7) {
				// REX-Prefix == 01000R0B
				*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
				
				lookupregrm %= 8;
				lookupregreg %= 8;
			}
			
			// Append the prefix byte 0x66
			// to consider operands as 16bits.
			*arrayu8append1(&b->binary) = 0x66;
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x87;
			
			if (lookupregrm != LOOKUPREGRSP && lookupregrm != LOOKUPREGRBP) {
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((lookupregreg<<3)|lookupregrm);
				
			} else {
				// When I get here, I use a SIB byte.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b01;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
		}
		
		void ldst16 (uint r1, uint r2) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				add(unusedreg, r2);
				ldst16r(r1, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else {
				// Specification from the Intel manual.
				// 87 /r	XCHG r/m16, r16		#Exchange r16 with doubleword from r/m16.
				
				u8 lookupregrm = lookupreg(r2);
				u8 lookupregreg = lookupreg(r1);
				
				u8 lookupregrmgt7 = (lookupregrm > 7);
				u8 lookupregreggt7 = (lookupregreg > 7);
				
				if (lookupregreggt7 || lookupregrmgt7) {
					// REX-Prefix == 01000R0B
					*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
					
					lookupregrm %= 8;
					lookupregreg %= 8;
				}
				
				// Append the prefix byte 0x66
				// to consider operands as 16bits.
				*arrayu8append1(&b->binary) = 0x66;
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x87;
				
				if (b->isimmused == IMM32) {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append32bitsimm(0);
					
				} else {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append8bitsimm(0);
				}
			}
		}
		
		void ldst16i (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				ldst16r(r, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else {
				// Specification from the Intel manual.
				// 87 /r	XCHG r/m16, r16		#Exchange r16 with doubleword from r/m16.
				
				u8 lookupregr = lookupreg(r);
				
				u8 lookupregrgt7 = (lookupregr > 7);
				
				if (lookupregrgt7) {
					// REX-Prefix == 01000R00
					*arrayu8append1(&b->binary) = (0x40|(lookupregrgt7<<2));
					
					lookupregr %= 8;
				}
				
				// Append the prefix byte 0x66
				// to consider operands as 16bits.
				*arrayu8append1(&b->binary) = 0x66;
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x87;
				
				// Note that in x64, a ModR/M byte with
				// MOD == 0b00 and R/M == 0b101 correspond
				// to RIP-relative addressing; hence
				// the reason an alternate encoding
				// that use an extra SIB byte is used.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = (((lookupregr)<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: 0b101; #A register for base is not used and a 32bits immediate is expected.
				*arrayu8append1(&b->binary) = ((0b100<<3)|0b101);
				
				// Append the immediate value.
				append32bitsimm(0);
			}
		}
		
		void ldst32r (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 87 /r	XCHG r/m32, r32		#Exchange r32 with doubleword from r/m32.
			
			u8 lookupregrm = lookupreg(r2);
			u8 lookupregreg = lookupreg(r1);
			
			u8 lookupregrmgt7 = (lookupregrm > 7);
			u8 lookupregreggt7 = (lookupregreg > 7);
			
			if (lookupregreggt7 || lookupregrmgt7) {
				// REX-Prefix == 01000R0B
				*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
				
				lookupregrm %= 8;
				lookupregreg %= 8;
			}
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x87;
			
			if (lookupregrm != LOOKUPREGRSP && lookupregrm != LOOKUPREGRBP) {
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((lookupregreg<<3)|lookupregrm);
				
			} else {
				// When I get here, I use a SIB byte.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b01;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
		}
		
		void ldst32 (uint r1, uint r2) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				add(unusedreg, r2);
				ldst32r(r1, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else {
				// Specification from the Intel manual.
				// 87 /r	XCHG r/m32, r32		#Exchange r32 with doubleword from r/m32.
				
				u8 lookupregrm = lookupreg(r2);
				u8 lookupregreg = lookupreg(r1);
				
				u8 lookupregrmgt7 = (lookupregrm > 7);
				u8 lookupregreggt7 = (lookupregreg > 7);
				
				if (lookupregreggt7 || lookupregrmgt7) {
					// REX-Prefix == 01000R0B
					*arrayu8append1(&b->binary) = (0x40|(lookupregreggt7<<2)|lookupregrmgt7);
					
					lookupregrm %= 8;
					lookupregreg %= 8;
				}
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x87;
				
				if (b->isimmused == IMM32) {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append32bitsimm(0);
					
				} else {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append8bitsimm(0);
				}
			}
		}
		
		void ldst32i (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				ldst32r(r, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else {
				// Specification from the Intel manual.
				// 87 /r	XCHG r/m32, r32		#Exchange r32 with doubleword from r/m32.
				
				u8 lookupregr = lookupreg(r);
				
				u8 lookupregrgt7 = (lookupregr > 7);
				
				if (lookupregrgt7) {
					// REX-Prefix == 01000R00
					*arrayu8append1(&b->binary) = (0x40|(lookupregrgt7<<2));
					
					lookupregr %= 8;
				}
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x87;
				
				// Note that in x64, a ModR/M byte with
				// MOD == 0b00 and R/M == 0b101 correspond
				// to RIP-relative addressing; hence
				// the reason an alternate encoding
				// that use an extra SIB byte is used.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = (((lookupregr)<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: 0b101; #A register for base is not used and a 32bits immediate is expected.
				*arrayu8append1(&b->binary) = ((0b100<<3)|0b101);
				
				// Append the immediate value.
				append32bitsimm(0);
			}
		}
		
		void ldst64r (uint r1, uint r2) {
			// Specification from the Intel manual.
			// REX.W 87 /r	XCHG r/m64, r64		#Exchange r64 with doubleword from r/m64.
			
			u8 lookupregrm = lookupreg(r2);
			u8 lookupregreg = lookupreg(r1);
			
			u8 lookupregrmgt7 = (lookupregrm > 7);
			u8 lookupregreggt7 = (lookupregreg > 7);
			
			// REX-Prefix == 0100WR0B
			*arrayu8append1(&b->binary) = (0x48|(lookupregreggt7<<2)|lookupregrmgt7);
			
			lookupregrm %= 8;
			lookupregreg %= 8;
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x87;
			
			if (lookupregrm != LOOKUPREGRSP && lookupregrm != LOOKUPREGRBP) {
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((lookupregreg<<3)|lookupregrm);
				
			} else {
				// When I get here, I use a SIB byte.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b01;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: rm[2:0]; #Only the 3 least significant bits matters.
				*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
		}
		
		void ldst64 (uint r1, uint r2) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				add(unusedreg, r2);
				ldst64r(r1, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else {
				// Specification from the Intel manual.
				// REX.W 87 /r	XCHG r/m64, r64		#Exchange r64 with doubleword from r/m64.
				
				u8 lookupregrm = lookupreg(r2);
				u8 lookupregreg = lookupreg(r1);
				
				u8 lookupregrmgt7 = (lookupregrm > 7);
				u8 lookupregreggt7 = (lookupregreg > 7);
				
				// REX-Prefix == 0100WR0B
				*arrayu8append1(&b->binary) = (0x48|(lookupregreggt7<<2)|lookupregrmgt7);
				
				lookupregrm %= 8;
				lookupregreg %= 8;
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x87;
				
				if (b->isimmused == IMM32) {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b10;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append32bitsimm(0);
					
				} else {
					
					if (lookupregrm != LOOKUPREGRSP) {
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|lookupregrm);
						
					} else {
						// When I get here, I use a SIB byte.
						
						// Append the ModR/M byte.
						// From the most significant bit to the least significant bit.
						// MOD == 0b01;
						// REG == reg[2:0]; #Only the 3 least significant bits matters.
						// R/M == 0b100;
						*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupregreg<<3)|0b100);
						
						// Append the SIB byte.
						// scale: 0b00;
						// index: 0b100; #A register for index is not used and scale is ignored.
						// base: rm[2:0]; #Only the 3 least significant bits matters.
						*arrayu8append1(&b->binary) = ((0b100<<3)|lookupregrm);
					}
					
					// Append the immediate value.
					append8bitsimm(0);
				}
			}
		}
		
		void ldst64i (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM64) {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				ldst64r(r, unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
				
			} else {
				// Specification from the Intel manual.
				// REX.W 87 /r	XCHG r/m64, r64		#Exchange r64 with doubleword from r/m64.
				
				u8 lookupregr = lookupreg(r);
				
				u8 lookupregrgt7 = (lookupregr > 7);
				
				// REX-Prefix == 0100WR00
				*arrayu8append1(&b->binary) = (0x48|(lookupregrgt7<<2));
				
				lookupregr %= 8;
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x87;
				
				// Note that in x64, a ModR/M byte with
				// MOD == 0b00 and R/M == 0b101 correspond
				// to RIP-relative addressing; hence
				// the reason an alternate encoding
				// that use an extra SIB byte is used.
				
				// Append the ModR/M byte.
				// From the most significant bit to the least significant bit.
				// MOD == 0b00;
				// REG == reg[2:0]; #Only the 3 least significant bits matters.
				// R/M == 0b100;
				*arrayu8append1(&b->binary) = (((lookupregr)<<3)|0b100);
				
				// Append the SIB byte.
				// scale: 0b00;
				// index: 0b100; #A register for index is not used and scale is ignored.
				// base: 0b101; #A register for base is not used and a 32bits immediate is expected.
				*arrayu8append1(&b->binary) = ((0b100<<3)|0b101);
				
				// Append the immediate value.
				append32bitsimm(0);
			}
		}
		
		void mem8cpy (uint r1, uint r2, uint r3) {
			// r1, r2, r3 are assumed
			// different registers.
			
			uint rdiwassaved = 0;
			uint rsiwassaved = 0;
			uint rcxwassaved = 0;
			
			if (r1 != RDI && r2 != RDI && r3 != RDI) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDI)) {
					savereg(RDI); rdiwassaved = 1;
				} else regsinusetmp[RDI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != RSI && r2 != RSI && r3 != RSI) {
				// Save register value
				// if it is being used.
				if (isreginuse(RSI)) {
					savereg(RSI); rsiwassaved = 1;
				} else regsinusetmp[RSI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != RCX && r2 != RCX && r3 != RCX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RCX)) {
					savereg(RCX); rcxwassaved = 1;
				} else regsinusetmp[RCX] = 1; // Mark unused register as temporarily used.
			}
			
			// Set RDI, RSI, RCX
			// respectively
			// to r1, r2, r3 by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r3 != RCX) push(r3);
			if (r2 != RSI) push(r2);
			if (r1 != RDI) {
				push(r1);
				pop(RDI);
			}
			if (r2 != RSI) pop(RSI);
			if (r3 != RCX) pop(RCX);
			
			// Clear direction flag to
			// select incremental copy.
			
			// Specification from the Intel manual.
			// FC	CLD	#Clear DF Flag.
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xfc;
			
			// Repeat the instruction movsb
			// until RCX become null.
			
			// Specification from the Intel manual.
			// F3 A4	MOVSB		#Move 8bits data from address RSI to address RDI.
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0xf3;
			*arrayu8append1(&b->binary) = 0xa4;
			
			// Set r1, r2
			// respectively
			// to RSI, RDI by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r2 != RSI) push(RSI);
			if (r1 != RDI) {
				push(RDI);
				pop(r1);
			}
			if (r2 != RSI) pop(r2);
			
			// If r3 != RCX, the value
			// of r3 is set null because
			// RCX is certainly null.
			if (r3 != RCX) xor(r3, r3);
			
			if (rcxwassaved) {
				// Restore register value.
				restorereg(RCX);
			// Unmark register as temporarily used.
			} else regsinusetmp[RCX] = 0;
			
			if (rsiwassaved) {
				// Restore register value.
				restorereg(RSI);
			// Unmark register as temporarily used.
			} else regsinusetmp[RSI] = 0;
			
			if (rdiwassaved) {
				// Restore register value.
				restorereg(RDI);
			// Unmark register as temporarily used.
			} else regsinusetmp[RDI] = 0;
		}
		
		void mem8cpy2 (uint r1, uint r2, uint r3) {
			// r1, r2, r3 are assumed
			// different registers.
			
			uint rdiwassaved = 0;
			uint rsiwassaved = 0;
			uint rcxwassaved = 0;
			
			if (r1 != RDI && r2 != RDI && r3 != RDI) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDI)) {
					savereg(RDI); rdiwassaved = 1;
				} else regsinusetmp[RDI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != RSI && r2 != RSI && r3 != RSI) {
				// Save register value
				// if it is being used.
				if (isreginuse(RSI)) {
					savereg(RSI); rsiwassaved = 1;
				} else regsinusetmp[RSI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != RCX && r2 != RCX && r3 != RCX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RCX)) {
					savereg(RCX); rcxwassaved = 1;
				} else regsinusetmp[RCX] = 1; // Mark unused register as temporarily used.
			}
			
			// Set RDI, RSI, RCX
			// respectively
			// to r1, r2, r3 by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r3 != RCX) push(r3);
			if (r2 != RSI) push(r2);
			if (r1 != RDI) {
				push(r1);
				pop(RDI);
			}
			if (r2 != RSI) pop(RSI);
			if (r3 != RCX) pop(RCX);
			
			// Set direction flag to
			// select decremental copy.
			
			// Specification from the Intel manual.
			// FD	STD	#Set DF Flag.
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xfd;
			
			// Repeat the instruction movsb
			// until RCX become null.
			
			// Specification from the Intel manual.
			// F3 A4	MOVSB		#Move 8bits data from address RSI to address RDI.
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0xf3;
			*arrayu8append1(&b->binary) = 0xa4;
			
			// Set r1, r2
			// respectively
			// to RSI, RDI by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r2 != RSI) push(RSI);
			if (r1 != RDI) {
				push(RDI);
				pop(r1);
			}
			if (r2 != RSI) pop(r2);
			
			// If r3 != RCX, the value
			// of r3 is set null because
			// RCX is certainly null.
			if (r3 != RCX) xor(r3, r3);
			
			if (rcxwassaved) {
				// Restore register value.
				restorereg(RCX);
			// Unmark register as temporarily used.
			} else regsinusetmp[RCX] = 0;
			
			if (rsiwassaved) {
				// Restore register value.
				restorereg(RSI);
			// Unmark register as temporarily used.
			} else regsinusetmp[RSI] = 0;
			
			if (rdiwassaved) {
				// Restore register value.
				restorereg(RDI);
			// Unmark register as temporarily used.
			} else regsinusetmp[RDI] = 0;
		}
		
		void mem16cpy (uint r1, uint r2, uint r3) {
			// r1, r2, r3 are assumed
			// different registers.
			
			uint rdiwassaved = 0;
			uint rsiwassaved = 0;
			uint rcxwassaved = 0;
			
			if (r1 != RDI && r2 != RDI && r3 != RDI) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDI)) {
					savereg(RDI); rdiwassaved = 1;
				} else regsinusetmp[RDI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != RSI && r2 != RSI && r3 != RSI) {
				// Save register value
				// if it is being used.
				if (isreginuse(RSI)) {
					savereg(RSI); rsiwassaved = 1;
				} else regsinusetmp[RSI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != RCX && r2 != RCX && r3 != RCX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RCX)) {
					savereg(RCX); rcxwassaved = 1;
				} else regsinusetmp[RCX] = 1; // Mark unused register as temporarily used.
			}
			
			// Set RDI, RSI, RCX
			// respectively
			// to r1, r2, r3 by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r3 != RCX) push(r3);
			if (r2 != RSI) push(r2);
			if (r1 != RDI) {
				push(r1);
				pop(RDI);
			}
			if (r2 != RSI) pop(RSI);
			if (r3 != RCX) pop(RCX);
			
			// Clear direction flag to
			// select incremental copy.
			
			// Specification from the Intel manual.
			// FC	CLD	#Clear DF Flag.
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xfc;
			
			// Append the prefix byte 0x66
			// to consider operands as 16bits.
			*arrayu8append1(&b->binary) = 0x66;
			
			// Repeat the instruction movsd
			// until RCX become null.
			
			// Specification from the Intel manual.
			// F3 A5	MOVSW		#Move 16bits data from address RSI to address RDI.
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0xf3;
			*arrayu8append1(&b->binary) = 0xa5;
			
			// Set r1, r2
			// respectively
			// to RSI, RDI by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r2 != RSI) push(RSI);
			if (r1 != RDI) {
				push(RDI);
				pop(r1);
			}
			if (r2 != RSI) pop(r2);
			
			// If r3 != RCX, the value
			// of r3 is set null because
			// RCX is certainly null.
			if (r3 != RCX) xor(r3, r3);
			
			if (rcxwassaved) {
				// Restore register value.
				restorereg(RCX);
			// Unmark register as temporarily used.
			} else regsinusetmp[RCX] = 0;
			
			if (rsiwassaved) {
				// Restore register value.
				restorereg(RSI);
			// Unmark register as temporarily used.
			} else regsinusetmp[RSI] = 0;
			
			if (rdiwassaved) {
				// Restore register value.
				restorereg(RDI);
			// Unmark register as temporarily used.
			} else regsinusetmp[RDI] = 0;
		}
		
		void mem16cpy2 (uint r1, uint r2, uint r3) {
			// r1, r2, r3 are assumed
			// different registers.
			
			uint rdiwassaved = 0;
			uint rsiwassaved = 0;
			uint rcxwassaved = 0;
			
			if (r1 != RDI && r2 != RDI && r3 != RDI) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDI)) {
					savereg(RDI); rdiwassaved = 1;
				} else regsinusetmp[RDI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != RSI && r2 != RSI && r3 != RSI) {
				// Save register value
				// if it is being used.
				if (isreginuse(RSI)) {
					savereg(RSI); rsiwassaved = 1;
				} else regsinusetmp[RSI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != RCX && r2 != RCX && r3 != RCX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RCX)) {
					savereg(RCX); rcxwassaved = 1;
				} else regsinusetmp[RCX] = 1; // Mark unused register as temporarily used.
			}
			
			// Set RDI, RSI, RCX
			// respectively
			// to r1, r2, r3 by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r3 != RCX) push(r3);
			if (r2 != RSI) push(r2);
			if (r1 != RDI) {
				push(r1);
				pop(RDI);
			}
			if (r2 != RSI) pop(RSI);
			if (r3 != RCX) pop(RCX);
			
			// Set direction flag to
			// select decremental copy.
			
			// Specification from the Intel manual.
			// FD	STD	#Set DF Flag.
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xfd;
			
			// Append the prefix byte 0x66
			// to consider operands as 16bits.
			*arrayu8append1(&b->binary) = 0x66;
			
			// Repeat the instruction movsd
			// until RCX become null.
			
			// Specification from the Intel manual.
			// F3 A5	MOVSW		#Move 16bits data from address RSI to address RDI.
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0xf3;
			*arrayu8append1(&b->binary) = 0xa5;
			
			// Set r1, r2
			// respectively
			// to RSI, RDI by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r2 != RSI) push(RSI);
			if (r1 != RDI) {
				push(RDI);
				pop(r1);
			}
			if (r2 != RSI) pop(r2);
			
			// If r3 != RCX, the value
			// of r3 is set null because
			// RCX is certainly null.
			if (r3 != RCX) xor(r3, r3);
			
			if (rcxwassaved) {
				// Restore register value.
				restorereg(RCX);
			// Unmark register as temporarily used.
			} else regsinusetmp[RCX] = 0;
			
			if (rsiwassaved) {
				// Restore register value.
				restorereg(RSI);
			// Unmark register as temporarily used.
			} else regsinusetmp[RSI] = 0;
			
			if (rdiwassaved) {
				// Restore register value.
				restorereg(RDI);
			// Unmark register as temporarily used.
			} else regsinusetmp[RDI] = 0;
		}
		
		void mem32cpy (uint r1, uint r2, uint r3) {
			// r1, r2, r3 are assumed
			// different registers.
			
			uint rdiwassaved = 0;
			uint rsiwassaved = 0;
			uint rcxwassaved = 0;
			
			if (r1 != RDI && r2 != RDI && r3 != RDI) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDI)) {
					savereg(RDI); rdiwassaved = 1;
				} else regsinusetmp[RDI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != RSI && r2 != RSI && r3 != RSI) {
				// Save register value
				// if it is being used.
				if (isreginuse(RSI)) {
					savereg(RSI); rsiwassaved = 1;
				} else regsinusetmp[RSI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != RCX && r2 != RCX && r3 != RCX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RCX)) {
					savereg(RCX); rcxwassaved = 1;
				} else regsinusetmp[RCX] = 1; // Mark unused register as temporarily used.
			}
			
			// Set RDI, RSI, RCX
			// respectively
			// to r1, r2, r3 by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r3 != RCX) push(r3);
			if (r2 != RSI) push(r2);
			if (r1 != RDI) {
				push(r1);
				pop(RDI);
			}
			if (r2 != RSI) pop(RSI);
			if (r3 != RCX) pop(RCX);
			
			// Clear direction flag to
			// select incremental copy.
			
			// Specification from the Intel manual.
			// FC	CLD	#Clear DF Flag.
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xfc;
			
			// Repeat the instruction movsd
			// until RCX become null.
			
			// Specification from the Intel manual.
			// F3 A5	MOVSD		#Move 32bits data from address RSI to address RDI.
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0xf3;
			*arrayu8append1(&b->binary) = 0xa5;
			
			// Set r1, r2
			// respectively
			// to RSI, RDI by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r2 != RSI) push(RSI);
			if (r1 != RDI) {
				push(RDI);
				pop(r1);
			}
			if (r2 != RSI) pop(r2);
			
			// If r3 != RCX, the value
			// of r3 is set null because
			// RCX is certainly null.
			if (r3 != RCX) xor(r3, r3);
			
			if (rcxwassaved) {
				// Restore register value.
				restorereg(RCX);
			// Unmark register as temporarily used.
			} else regsinusetmp[RCX] = 0;
			
			if (rsiwassaved) {
				// Restore register value.
				restorereg(RSI);
			// Unmark register as temporarily used.
			} else regsinusetmp[RSI] = 0;
			
			if (rdiwassaved) {
				// Restore register value.
				restorereg(RDI);
			// Unmark register as temporarily used.
			} else regsinusetmp[RDI] = 0;
		}
		
		void mem32cpy2 (uint r1, uint r2, uint r3) {
			// r1, r2, r3 are assumed
			// different registers.
			
			uint rdiwassaved = 0;
			uint rsiwassaved = 0;
			uint rcxwassaved = 0;
			
			if (r1 != RDI && r2 != RDI && r3 != RDI) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDI)) {
					savereg(RDI); rdiwassaved = 1;
				} else regsinusetmp[RDI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != RSI && r2 != RSI && r3 != RSI) {
				// Save register value
				// if it is being used.
				if (isreginuse(RSI)) {
					savereg(RSI); rsiwassaved = 1;
				} else regsinusetmp[RSI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != RCX && r2 != RCX && r3 != RCX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RCX)) {
					savereg(RCX); rcxwassaved = 1;
				} else regsinusetmp[RCX] = 1; // Mark unused register as temporarily used.
			}
			
			// Set RDI, RSI, RCX
			// respectively
			// to r1, r2, r3 by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r3 != RCX) push(r3);
			if (r2 != RSI) push(r2);
			if (r1 != RDI) {
				push(r1);
				pop(RDI);
			}
			if (r2 != RSI) pop(RSI);
			if (r3 != RCX) pop(RCX);
			
			// Set direction flag to
			// select decremental copy.
			
			// Specification from the Intel manual.
			// FD	STD	#Set DF Flag.
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xfd;
			
			// Repeat the instruction movsd
			// until RCX become null.
			
			// Specification from the Intel manual.
			// F3 A5	MOVSD		#Move 32bits data from address RSI to address RDI.
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0xf3;
			*arrayu8append1(&b->binary) = 0xa5;
			
			// Set r1, r2
			// respectively
			// to RSI, RDI by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r2 != RSI) push(RSI);
			if (r1 != RDI) {
				push(RDI);
				pop(r1);
			}
			if (r2 != RSI) pop(r2);
			
			// If r3 != RCX, the value
			// of r3 is set null because
			// RCX is certainly null.
			if (r3 != RCX) xor(r3, r3);
			
			if (rcxwassaved) {
				// Restore register value.
				restorereg(RCX);
			// Unmark register as temporarily used.
			} else regsinusetmp[RCX] = 0;
			
			if (rsiwassaved) {
				// Restore register value.
				restorereg(RSI);
			// Unmark register as temporarily used.
			} else regsinusetmp[RSI] = 0;
			
			if (rdiwassaved) {
				// Restore register value.
				restorereg(RDI);
			// Unmark register as temporarily used.
			} else regsinusetmp[RDI] = 0;
		}
		
		void mem64cpy (uint r1, uint r2, uint r3) {
			// r1, r2, r3 are assumed
			// different registers.
			
			uint rdiwassaved = 0;
			uint rsiwassaved = 0;
			uint rcxwassaved = 0;
			
			if (r1 != RDI && r2 != RDI && r3 != RDI) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDI)) {
					savereg(RDI); rdiwassaved = 1;
				} else regsinusetmp[RDI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != RSI && r2 != RSI && r3 != RSI) {
				// Save register value
				// if it is being used.
				if (isreginuse(RSI)) {
					savereg(RSI); rsiwassaved = 1;
				} else regsinusetmp[RSI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != RCX && r2 != RCX && r3 != RCX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RCX)) {
					savereg(RCX); rcxwassaved = 1;
				} else regsinusetmp[RCX] = 1; // Mark unused register as temporarily used.
			}
			
			// Set RDI, RSI, RCX
			// respectively
			// to r1, r2, r3 by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r3 != RCX) push(r3);
			if (r2 != RSI) push(r2);
			if (r1 != RDI) {
				push(r1);
				pop(RDI);
			}
			if (r2 != RSI) pop(RSI);
			if (r3 != RCX) pop(RCX);
			
			// Clear direction flag to
			// select incremental copy.
			
			// Specification from the Intel manual.
			// FC	CLD	#Clear DF Flag.
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xfc;
			
			// Repeat the instruction movsd
			// until RCX become null.
			
			// Specification from the Intel manual.
			// F3 REX.W A5	MOVSQ		#Move 64bits data from address RSI to address RDI.
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0xf3;
			*arrayu8append1(&b->binary) = 0x48;
			*arrayu8append1(&b->binary) = 0xa5;
			
			// Set r1, r2
			// respectively
			// to RSI, RDI by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r2 != RSI) push(RSI);
			if (r1 != RDI) {
				push(RDI);
				pop(r1);
			}
			if (r2 != RSI) pop(r2);
			
			// If r3 != RCX, the value
			// of r3 is set null because
			// RCX is certainly null.
			if (r3 != RCX) xor(r3, r3);
			
			if (rcxwassaved) {
				// Restore register value.
				restorereg(RCX);
			// Unmark register as temporarily used.
			} else regsinusetmp[RCX] = 0;
			
			if (rsiwassaved) {
				// Restore register value.
				restorereg(RSI);
			// Unmark register as temporarily used.
			} else regsinusetmp[RSI] = 0;
			
			if (rdiwassaved) {
				// Restore register value.
				restorereg(RDI);
			// Unmark register as temporarily used.
			} else regsinusetmp[RDI] = 0;
		}
		
		void mem64cpy2 (uint r1, uint r2, uint r3) {
			// r1, r2, r3 are assumed
			// different registers.
			
			uint rdiwassaved = 0;
			uint rsiwassaved = 0;
			uint rcxwassaved = 0;
			
			if (r1 != RDI && r2 != RDI && r3 != RDI) {
				// Save register value
				// if it is being used.
				if (isreginuse(RDI)) {
					savereg(RDI); rdiwassaved = 1;
				} else regsinusetmp[RDI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != RSI && r2 != RSI && r3 != RSI) {
				// Save register value
				// if it is being used.
				if (isreginuse(RSI)) {
					savereg(RSI); rsiwassaved = 1;
				} else regsinusetmp[RSI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != RCX && r2 != RCX && r3 != RCX) {
				// Save register value
				// if it is being used.
				if (isreginuse(RCX)) {
					savereg(RCX); rcxwassaved = 1;
				} else regsinusetmp[RCX] = 1; // Mark unused register as temporarily used.
			}
			
			// Set RDI, RSI, RCX
			// respectively
			// to r1, r2, r3 by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r3 != RCX) push(r3);
			if (r2 != RSI) push(r2);
			if (r1 != RDI) {
				push(r1);
				pop(RDI);
			}
			if (r2 != RSI) pop(RSI);
			if (r3 != RCX) pop(RCX);
			
			// Set direction flag to
			// select decremental copy.
			
			// Specification from the Intel manual.
			// FD	STD	#Set DF Flag.
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xfd;
			
			// Repeat the instruction movsd
			// until RCX become null.
			
			// Specification from the Intel manual.
			// F3 REX.W A5	MOVSQ		#Move 64bits data from address RSI to address RDI.
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0xf3;
			*arrayu8append1(&b->binary) = 0x48;
			*arrayu8append1(&b->binary) = 0xa5;
			
			// Set r1, r2
			// respectively
			// to RSI, RDI by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r2 != RSI) push(RSI);
			if (r1 != RDI) {
				push(RDI);
				pop(r1);
			}
			if (r2 != RSI) pop(r2);
			
			// If r3 != RCX, the value
			// of r3 is set null because
			// RCX is certainly null.
			if (r3 != RCX) xor(r3, r3);
			
			if (rcxwassaved) {
				// Restore register value.
				restorereg(RCX);
			// Unmark register as temporarily used.
			} else regsinusetmp[RCX] = 0;
			
			if (rsiwassaved) {
				// Restore register value.
				restorereg(RSI);
			// Unmark register as temporarily used.
			} else regsinusetmp[RSI] = 0;
			
			if (rdiwassaved) {
				// Restore register value.
				restorereg(RDI);
			// Unmark register as temporarily used.
			} else regsinusetmp[RDI] = 0;
		}
		
		void pushi (u64 imm) {
			
			if ((((sint)imm < 0) ? -imm : imm) < (1 << 7)) {
				// Specification from the Intel manual.
				// 6A		PUSH imm8	#Push imm8 (Sign-extended).
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x6a;
				
				// Append the 8bits immediate value.
				*arrayu8append1(&b->binary) = imm;
				
			}
			#if 0
			else if ((((sint)imm < 0) ? -imm : imm) < (1 << 15)) {
				// Specification from the Intel manual.
				// 68		PUSH imm16	#Push imm16 (Sign-extended).
				// ### Using the imm16 variant
				// ### (by prefixing 0x66),
				// ### would segfault.
				
				// Append the prefix byte 0x66
				// to consider operands as 16bits.
				*arrayu8append1(&b->binary) = 0x66;
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x68;
				
				// Append the 16bits immediate value.
				*(u16*)arrayu8append2(&b->binary, sizeof(u16)) = imm;
				
			}
			#endif
			else if ((((sint)imm < 0) ? -imm : imm) < (1 << 31)) {
				// Specification from the Intel manual.
				// 68		PUSH imm32	#Push imm32.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x68;
				
				// Append the 32bits immediate value.
				*(u32*)arrayu8append2(&b->binary, sizeof(u32)) = imm;
				
			} else {
				// Look for an unused register.
				uint unusedreg = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!unusedreg) throwerror();
				
				regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
				
				ldi(unusedreg, 0);
				push(unusedreg);
				
				regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
			}
		}
		
		// Function used to compute
		// the log2 ceiling of the
		// value given as argument.
		// When the argument is 0 or 1,
		// the value returned is 1.
		uint clog2 (uint a) {
			
			if (a > 1) {
				
				--a;
				
				uint n = 0;
				
				while (a) {
					a >>= 1;
					++n;
				}
				
				return n;
				
			} else return 1;
		}
		
		// This function save in the
		// stack, all used registers;
		// except the register for which
		// the id is given as argument;
		// note that %0 is never saved.
		void pushusedregs (uint e) {
			
			uint r = 1;
			
			do {
				if (isreginuse(r) && r != e) push(r);
				
			} while (++r < REGCOUNT);
		}
		
		// This function restore from
		// the stack, all used registers
		// in the reverse order in which
		// they were saved by pushusedregs();
		// except the register for which
		// the id is given as argument;
		// note that %0 is never restored.
		void popusedregs (uint e) {
			
			uint r = REGCOUNT-1;
			
			do {
				if (isreginuse(r) && r != e) pop(r);
				
			} while (--r);
		}
		
		while (1) {
			
			if (toredo.ptr) {
				// I get here when redoing lyricalinstruction.
				
				// The backenddata has already
				// been allocated, since I am
				// redoing lyricalinstruction.
				b = i->backenddata;
				
				if (bintreefind(toredo, (uint)i)) {
					
					mmfree(b->binary.ptr);
					
					b->binary = arrayu8null;
					
				} else goto skipredo;
				
				// Reset the immediates misc values null
				// as they are fields that get incremented.
				b->immmiscvalue = 0;
				b->imm2miscvalue = 0;
				
			} else {
				
				b = (backenddata*)mmallocz(sizeof(backenddata));
				
				i->backenddata = b;
			}
			
			switch(i->op) {
				
				case LYRICALOPADD:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						add(i->r1, i->r3);
						
					} else {
						
						add(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPADDI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					addi(i->r1);
					
					break;
					
				case LYRICALOPSUB:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						sub(i->r1, i->r3);
						
					} else if (i->r1 != i->r2) {
						
						sub(i->r3, i->r2);
						
						neg(i->r1);
						
					} else {
						
						sub(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPNEG:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					neg(i->r1);
					
					break;
					
				case LYRICALOPMUL:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						mul(i->r1, i->r3);
						
					} else {
						
						mul(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPMULH:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						mulh(i->r1, i->r3);
						
					} else {
						
						mulh(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPDIV:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						div(i->r1, i->r3);
						
					} else if (i->r1 != i->r2) {
						// Look for an unused register.
						uint unusedreg = findunusedreg(0);
						
						// The compiler should have insured
						// that there are enough unused
						// registers available.
						if (!unusedreg) throwerror();
						
						regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
						
						cpy(unusedreg, i->r1);
						cpy(i->r1, i->r2);
						div(i->r1, unusedreg);
						
						regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
						
					} else {
						
						div(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPMOD:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						mod(i->r1, i->r3);
						
					} else if (i->r1 != i->r2) {
						// Look for an unused register.
						uint unusedreg = findunusedreg(0);
						
						// The compiler should have insured
						// that there are enough unused
						// registers available.
						if (!unusedreg) throwerror();
						
						regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
						
						cpy(unusedreg, i->r1);
						cpy(i->r1, i->r2);
						mod(i->r1, unusedreg);
						
						regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
						
					} else {
						
						mod(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPMULHU:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						mulhu(i->r1, i->r3);
						
					} else {
						
						mulhu(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPDIVU:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						divu(i->r1, i->r3);
						
					} else if (i->r1 != i->r2) {
						// Look for an unused register.
						uint unusedreg = findunusedreg(0);
						
						// The compiler should have insured
						// that there are enough unused
						// registers available.
						if (!unusedreg) throwerror();
						
						regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
						
						cpy(unusedreg, i->r1);
						cpy(i->r1, i->r2);
						divu(i->r1, unusedreg);
						
						regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
						
					} else {
						
						divu(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPMODU:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						modu(i->r1, i->r3);
						
					} else if (i->r1 != i->r2) {
						// Look for an unused register.
						uint unusedreg = findunusedreg(0);
						
						// The compiler should have insured
						// that there are enough unused
						// registers available.
						if (!unusedreg) throwerror();
						
						regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
						
						cpy(unusedreg, i->r1);
						cpy(i->r1, i->r2);
						modu(i->r1, unusedreg);
						
						regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
						
					} else {
						
						modu(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPMULI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					muli(i->r1);
					
					break;
					
				case LYRICALOPMULHI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					mulhi(i->r1);
					
					break;
					
				case LYRICALOPDIVI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					divi(i->r1);
					
					break;
					
				case LYRICALOPMODI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					modi(i->r1);
					
					break;
					
				case LYRICALOPDIVI2:
					
					if (i->r1 != i->r2) {
						
						ldi(i->r1, 0);
						div(i->r1, i->r2);
						
					} else {
						// Look for an unused register.
						uint unusedreg = findunusedreg(RAX);
						
						// The compiler should have insured
						// that there are enough unused
						// registers available.
						if (!unusedreg) throwerror();
						
						regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
						
						ldi(unusedreg, 0);
						div(unusedreg, i->r1);
						cpy(i->r1, unusedreg);
						
						regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
					}
					
					break;
					
				case LYRICALOPMODI2:
					
					if (i->r1 != i->r2) {
						
						ldi(i->r1, 0);
						mod(i->r1, i->r2);
						
					} else {
						// Look for an unused register.
						uint unusedreg = findunusedreg(RAX);
						
						// The compiler should have insured
						// that there are enough unused
						// registers available.
						if (!unusedreg) throwerror();
						
						regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
						
						ldi(unusedreg, 0);
						mod(unusedreg, i->r1);
						cpy(i->r1, unusedreg);
						
						regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
					}
					
					break;
					
				case LYRICALOPMULHUI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					mulhui(i->r1);
					
					break;
					
				case LYRICALOPDIVUI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					divui(i->r1);
					
					break;
					
				case LYRICALOPMODUI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					modui(i->r1);
					
					break;
					
				case LYRICALOPDIVUI2:
					
					if (i->r1 != i->r2) {
						
						ldi(i->r1, 0);
						divu(i->r1, i->r2);
						
					} else {
						// Look for an unused register.
						uint unusedreg = findunusedreg(RAX);
						
						// The compiler should have insured
						// that there are enough unused
						// registers available.
						if (!unusedreg) throwerror();
						
						regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
						
						ldi(unusedreg, 0);
						divu(unusedreg, i->r1);
						cpy(i->r1, unusedreg);
						
						regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
					}
					
					break;
					
				case LYRICALOPMODUI2:
					
					if (i->r1 != i->r2) {
						
						ldi(i->r1, 0);
						modu(i->r1, i->r2);
						
					} else {
						// Look for an unused register.
						uint unusedreg = findunusedreg(RAX);
						
						// The compiler should have insured
						// that there are enough unused
						// registers available.
						if (!unusedreg) throwerror();
						
						regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
						
						ldi(unusedreg, 0);
						modu(unusedreg, i->r1);
						cpy(i->r1, unusedreg);
						
						regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
					}
					
					break;
					
				case LYRICALOPAND:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						and(i->r1, i->r3);
						
					} else {
						
						and(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPANDI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					andi(i->r1);
					
					break;
					
				case LYRICALOPOR:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						or(i->r1, i->r3);
						
					} else {
						
						or(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPORI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					ori(i->r1);
					
					break;
					
				case LYRICALOPXOR:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						xor(i->r1, i->r3);
						
					} else {
						
						xor(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPXORI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					xori(i->r1);
					
					break;
					
				case LYRICALOPNOT:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					not(i->r1);
					
					break;
					
				case LYRICALOPCPY:
					
					cpy(i->r1, i->r2);
					
					break;
					
				case LYRICALOPSLL:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						sll(i->r1, i->r3);
						
					} else if (i->r1 != i->r2) {
						// Look for an unused register.
						uint unusedreg = findunusedreg(RCX);
						
						// The compiler should have insured
						// that there are enough unused
						// registers available.
						if (!unusedreg) throwerror();
						
						regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
						
						cpy(unusedreg, i->r1);
						cpy(i->r1, i->r2);
						sll(i->r1, unusedreg);
						
						regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
						
					} else {
						
						sll(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPSLLI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					slli(i->r1);
					
					break;
					
				case LYRICALOPSLLI2:
					
					if (i->r1 != i->r2) {
						
						ldi(i->r1, 0);
						sll(i->r1, i->r2);
						
					} else {
						// Look for an unused register.
						uint unusedreg = findunusedreg(0);
						
						// The compiler should have insured
						// that there are enough unused
						// registers available.
						if (!unusedreg) throwerror();
						
						regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
						
						ldi(unusedreg, 0);
						sll(unusedreg, i->r1);
						cpy(i->r1, unusedreg);
						
						regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
					}
					
					break;
					
				case LYRICALOPSRL:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						srl(i->r1, i->r3);
						
					} else if (i->r1 != i->r2) {
						// Look for an unused register.
						uint unusedreg = findunusedreg(RCX);
						
						// The compiler should have insured
						// that there are enough unused
						// registers available.
						if (!unusedreg) throwerror();
						
						regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
						
						cpy(unusedreg, i->r1);
						cpy(i->r1, i->r2);
						srl(i->r1, unusedreg);
						
						regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
						
					} else {
						
						srl(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPSRLI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					srli(i->r1);
					
					break;
					
				case LYRICALOPSRLI2:
					
					if (i->r1 != i->r2) {
						
						ldi(i->r1, 0);
						srl(i->r1, i->r2);
						
					} else {
						// Look for an unused register.
						uint unusedreg = findunusedreg(0);
						
						// The compiler should have insured
						// that there are enough unused
						// registers available.
						if (!unusedreg) throwerror();
						
						regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
						
						ldi(unusedreg, 0);
						srl(unusedreg, i->r1);
						cpy(i->r1, unusedreg);
						
						regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
					}
					
					break;
					
				case LYRICALOPSRA:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						sra(i->r1, i->r3);
						
					} else if (i->r1 != i->r2) {
						// Look for an unused register.
						uint unusedreg = findunusedreg(RCX);
						
						// The compiler should have insured
						// that there are enough unused
						// registers available.
						if (!unusedreg) throwerror();
						
						regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
						
						cpy(unusedreg, i->r1);
						cpy(i->r1, i->r2);
						sra(i->r1, unusedreg);
						
						regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
						
					} else {
						
						sra(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPSRAI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					srai(i->r1);
					
					break;
					
				case LYRICALOPSRAI2:
					
					if (i->r1 != i->r2) {
						
						ldi(i->r1, 0);
						sra(i->r1, i->r2);
						
					} else {
						// Look for an unused register.
						uint unusedreg = findunusedreg(0);
						
						// The compiler should have insured
						// that there are enough unused
						// registers available.
						if (!unusedreg) throwerror();
						
						regsinusetmp[unusedreg] = 1; // Mark unused register as temporarily used.
						
						ldi(unusedreg, 0);
						sra(unusedreg, i->r1);
						cpy(i->r1, unusedreg);
						
						regsinusetmp[unusedreg] = 0; // Unmark register as temporarily used.
					}
					
					break;
					
				case LYRICALOPZXT:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					// A single lyricalimmval
					// of type LYRICALIMMVALUE is
					// guaranteed to have been used.
					zxt(i->r1, *(u8*)&i->imm->n);
					
					break;
					
				case LYRICALOPSXT:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					// A single lyricalimmval
					// of type LYRICALIMMVALUE is
					// guaranteed to have been used.
					sxt(i->r1, *(u8*)&i->imm->n);
					
					break;
					
				case LYRICALOPSEQ:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						seq(i->r1, i->r3);
						
					} else {
						
						seq(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPSNE:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						sne(i->r1, i->r3);
						
					} else {
						
						sne(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPSEQI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					seqi(i->r1);
					
					break;
					
				case LYRICALOPSNEI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					snei(i->r1);
					
					break;
					
				case LYRICALOPSLT:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						slt(i->r1,i->r3);
						
					} else if (i->r1 != i->r2) {
						
						sgt(i->r3, i->r2);
						
					} else {
						
						slt(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPSLTE:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						slte(i->r1, i->r3);
						
					} else if (i->r1 != i->r2) {
						
						sgte(i->r3, i->r2);
						
					} else {
						
						slte(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPSLTU:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						sltu(i->r1, i->r3);
						
					} else if (i->r1 != i->r2) {
						
						sgtu(i->r3, i->r2);
						
					} else {
						
						sltu(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPSLTEU:
					
					if (i->r1 != i->r3) {
						
						if (i->r1 != i->r2) cpy(i->r1, i->r2);
						
						slteu(i->r1, i->r3);
						
					} else if (i->r1 != i->r2) {
						
						sgteu(i->r3, i->r2);
						
					} else {
						
						slteu(i->r1, i->r2);
					}
					
					break;
					
				case LYRICALOPSLTI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					slti(i->r1);
					
					break;
					
				case LYRICALOPSLTEI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					sltei(i->r1);
					
					break;
					
				case LYRICALOPSLTUI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					sltui(i->r1);
					
					break;
					
				case LYRICALOPSLTEUI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					slteui(i->r1);
					
					break;
					
				case LYRICALOPSGTI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					sgti(i->r1);
					
					break;
					
				case LYRICALOPSGTEI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					sgtei(i->r1);
					
					break;
					
				case LYRICALOPSGTUI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					sgtui(i->r1);
					
					break;
					
				case LYRICALOPSGTEUI:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					sgteui(i->r1);
					
					break;
					
				case LYRICALOPSZ:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					sz(i->r1);
					
					break;
					
				case LYRICALOPSNZ:
					
					if (i->r1 != i->r2) cpy(i->r1, i->r2);
					
					snz(i->r1);
					
					break;
					
				case LYRICALOPJEQ:
					
					jeq(i->r1, i->r2);
					
					break;
					
				case LYRICALOPJEQI:
					
					jeqi(i->r1, i->r2);
					
					break;
					
				case LYRICALOPJEQR:
					
					jeqr(i->r1, i->r2, i->r3);
					
					break;
					
				case LYRICALOPJNE:
					
					jne(i->r1, i->r2);
					
					break;
					
				case LYRICALOPJNEI:
					
					jnei(i->r1, i->r2);
					
					break;
					
				case LYRICALOPJNER:
					
					jner(i->r1, i->r2, i->r3);
					
					break;
					
				case LYRICALOPJLT:
					
					jlt(i->r1, i->r2);
					
					break;
					
				case LYRICALOPJLTI:
					
					jlti(i->r1, i->r2);
					
					break;
					
				case LYRICALOPJLTR:
					
					jltr(i->r1, i->r2, i->r3);
					
					break;
					
				case LYRICALOPJLTE:
					
					jlte(i->r1, i->r2);
					
					break;
					
				case LYRICALOPJLTEI:
					
					jltei(i->r1, i->r2);
					
					break;
					
				case LYRICALOPJLTER:
					
					jlter(i->r1, i->r2, i->r3);
					
					break;
					
				case LYRICALOPJLTU:
					
					jltu(i->r1, i->r2);
					
					break;
					
				case LYRICALOPJLTUI:
					
					jltui(i->r1, i->r2);
					
					break;
					
				case LYRICALOPJLTUR:
					
					jltur(i->r1, i->r2, i->r3);
					
					break;
					
				case LYRICALOPJLTEU:
					
					jlteu(i->r1, i->r2);
					
					break;
					
				case LYRICALOPJLTEUI:
					
					jlteui(i->r1, i->r2);
					
					break;
					
				case LYRICALOPJLTEUR:
					
					jlteur(i->r1, i->r2, i->r3);
					
					break;
					
				case LYRICALOPJZ:
					
					jz(i->r1);
					
					break;
					
				case LYRICALOPJZI:
					
					jzi(i->r1);
					
					break;
					
				case LYRICALOPJZR:
					
					jzr(i->r1, i->r2);
					
					break;
					
				case LYRICALOPJNZ:
					
					jnz(i->r1);
					
					break;
					
				case LYRICALOPJNZI:
					
					jnzi(i->r1);
					
					break;
					
				case LYRICALOPJNZR:
					
					jnzr(i->r1, i->r2);
					
					break;
					
				case LYRICALOPJ:
					
					j();
					
					break;
					
				case LYRICALOPJI:
					
					ji();
					
					break;
					
				case LYRICALOPJR:
					
					jr(i->r1);
					
					break;
					
				case LYRICALOPJL:
					
					jl(i->r1);
					
					break;
					
				case LYRICALOPJLI:
					
					jli(i->r1);
					
					break;
					
				case LYRICALOPJLR:
					
					jlr(i->r1, i->r2);
					
					break;
					
				case LYRICALOPJPUSH:
					
					jpush();
					
					break;
					
				case LYRICALOPJPUSHI:
					
					jpushi();
					
					break;
					
				case LYRICALOPJPUSHR:
					
					jpushr(i->r1);
					
					break;
					
				case LYRICALOPJPOP:
					
					jpop();
					
					break;
					
				case LYRICALOPAFIP:
					
					afip(i->r1);
					
					break;
					
				case LYRICALOPLI:
					
					ldi(i->r1, 0);
					
					break;
					
				case LYRICALOPLD8:
					
					ld8(i->r1, i->r2);
					
					break;
					
				case LYRICALOPLD8R:
					
					ld8r(i->r1, i->r2);
					
					break;
					
				case LYRICALOPLD8I:
					
					ld8i(i->r1);
					
					break;
					
				case LYRICALOPLD16:
					
					ld16(i->r1, i->r2);
					
					break;
					
				case LYRICALOPLD16R:
					
					ld16r(i->r1, i->r2);
					
					break;
					
				case LYRICALOPLD16I:
					
					ld16i(i->r1);
					
					break;
					
				case LYRICALOPLD32:
					
					ld32(i->r1, i->r2);
					
					break;
					
				case LYRICALOPLD32R:
					
					ld32r(i->r1, i->r2);
					
					break;
					
				case LYRICALOPLD32I:
					
					ld32i(i->r1);
					
					break;
					
				case LYRICALOPLD64:
					
					ld64(i->r1, i->r2);
					
					break;
					
				case LYRICALOPLD64R:
					
					ld64r(i->r1, i->r2);
					
					break;
					
				case LYRICALOPLD64I:
					
					ld64i(i->r1);
					
					break;
					
				case LYRICALOPST8:
					
					st8(i->r1, i->r2);
					
					break;
					
				case LYRICALOPST8R:
					
					st8r(i->r1, i->r2);
					
					break;
					
				case LYRICALOPST8I:
					
					st8i(i->r1);
					
					break;
					
				case LYRICALOPST16:
					
					st16(i->r1, i->r2);
					
					break;
					
				case LYRICALOPST16R:
					
					st16r(i->r1, i->r2);
					
					break;
					
				case LYRICALOPST16I:
					
					st16i(i->r1);
					
					break;
					
				case LYRICALOPST32:
					
					st32(i->r1, i->r2);
					
					break;
					
				case LYRICALOPST32R:
					
					st32r(i->r1, i->r2);
					
					break;
					
				case LYRICALOPST32I:
					
					st32i(i->r1);
					
					break;
					
				case LYRICALOPST64:
					
					st64(i->r1, i->r2);
					
					break;
					
				case LYRICALOPST64R:
					
					st64r(i->r1, i->r2);
					
					break;
					
				case LYRICALOPST64I:
					
					st64i(i->r1);
					
					break;
					
				case LYRICALOPLDST8:
					
					ldst8(i->r1, i->r2);
					
					break;
					
				case LYRICALOPLDST8R:
					
					ldst8r(i->r1, i->r2);
					
					break;
					
				case LYRICALOPLDST8I:
					
					ldst8i(i->r1);
					
					break;
					
				case LYRICALOPLDST16:
					
					ldst16(i->r1, i->r2);
					
					break;
					
				case LYRICALOPLDST16R:
					
					ldst16r(i->r1, i->r2);
					
					break;
					
				case LYRICALOPLDST16I:
					
					ldst16i(i->r1);
					
					break;
					
				case LYRICALOPLDST32:
					
					ldst32(i->r1, i->r2);
					
					break;
					
				case LYRICALOPLDST32R:
					
					ldst32r(i->r1, i->r2);
					
					break;
					
				case LYRICALOPLDST32I:
					
					ldst32i(i->r1);
					
					break;
					
				case LYRICALOPLDST64:
					
					ldst64(i->r1, i->r2);
					
					break;
					
				case LYRICALOPLDST64R:
					
					ldst64r(i->r1, i->r2);
					
					break;
					
				case LYRICALOPLDST64I:
					
					ldst64i(i->r1);
					
					break;
					
				case LYRICALOPMEM8CPY:
					
					mem8cpy(i->r1, i->r2, i->r3);
					
					break;
					
				case LYRICALOPMEM8CPYI: {
					// Look for an unused register.
					uint unusedreg = findunusedreg(RCX);
					
					// The compiler should have insured
					// that there are enough unused
					// registers available.
					if (!unusedreg) throwerror();
					
					// Mark unused register as temporarily used.
					regsinusetmp[unusedreg] = 1;
					
					ldi(unusedreg, 0);
					
					mem8cpy(i->r1, i->r2, unusedreg);
					
					// Unmark register as temporarily used.
					regsinusetmp[unusedreg] = 0;
				
					break;
				}
				
				case LYRICALOPMEM8CPY2:
					
					mem8cpy2(i->r1, i->r2, i->r3);
					
					break;
					
				case LYRICALOPMEM8CPYI2: {
					// Look for an unused register.
					uint unusedreg = findunusedreg(RCX);
					
					// The compiler should have insured
					// that there are enough unused
					// registers available.
					if (!unusedreg) throwerror();
					
					// Mark unused register as temporarily used.
					regsinusetmp[unusedreg] = 1;
					
					ldi(unusedreg, 0);
					
					mem8cpy2(i->r1, i->r2, unusedreg);
					
					// Unmark register as temporarily used.
					regsinusetmp[unusedreg] = 0;
				
					break;
				}
				
				case LYRICALOPMEM16CPY:
					
					mem16cpy(i->r1, i->r2, i->r3);
					
					break;
					
				case LYRICALOPMEM16CPYI: {
					// Look for an unused register.
					uint unusedreg = findunusedreg(RCX);
					
					// The compiler should have insured
					// that there are enough unused
					// registers available.
					if (!unusedreg) throwerror();
					
					// Mark unused register as temporarily used.
					regsinusetmp[unusedreg] = 1;
					
					ldi(unusedreg, 0);
					
					mem16cpy(i->r1, i->r2, unusedreg);
					
					// Unmark register as temporarily used.
					regsinusetmp[unusedreg] = 0;
				
					break;
				}
				
				case LYRICALOPMEM16CPY2:
					
					mem16cpy2(i->r1, i->r2, i->r3);
					
					break;
					
				case LYRICALOPMEM16CPYI2: {
					// Look for an unused register.
					uint unusedreg = findunusedreg(RCX);
					
					// The compiler should have insured
					// that there are enough unused
					// registers available.
					if (!unusedreg) throwerror();
					
					// Mark unused register as temporarily used.
					regsinusetmp[unusedreg] = 1;
					
					ldi(unusedreg, 0);
					
					mem16cpy2(i->r1, i->r2, unusedreg);
					
					// Unmark register as temporarily used.
					regsinusetmp[unusedreg] = 0;
				
					break;
				}
				
				case LYRICALOPMEM64CPY:
					
					mem64cpy(i->r1, i->r2, i->r3);
					
					break;
					
				case LYRICALOPMEM64CPYI: {
					// Look for an unused register.
					uint unusedreg = findunusedreg(RCX);
					
					// The compiler should have insured
					// that there are enough unused
					// registers available.
					if (!unusedreg) throwerror();
					
					// Mark unused register as temporarily used.
					regsinusetmp[unusedreg] = 1;
					
					ldi(unusedreg, 0);
					
					mem64cpy(i->r1, i->r2, unusedreg);
					
					// Unmark register as temporarily used.
					regsinusetmp[unusedreg] = 0;
				
					break;
				}
				
				case LYRICALOPMEM64CPY2:
					
					mem64cpy2(i->r1, i->r2, i->r3);
					
					break;
					
				case LYRICALOPMEM64CPYI2: {
					// Look for an unused register.
					uint unusedreg = findunusedreg(RCX);
					
					// The compiler should have insured
					// that there are enough unused
					// registers available.
					if (!unusedreg) throwerror();
					
					// Mark unused register as temporarily used.
					regsinusetmp[unusedreg] = 1;
					
					ldi(unusedreg, 0);
					
					mem64cpy2(i->r1, i->r2, unusedreg);
					
					// Unmark register as temporarily used.
					regsinusetmp[unusedreg] = 0;
				
					break;
				}
				
				#if defined(LYRICALX64LINUX) && defined(__linux__)
				
				case LYRICALOPPAGEALLOC: {
					
					uint r1 = i->r1;
					uint r2 = i->r2;
					
					uint raxwassaved = 0;
					uint rdiwassaved = 0;
					uint rsiwassaved = 0;
					uint rdxwassaved = 0;
					uint rcxwassaved = 0;
					uint r8wassaved = 0;
					uint r9wassaved = 0;
					uint r10wassaved = 0;
					uint r11wassaved = 0;
					
					// The instruction syscall
					// destroy %RCX and %R11.
					// So %RCX and %R11 get saved if
					// they were used for that reason.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(RAX) && r1 != RAX) {
						savereg(RAX); raxwassaved = 1;
					} else regsinusetmp[RAX] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(RDI) && r1 != RDI) {
						savereg(RDI); rdiwassaved = 1;
					} else regsinusetmp[RDI] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(RSI) && r1 != RSI) {
						savereg(RSI); rsiwassaved = 1;
					} else regsinusetmp[RSI] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(RDX) && r1 != RDX) {
						savereg(RDX); rdxwassaved = 1;
					} else regsinusetmp[RDX] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(RCX) && r1 != RCX) {
						savereg(RCX); rcxwassaved = 1;
					} else regsinusetmp[RCX] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(R8) && r1 != R8) {
						savereg(R8); r8wassaved = 1;
					} else regsinusetmp[R8] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(R9) && r1 != R9) {
						savereg(R9); r9wassaved = 1;
					} else regsinusetmp[R9] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(R10) && r1 != R10) {
						savereg(R10); r10wassaved = 1;
					} else regsinusetmp[R10] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(R11) && r1 != R11) {
						savereg(R11); r11wassaved = 1;
					} else regsinusetmp[R11] = 1; // Mark unused register as temporarily used.
					
					// Generate the instructions
					// to use the syscall mmap():
					// void* mmap(void* addr, uint length, uint prot,
					//      uint flags, uint fd, uint offset);
					
					// Compute in RSI the byte count
					// equivalent of the page count.
					// This is done first in case r2
					// is any of the registers later
					// modified.
					if (r2 != RSI) cpy(RSI, r2);
					sllimm(RSI, clog2(PAGESIZE));
					
					loadimm(RAX, 9); // syscall mmap().
					loadimm(RDI, 0); // addr.
					loadimm(RDX, PROT_READ|PROT_WRITE); // prot.
					loadimm(R10, MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED); // flags.
					loadimm(R8, 0); // fd.
					loadimm(R9, 0); // offset.
					
					// Append the instruction SYSCALL.
					*arrayu8append1(&b->binary) = 0x0f;
					*arrayu8append1(&b->binary) = 0x05;
					
					if (r1 != RAX) cpy(r1, RAX);
					
					if (r11wassaved) {
						// Restore register value.
						restorereg(R11);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[R11] = 0;
					
					if (r10wassaved) {
						// Restore register value.
						restorereg(R10);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[R10] = 0;
					
					if (r9wassaved) {
						// Restore register value.
						restorereg(R9);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[R9] = 0;
					
					if (r8wassaved) {
						// Restore register value.
						restorereg(R8);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[R8] = 0;
					
					if (rcxwassaved) {
						// Restore register value.
						restorereg(RCX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RCX] = 0;
					
					if (rdxwassaved) {
						// Restore register value.
						restorereg(RDX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RDX] = 0;
					
					if (rsiwassaved) {
						// Restore register value.
						restorereg(RSI);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RSI] = 0;
					
					if (rdiwassaved) {
						// Restore register value.
						restorereg(RDI);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RDI] = 0;
					
					if (raxwassaved) {
						// Restore register value.
						restorereg(RAX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RAX] = 0;
					
					break;
				}
				
				case LYRICALOPPAGEALLOCI:
				case LYRICALOPSTACKPAGEALLOC: {
					
					uint r1 = i->r1;
					
					uint raxwassaved = 0;
					uint rdiwassaved = 0;
					uint rsiwassaved = 0;
					uint rdxwassaved = 0;
					uint rcxwassaved = 0;
					uint r8wassaved = 0;
					uint r9wassaved = 0;
					uint r10wassaved = 0;
					uint r11wassaved = 0;
					
					// The instruction syscall
					// destroy %RCX and %R11.
					// So %RCX and %R11 get saved if
					// they were used for that reason.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(RAX) && r1 != RAX) {
						savereg(RAX); raxwassaved = 1;
					} else regsinusetmp[RAX] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(RDI) && r1 != RDI) {
						savereg(RDI); rdiwassaved = 1;
					} else regsinusetmp[RDI] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(RSI) && r1 != RSI) {
						savereg(RSI); rsiwassaved = 1;
					} else regsinusetmp[RSI] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(RDX) && r1 != RDX) {
						savereg(RDX); rdxwassaved = 1;
					} else regsinusetmp[RDX] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(RCX) && r1 != RCX) {
						savereg(RCX); rcxwassaved = 1;
					} else regsinusetmp[RCX] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(R8) && r1 != R8) {
						savereg(R8); r8wassaved = 1;
					} else regsinusetmp[R8] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(R9) && r1 != R9) {
						savereg(R9); r9wassaved = 1;
					} else regsinusetmp[R9] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(R10) && r1 != R10) {
						savereg(R10); r10wassaved = 1;
					} else regsinusetmp[R10] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(R11) && r1 != R11) {
						savereg(R11); r11wassaved = 1;
					} else regsinusetmp[R11] = 1; // Mark unused register as temporarily used.
					
					// Generate the instructions
					// to use the syscall mmap():
					// void* mmap(void* addr, uint length, uint prot,
					//      uint flags, uint fd, uint offset);
					
					loadimm(RAX, 9); // syscall mmap().
					loadimm(RDI, 0); // addr.
					loadimm(RSI, (i->op == LYRICALOPSTACKPAGEALLOC) ? PAGESIZE : ((*(u64*)&i->imm->n)*PAGESIZE)); // length.
					loadimm(RDX, PROT_READ|PROT_WRITE); // prot.
					loadimm(R10, MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED|((i->op == LYRICALOPSTACKPAGEALLOC) ? MAP_STACK : 0)); // flags.
					loadimm(R8, 0); // fd.
					loadimm(R9, 0); // offset.
					
					// Append the instruction SYSCALL.
					*arrayu8append1(&b->binary) = 0x0f;
					*arrayu8append1(&b->binary) = 0x05;
					
					if (r1 != RAX) cpy(r1, RAX);
					
					if (r11wassaved) {
						// Restore register value.
						restorereg(R11);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[R11] = 0;
					
					if (r10wassaved) {
						// Restore register value.
						restorereg(R10);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[R10] = 0;
					
					if (r9wassaved) {
						// Restore register value.
						restorereg(R9);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[R9] = 0;
					
					if (r8wassaved) {
						// Restore register value.
						restorereg(R8);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[R8] = 0;
					
					if (rcxwassaved) {
						// Restore register value.
						restorereg(RCX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RCX] = 0;
					
					if (rdxwassaved) {
						// Restore register value.
						restorereg(RDX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RDX] = 0;
					
					if (rsiwassaved) {
						// Restore register value.
						restorereg(RSI);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RSI] = 0;
					
					if (rdiwassaved) {
						// Restore register value.
						restorereg(RDI);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RDI] = 0;
					
					if (raxwassaved) {
						// Restore register value.
						restorereg(RAX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RAX] = 0;
					
					break;
				}
				
				case LYRICALOPPAGEFREE: {
					
					uint r1 = i->r1;
					uint r2 = i->r2;
					
					uint r1r2wasxchged = 0;
					
					if (r1 != RDI && r2 == RDI) {
						// If I get here, I xchg r1 and r2
						// so as to generate the least
						// amount of intructions while
						// setting up the page address
						// in RDI.
						
						xchg(r1, r2);
						
						// r2 is made to refer
						// to the register that
						// contain its value.
						r1 ^= r2; r2 ^= r1; r1 ^= r2;
						// If r1 was RSI, then
						// the instruction cpy(RSI, r2)
						// will not get generated.
						// so exchanging r1 and r2
						// values here has the potential
						// of saving instructions.
						
						r1r2wasxchged = 1;
					}
					
					uint raxwassaved = 0;
					uint rdiwassaved = 0;
					uint rsiwassaved = 0;
					uint rcxwassaved = 0;
					uint r11wassaved = 0;
					
					// The instruction syscall
					// destroy %RCX and %R11.
					// So %RCX and %R11 get saved if
					// they were used for that reason.
					
					// Save register value
					// if it is being used.
					if (isreginuse(RAX)) {
						savereg(RAX); raxwassaved = 1;
					} else regsinusetmp[RAX] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used.
					if (isreginuse(RDI)) {
						savereg(RDI); rdiwassaved = 1;
					} else regsinusetmp[RDI] = 1; // Mark unused register as temporarily used.
					
					if (r1 != RDI) cpy(RDI, r1);
					
					// RoundDown to pagesize 0x1000.
					andimm(RDI, -0x1000);
					
					// Save register value
					// if it is being used.
					if (isreginuse(RSI)) {
						savereg(RSI); rsiwassaved = 1;
					} else regsinusetmp[RSI] = 1; // Mark unused register as temporarily used.
					
					// Compute in RSI the byte count
					// equivalent of the page count.
					if (r2 != RSI) cpy(RSI, r2);
					sllimm(RSI, clog2(PAGESIZE));
					
					// Save register value
					// if it is being used.
					if (isreginuse(RCX)) {
						savereg(RCX); rcxwassaved = 1;
					} else regsinusetmp[RCX] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used.
					if (isreginuse(R11)) {
						savereg(R11); r11wassaved = 1;
					} else regsinusetmp[R11] = 1; // Mark unused register as temporarily used.
					
					// Generate the instructions
					// to use the syscall munmap():
					// uint munmap(void* addr, uint length);
					
					loadimm(RAX, 11); // syscall munmap().
					
					// Append the instruction SYSCALL.
					*arrayu8append1(&b->binary) = 0x0f;
					*arrayu8append1(&b->binary) = 0x05;
					
					if (r11wassaved) {
						// Restore register value.
						restorereg(R11);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[R11] = 0;
					
					if (rcxwassaved) {
						// Restore register value.
						restorereg(RCX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RCX] = 0;
					
					if (rsiwassaved) {
						// Restore register value.
						restorereg(RSI);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RSI] = 0;
					
					if (rdiwassaved) {
						// Restore register value.
						restorereg(RDI);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RDI] = 0;
					
					if (raxwassaved) {
						// Restore register value.
						restorereg(RAX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RAX] = 0;
					
					if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
					
					break;
				}
				
				case LYRICALOPPAGEFREEI:
				case LYRICALOPSTACKPAGEFREE: {
					
					uint r1 = i->r1;
					
					uint raxwassaved = 0;
					uint rdiwassaved = 0;
					uint rsiwassaved = 0;
					uint rcxwassaved = 0;
					uint r11wassaved = 0;
					
					// The instruction syscall
					// destroy %RCX and %R11.
					// So %RCX and %R11 get saved if
					// they were used for that reason.
					
					// Save register value
					// if it is being used.
					if (isreginuse(RAX)) {
						savereg(RAX); raxwassaved = 1;
					} else regsinusetmp[RAX] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used.
					if (isreginuse(RDI)) {
						savereg(RDI); rdiwassaved = 1;
					} else regsinusetmp[RDI] = 1; // Mark unused register as temporarily used.
					
					if (r1 != RDI) cpy(RDI, r1);
					
					// RoundDown to pagesize 0x1000.
					andimm(RDI, -0x1000);
					
					// Save register value
					// if it is being used.
					if (isreginuse(RSI)) {
						savereg(RSI); rsiwassaved = 1;
					} else regsinusetmp[RSI] = 1; // Mark unused register as temporarily used.
					
					// Compute in RSI the byte count
					// equivalent of the page count.
					loadimm(RSI, (i->op == LYRICALOPSTACKPAGEFREE) ? PAGESIZE : ((*(u64*)&i->imm->n)*PAGESIZE));
					
					// Save register value
					// if it is being used.
					if (isreginuse(RCX)) {
						savereg(RCX); rcxwassaved = 1;
					} else regsinusetmp[RCX] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used.
					if (isreginuse(R11)) {
						savereg(R11); r11wassaved = 1;
					} else regsinusetmp[R11] = 1; // Mark unused register as temporarily used.
					
					// Generate the instructions
					// to use the syscall munmap():
					// uint munmap(void* addr, uint length);
					
					loadimm(RAX, 11); // syscall munmap().
					
					// Append the instruction SYSCALL.
					*arrayu8append1(&b->binary) = 0x0f;
					*arrayu8append1(&b->binary) = 0x05;
					
					if (r11wassaved) {
						// Restore register value.
						restorereg(R11);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[R11] = 0;
					
					if (rcxwassaved) {
						// Restore register value.
						restorereg(RCX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RCX] = 0;
					
					if (rsiwassaved) {
						// Restore register value.
						restorereg(RSI);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RSI] = 0;
					
					if (rdiwassaved) {
						// Restore register value.
						restorereg(RDI);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RDI] = 0;
					
					if (raxwassaved) {
						// Restore register value.
						restorereg(RAX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[RAX] = 0;
					
					break;
				}
				
				#else
				
				case LYRICALOPPAGEALLOC: {
					
					uint r1 = i->r1;
					
					pushusedregs(r1);
					
					uint r2 = i->r2;
					
					uint raxwassetused = 0;
					uint rdiwassetused = 0;
					uint rsiwassetused = 0;
					uint rdxwassetused = 0;
					uint rcxwassetused = 0;
					uint r8wassetused = 0;
					uint r9wassetused = 0;
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RAX)) {
						regsinusetmp[RAX] = 1;
						raxwassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RDI)) {
						regsinusetmp[RDI] = 1;
						rdiwassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RSI)) {
						regsinusetmp[RSI] = 1;
						rsiwassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RDX)) {
						regsinusetmp[RDX] = 1;
						rdxwassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RCX)) {
						regsinusetmp[RCX] = 1;
						rcxwassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(R8)) {
						regsinusetmp[R8] = 1;
						r8wassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(R9)) {
						regsinusetmp[R9] = 1;
						r9wassetused = 1;
					}
					
					// The calling convention of
					// the lyrical syscall function
					// is sysv_abi.
					
					// mmap() lyrical syscall
					// arguments get passed as follow:
					// offset:      0 (Pushed in the stack)
					// fd:          0 (R9)
					// flags:       MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED (R8)
					// prot:        PROT_READ|PROT_WRITE (RCX)
					// length:      r2*PAGESIZE (RDX)
					// start:       0 (RSI)
					// LYRICALSYSCALLMMAP (RDI)
					
					// Compute in RDX the byte count
					// equivalent of the page count.
					if (r2 != RDX) cpy(RDX, r2);
					sllimm(RDX, clog2(PAGESIZE));
					
					loadimm(RDI, LYRICALSYSCALLMMAP);
					xor(RSI, RSI);
					loadimm(RCX, PROT_READ|PROT_WRITE);
					loadimm(R8, MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED);
					xor(R9, R9);
					pushi(0);
					
					// Generate instructions to retrieve
					// the lyrical syscall function address
					// and store it in RAX.
					afip2(RAX);
					ld64r(RAX, RAX);
					
					jpushr(RAX);
					
					// Retrieve result from register RAX.
					if (r1 != RAX) cpy(r1, RAX);
					
					// I restore the stack pointer register,
					// incrementing it by the byte count
					// of all the arguments pushed.
					
					// Specification from the Intel manual.
					// REX.W 83 /0 ib     ADD r/m64, imm8         #Add sign-extended imm8 to r/m64.
					
					// Append the opcode byte.
					*arrayu8append1(&b->binary) = 0x48;
					*arrayu8append1(&b->binary) = 0x83;
					
					// Append the ModR/M byte.
					// From the most significant bit to the least significant bit.
					// MOD == 0b11;
					// REG == op[2:0]; #Only the 3 least significant bits matters.
					// R/M == rm[2:0]; #Only the 3 least significant bits matters.
					*arrayu8append1(&b->binary) = ((0b11<<6)|lookupreg(RSP));
					
					// Append the 8bits immediate value.
					*arrayu8append1(&b->binary) = (1*sizeof(u64));
					
					// Unmark register as temporarily used if it was marked.
					if (r9wassetused) regsinusetmp[R9] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (r8wassetused) regsinusetmp[R8] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (rcxwassetused) regsinusetmp[RCX] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (rdxwassetused) regsinusetmp[RDX] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (rsiwassetused) regsinusetmp[RSI] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (rdiwassetused) regsinusetmp[RDI] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (raxwassetused) regsinusetmp[RAX] = 0;
					
					popusedregs(r1);
					
					break;
				}
				
				case LYRICALOPPAGEALLOCI:
				case LYRICALOPSTACKPAGEALLOC: {
					
					uint r1 = i->r1;
					
					pushusedregs(r1);
					
					uint raxwassetused = 0;
					uint rdiwassetused = 0;
					uint rsiwassetused = 0;
					uint rdxwassetused = 0;
					uint rcxwassetused = 0;
					uint r8wassetused = 0;
					uint r9wassetused = 0;
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RAX)) {
						regsinusetmp[RAX] = 1;
						raxwassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RDI)) {
						regsinusetmp[RDI] = 1;
						rdiwassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RSI)) {
						regsinusetmp[RSI] = 1;
						rsiwassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RDX)) {
						regsinusetmp[RDX] = 1;
						rdxwassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RCX)) {
						regsinusetmp[RCX] = 1;
						rcxwassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(R8)) {
						regsinusetmp[R8] = 1;
						r8wassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(R9)) {
						regsinusetmp[R9] = 1;
						r9wassetused = 1;
					}
					
					// The calling convention of
					// the lyrical syscall function
					// is sysv_abi.
					
					// mmap() lyrical syscall
					// arguments get passed as follow:
					// offset:      0 (Pushed in the stack)
					// fd:          0 (R9)
					// flags:       (MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED|((i->op == LYRICALOPSTACKPAGEALLOC) ? MAP_STACK : 0)) (R8)
					// prot:        PROT_READ|PROT_WRITE (RCX)
					// length:      ((i->op == LYRICALOPSTACKPAGEALLOC) ? PAGESIZE : (i->imm->n*PAGESIZE)) (RDX)
					// start:       0 (RSI)
					// LYRICALSYSCALLMMAP (RDI)
					
					// Compute in RDX the byte count
					// equivalent of the page count.
					if (i->op == LYRICALOPSTACKPAGEALLOC)
						loadimm(RDX, PAGESIZE);
					else loadimm(RDX, (*(u64*)&i->imm->n)*PAGESIZE);
					sllimm(RDX, clog2(PAGESIZE));
					
					loadimm(RDI, LYRICALSYSCALLMMAP);
					xor(RSI, RSI);
					loadimm(RCX, PROT_READ|PROT_WRITE);
					loadimm(R8, MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED|((i->op == LYRICALOPSTACKPAGEALLOC) ? MAP_STACK : 0));
					xor(R9, R9);
					pushi(0);
					
					// Generate instructions to retrieve
					// the lyrical syscall function address
					// and store it in RAX.
					afip2(RAX);
					ld64r(RAX, RAX);
					
					jpushr(RAX);
					
					// Retrieve result from register RAX.
					if (r1 != RAX) cpy(r1, RAX);
					
					// I restore the stack pointer register,
					// incrementing it by the byte count
					// of all the arguments pushed.
					
					// Specification from the Intel manual.
					// 83 /0 ib     ADD r/m64, imm8         #Add sign-extended imm8 to r/m64.
					
					// Append the opcode byte.
					*arrayu8append1(&b->binary) = 0x48;
					*arrayu8append1(&b->binary) = 0x83;
					
					// Append the ModR/M byte.
					// From the most significant bit to the least significant bit.
					// MOD == 0b11;
					// REG == op[2:0]; #Only the 3 least significant bits matters.
					// R/M == rm[2:0]; #Only the 3 least significant bits matters.
					*arrayu8append1(&b->binary) = ((0b11<<6)|lookupreg(RSP));
					
					// Append the 8bits immediate value.
					*arrayu8append1(&b->binary) = (1*sizeof(u64));
					
					// Unmark register as temporarily used if it was marked.
					if (r9wassetused) regsinusetmp[R9] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (r8wassetused) regsinusetmp[R8] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (rcxwassetused) regsinusetmp[RCX] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (rdxwassetused) regsinusetmp[RDX] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (rsiwassetused) regsinusetmp[RSI] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (rdiwassetused) regsinusetmp[RDI] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (raxwassetused) regsinusetmp[RAX] = 0;
					
					popusedregs(r1);
					
					break;
				}
				
				case LYRICALOPPAGEFREE: {
					
					pushusedregs(0);
					
					uint r1 = i->r1;
					
					uint r2 = i->r2;
					
					uint raxwassetused = 0;
					uint rdiwassetused = 0;
					uint rsiwassetused = 0;
					uint rdxwassetused = 0;
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RAX)) {
						regsinusetmp[RAX] = 1;
						raxwassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RDI)) {
						regsinusetmp[RDI] = 1;
						rdiwassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RSI)) {
						regsinusetmp[RSI] = 1;
						rsiwassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RDX)) {
						regsinusetmp[RDX] = 1;
						rdxwassetused = 1;
					}
					
					// The calling convention of
					// the lyrical syscall function
					// is sysv_abi.
					
					// munmap() lyrical syscall
					// arguments get passed as follow:
					// length:	r2 (RDX)
					// addr:	r1 (RSI)
					// LYRICALSYSCALLMUNMAP (RDI)
					
					if (r1 != RSI) { // Evaluated first in case r1 is RDX or RDI.
						
						if (r2 == RSI) cpy(RDX, r2);
						
						cpy(RSI, r1);
					}
					
					// Compute in RDX the byte count
					// equivalent of the page count.
					if (r2 != RDX) cpy(RDX, r2);
					sllimm(RDX, clog2(PAGESIZE));
					
					loadimm(RDI, LYRICALSYSCALLMUNMAP);
					
					// Generate instructions to retrieve
					// the lyrical syscall function address
					// and store it in RAX.
					afip2(RAX);
					ld64r(RAX, RAX);
					
					jpushr(RAX);
					
					// Unmark register as temporarily used if it was marked.
					if (rdxwassetused) regsinusetmp[RDX] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (rsiwassetused) regsinusetmp[RSI] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (rdiwassetused) regsinusetmp[RDI] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (raxwassetused) regsinusetmp[RAX] = 0;
					
					popusedregs(0);
					
					break;
				}
				
				case LYRICALOPPAGEFREEI:
				case LYRICALOPSTACKPAGEFREE: {
					
					pushusedregs(0);
					
					uint r1 = i->r1;
					
					uint raxwassetused = 0;
					uint rdiwassetused = 0;
					uint rsiwassetused = 0;
					uint rdxwassetused = 0;
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RAX)) {
						regsinusetmp[RAX] = 1;
						raxwassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RDI)) {
						regsinusetmp[RDI] = 1;
						rdiwassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RSI)) {
						regsinusetmp[RSI] = 1;
						rsiwassetused = 1;
					}
					
					// Mark register as temporarily used if unused.
					if (!isreginuse(RDX)) {
						regsinusetmp[RDX] = 1;
						rdxwassetused = 1;
					}
					
					// The calling convention of
					// the lyrical syscall function
					// is sysv_abi.
					
					// munmap() lyrical syscall
					// arguments get passed as follow:
					// length:	((i->op == LYRICALOPSTACKPAGEFREE) ? PAGESIZE : (i->imm->n*PAGESIZE)) (RDX)
					// addr:	r1 (RSI)
					// LYRICALSYSCALLMUNMAP (RDI)
					
					if (r1 != RSI) cpy(RSI, r1); // Evaluated first in case r1 is RDX or RDI.
					
					// Compute in RDX the byte count
					// equivalent of the page count.
					if (i->op == LYRICALOPSTACKPAGEFREE)
						loadimm(RDX, PAGESIZE);
					else loadimm(RDX, (*(u64*)&i->imm->n)*PAGESIZE);
					sllimm(RDX, clog2(PAGESIZE));
					
					loadimm(RDI, LYRICALSYSCALLMUNMAP);
					
					// Generate instructions to retrieve
					// the lyrical syscall function address
					// and store it in RAX.
					afip2(RAX);
					ld64r(RAX, RAX);
					
					jpushr(RAX);
					
					// Unmark register as temporarily used if it was marked.
					if (rdxwassetused) regsinusetmp[RDX] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (rsiwassetused) regsinusetmp[RSI] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (rdiwassetused) regsinusetmp[RDI] = 0;
					
					// Unmark register as temporarily used if it was marked.
					if (raxwassetused) regsinusetmp[RAX] = 0;
					
					popusedregs(0);
					
					break;
				}
				
				#endif
				
				case LYRICALOPMACHINECODE: {
					
					uint iopmachinecodesz = stringmmsz(i->opmachinecode);
					
					bytcpy(
						arrayu8append2(&b->binary, iopmachinecodesz),
						i->opmachinecode.ptr,
						iopmachinecodesz);
					
					break;
				}
				
				case LYRICALOPNOP:
				case LYRICALOPCOMMENT:
					
					// Nothing is done.
					
					break;
					
				default:
					
					// It is an error if I have
					// an unrecognized instruction.
					
					throwerror();
			}
			
			skipredo:;
			
			uint bbinarysz = arrayu8sz(b->binary);
			
			uint ibinsz = i->binsz;
			
			// Pad the binary executable equivalent
			// if it must have a specific size.
			if (ibinsz) {
				// Throw an error if the size of
				// the binary executable equivalent
				// is already larger than the size
				// that it must have.
				if (bbinarysz > ibinsz) throwerror();
				
				while (bbinarysz < ibinsz) {
					nop();
					bbinarysz += NOPINSTRSZ;
				}
			}
			
			b->binaryoffset = executableinstrsz;
			
			// Report in the lyricalcompileresult,
			// the offset of the binary generated
			// for the lyricalinstruction.
			i->dbginfo.binoffset = executableinstrsz;
			
			executableinstrsz += bbinarysz;
			
			if ((i = i->next) == f->i->next) break;
		}
		
	} while ((f = f->next) != compileresult.rootfunc);
	
	// When I get here f == compileresult.rootfunc;
	
	if (toredo.ptr) {
		bintreeempty(&toredo);
	}
	
	// Variable to return.
	lyricalbackendx64result* retvar = mmalloc(sizeof(lyricalbackendx64result));
	
	uint compileresultstringregionsz = arrayu8sz(compileresult.stringregion);
	
	// Constant strings region size
	// which will be aligned and used
	// to generate the executable binary.
	uint constantstringssz = compileresultstringregionsz;
	
	retvar->executableinstrsz = executableinstrsz;
	retvar->constantstringssz = constantstringssz;
	retvar->globalvarregionsz = compileresult.globalregionsz;
	
	if (flag&LYRICALBACKENDX64PAGEALIGNED) {
		
		executableinstrsz = ROUNDUPTOPOWEROFTWO(executableinstrsz, 0x1000); // Aligning to pagesize.
		
		constantstringssz = ROUNDUPTOPOWEROFTWO(constantstringssz, 0x1000); // Aligning to pagesize.
		
	} else {
		
		executableinstrsz = ROUNDUPTOPOWEROFTWO(executableinstrsz, 4); // Aligning to 4bytes.
		
		constantstringssz = ROUNDUPTOPOWEROFTWO(constantstringssz, 4); // Aligning to 4bytes.
	}
	
	// I allocate memory for
	// the executable binary
	// to return.
	retvar->execbin.ptr = mmalloc(executableinstrsz + constantstringssz);
	
	// Copy the constant strings to the result.
	bytcpy(
		retvar->execbin.ptr + executableinstrsz,
		compileresult.stringregion.ptr,
		compileresultstringregionsz);
	
	if (flag&LYRICALBACKENDX64COMPACTPAGEALIGNED) {
		
		executableinstrsz = ROUNDUPTOPOWEROFTWO(executableinstrsz, 0x1000); // Aligning to pagesize.
		
		constantstringssz = ROUNDUPTOPOWEROFTWO(constantstringssz, 0x1000); // Aligning to pagesize.
	}
	
	retvar->exportinfo = arrayu8null;
	retvar->importinfo = arrayu8null;
	
	// Variable used to save the linenumber of
	// the last debug information entry generated.
	// It is also used to determine whether
	// there was any debug information generated.
	uint saveddbginfolinenumber = 0;
	
	// Copy in the result,
	// the instructions to
	// execute resolving
	// the immediate values.
	do {
		// Note that f->i is always non-null,
		// so there is no need to check it.
		
		// Note that f->i point to the last
		// intruction that was generated and
		// f->i->next point to the first
		// instruction generated.
		i = f->i->next;
		
		// If the function is an export or import,
		// list it in the corresponding variable.
		if (f->toexport) {
			// The string null-terminating byte
			// will be included in the size.
			uint flinkingsignatureptrmmsz = mmsz(f->linkingsignature.ptr);
			
			bytcpy(
				arrayu8append2(&retvar->exportinfo, flinkingsignatureptrmmsz),
				f->linkingsignature.ptr,
				flinkingsignatureptrmmsz);
			
			// Write in little endian, the offset of
			// the function within the binary executable.
			
			uint n = ((backenddata*)i->backenddata)->binaryoffset;
			
			uint sa = 0; // Shift amount.
			do *arrayu8append1(&retvar->exportinfo) = (n>>sa); while ((sa += 8) < (sizeof(u64)*8));
			
		} else if (f->toimport) {
			// The string null-terminating byte
			// will be included in the size.
			uint flinkingsignatureptrmmsz = mmsz(f->linkingsignature.ptr);
			
			bytcpy(
				arrayu8append2(&retvar->importinfo, flinkingsignatureptrmmsz),
				f->linkingsignature.ptr,
				flinkingsignatureptrmmsz);
			
			// Write in little endian, the offset
			// within the string region from where
			// the import address is to be retrieved.
			
			uint n = (f->toimport -1);
			
			uint sa = 0; // Shift amount.
			do *arrayu8append1(&retvar->importinfo) = (n>>sa); while ((sa += 8) < (sizeof(u64)*8));
		}
		
		do {
			b = (backenddata*)i->backenddata;
			
			// If the instruction used
			// an immediate2 value,
			// I resolve it and write it
			// in the appropriate location
			// within its binary.
			if (b->isimm2used) {
				// This variable will hold the value
				// of the immediate2 value to write.
				u64 imm2value = b->imm2miscvalue;
				
				// Calculate the relative address
				// from the next instruction.
				// The sys field is located right after
				// the fields arg and env which are at
				// the top of the global variable region.
				imm2value += ((executableinstrsz + constantstringssz + (2*sizeof(u64))) -
					(b->binaryoffset + b->imm2fieldoffset +
						((b->isimm2used == IMM64) ? sizeof(u64) :
							(b->isimm2used == IMM32) ? sizeof(u32) :
							sizeof(u8))));
				
				if (b->isimm2used == IMM8) {
					// Check whether the absolute
					// value of the offset can
					// be encoded using 8bits.
					if ((((sint)imm2value < 0) ? -imm2value : imm2value) < (1 << 7)) {
						// An 8bits offset can be used.
						
						// Write the computed offset value
						// in the appropriate location within
						// the binary of the instruction.
						// An 8 bits value is written.
						
						*(u8*)(b->binary.ptr + b->imm2fieldoffset) = imm2value;
						
					} else {
						// An 8bits offset cannot be used.
						
						if ((((sint)imm2value < 0) ? -imm2value : imm2value) < (1 << 31)) {
							// A 32bits offset is what would fit.
							
							// I set the backenddata
							// so that a 32bits offset
							// get tried instead when
							// the lyricalinstruction
							// will be redone.
							b->isimm2used = IMM32;
							
						} else {
							// A 64bits offset is what would fit.
							
							// I set the backenddata
							// so that a 64bits offset
							// get tried instead when
							// the lyricalinstruction
							// will be redone.
							b->isimm2used = IMM64;
						}
						
						// I save the lyricalinstruction* to redo.
						bintreeadd(&toredo, (uint)i, i);
					}
					
				} else if (b->isimm2used == IMM32) {
					// Check whether the absolute
					// value of the offset can
					// be encoded using 16bits.
					if ((((sint)imm2value < 0) ? -imm2value : imm2value) < (1 << 31)) {
						// A 32bits offset can be used.
						
						// Write the computed offset value
						// in the appropriate location within
						// the binary of the instruction.
						// A 32 bits value is written.
						
						*(u32*)(b->binary.ptr + b->imm2fieldoffset) = imm2value;
						
					} else {
						// A 32bits offset cannot be used.
						// A 64bits offset is what would fit.
						
						// I set the backenddata
						// so that a 64bits offset
						// get tried instead when
						// the lyricalinstruction
						// will be redone.
						b->isimm2used = IMM64;
						
						// I save the lyricalinstruction* to redo.
						bintreeadd(&toredo, (uint)i, i);
					}
					
				} else if (b->isimm2used == IMM64) {
					// Write the computed offset value
					// in the appropriate location within
					// the binary of the instruction.
					// A 64 bits value is written.
					
					*(u64*)(b->binary.ptr + b->imm2fieldoffset) = imm2value;
					
				// This else case is not necessary.
				// It is only there to make sure
				// that b->isimm2used was valid.
				} else throwerror();
			}
			
			// If the instruction used
			// an immediate value,
			// I resolve it and write it
			// in the appropriate location
			// within its binary.
			if (b->isimmused) {
				// This variable will hold the value
				// of the immediate value to write.
				u64 immvalue = b->immmiscvalue;
				
				lyricalimmval* imm = i->imm;
				
				// When calculating relative addresses
				// from the next instruction, there is no
				// need to check whether there is a next
				// instruction because, there will always
				// be at least the instruction used
				// to return from the function.
				
				keepprocessing:
				
				switch (imm->type) {
					
					case LYRICALIMMVALUE:
						
						immvalue += *(u64*)&imm->n;
						
						break;
						
					case LYRICALIMMOFFSETTOINSTRUCTION:
						// Calculate the relative address
						// from the next instruction.
						immvalue += (((backenddata*)imm->i->backenddata)->binaryoffset -
							(b->binaryoffset + b->immfieldoffset +
								((b->isimmused == IMM64) ? sizeof(u64) :
									(b->isimmused == IMM32) ? sizeof(u32) :
									sizeof(u8))));
						
						break;
						
					case LYRICALIMMOFFSETTOFUNCTION:
						// Calculate the relative address
						// from the next instruction.
						// Note that the field i of
						// an lyricalfunction always
						// points to the last instruction
						// generated, so to get to
						// the first instruction generated,
						// I use the field next since
						// the instructions of an lyricalfunction
						// pointed by their field i
						// form a circular linkedlist.
						immvalue += (((backenddata*)imm->f->i->next->backenddata)->binaryoffset -
							(b->binaryoffset + b->immfieldoffset +
								((b->isimmused == IMM64) ? sizeof(u64) :
									(b->isimmused == IMM32) ? sizeof(u32) :
									sizeof(u8))));
						
						break;
						
					case LYRICALIMMOFFSETTOGLOBALREGION:
						// Calculate the relative address
						// from the next instruction.
						immvalue += ((executableinstrsz + constantstringssz) -
							(b->binaryoffset + b->immfieldoffset +
								((b->isimmused == IMM64) ? sizeof(u64) :
									(b->isimmused == IMM32) ? sizeof(u32) :
									sizeof(u8))));
						
						break;
						
					case LYRICALIMMOFFSETTOSTRINGREGION:
						// Calculate the relative address
						// from the next instruction.
						immvalue += (executableinstrsz -
							(b->binaryoffset + b->immfieldoffset +
								((b->isimmused == IMM64) ? sizeof(u64) :
									(b->isimmused == IMM32) ? sizeof(u32) :
									sizeof(u8))));
						
						break;
				}
				
				if (imm->next) {
					imm = imm->next;
					goto keepprocessing;
				}
				
				if (b->isimmused == IMM8) {
					// Check whether the absolute
					// value of the immediate can
					// be encoded using 8bits.
					if ((((sint)immvalue < 0) ? -immvalue : immvalue) < (1 << 7)) {
						// An 8bits immediate can be used.
						
						// Write the computed immediate value
						// in the appropriate location within
						// the binary of the instruction.
						// An 8 bits value is written.
						
						*(u8*)(b->binary.ptr + b->immfieldoffset) = immvalue;
						
					} else {
						// An 8bits immediate cannot be used.
						
						if ((((sint)immvalue < 0) ? -immvalue : immvalue) < (1 << 31)) {
							// A 32bits immediate is what would fit.
							
							// I set the backenddata
							// so that a 32bits immediate
							// get tried instead when
							// the lyricalinstruction
							// will be redone.
							b->isimmused = IMM32;
							
						} else {
							// A 64bits immediate is what would fit.
							
							// I set the backenddata
							// so that a 64bits immediate
							// get tried instead when
							// the lyricalinstruction
							// will be redone.
							b->isimmused = IMM64;
						}
						
						// I save the lyricalinstruction* to redo.
						bintreeadd(&toredo, (uint)i, i);
					}
					
				} else if (b->isimmused == IMM32) {
					// Check whether the absolute
					// value of the immediate can
					// be encoded using 16bits.
					if ((((sint)immvalue < 0) ? -immvalue : immvalue) < (1 << 31)) {
						// A 32bits immediate can be used.
						
						// Write the computed immediate value
						// in the appropriate location within
						// the binary of the instruction.
						// A 32 bits value is written.
						
						*(u32*)(b->binary.ptr + b->immfieldoffset) = immvalue;
						
					} else {
						// A 32bits immediate cannot be used.
						// A 64bits immediate is what would fit.
						
						// I set the backenddata
						// so that a 64bits immediate
						// get tried instead when
						// the lyricalinstruction
						// will be redone.
						b->isimmused = IMM64;
						
						// I save the lyricalinstruction* to redo.
						bintreeadd(&toredo, (uint)i, i);
					}
					
				} else if (b->isimmused == IMM64) {
					// Write the computed immediate value
					// in the appropriate location within
					// the binary of the instruction.
					// A 64 bits value is written.
					
					*(u64*)(b->binary.ptr + b->immfieldoffset) = immvalue;
					
				// This else case is not necessary.
				// It is only there to make sure
				// that b->isimmused was valid.
				} else throwerror();
			}
			
			// If there was at least one lyricalinstruction
			// to redo, there is no need to keep copying data
			// to retvar->execbin and processing debug information.
			if (!toredo.ptr && b->binary.ptr) {
				
				bytcpy(
					retvar->execbin.ptr + b->binaryoffset,
					b->binary.ptr,
					arrayu8sz(b->binary));
				
				if (i->dbginfo.linenumber) {
					// I generate a new section1 debug information entry
					// in dbginfosection1 only for a different source code line.
					if (i->dbginfo.linenumber != saveddbginfolinenumber) {
						// Function which return
						// the offset of the string
						// i->dbginfo.filepath among
						// all filepath strings.
						uint dbginfofilepathoffset () {
							// Value to return.
							uint retvar = 0;
							
							uint dbginfosection2sz = arrayu64sz(dbginfosection2);
							
							u8* filepath = i->dbginfo.filepath.ptr;
							
							uint n = 0;
							
							while (n < dbginfosection2sz) {
								
								u8* ptr = (u8*)dbginfosection2.ptr[n];
								
								if (ptr == filepath) return retvar;
								
								// The string null-terminating byte
								// will be included in the size.
								retvar += mmsz(ptr);
								
								++n;
							}
							
							// If I get here, the filepath
							// could not be found in
							// the array dbginfosection2.
							
							// I add the filepath to
							// the array dbginfosection2.
							*arrayu64append1(&dbginfosection2) = (uint)filepath;
							
							// The string null-terminating byte
							// will be included in the size.
							dbginfosection2len += mmsz(filepath);
							
							return retvar;
						}
						
						*arrayu64append1(&dbginfosection1) = b->binaryoffset; // binoffset.
						*arrayu64append1(&dbginfosection1) = dbginfofilepathoffset(); // filepath.
						*arrayu64append1(&dbginfosection1) = i->dbginfo.linenumber; // linenumber.
						*arrayu64append1(&dbginfosection1) = i->dbginfo.lineoffset; // lineoffset.
						
						// Save what was the linenumber for which
						// the debug information entry was generated.
						saveddbginfolinenumber = i->dbginfo.linenumber;
					}
				}
			}
			
			i = i->next;
			
		} while (i != f->i->next);
		
	} while ((f = f->next) != compileresult.rootfunc);
	
	// When I get here f == compileresult.rootfunc;
	
	// Check if there was lyricalinstruction to redo.
	if (toredo.ptr) {
		
		mmfree(retvar->execbin.ptr);
		
		if (dbginfosection1.ptr) mmfree(dbginfosection1.ptr);
		
		if (dbginfosection2.ptr) mmfree(dbginfosection2.ptr);
		
		goto redo;
	}
	
	if (saveddbginfolinenumber) {
		// Retrieve the last instruction
		// that was converted.
		b = (backenddata*)compileresult.rootfunc->prev->i->backenddata;
		
		// Mark the end of section1
		// debug information, accumulated
		// in dbginfosection1, with an entry
		// for which the field binoffset
		// is the upper limit offset.
		*arrayu64append1(&dbginfosection1) = b->binaryoffset + arrayu8sz(b->binary); // binoffset.
		*arrayu64append1(&dbginfosection1) = 0; // filepath.
		*arrayu64append1(&dbginfosection1) = 0; // linenumber.
		*arrayu64append1(&dbginfosection1) = 0; // lineoffset.
		
		retvar->dbginfo.ptr = (u8*)dbginfosection1.ptr;
		
		uint retvardbginfosz = arrayu64sz(dbginfosection1) * sizeof(u64);
		
		// Save the current value of retvardbginfosz.
		uint n = retvardbginfosz;
		
		// Write the byte usage of section1
		// debug information in the first u64
		// that was reserved for that purpose.
		*(u64*)retvar->dbginfo.ptr = retvardbginfosz - sizeof(u64);
		
		// Increment the size of debug information
		// data to return by the amount to be used
		// by section2 debug information.
		// sizeof(u64) account for the space
		// needed to store the usage of section2
		// debug information.
		retvardbginfosz += dbginfosection2len + sizeof(u64);
		
		// Resize the debug information data
		// to return, by the amount needed
		// for section2 debug information.
		retvar->dbginfo.ptr = mmrealloc(retvar->dbginfo.ptr, retvardbginfosz);
		
		// Write section2 debug information.
		
		void* dbginfo = retvar->dbginfo.ptr + n;
		
		// Write the byte usage of
		// section2 debug information.
		*(u64*)dbginfo = dbginfosection2len;
		
		dbginfo += sizeof(u64);
		
		n = 0;
		
		uint dbginfosection2sz = arrayu64sz(dbginfosection2);
		
		while (n < dbginfosection2sz) {
			
			u8* filepath = (u8*)dbginfosection2.ptr[n];
			
			// The string null-terminating byte
			// will be included in the size.
			uint filepathsz = mmsz(filepath);
			
			bytcpy(dbginfo, filepath, filepathsz);
			
			dbginfo += filepathsz;
			
			++n;
		}
		
	} else retvar->dbginfo = arrayu8null;
	
	mmsessionextract(convertsession, retvar);
	
	mmsessionextract(convertsession, retvar->execbin.ptr);
	
	if (retvar->exportinfo.ptr) mmsessionextract(convertsession, retvar->exportinfo.ptr);
	
	if (retvar->importinfo.ptr) mmsessionextract(convertsession, retvar->importinfo.ptr);
	
	if (retvar->dbginfo.ptr) mmsessionextract(convertsession, retvar->dbginfo.ptr);
	
	// Free every other memory that was allocated.
	mmsessionfree(convertsession, MMDOSUBSESSIONS);
	
	return retvar;
	
	// Label to return to
	// when an error happen.
	// An error happen only
	// when an unrecognized
	// instruction is found.
	error:
	
	// Since an error was thrown,
	// I free any memory block
	// that was allocated.
	mmsessionfree(convertsession, MMDOSUBSESSIONS);
	
	return 0;
}


// Free the memory used by the object
// returned by lyricalbackendx64().
void lyricalbackendx64resultfree (lyricalbackendx64result* o) {
	
	if (o->execbin.ptr) mmfree(o->execbin.ptr);
	
	if (o->exportinfo.ptr) mmfree(o->exportinfo.ptr);
	
	if (o->importinfo.ptr) mmfree(o->importinfo.ptr);
	
	if (o->dbginfo.ptr) mmfree(o->dbginfo.ptr);
	
	mmfree(o);
}
