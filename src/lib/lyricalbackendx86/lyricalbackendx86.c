
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
#include <arrayu32.h>
#include <bintree.h>
#include <lyrical.h>


// # On Linux, to disassemble a raw binary file, use the following command:
// objdump --start-address=0x0 -D -b binary -mi386 -Mintel,addr32,data32 <filename>
// # Modify the option --start-address to point
// # to where disassembling should start.


#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "little endian required"
#endif


// Structure used by
// lyricalbackendx86()
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
	// followed by a u32 written in little endian
	// that is the offset within the executable
	// where the export is to be found.
	
	// Export information.
	arrayu8 exportinfo;
	
	// Import information is a serie of null
	// terminated function signature string that are
	// followed by a u32 written in little endian
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
	// 	u32 binoffset: Offset of the instruction in the binary.
	// 	u32 filepath: Offset of the source code absolute filepath string within section2 debug information.
	// 	u32 linenumber: Line number within the source code.
	// 	u32 lineoffset: Byte offset of the line within the source code.
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
	// u32 size in bytes of section1 debug information.
	// ------------------------------------------------
	// Section1 debug information.
	// ------------------------------------------------
	// u32 size in bytes of section2 debug information.
	// ------------------------------------------------
	// Section2 debug information.
	// ------------------------------------------------
	
	// Debug information.
	arrayu8 dbginfo;
	
} lyricalbackendx86result;

// enum used with the argument flag
// of lyricalbackendx86().
typedef enum {
	// When used, the constant strings
	// and global variable regions are
	// aligned to 32 bits (4 bytes).
	LYRICALBACKENDX86COMPACT,
	
	// When used, the executable binary is
	// generated as if LYRICALBACKENDX86COMPACT
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
	LYRICALBACKENDX86COMPACTPAGEALIGNED,
	
	// When used, the constant strings
	// and global variable regions are
	// aligned to a pagesize (4096 bytes).
	LYRICALBACKENDX86PAGEALIGNED,
	
} lyricalbackendx86flag;

enum {
	IMM32 = 1,	// 32bits immediate.
	IMM8,		// 8bits immediate.
};

// Backend which convert
// the result of the compilation
// to x86 instructions.
// The field rootfunc of the
// lyricalcompileresult used as
// argument should be non-null.
// Null is returned on faillure.
lyricalbackendx86result* lyricalbackendx86 (lyricalcompileresult compileresult, lyricalbackendx86flag flag) {
	// In generating instructions,
	// this function assume that
	// the D bit of the Code Segment
	// is set so that the default operand
	// and address sizes are 32bits.
	
	// The overall format of x86
	// instructions is as follow:
	// | Prefix | Opcode | ModR/M | SIB | Disp | Imm |
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
	// greater than or equal to sizeof(u32).
	if (sizeof(u32) > sizeof(((lyricalimmval){}).n))
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
		// value is 32 bits.
		// When 2, the immediate
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
		u32 immmiscvalue;
		
		// Similar to the field field immmiscvalue,
		// but used for the secondary immediate.
		u32 imm2miscvalue;
		
	} backenddata;
	
	lyricalfunction* f = compileresult.rootfunc;
	
	// I create a new session
	// which will allow to regain
	// any allocated memory block
	// if an error is thrown.
	mmsession convertsession = mmsessionnew();
	
	// Count of registers.
	enum {REGCOUNT = 8};
	
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
	uint savedregs[REGCOUNT] = {0, 0, 0, 0, 0, 0, 0, 0};
	
	// Array that will be used
	// to temporarily mark an
	// unused register used.
	uint regsinusetmp[REGCOUNT] = {0, 0, 0, 0, 0, 0, 0, 0};
	
	// Binary tree that will be used
	// to store all lyricalinstruction*
	// to redo.
	bintree toredo = bintreenull;
	
	// Label I jump to in order to
	// begin redoing lyricalinstruction.
	redo:;
	
	// Array which will hold
	// section1 debug information.
	arrayu32 dbginfosection1;
	
	// Array which will hold
	// pointers to strings for
	// section2 debug information.
	arrayu32 dbginfosection2;
	// Variable which store the total length
	// of all strings for which the pointers
	// are stored in dbginfosection2.
	uint dbginfosection2len = 0;
	
	dbginfosection1 = arrayu32null;
	
	// Reserve space at the top
	// for storing the byte usage
	// of section1 debug information.
	*arrayu32append1(&dbginfosection1) = 0;
	
	dbginfosection2 = arrayu32null;
	
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
		
		// Enum used to map x86 register names
		// to Lyrical register numbering.
		enum {
			ESP = 0,
			EAX = 1,
			EBX = 2,
			ECX = 3,
			EDX = 4,
			EBP = 5,
			EDI = 6,
			ESI = 7
		};
		
		// Convert the value of the field id
		// of an lyricalreg to its equivalent
		// value useable in the field REG or
		// R/M of the ModR/M byte, or the field
		// BASE or INDEX of the SIB byte.
		u8 lookupreg (uint id) {
			
			switch(id) {
				
				case ESP: return 4;
				
				case EAX: return 0;
				
				case EBX: return 3;
				
				case ECX: return 1;
				
				case EDX: return 2;
				
				case EBP: return 5;
				
				case EDI: return 7;
				
				case ESI: return 6;
				
				default: throwerror();
			}
		}
		
		// Function used to generate
		// the ModR/M byte when I have
		// a single register operand.
		void appendmodrmfor1reg (uint op, uint rm) {
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == op[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters.
			*arrayu8append1(&b->binary) = ((3<<6)|(op<<3)|(lookupreg(rm)));
		}
		
		// Function is used to generate
		// the ModR/M byte when I have
		// 2 register operands.
		void appendmodrmfor2reg (uint reg, uint rm) {
			// From the most significant bit to the least significant bit.
			// MOD == 0b11;
			// REG == reg[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
			*arrayu8append1(&b->binary) = ((3<<6)|(lookupreg(reg)<<3)|(lookupreg(rm)));
		}
		
		// Function is used to generate
		// the ModR/M byte when I have
		// 2 register operands with
		// rm being a memory operand.
		// rm cannot be ESP nor EBP.
		void modrmfor2regformemaccess (uint reg, uint rm) {
			// From the most significant bit to the least significant bit.
			// MOD == 0b00;
			// REG == reg[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
			*arrayu8append1(&b->binary) = ((lookupreg(reg)<<3)|(lookupreg(rm)));
		}
		
		// Function is used to generate
		// the ModR/M byte when I have
		// 2 register operands with
		// rm being a memory operand.
		// rm cannot be ESP.
		void appendmodrmfor2regformemaccesswith8imm (uint reg, uint rm) {
			// From the most significant bit to the least significant bit.
			// MOD == 0b01;
			// REG == reg[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
			*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupreg(reg)<<3)|(lookupreg(rm)));
		}
		
		// Function is used to generate
		// the ModR/M byte when I have
		// 2 register operands with
		// rm being a memory operand.
		// rm cannot be ESP.
		void appendmodrmfor2regformemaccesswith32imm (uint reg, uint rm) {
			// From the most significant bit to the least significant bit.
			// MOD == 0b10;
			// REG == reg[2:0]; #Only the 3 least significant bits matters.
			// R/M == rm[2:0]; #Only the 3 least significant bits matters. */
			*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupreg(reg)<<3)|(lookupreg(rm)));
		}
		
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
		// immmiscvalue (increment it).
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
		// imm2miscvalue (increment it).
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
		
		// This function is used to encode the modrm
		// and sib byte for instructions requiring
		// a register operand and memory operand.
		// The argument reg hold the register number
		// for the register operand while the argument
		// base hold the register number for the base
		// address to use for the memory operand;
		// a 32bits displacement is required to be
		// appended after a call to this function.
		void appendmodrmsib32bitsimm (uint reg, uint base) {
			// Here below I describe how
			// I encode the modrm and sib bytes:
			
			// Encoding of the ModR/M byte:
			// mod: 0b10
			// reg: #Used to encode the number of the register operand.
			// r/m: 0b100
			
			// Encoding of the SIB byte:
			// scale: 0b00;
			// index: 0b100; #A register for index is not used and scale is ignored.
			// base: #Hold the register number to use as base.
			
			// The above will encode:
			// Baseregister + disp32;
			
			// The 32bit displacement should be
			// appended after the modrm and sib bytes.
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = ((0b10<<6)|(lookupreg(reg)<<3)|0b100);
			
			// Append the SIB byte.
			*arrayu8append1(&b->binary) = ((0b100<<3)|lookupreg(base));
		}
		
		// This function is used to encode the modrm
		// and sib byte for instructions requiring
		// a register operand and memory operand.
		// The argument reg hold the register number
		// for the register operand while the argument
		// base hold the register number for the base
		// address to use for the memory operand;
		// an 8bits displacement is required to be
		// appended after a call to this function.
		void appendmodrmsib8bitsimm (uint reg, uint base) {
			// Here below I describe how
			// I encode the modrm and sib bytes:
			
			// Encoding of the ModR/M byte:
			// mod: 0b01;
			// reg: #Used to encode the number of the register operand.
			// r/m: 0b100
			
			// Encoding of the SIB byte:
			// scale: 0b00;
			// index: 0b100; #A register for index is not used and scale is ignored.
			// base: #Hold the register number to use as base.
			
			// The above will encode:
			// Baseregister + disp8;
			
			// The 8bit displacement should be
			// appended after the modrm and sib bytes.
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = ((0b01<<6)|(lookupreg(reg)<<3)|0b100);
			
			// Append the SIB byte.
			*arrayu8append1(&b->binary) = ((0b100<<3)|lookupreg(base));
		}
		
		// This function is used to encode
		// the modrm for instructions requiring
		// a register operand and immediate address.
		// The argument reg hold the register number
		// for the register operand; a 32bits immediate
		// value is required to be appended after
		// a call to this function.
		void appendmodrmimmaddr (uint reg) {
			// Here below I describe how
			// I encode the modrm byte:
			
			// Encoding of the ModR/M byte:
			// mod: 0b00
			// reg: #Used to encode the number of the register operand.
			// r/m: 0b101
			
			// The 32bit displacement should be
			// appended after the modrm byte.
			
			// Append the ModR/M byte.
			*arrayu8append1(&b->binary) = ((lookupreg(reg)<<3)|0b101);
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
			// 89 /r	MOV r/m32,r32		#Move r32 to r/m32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x89;
			
			// Append the ModR/M byte.
			appendmodrmfor2reg(r2, r1);
		}
		
		void push (uint r) {
			// Specification from the Intel manual.
			// 50+rd	PUSH r32	#Push r32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = (0x50+lookupreg(r));
		}
		
		void pop (uint r) {
			// The behavior is erroneous
			// when this instruction
			// is used with ESP; and r
			// will never be ESP since
			// the compiler do not use it
			// when allocating registers
			// to variables.
			
			// Specification from the Intel manual.
			// 58+rd	POP r32		#Pop r32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = (0x58+lookupreg(r));
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
		// either EAX, EBX, ECX, EDX
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
					// is either EAX, EBX, ECX, EDX and
					// was not marked as temporarily used.
					if ((s < EAX || s > EDX) || regsinusetmp[s]) {
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
			// 01 /r	ADD r/m32, r32		#Add r32 to r/m32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x01;
			
			// Append the ModR/M byte.
			appendmodrmfor2reg(r2, r1);
		}
		
		void addi (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// 81 /0 id	ADD r/m32, imm32	#Add imm32 to r/m32.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x81;
				
				// Append the ModR/M byte.
				appendmodrmfor1reg(0, r);
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// 83 /0 ib	ADD r/m32, imm8	#Add imm8 (sign-extended to 32bits) to r/m32.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x83;
				
				// Append the ModR/M byte.
				appendmodrmfor1reg(0, r);
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void addi2 (uint r) {
			// If b->isimm2used is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimm2used == IMM32) {
				// Specification from the Intel manual.
				// 81 /0 id	ADD r/m32, imm32	#Add imm32 to r/m32.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x81;
				
				// Append the ModR/M byte.
				appendmodrmfor1reg(0, r);
				
				// Append the immediate2 value.
				append32bitsimm2(0);
				
			} else {
				// Specification from the Intel manual.
				// 83 /0 ib	ADD r/m32, imm8	#Add imm8 (sign-extended to 32bits) to r/m32.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x83;
				
				// Append the ModR/M byte.
				appendmodrmfor1reg(0, r);
				
				// Append the immediate2 value.
				append8bitsimm2(0);
			}
		}
		
		// Commented out; kept for reference.
		#if 0
		void inc (uint r) {
			// Specification from the Intel manual.
			// 40+rd	INC r32		#Increment r32 by 1.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = (0x40+lookupreg(r));
		}
		
		// Increment register by 32bits immediate
		// value given as argument.
		void inc32 (uint r, u32 n) {
			// Specification from the Intel manual.
			// 81 /0 id	ADD r/m32, imm32	#Add imm32 to r/m32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x81;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, r);
			
			// Append 32bits immediate value.
			*(u32*)arrayu8append2(&b->binary, sizeof(u32)) = n;
		}
		
		// Increment register by 8bits immediate
		// value given as argument.
		void inc8 (uint r, u8 n) {
			// Specification from the Intel manual.
			// 83 /0 ib	ADD r/m32, imm8	#Add imm8 (sign-extended to 32bits) to r/m32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x83;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, r);
			
			// Append 8bits immediate value.
			*arrayu8append1(&b->binary) = n;
		}
		#endif
		
		void sub (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 29 /r	SUB r/m32, r32		#Subtract r32 from r/m32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x29;
			
			// Append the ModR/M byte.
			appendmodrmfor2reg(r2, r1);
		}
		
		// Not used; kept for reference.
		#if 0
		void subi (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// 81 /5 id	SUB r/m32, imm32	#Substract imm32 from r/m32.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x81;
				
				// Append the ModR/M byte.
				appendmodrmfor1reg(5, r);
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// 83 /5 ib	ADD r/m32, imm8	#Substract imm8 (sign-extended to 32bits) from r/m32.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x83;
				
				// Append the ModR/M byte.
				appendmodrmfor1reg(5, r);
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		#endif
		
		void neg (uint r) {
			// Specification from the Intel manual.
			// F7 /3	NEG r/m32	#Two's complement negate r/m32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(3, r);
		}
		
		// Function used to atomically
		// xchg the value of two registers.
		void xchg (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 87 /r	XCHG r/m32, r32		#Exchange r32 with doubleword from r/m32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x87;
			
			// Append the ModR/M byte.
			appendmodrmfor2reg(r2, r1);
		}
		
		void mul (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 0F AF /r	IMUL r32, r/m32		#reg = reg * r/m32.
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0xaf;
			
			// Append the ModR/M byte.
			appendmodrmfor2reg(r1, r2);
		}
		
		void mulh (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r1 != EAX && r2 == EAX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the multiplicator
				// in EAX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint x;
			
			if (r2 == EAX) {
				// Look for an unused register.
				x = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!x) throwerror();
				
				regsinusetmp[x] = 1; // Mark unused register as temporarily used.
				
				cpy(x, r2);
				
			} else x = r2;
			
			uint eaxwassaved = 0;
			uint edxwassaved = 0;
			
			if (r1 != EAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EAX)) {
					savereg(EAX); eaxwassaved = 1;
				} else regsinusetmp[EAX] = 1; // Mark unused register as temporarily used.
				
				cpy(EAX, r1);
			}
			
			if (r1 != EDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDX)) {
					savereg(EDX); edxwassaved = 1;
				} else regsinusetmp[EDX] = 1; // Mark unused register as temporarily used.
			}
			
			// Specification from the Intel manual.
			// F7 /5	IMUL r/m32	#Signed multiply EDX:EAX = EAX * r/m32,
			// with the low result stored in EAX and high result stored in EDX.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(5, x);
			
			if (r1 != EDX) {
				
				cpy(r1, EDX);
				
				if (edxwassaved) {
					// Restore register value.
					restorereg(EDX);
				} else regsinusetmp[EDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1 != EAX) {
				
				if (eaxwassaved) {
					// Restore register value.
					restorereg(EAX);
				} else regsinusetmp[EAX] = 0; // Unmark register as temporarily used.
			}
			
			if (x != r2) regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void xor (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 31 /r	XOR r/m32, r32		#r/m32 XOR r32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x31;
			
			// Append the ModR/M byte.
			appendmodrmfor2reg(r2, r1);
		}
		
		void div (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r1 != EAX && r2 == EAX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the dividend
				// in EDX:EAX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint x;
			
			if (r2 == EAX || r2 == EDX) {
				// Look for an unused register.
				x = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!x) throwerror();
				
				regsinusetmp[x] = 1; // Mark unused register as temporarily used.
				
				cpy(x, r2);
				
			} else x = r2;
			
			uint eaxwassaved = 0;
			uint edxwassaved = 0;
			
			if (r1 != EAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EAX)) {
					savereg(EAX); eaxwassaved = 1;
				} else regsinusetmp[EAX] = 1; // Mark unused register as temporarily used.
				
				cpy(EAX, r1);
			}
			
			if (r1 != EDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDX)) {
					savereg(EDX); edxwassaved = 1;
				} else regsinusetmp[EDX] = 1; // Mark unused register as temporarily used.
			}
			
			xor(EDX, EDX);
			
			// Specification from the Intel manual.
			// F7 /7	IDIV r/m32	#Signed divide EDX:EAX by r/m32,
			// with quotient stored in EAX and remainder stored in EDX.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(7, x);
			
			if (r1 != EDX) {
				
				if (edxwassaved) {
					// Restore register value.
					restorereg(EDX);
				} else regsinusetmp[EDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1 != EAX) {
				
				cpy(r1, EAX);
				
				if (eaxwassaved) {
					// Restore register value.
					restorereg(EAX);
				} else regsinusetmp[EAX] = 0; // Unmark register as temporarily used.
			}
			
			if (x != r2) regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void mod (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r1 != EAX && r2 == EAX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the dividend
				// in EDX:EAX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint x;
			
			if (r2 == EAX || r2 == EDX) {
				// Look for an unused register.
				x = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!x) throwerror();
				
				regsinusetmp[x] = 1; // Mark unused register as temporarily used.
				
				cpy(x, r2);
				
			} else x = r2;
			
			uint eaxwassaved = 0;
			uint edxwassaved = 0;
			
			if (r1 != EAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EAX)) {
					savereg(EAX); eaxwassaved = 1;
				} else regsinusetmp[EAX] = 1; // Mark unused register as temporarily used.
				
				cpy(EAX, r1);
			}
			
			if (r1 != EDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDX)) {
					savereg(EDX); edxwassaved = 1;
				} else regsinusetmp[EDX] = 1; // Mark unused register as temporarily used.
			}
			
			xor(EDX, EDX);
			
			// Specification from the Intel manual.
			// F7 /7	IDIV r/m32	#Signed divide EDX:EAX by r/m32,
			// with quotient stored in EAX and remainder stored in EDX.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(7, x);
			
			if (r1 != EDX) {
				
				cpy(r1, EDX);
				
				if (edxwassaved) {
					// Restore register value.
					restorereg(EDX);
				} else regsinusetmp[EDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1 != EAX) {
				
				if (eaxwassaved) {
					// Restore register value.
					restorereg(EAX);
				} else regsinusetmp[EAX] = 0; // Unmark register as temporarily used.
			}
			
			if (x != r2) regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void mulhu (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r1 != EAX && r2 == EAX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the multiplicator
				// in EAX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint x;
			
			if (r2 == EAX) {
				// Look for an unused register.
				x = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!x) throwerror();
				
				regsinusetmp[x] = 1; // Mark unused register as temporarily used.
				
				cpy(x, r2);
				
			} else x = r2;
			
			uint eaxwassaved = 0;
			uint edxwassaved = 0;
			
			if (r1 != EAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EAX)) {
					savereg(EAX); eaxwassaved = 1;
				} else regsinusetmp[EAX] = 1; // Mark unused register as temporarily used.
				
				cpy(EAX, r1);
			}
			
			if (r1 != EDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDX)) {
					savereg(EDX); edxwassaved = 1;
				} else regsinusetmp[EDX] = 1; // Mark unused register as temporarily used.
			}
			
			// Specification from the Intel manual.
			// F7 /4	MUL r/m32	#Unsigned multiply EDX:EAX = EAX * r/m32,
			// with the low result stored in EAX and high result stored in EDX.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(4, x);
			
			if (r1 != EDX) {
				
				cpy(r1, EDX);
				
				if (edxwassaved) {
					// Restore register value.
					restorereg(EDX);
				} else regsinusetmp[EDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1 != EAX) {
				
				if (eaxwassaved) {
					// Restore register value.
					restorereg(EAX);
				} else regsinusetmp[EAX] = 0; // Unmark register as temporarily used.
			}
			
			if (x != r2) regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void divu (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r1 != EAX && r2 == EAX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the dividend
				// in EDX:EAX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint x;
			
			if (r2 == EAX || r2 == EDX) {
				// Look for an unused register.
				x = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!x) throwerror();
				
				regsinusetmp[x] = 1; // Mark unused register as temporarily used.
				
				cpy(x, r2);
				
			} else x = r2;
			
			uint eaxwassaved = 0;
			uint edxwassaved = 0;
			
			if (r1 != EAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EAX)) {
					savereg(EAX); eaxwassaved = 1;
				} else regsinusetmp[EAX] = 1; // Mark unused register as temporarily used.
				
				cpy(EAX, r1);
			}
			
			if (r1 != EDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDX)) {
					savereg(EDX); edxwassaved = 1;
				} else regsinusetmp[EDX] = 1; // Mark unused register as temporarily used.
			}
			
			xor(EDX, EDX);
			
			// Specification from the Intel manual.
			// F7 /6	DIV r/m32	#Unsigned divide EDX:EAX by r/m32,
			// with quotient stored in EAX and remainder stored in EDX.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(6, x);
			
			if (r1 != EDX) {
				
				if (edxwassaved) {
					// Restore register value.
					restorereg(EDX);
				} else regsinusetmp[EDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1 != EAX) {
				
				cpy(r1, EAX);
				
				if (eaxwassaved) {
					// Restore register value.
					restorereg(EAX);
				} else regsinusetmp[EAX] = 0; // Unmark register as temporarily used.
			}
			
			if (x != r2) regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void modu (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r1 != EAX && r2 == EAX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the dividend
				// in EDX:EAX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint x;
			
			if (r2 == EAX || r2 == EDX) {
				// Look for an unused register.
				x = findunusedreg(0);
				
				// The compiler should have insured
				// that there are enough unused
				// registers available.
				if (!x) throwerror();
				
				regsinusetmp[x] = 1; // Mark unused register as temporarily used.
				
				cpy(x, r2);
				
			} else x = r2;
			
			uint eaxwassaved = 0;
			uint edxwassaved = 0;
			
			if (r1 != EAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EAX)) {
					savereg(EAX); eaxwassaved = 1;
				} else regsinusetmp[EAX] = 1; // Mark unused register as temporarily used.
				
				cpy(EAX, r1);
			}
			
			if (r1 != EDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDX)) {
					savereg(EDX); edxwassaved = 1;
				} else regsinusetmp[EDX] = 1; // Mark unused register as temporarily used.
			}
			
			xor(EDX, EDX);
			
			// Specification from the Intel manual.
			// F7 /6	DIV r/m32	#Unsigned divide EDX:EAX by r/m32,
			// with quotient stored in EAX and remainder stored in EDX.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(6, x);
			
			if (r1 != EDX) {
				
				cpy(r1, EDX);
				
				if (edxwassaved) {
					// Restore register value.
					restorereg(EDX);
				} else regsinusetmp[EDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1 != EAX) {
				
				if (eaxwassaved) {
					// Restore register value.
					restorereg(EAX);
				} else regsinusetmp[EAX] = 0; // Unmark register as temporarily used.
			}
			
			if (x != r2) regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void muli (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// 69 /r id	IMUL r32, r/m32, imm32		#reg = r/m32 * imm32.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x69;
				
				// Append the ModR/M byte.
				appendmodrmfor2reg(r, r);
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// 6b /r id	IMUL r32, r/m32, imm8		#reg = r/m32 * imm8.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x6b;
				
				// Append the ModR/M byte.
				appendmodrmfor2reg(r, r);
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		// Load immediate value given as argument.
		void loadimm (uint r, u32 n) {
			// Specification from the Intel manual.
			// B8+rd id	MOV r32, imm32		#Move imm32 to r32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = (0xb8+lookupreg(r));
			
			// Append 32bits immediate value.
			*(u32*)arrayu8append2(&b->binary, sizeof(u32)) = n;
		}
		
		// Function used to load the
		// immediate value associated
		// with the lyricalinstruction
		// pointed by i; the value of
		// the argument n get added
		// to the lyricalinstruction
		// immediate value.
		void ldi (uint r, u32 n) {
			
			lyricalimmval* imm = i->imm;
			
			do {
				// If the immediate value is null,
				// use xor() to set the register null.
				if (imm->type != LYRICALIMMVALUE || *(u64*)&imm->n || n) {
					// Specification from the Intel manual.
					// B8+rd id	MOV r32, imm32		#Move imm32 to r32.
					
					// Append the opcode byte.
					*arrayu8append1(&b->binary) = (0xb8+lookupreg(r));
					
					// Append the immediate value.
					append32bitsimm(n);
					
					return;
				}
				
			} while (imm = imm->next);
			
			xor(r, r);
		}
		
		void mulhi (uint r) {
			
			uint eaxwassaved = 0;
			uint edxwassaved = 0;
			
			if (r != EAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EAX)) {
					savereg(EAX); eaxwassaved = 1;
				} else regsinusetmp[EAX] = 1; // Mark unused register as temporarily used.
				
				cpy(EAX, r);
			}
			
			if (r != EDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDX)) {
					savereg(EDX); edxwassaved = 1;
				} else regsinusetmp[EDX] = 1; // Mark unused register as temporarily used.
			}
			
			ldi(EDX, 0);
			
			// Specification from the Intel manual.
			// F7 /5	IMUL r/m32	#Signed multiply EDX:EAX = EAX * r/m32,
			// with the low result stored in EAX and high result stored in EDX.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(5, EDX);
			
			if (r != EDX) {
				
				cpy(r, EDX);
				
				if (edxwassaved) {
					// Restore register value.
					restorereg(EDX);
				} else regsinusetmp[EDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r != EAX) {
				
				if (eaxwassaved) {
					// Restore register value.
					restorereg(EAX);
				} else regsinusetmp[EAX] = 0; // Unmark register as temporarily used.
			}
		}
		
		void divi (uint r) {
			
			uint eaxwassaved = 0;
			uint edxwassaved = 0;
			
			if (r != EAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EAX)) {
					savereg(EAX); eaxwassaved = 1;
				} else regsinusetmp[EAX] = 1; // Mark unused register as temporarily used.
				
				cpy(EAX, r);
			}
			
			if (r != EDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDX)) {
					savereg(EDX); edxwassaved = 1;
				} else regsinusetmp[EDX] = 1; // Mark unused register as temporarily used.
			}
			
			// Look for an unused register.
			uint x = findunusedreg(0);
			
			// The compiler should have insured
			// that there are enough unused
			// registers available.
			if (!x) throwerror();
			
			regsinusetmp[x] = 1; // Mark unused register as temporarily used.
			
			ldi(x, 0);
			
			xor(EDX, EDX);
			
			// Specification from the Intel manual.
			// F7 /7	IDIV r/m32	#Signed divide EDX:EAX by r/m32,
			// with quotient stored in EAX and remainder stored in EDX.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(7, x);
			
			regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r != EDX) {
				
				if (edxwassaved) {
					// Restore register value.
					restorereg(EDX);
				} else regsinusetmp[EDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r != EAX) {
				
				cpy(r, EAX);
				
				if (eaxwassaved) {
					// Restore register value.
					restorereg(EAX);
				} else regsinusetmp[EAX] = 0; // Unmark register as temporarily used.
			}
		}
		
		void modi (uint r) {
			
			uint eaxwassaved = 0;
			uint edxwassaved = 0;
			
			if (r != EAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EAX)) {
					savereg(EAX); eaxwassaved = 1;
				} else regsinusetmp[EAX] = 1; // Mark unused register as temporarily used.
				
				cpy(EAX, r);
			}
			
			if (r != EDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDX)) {
					savereg(EDX); edxwassaved = 1;
				} else regsinusetmp[EDX] = 1; // Mark unused register as temporarily used.
			}
			
			// Look for an unused register.
			uint x = findunusedreg(0);
			
			// The compiler should have insured
			// that there are enough unused
			// registers available.
			if (!x) throwerror();
			
			regsinusetmp[x] = 1; // Mark unused register as temporarily used.
			
			ldi(x, 0);
			
			xor(EDX, EDX);
			
			// Specification from the Intel manual.
			// F7 /7	IDIV r/m32	#Signed divide EDX:EAX by r/m32,
			// with quotient stored in EAX and remainder stored in EDX.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(7, x);
			
			regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r != EDX) {
				
				cpy(r, EDX);
				
				if (edxwassaved) {
					// Restore register value.
					restorereg(EDX);
				} else regsinusetmp[EDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r != EAX) {
				
				if (eaxwassaved) {
					// Restore register value.
					restorereg(EAX);
				} else regsinusetmp[EAX] = 0; // Unmark register as temporarily used.
			}
		}
		
		void mulhui (uint r) {
			
			uint eaxwassaved = 0;
			uint edxwassaved = 0;
			
			if (r != EAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EAX)) {
					savereg(EAX); eaxwassaved = 1;
				} else regsinusetmp[EAX] = 1; // Mark unused register as temporarily used.
				
				cpy(EAX, r);
			}
			
			if (r != EDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDX)) {
					savereg(EDX); edxwassaved = 1;
				} else regsinusetmp[EDX] = 1; // Mark unused register as temporarily used.
			}
			
			ldi(EDX, 0);
			
			// Specification from the Intel manual.
			// F7 /4	MUL r/m32	#Unsigned multiply EDX:EAX = EAX * r/m32,
			// with the low result stored in EAX and high result stored in EDX.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(4, EDX);
			
			if (r != EDX) {
				
				cpy(r, EDX);
				
				if (edxwassaved) {
					// Restore register value.
					restorereg(EDX);
				} else regsinusetmp[EDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r != EAX) {
				
				if (eaxwassaved) {
					// Restore register value.
					restorereg(EAX);
				} else regsinusetmp[EAX] = 0; // Unmark register as temporarily used.
			}
		}
		
		void divui (uint r) {
			
			uint eaxwassaved = 0;
			uint edxwassaved = 0;
			
			if (r != EAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EAX)) {
					savereg(EAX); eaxwassaved = 1;
				} else regsinusetmp[EAX] = 1; // Mark unused register as temporarily used.
				
				cpy(EAX, r);
			}
			
			if (r != EDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDX)) {
					savereg(EDX); edxwassaved = 1;
				} else regsinusetmp[EDX] = 1; // Mark unused register as temporarily used.
			}
			
			// Look for an unused register.
			uint x = findunusedreg(0);
			
			// The compiler should have insured
			// that there are enough unused
			// registers available.
			if (!x) throwerror();
			
			regsinusetmp[x] = 1; // Mark unused register as temporarily used.
			
			ldi(x, 0);
			
			xor(EDX, EDX);
			
			// Specification from the Intel manual.
			// F7 /6	DIV r/m32	#Unsigned divide EDX:EAX by r/m32,,
			// with quotient stored in EAX and remainder stored in EDX.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(6, x);
			
			regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r != EDX) {
				
				if (edxwassaved) {
					// Restore register value.
					restorereg(EDX);
				} else regsinusetmp[EDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r != EAX) {
				
				cpy(r, EAX);
				
				if (eaxwassaved) {
					// Restore register value.
					restorereg(EAX);
				} else regsinusetmp[EAX] = 0; // Unmark register as temporarily used.
			}
		}
		
		void modui (uint r) {
			
			uint eaxwassaved = 0;
			uint edxwassaved = 0;
			
			if (r != EAX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EAX)) {
					savereg(EAX); eaxwassaved = 1;
				} else regsinusetmp[EAX] = 1; // Mark unused register as temporarily used.
				
				cpy(EAX, r);
			}
			
			if (r != EDX) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDX)) {
					savereg(EDX); edxwassaved = 1;
				} else regsinusetmp[EDX] = 1; // Mark unused register as temporarily used.
			}
			
			// Look for an unused register.
			uint x = findunusedreg(0);
			
			// The compiler should have insured
			// that there are enough unused
			// registers available.
			if (!x) throwerror();
			
			regsinusetmp[x] = 1; // Mark unused register as temporarily used.
			
			ldi(x, 0);
			
			xor(EDX, EDX);
			
			// Specification from the Intel manual.
			// F7 /6	DIV r/m32	#Unsigned divide EDX:EAX by r/m32,
			// with quotient stored in EAX and remainder stored in EDX.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(6, x);
			
			regsinusetmp[x] = 0; // Unmark register as temporarily used.
			
			if (r != EDX) {
				
				cpy(r, EDX);
				
				if (edxwassaved) {
					// Restore register value.
					restorereg(EDX);
				} else regsinusetmp[EDX] = 0; // Unmark register as temporarily used.
			}
			
			if (r != EAX) {
				
				if (eaxwassaved) {
					// Restore register value.
					restorereg(EAX);
				} else regsinusetmp[EAX] = 0; // Unmark register as temporarily used.
			}
		}
		
		void and (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 21 /r	AND r/m32, r32		#r/m32 AND r32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x21;
			
			// Append the ModR/M byte.
			appendmodrmfor2reg(r2, r1);
		}
		
		void andi (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// 81 /4 id	AND r/m32, imm32	#r/m32 AND imm32.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x81;
				
				// Append the ModR/M byte.
				appendmodrmfor1reg(4, r);
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// 83 /4 ib	AND r/m32, imm8	#r/m32 AND imm8 (sign-extended to 32bits).
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x83;
				
				// Append the ModR/M byte.
				appendmodrmfor1reg(4, r);
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void andimm (uint r, u32 n) {
			
			if ((((sint)n < 0) ? -n : n) < (1 << 7)) {
				// Specification from the Intel manual.
				// 83 /4 ib	AND r/m32, imm8	#r/m32 AND imm8 (sign-extended to 32bits).
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x83;
				
				// Append the ModR/M byte.
				appendmodrmfor1reg(4, r);
				
				// Append the immediate value.
				*arrayu8append1(&b->binary) = n;
				
			} else {
				// Specification from the Intel manual.
				// 81 /4 id	AND r/m32, imm32	#r/m32 AND imm32.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x81;
				
				// Append the ModR/M byte.
				appendmodrmfor1reg(4, r);
				
				// Append 32bits immediate value.
				*(u32*)arrayu8append2(&b->binary, sizeof(u32)) = n;
			}
		}
		
		void or (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 09 /r	OR r/m32, r32		#r/m32 OR r32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x09;
			
			// Append the ModR/M byte.
			appendmodrmfor2reg(r2, r1);
		}
		
		void ori (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// 81 /1 id	OR r/m32, imm32	#r/m32 OR imm32.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x81;
				
				// Append the ModR/M byte.
				appendmodrmfor1reg(1, r);
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// 83 /1 ib	OR r/m32, imm8	#r/m32 OR imm8 (sign-extended to 32bits).
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x83;
				
				// Append the ModR/M byte.
				appendmodrmfor1reg(1, r);
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void xori (uint r) {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// 81 /6 id	XOR r/m32, imm32	#r/m32 XOR imm32.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x81;
				
				// Append the ModR/M byte.
				appendmodrmfor1reg(6, r);
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// 83 /6 ib	XOR r/m32, imm8	#r/m32 XOR imm8 (sign-extended to 32bits).
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x83;
				
				// Append the ModR/M byte.
				appendmodrmfor1reg(6, r);
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void not (uint r) {
			// Specification from the Intel manual.
			// F7 /2	NOT r/m32	#Reverse each bit of r/m32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xf7;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(2, r);
		}
		
		void sll (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r2 != ECX && r1 == ECX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the shift amount
				// in ECX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint ecxwassaved = 0;
			
			if (r2 != ECX) {
				// Save register value
				// if it is being used.
				if (isreginuse(ECX)) {
					savereg(ECX); ecxwassaved = 1;
				} else regsinusetmp[ECX] = 1; // Mark unused register as temporarily used.
				
				cpy(ECX, r2);
			}
			
			// Specification from the Intel manual.
			// D3 /4	SHL r/m32, CL		#Shift logically r/m32 left CL times.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xd3;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(4, r1);
			
			if (r2 != ECX) {
				
				if (ecxwassaved) {
					// Restore register value.
					restorereg(ECX);
				} else regsinusetmp[ECX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void slli (uint r) {
			// Specification from the Intel manual.
			// C1 /4 ib	SHL r/m32, imm8		#Shift logically r/m32 left imm8 times.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xc1;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(4, r);
			
			// Append the immediate value.
			append8bitsimm(0);
		}
		
		void sllimm (uint r, u8 n) {
			// Specification from the Intel manual.
			// C1 /4 ib	SHL r/m32, imm8		#Shift logically r/m32 left imm8 times.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xc1;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(4, r);
			
			// Append the 8bits immediate value.
			*arrayu8append1(&b->binary) = n;
		}
		
		void srl (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r2 != ECX && r1 == ECX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the shift amount
				// in ECX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint ecxwassaved = 0;
			
			if (r2 != ECX) {
				// Save register value
				// if it is being used.
				if (isreginuse(ECX)) {
					savereg(ECX); ecxwassaved = 1;
				} else regsinusetmp[ECX] = 1; // Mark unused register as temporarily used.
				
				cpy(ECX, r2);
			}
			
			// Specification from the Intel manual.
			// D3 /5	SHR r/m32, CL		#Shift logically r/m32 right CL times.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xd3;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(5, r1);
			
			if (r2 != ECX) {
				
				if (ecxwassaved) {
					// Restore register value.
					restorereg(ECX);
				} else regsinusetmp[ECX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void srli (uint r) {
			// Specification from the Intel manual.
			// C1 /5 ib	SHR r/m32, imm8		#Shift logically r/m32 right imm8 times.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xc1;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(5, r);
			
			// Append the immediate value.
			append8bitsimm(0);
		}
		
		void sra (uint r1, uint r2) {
			
			uint r1r2wasxchged = 0;
			
			if (r2 != ECX && r1 == ECX) {
				// If I get here, I xchg r1 and r2
				// so as to generate the least
				// amount of intructions while
				// setting up the shift amount
				// in ECX.
				
				xchg(r1, r2);
				
				// Set r1 and r2 to the register id
				// that contain their values.
				r1 ^= r2; r2 ^= r1; r1 ^= r2;
				
				r1r2wasxchged = 1;
			}
			
			uint ecxwassaved = 0;
			
			if (r2 != ECX) {
				// Save register value
				// if it is being used.
				if (isreginuse(ECX)) {
					savereg(ECX); ecxwassaved = 1;
				} else regsinusetmp[ECX] = 1; // Mark unused register as temporarily used.
				
				cpy(ECX, r2);
			}
			
			// Specification from the Intel manual.
			// D3 /7	SAR r/m32, CL		#Shift arithmetically r/m32 right CL times.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xd3;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(7, r1);
			
			if (r2 != ECX) {
				
				if (ecxwassaved) {
					// Restore register value.
					restorereg(ECX);
				} else regsinusetmp[ECX] = 0; // Unmark register as temporarily used.
			}
			
			if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
		}
		
		void srai (uint r) {
			// Specification from the Intel manual.
			// C1 /7 ib	SAR r/m32, imm8		#Shift arithmetically r/m32 right imm8 times.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xc1;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(7, r);
			
			// Append the immediate value.
			append8bitsimm(0);
		}
		
		void sraimm (uint r, u8 n) {
			// Specification from the Intel manual.
			// C1 /7 ib	SAR r/m32, imm8		#Shift arithmetically r/m32 right imm8 times.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xc1;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(7, r);
			
			// Append the shift amount.
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
				// either EAX, EBX, ECX, EDX;
				
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
				// of EAX, EBX, ECX, EDX.
				if (r < EAX || r > EDX) {
					
					if (!(rvalid = findunusedregfor8bitsoperand(0))) {
						// I get here if r was not
						// either of EAX, EBX, ECX, EDX
						// and I could not find a valid
						// unused register to use.
						
						rvalid = EAX;
						
						savereg(rvalid); rvalidwassaved = 1;
						
					} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
					
					cpy(rvalid, r);
					
				} else rvalid = r;
				
				// Specification from the Intel manual.
				// 0F B6 /r	MOVZX r32, r/m8		#Move r/m8 to r32, zero-extension.
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0xb6;
				
				// Append the ModR/M byte.
				appendmodrmfor2reg(rvalid, rvalid);
				
				if (rvalid != r) {
					
					cpy(r, rvalid);
					
					if (rvalidwassaved) restorereg(rvalid);
					// Unmark register as temporarily used.
					else regsinusetmp[rvalid] = 0;
				}
				
			} else if (n == 16) {
				// Specification from the Intel manual.
				// 0F B7 /r	MOVZX r32, r/m16	#Move r/m16 to r32, zero-extension.
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0xb7;
				
				// Append the ModR/M byte.
				appendmodrmfor2reg(r, r);
				
			} else {
				// I get here if n is not 8 or 16.
				// Zero extension is done using
				// the operation "and".
				
				// Compute the immediate value
				// to use with the operation "and"
				// in order to do zero extension.
				u32 bitselect = ((u32)1 << n) - 1;
				
				if (bitselect < (1<<7)) {
					// Specification from the Intel manual.
					// 83 /4 ib	AND r/m32, imm8		#r/m32 AND imm8 (sign-extended).
					
					// Append the opcode byte.
					*arrayu8append1(&b->binary) = 0x83;
					
					// Append the ModR/M byte.
					appendmodrmfor1reg(4, r);
					
					// Append the 8bits immediate value.
					*arrayu8append1(&b->binary) = bitselect;
					
				} else {
					// Specification from the Intel manual.
					// 81 /4 id	AND r/m32, imm32	#r/m32 AND imm32.
					
					// Append the opcode byte.
					*arrayu8append1(&b->binary) = 0x81;
					
					// Append the ModR/M byte.
					appendmodrmfor1reg(4, r);
					
					// Append the 32bits immediate value.
					*(u32*)arrayu8append2(&b->binary, sizeof(u32)) = bitselect;
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
				// either EAX, EBX, ECX, EDX;
				
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
				// of EAX, EBX, ECX, EDX.
				if (r < EAX || r > EDX) {
					
					if (!(rvalid = findunusedregfor8bitsoperand(0))) {
						// I get here if r was not
						// either of EAX, EBX, ECX, EDX
						// and I could not find a valid
						// unused register to use.
						
						rvalid = EAX;
						
						savereg(rvalid); rvalidwassaved = 1;
						
					} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
					
					cpy(rvalid, r);
					
				} else rvalid = r;
				
				// Specification from the Intel manual.
				// 0F BE /r	MOVSX r32, r/m8		#Move r/m8 to r32, sign-extension.
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0xbe;
				
				// Append the ModR/M byte.
				appendmodrmfor2reg(rvalid, rvalid);
				
				if (rvalid != r) {
					
					cpy(r, rvalid);
					
					if (rvalidwassaved) restorereg(rvalid);
					// Unmark register as temporarily used.
					else regsinusetmp[rvalid] = 0;
				}
				
			} else if (n == 16) {
				// Specification from the Intel manual.
				// 0F BF /r	MOVSX r32, r/m16	#Move r/m16 to r32, sign-extension.
				
				// Append the opcode bytes.
				*arrayu8append1(&b->binary) = 0x0f;
				*arrayu8append1(&b->binary) = 0xbf;
				
				// Append the ModR/M byte.
				appendmodrmfor2reg(r, r);
				
			} else {
				// I get here if n is not 8 or 16.
				// Sign extension is done shifting left
				// then arithmetically shifting right.
				
				// Compute the amount
				// by which to shift
				// left then right in
				// order to sign extend.
				n = 32 - n;
				
				sllimm(r, n);
				sraimm(r, n);
			}
		}
		
		void cmp (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 39 /r	CMP r/m32, r32		#Compare r32 with r/m32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x39;
			
			// Append the ModR/M byte.
			appendmodrmfor2reg(r2, r1);
		}
		
		void seq (uint r1, uint r2) {
			// r1 must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r1 < EAX || r1 > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != EAX) rvalid = EAX;
					else rvalid = EBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// 0F 94	SETE r/m8	#Set byte if equal (ZF=1).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x94;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r1 < EAX || r1 > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != EAX) rvalid = EAX;
					else rvalid = EBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// 0F 95	SETNE r/m8	#Set byte if not equal (ZF=0).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x95;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			if (b->isimmused == IMM32) {
				// Specification from the Intel manual.
				// 81 /7 id	CMP r/m32, imm32	#Compare imm32 with r/m32.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x81;
				
				// Append the ModR/M byte.
				appendmodrmfor1reg(7, r);
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				// Specification from the Intel manual.
				// 83 /7 ib	CMP r/m32, imm8	#Compare imm8 (sign-extended to 32bits) with r/m32.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x83;
				
				// Append the ModR/M byte.
				appendmodrmfor1reg(7, r);
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void seqi (uint r) {
			// r must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r < EAX || r > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = EAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// 0F 94	SETE r/m8	#Set byte if equal (ZF=1).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x94;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r < EAX || r > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = EAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// 0F 95	SETNE r/m8	#Set byte if not equal (ZF=0).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x95;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r1 < EAX || r1 > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != EAX) rvalid = EAX;
					else rvalid = EBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// 0F 9C	SETL r/m8	#Set byte if less (SF≠OF).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x9c;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r1 < EAX || r1 > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != EAX) rvalid = EAX;
					else rvalid = EBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// 0F 9E	SETLE r/m8	#Set byte if less or equal (ZF=1 or SF≠OF).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x9e;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r1 < EAX || r1 > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != EAX) rvalid = EAX;
					else rvalid = EBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// 0F 92	SETB r/m8	#Set byte if below (CF=1).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x92;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r1 < EAX || r1 > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != EAX) rvalid = EAX;
					else rvalid = EBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// 0F 96	SETBE r/m8	#Set byte if below or equal (CF=1 or ZF=1).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x96;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r1 < EAX || r1 > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != EAX) rvalid = EAX;
					else rvalid = EBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// 0F 9F	SETG r/m8	#Set byte if greater (ZF=0 and SF=OF).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x9f;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r1 < EAX || r1 > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != EAX) rvalid = EAX;
					else rvalid = EBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// 0F 9D	SETGE r/m8	#Set byte if greater or equal (SF=OF).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x9d;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r1 < EAX || r1 > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != EAX) rvalid = EAX;
					else rvalid = EBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// 0F 97	SETA r/m8	#Set byte if above (CF=0 and ZF=0).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x97;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r1 < EAX || r1 > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != EAX) rvalid = EAX;
					else rvalid = EBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			cmp(rvalid, r2);
			
			// Specification from the Intel manual.
			// 0F 93	SETAE r/m8	#Set byte if above or equal (CF=0).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x93;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r < EAX || r > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = EAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// 0F 9C	SETL r/m8	#Set byte if less (SF≠OF).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x9c;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r < EAX || r > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = EAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// 0F 9E	SETLE r/m8	#Set byte if less or equal (ZF=1 or SF≠OF).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x9e;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r < EAX || r > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = EAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// 0F 92	SETB r/m8	#Set byte if below (CF=1).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x92;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r < EAX || r > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = EAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// 0F 96	SETBE r/m8	#Set byte if below or equal (CF=1 or ZF=1).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x96;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r < EAX || r > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = EAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// 0F 9F	SETG r/m8	#Set byte if greater (ZF=0 and SF=OF).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x9f;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r < EAX || r > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = EAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// 0F 9D	SETGE r/m8	#Set byte if greater or equal (SF=OF).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x9d;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r < EAX || r > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = EAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// 0F 97	SETA r/m8	#Set byte if above (CF=0 and ZF=0).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x97;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r < EAX || r > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = EAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmpi(rvalid);
			
			// Specification from the Intel manual.
			// 0F 93	SETAE r/m8	#Set byte if above or equal (CF=0).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x93;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// 85 /r	TEST r/m64, r64		# AND r32 with r/m32; set SF, ZF, PF according to result.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x85;
			
			// Append the ModR/M byte.
			appendmodrmfor2reg(r, r);
		}
		
		void sz (uint r) {
			// r must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r < EAX || r > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = EAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmptozero(rvalid);
			
			// Specification from the Intel manual.
			// 0F 94	SETZ r/m8	#Set byte if zero (ZF=1).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x94;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
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
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r < EAX || r > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = EAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			cmptozero(rvalid);
			
			// Specification from the Intel manual.
			// 0F 95	SETNZ r/m8	#Set byte if not zero (ZF=0).
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0x95;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, rvalid);
			
			// I zero extend the 8 bits result.
			zxt(rvalid, 8);
			
			if (rvalid != r) {
				
				cpy(r, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void jeq (uint r1, uint r2) {
			
			cmp(r1, r2);
			
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM32) {
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
		
		void jr (uint r) {
			// Specification from the Intel manual.
			// FF /4	JMP r/m32	#Jump absolute.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xff;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(4, r);
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
			// The x86 instruction set
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
			if (b->isimmused == IMM32) {
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
			if (b->isimmused == IMM32) {
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
			if (b->isimmused == IMM32) {
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
			if (b->isimmused == IMM32) {
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
			if (b->isimmused == IMM32) {
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
			if (b->isimmused == IMM32) {
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
			if (b->isimmused == IMM32) {
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
		
		void j () {
			// If b->isimmused is set, then
			// I am redoing the lyricalinstruction.
			if (b->isimmused == IMM32) {
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
				*arrayu8append1(&b->binary) = 0xeb;
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void jl (uint r) {
			// In the x86 32bits instruction set,
			// there is no single instruction
			// to obtain an address relative to
			// the next instruction address; so
			// I use a combination of instructions.
			
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
			
			// Increment r by the size of pop(),
			// the increment instruction itself,
			// and the branching instruction.
			
			// Specification from the Intel manual.
			// 83 /0 ib	ADD r/m32, imm8	#Add sign-extended imm8 to r/m32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x83;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, r);
			
			// Append 8bits immediate field.
			u8* imm = arrayu8append1(&b->binary);
			
			j();
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jlr (uint r1, uint r2) {
			// In the x86 32bits instruction set,
			// there is no single instruction
			// to obtain an address relative to
			// the next instruction address; so
			// I use a combination of instructions.
			
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
			
			// Pop in r1 the return address
			// which is the address of
			// the POP instruction itself.
			pop(r1);
			
			// Increment r1 by the size of pop(),
			// the increment instruction itself,
			// and the branching instruction.
			
			// Specification from the Intel manual.
			// 83 /0 ib	ADD r/m32, imm8	#Add sign-extended imm8 to r/m32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x83;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(0, r1);
			
			// Append 8bits immediate field.
			u8* imm = arrayu8append1(&b->binary);
			
			jr(r2);
			
			bbinaryptr = b->binary.ptr;
			// Set the 8bits immediate field appended above.
			*imm = (mmsz(bbinaryptr) - ((uint)opstartptr - (uint)bbinaryptr));
		}
		
		void jli (uint r) {
			// The x86 instruction set
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
		
		void jpush () {
			// Specification from the Intel manual.
			// E8 cd	CALL rel32	#Call relative.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xe8;
			
			// Append the immediate value.
			append32bitsimm(0);
		}
		
		void jpushr (uint r) {
			// Specification from the Intel manual.
			// FF /2	CALL r/m32	#Call absolute.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xff;
			
			// Append the ModR/M byte.
			appendmodrmfor1reg(2, r);
		}
		
		void jpushi () {
			// The x86 instruction set
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
		
		void afip (uint r) {
			// In the x86 32bits instruction set,
			// there is no single instruction
			// to obtain an address relative to
			// the next instruction address; so
			// I use a combination of instructions
			
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
		}
		
		void afip2 (uint r) {
			// In the x86 32bits instruction set,
			// there is no single instruction
			// to obtain an address relative to
			// the next instruction address; so
			// I use a combination of instructions
			
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
			
			// Increment the immediate misc value
			// to account for instructions from
			// after the CALL instruction.
			b->imm2miscvalue += (mmsz(b->binary.ptr) - mmszbbinaryptr);
		}
		
		void ld8r (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 0F B6 /r	MOVZX r32, r/m8		#Move r/m8 to r32, zero-extension.
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0xb6;
			
			if (r2 != ESP && r2 != EBP)
				modrmfor2regformemaccess(r1, r2);
			else {
				// Append the ModR/M and SIB bytes.
				appendmodrmsib8bitsimm(r1, r2);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
		}
		
		void ld8 (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 0F B6 /r	MOVZX r32, r/m8		#Move r/m8 to r32, zero-extension.
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0xb6;
			
			if (b->isimmused == IMM32) {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith32imm(r1, r2);
				else appendmodrmsib32bitsimm(r1, r2);
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith8imm(r1, r2);
				else appendmodrmsib8bitsimm(r1, r2);
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void ld8i (uint r) {
			// Specification from the Intel manual.
			// 0F B6 /r	MOVZX r32, r/m8		#Move r/m8 to r32, zero-extension.
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0xb6;
			
			// Append the ModR/M byte.
			appendmodrmimmaddr(r);
			
			// Append the immediate value.
			append32bitsimm(0);
		}
		
		void ld16r (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 0F B7 /r	MOVZX r32, r/m16	#Move r/m16 to r32, zero-extension.
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0xb7;
			
			if (r2 != ESP && r2 != EBP)
				modrmfor2regformemaccess(r1, r2);
			else {
				// Append the ModR/M and SIB bytes.
				appendmodrmsib8bitsimm(r1, r2);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
		}
		
		void ld16 (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 0F B7 /r	MOVZX r32, r/m16	#Move r/m16 to r32, zero-extension.
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0xb7;
			
			if (b->isimmused == IMM32) {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith32imm(r1, r2);
				else appendmodrmsib32bitsimm(r1, r2);
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith8imm(r1, r2);
				else appendmodrmsib8bitsimm(r1, r2);
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void ld16i (uint r) {
			// Specification from the Intel manual.
			// 0F B7 /r	MOVZX r32, r/m16	#Move r/m16 to r32, zero-extension.
			
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0x0f;
			*arrayu8append1(&b->binary) = 0xb7;
			
			// Append the ModR/M byte.
			appendmodrmimmaddr(r);
			
			// Append the immediate value.
			append32bitsimm(0);
		}
		
		void ld32r (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 8B /r	MOV r32,r/m32		#Move r/m32 to r32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x8b;
			
			if (r2 != ESP && r2 != EBP)
				modrmfor2regformemaccess(r1, r2);
			else {
				// Append the ModR/M and SIB bytes.
				appendmodrmsib8bitsimm(r1, r2);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
		}
		
		void ld32 (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 8B /r	MOV r32,r/m32		#Move r/m32 to r32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x8b;
			
			if (b->isimmused == IMM32) {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith32imm(r1, r2);
				else appendmodrmsib32bitsimm(r1, r2);
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith8imm(r1, r2);
				else appendmodrmsib8bitsimm(r1, r2);
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void ld32i (uint r) {
			// Specification from the Intel manual.
			// 8B /r	MOV r32,r/m32		#Move r/m32 to r32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x8b;
			
			// Append the ModR/M byte.
			appendmodrmimmaddr(r);
			
			// Append the immediate value.
			append32bitsimm(0);
		}
		
		void st8r (uint r1, uint r2) {
			// r1 must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r1 < EAX || r1 > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != EAX) rvalid = EAX;
					else rvalid = EBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			// Specification from the Intel manual.
			// 88 /r	MOV r/m8,r8		#Move r8 to r/m8.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x88;
			
			if (r2 != ESP && r2 != EBP)
				modrmfor2regformemaccess(rvalid, r2);
			else {
				// Append the ModR/M and SIB bytes.
				appendmodrmsib8bitsimm(rvalid, r2);
				
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
			// r1 must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r1 < EAX || r1 > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != EAX) rvalid = EAX;
					else rvalid = EBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			// Specification from the Intel manual.
			// 88 /r	MOV r/m8,r8		#Move r8 to r/m8.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x88;
			
			if (b->isimmused == IMM32) {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith32imm(rvalid, r2);
				else appendmodrmsib32bitsimm(rvalid, r2);
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith8imm(rvalid, r2);
				else appendmodrmsib8bitsimm(rvalid, r2);
				
				// Append the immediate value.
				append8bitsimm(0);
			}
			
			if (rvalid != r1) {
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void st8i (uint r) {
			// r must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r < EAX || r > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = EAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			// Specification from the Intel manual.
			// 88 /r	MOV r/m8,r8		#Move r8 to r/m8.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x88;
			
			// Append the ModR/M byte.
			appendmodrmimmaddr(rvalid);
			
			// Append the immediate value.
			append32bitsimm(0);
			
			if (rvalid != r) {
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void st16r (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 89 /r	MOV r/m16,r16		#Move r16 to r/m16.
			
			// Append the prefix byte 0x66
			// to consider operands as 16bits.
			*arrayu8append1(&b->binary) = 0x66;
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x89;
			
			if (r2 != ESP && r2 != EBP)
				modrmfor2regformemaccess(r1, r2);
			else {
				// Append the ModR/M and SIB bytes.
				appendmodrmsib8bitsimm(r1, r2);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
		}
		
		void st16 (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 89 /r	MOV r/m16,r16		#Move r16 to r/m16.
			
			// Append the prefix byte 0x66
			// to consider operands as 16bits.
			*arrayu8append1(&b->binary) = 0x66;
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x89;
			
			if (b->isimmused == IMM32) {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith32imm(r1, r2);
				else appendmodrmsib32bitsimm(r1, r2);
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith8imm(r1, r2);
				else appendmodrmsib8bitsimm(r1, r2);
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void st16i (uint r) {
			// Specification from the Intel manual.
			// 89 /r	MOV r/m16,r16		#Move r16 to r/m16.
			
			// Append the prefix byte 0x66
			// to consider operands as 16bits.
			*arrayu8append1(&b->binary) = 0x66;
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x89;
			
			// Append the ModR/M byte.
			appendmodrmimmaddr(r);
			
			// Append the immediate value.
			append32bitsimm(0);
		}
		
		void st32r (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 89 /r	MOV r/m32,r32		#Move r32 to r/m32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x89;
			
			if (r2 != ESP && r2 != EBP)
				modrmfor2regformemaccess(r1, r2);
			else {
				// Append the ModR/M and SIB bytes.
				appendmodrmsib8bitsimm(r1, r2);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
		}
		
		void st32 (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 89 /r	MOV r/m32,r32		#Move r32 to r/m32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x89;
			
			if (b->isimmused == IMM32) {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith32imm(r1, r2);
				else appendmodrmsib32bitsimm(r1, r2);
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith8imm(r1, r2);
				else appendmodrmsib8bitsimm(r1, r2);
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void st32i (uint r) {
			// Specification from the Intel manual.
			// 89 /r	MOV r/m32,r32		#Move r32 to r/m32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x89;
			
			// Append the ModR/M byte.
			appendmodrmimmaddr(r);
			
			// Append the immediate value.
			append32bitsimm(0);
		}
		
		void ldst8r (uint r1, uint r2) {
			// r1 must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r1 < EAX || r1 > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != EAX) rvalid = EAX;
					else rvalid = EBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			// Specification from the Intel manual.
			// 86 /r	XCHG r/m8, r8		#Exchange r8 with byte from r/m8.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x86;
			
			if (r2 != ESP && r2 != EBP)
				modrmfor2regformemaccess(rvalid, r2);
			else {
				// Append the ModR/M and SIB bytes.
				appendmodrmsib8bitsimm(rvalid, r2);
				
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
			// r1 must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r1 < EAX || r1 > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r1 was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					if (r2 != EAX) rvalid = EAX;
					else rvalid = EBX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r1);
				
			} else rvalid = r1;
			
			// Specification from the Intel manual.
			// 86 /r	XCHG r/m8, r8		#Exchange r8 with byte from r/m8.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x86;
			
			if (b->isimmused == IMM32) {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith32imm(rvalid, r2);
				else appendmodrmsib32bitsimm(rvalid, r2);
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith8imm(rvalid, r2);
				else appendmodrmsib8bitsimm(rvalid, r2);
				
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
		
		void ldst8i (uint r) {
			// r must be a register that
			// can be encoded in the field reg
			// of an instruction ModR/M when
			// it is used as an 8bits operand;
			// either EAX, EBX, ECX, EDX;
			
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
			// of EAX, EBX, ECX, EDX.
			if (r < EAX || r > EDX) {
				
				if (!(rvalid = findunusedregfor8bitsoperand(0))) {
					// I get here if r was not
					// either of EAX, EBX, ECX, EDX
					// and I could not find a valid
					// unused register to use.
					
					rvalid = EAX;
					
					savereg(rvalid); rvalidwassaved = 1;
					
				} else regsinusetmp[rvalid] = 1; // Mark unused register as temporarily used.
				
				cpy(rvalid, r);
				
			} else rvalid = r;
			
			// Specification from the Intel manual.
			// 86 /r	XCHG r/m8, r8		#Exchange r8 with byte from r/m8.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x86;
			
			// Append the ModR/M byte.
			appendmodrmimmaddr(rvalid);
			
			// Append the immediate value.
			append32bitsimm(0);
			
			if (rvalid != r) {
				
				cpy(r, rvalid);
				
				if (rvalidwassaved) restorereg(rvalid);
				// Unmark register as temporarily used.
				else regsinusetmp[rvalid] = 0;
			}
		}
		
		void ldst16r (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 87 /r	XCHG r/m16, r16		#Exchange r16 with doubleword from r/m16.
			
			// Append the prefix byte 0x66
			// to consider operands as 16bits.
			*arrayu8append1(&b->binary) = 0x66;
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x87;
			
			if (r2 != ESP && r2 != EBP)
				modrmfor2regformemaccess(r1, r2);
			else {
				// Append the ModR/M and SIB bytes.
				appendmodrmsib8bitsimm(r1, r2);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
		}
		
		void ldst16 (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 87 /r	XCHG r/m16, r16		#Exchange r16 with doubleword from r/m16.
			
			// Append the prefix byte 0x66
			// to consider operands as 16bits.
			*arrayu8append1(&b->binary) = 0x66;
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x87;
			
			if (b->isimmused == IMM32) {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith32imm(r1, r2);
				else appendmodrmsib32bitsimm(r1, r2);
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith8imm(r1, r2);
				else appendmodrmsib8bitsimm(r1, r2);
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void ldst16i (uint r) {
			// Specification from the Intel manual.
			// 87 /r	XCHG r/m16, r16		#Exchange r16 with doubleword from r/m16.
			
			// Append the prefix byte 0x66
			// to consider operands as 16bits.
			*arrayu8append1(&b->binary) = 0x66;
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x87;
			
			// Append the ModR/M byte.
			appendmodrmimmaddr(r);
			
			// Append the immediate value.
			append32bitsimm(0);
		}
		
		void ldst32r (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 87 /r	XCHG r/m32, r32		#Exchange r32 with doubleword from r/m32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x87;
			
			if (r2 != ESP && r2 != EBP)
				modrmfor2regformemaccess(r1, r2);
			else {
				// Append the ModR/M and SIB bytes.
				appendmodrmsib8bitsimm(r1, r2);
				
				// Append an 8bits immediate value of zero.
				*arrayu8append1(&b->binary) = 0;
			}
		}
		
		void ldst32 (uint r1, uint r2) {
			// Specification from the Intel manual.
			// 87 /r	XCHG r/m32, r32		#Exchange r32 with doubleword from r/m32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x87;
			
			if (b->isimmused == IMM32) {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith32imm(r1, r2);
				else appendmodrmsib32bitsimm(r1, r2);
				
				// Append the immediate value.
				append32bitsimm(0);
				
			} else {
				
				if (r2 != ESP)
					appendmodrmfor2regformemaccesswith8imm(r1, r2);
				else appendmodrmsib8bitsimm(r1, r2);
				
				// Append the immediate value.
				append8bitsimm(0);
			}
		}
		
		void ldst32i (uint r) {
			// Specification from the Intel manual.
			// 87 /r	XCHG r/m32, r32		#Exchange r32 with doubleword from r/m32.
			
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0x87;
			
			// Append the ModR/M byte.
			appendmodrmimmaddr(r);
			
			// Append the immediate value.
			append32bitsimm(0);
		}
		
		void mem8cpy (uint r1, uint r2, uint r3) {
			// r1, r2, r3 are assumed
			// different registers.
			
			uint ediwassaved = 0;
			uint esiwassaved = 0;
			uint ecxwassaved = 0;
			
			if (r1 != EDI && r2 != EDI && r3 != EDI) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDI)) {
					savereg(EDI); ediwassaved = 1;
				} else regsinusetmp[EDI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != ESI && r2 != ESI && r3 != ESI) {
				// Save register value
				// if it is being used.
				if (isreginuse(ESI)) {
					savereg(ESI); esiwassaved = 1;
				} else regsinusetmp[ESI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != ECX && r2 != ECX && r3 != ECX) {
				// Save register value
				// if it is being used.
				if (isreginuse(ECX)) {
					savereg(ECX); ecxwassaved = 1;
				} else regsinusetmp[ECX] = 1; // Mark unused register as temporarily used.
			}
			
			// Set EDI, ESI, ECX
			// respectively
			// to r1, r2, r3 by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r3 != ECX) push(r3);
			if (r2 != ESI) push(r2);
			if (r1 != EDI) {
				push(r1);
				pop(EDI);
			}
			if (r2 != ESI) pop(ESI);
			if (r3 != ECX) pop(ECX);
			
			// Clear direction flag to
			// select incremental copy.
			
			// Specification from the Intel manual.
			// FC	CLD	#Clear DF Flag.
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xfc;
			
			// Repeat the instruction movsb
			// until ECX become null.
			
			// Specification from the Intel manual.
			// F3 A4	MOVSB		#Move 8bits data from address ESI to address EDI.
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0xf3;
			*arrayu8append1(&b->binary) = 0xa4;
			
			// Set r1, r2
			// respectively
			// to ESI, EDI by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r2 != ESI) push(ESI);
			if (r1 != EDI) {
				push(EDI);
				pop(r1);
			}
			if (r2 != ESI) pop(r2);
			
			// If r3 != ECX, the value
			// of r3 is set null because
			// ECX is certainly null.
			if (r3 != ECX) xor(r3, r3);
			
			if (ecxwassaved) {
				// Restore register value.
				restorereg(ECX);
			// Unmark register as temporarily used.
			} else regsinusetmp[ECX] = 0;
			
			if (esiwassaved) {
				// Restore register value.
				restorereg(ESI);
			// Unmark register as temporarily used.
			} else regsinusetmp[ESI] = 0;
			
			if (ediwassaved) {
				// Restore register value.
				restorereg(EDI);
			// Unmark register as temporarily used.
			} else regsinusetmp[EDI] = 0;
		}
		
		void mem8cpy2 (uint r1, uint r2, uint r3) {
			// r1, r2, r3 are assumed
			// different registers.
			
			uint ediwassaved = 0;
			uint esiwassaved = 0;
			uint ecxwassaved = 0;
			
			if (r1 != EDI && r2 != EDI && r3 != EDI) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDI)) {
					savereg(EDI); ediwassaved = 1;
				} else regsinusetmp[EDI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != ESI && r2 != ESI && r3 != ESI) {
				// Save register value
				// if it is being used.
				if (isreginuse(ESI)) {
					savereg(ESI); esiwassaved = 1;
				} else regsinusetmp[ESI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != ECX && r2 != ECX && r3 != ECX) {
				// Save register value
				// if it is being used.
				if (isreginuse(ECX)) {
					savereg(ECX); ecxwassaved = 1;
				} else regsinusetmp[ECX] = 1; // Mark unused register as temporarily used.
			}
			
			// Set EDI, ESI, ECX
			// respectively
			// to r1, r2, r3 by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r3 != ECX) push(r3);
			if (r2 != ESI) push(r2);
			if (r1 != EDI) {
				push(r1);
				pop(EDI);
			}
			if (r2 != ESI) pop(ESI);
			if (r3 != ECX) pop(ECX);
			
			// Set direction flag to
			// select decremental copy.
			
			// Specification from the Intel manual.
			// FD	STD	#Set DF Flag.
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xfd;
			
			// Repeat the instruction movsb
			// until ECX become null.
			
			// Specification from the Intel manual.
			// F3 A4	MOVSB		#Move 8bits data from address ESI to address EDI.
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0xf3;
			*arrayu8append1(&b->binary) = 0xa4;
			
			// Set r1, r2
			// respectively
			// to ESI, EDI by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r2 != ESI) push(ESI);
			if (r1 != EDI) {
				push(EDI);
				pop(r1);
			}
			if (r2 != ESI) pop(r2);
			
			// If r3 != ECX, the value
			// of r3 is set null because
			// ECX is certainly null.
			if (r3 != ECX) xor(r3, r3);
			
			if (ecxwassaved) {
				// Restore register value.
				restorereg(ECX);
			// Unmark register as temporarily used.
			} else regsinusetmp[ECX] = 0;
			
			if (esiwassaved) {
				// Restore register value.
				restorereg(ESI);
			// Unmark register as temporarily used.
			} else regsinusetmp[ESI] = 0;
			
			if (ediwassaved) {
				// Restore register value.
				restorereg(EDI);
			// Unmark register as temporarily used.
			} else regsinusetmp[EDI] = 0;
		}
		
		void mem16cpy (uint r1, uint r2, uint r3) {
			// r1, r2, r3 are assumed
			// different registers.
			
			uint ediwassaved = 0;
			uint esiwassaved = 0;
			uint ecxwassaved = 0;
			
			if (r1 != EDI && r2 != EDI && r3 != EDI) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDI)) {
					savereg(EDI); ediwassaved = 1;
				} else regsinusetmp[EDI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != ESI && r2 != ESI && r3 != ESI) {
				// Save register value
				// if it is being used.
				if (isreginuse(ESI)) {
					savereg(ESI); esiwassaved = 1;
				} else regsinusetmp[ESI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != ECX && r2 != ECX && r3 != ECX) {
				// Save register value
				// if it is being used.
				if (isreginuse(ECX)) {
					savereg(ECX); ecxwassaved = 1;
				} else regsinusetmp[ECX] = 1; // Mark unused register as temporarily used.
			}
			
			// Set EDI, ESI, ECX
			// respectively
			// to r1, r2, r3 by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r3 != ECX) push(r3);
			if (r2 != ESI) push(r2);
			if (r1 != EDI) {
				push(r1);
				pop(EDI);
			}
			if (r2 != ESI) pop(ESI);
			if (r3 != ECX) pop(ECX);
			
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
			// until ECX become null.
			
			// Specification from the Intel manual.
			// F3 A5	MOVSW		#Move 16bits data from address ESI to address EDI.
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0xf3;
			*arrayu8append1(&b->binary) = 0xa5;
			
			// Set r1, r2
			// respectively
			// to ESI, EDI by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r2 != ESI) push(ESI);
			if (r1 != EDI) {
				push(EDI);
				pop(r1);
			}
			if (r2 != ESI) pop(r2);
			
			// If r3 != ECX, the value
			// of r3 is set null because
			// ECX is certainly null.
			if (r3 != ECX) xor(r3, r3);
			
			if (ecxwassaved) {
				// Restore register value.
				restorereg(ECX);
			// Unmark register as temporarily used.
			} else regsinusetmp[ECX] = 0;
			
			if (esiwassaved) {
				// Restore register value.
				restorereg(ESI);
			// Unmark register as temporarily used.
			} else regsinusetmp[ESI] = 0;
			
			if (ediwassaved) {
				// Restore register value.
				restorereg(EDI);
			// Unmark register as temporarily used.
			} else regsinusetmp[EDI] = 0;
		}
		
		void mem16cpy2 (uint r1, uint r2, uint r3) {
			// r1, r2, r3 are assumed
			// different registers.
			
			uint ediwassaved = 0;
			uint esiwassaved = 0;
			uint ecxwassaved = 0;
			
			if (r1 != EDI && r2 != EDI && r3 != EDI) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDI)) {
					savereg(EDI); ediwassaved = 1;
				} else regsinusetmp[EDI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != ESI && r2 != ESI && r3 != ESI) {
				// Save register value
				// if it is being used.
				if (isreginuse(ESI)) {
					savereg(ESI); esiwassaved = 1;
				} else regsinusetmp[ESI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != ECX && r2 != ECX && r3 != ECX) {
				// Save register value
				// if it is being used.
				if (isreginuse(ECX)) {
					savereg(ECX); ecxwassaved = 1;
				} else regsinusetmp[ECX] = 1; // Mark unused register as temporarily used.
			}
			
			// Set EDI, ESI, ECX
			// respectively
			// to r1, r2, r3 by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r3 != ECX) push(r3);
			if (r2 != ESI) push(r2);
			if (r1 != EDI) {
				push(r1);
				pop(EDI);
			}
			if (r2 != ESI) pop(ESI);
			if (r3 != ECX) pop(ECX);
			
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
			// until ECX become null.
			
			// Specification from the Intel manual.
			// F3 A5	MOVSW		#Move 16bits data from address ESI to address EDI.
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0xf3;
			*arrayu8append1(&b->binary) = 0xa5;
			
			// Set r1, r2
			// respectively
			// to ESI, EDI by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r2 != ESI) push(ESI);
			if (r1 != EDI) {
				push(EDI);
				pop(r1);
			}
			if (r2 != ESI) pop(r2);
			
			// If r3 != ECX, the value
			// of r3 is set null because
			// ECX is certainly null.
			if (r3 != ECX) xor(r3, r3);
			
			if (ecxwassaved) {
				// Restore register value.
				restorereg(ECX);
			// Unmark register as temporarily used.
			} else regsinusetmp[ECX] = 0;
			
			if (esiwassaved) {
				// Restore register value.
				restorereg(ESI);
			// Unmark register as temporarily used.
			} else regsinusetmp[ESI] = 0;
			
			if (ediwassaved) {
				// Restore register value.
				restorereg(EDI);
			// Unmark register as temporarily used.
			} else regsinusetmp[EDI] = 0;
		}
		
		void mem32cpy (uint r1, uint r2, uint r3) {
			// r1, r2, r3 are assumed
			// different registers.
			
			uint ediwassaved = 0;
			uint esiwassaved = 0;
			uint ecxwassaved = 0;
			
			if (r1 != EDI && r2 != EDI && r3 != EDI) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDI)) {
					savereg(EDI); ediwassaved = 1;
				} else regsinusetmp[EDI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != ESI && r2 != ESI && r3 != ESI) {
				// Save register value
				// if it is being used.
				if (isreginuse(ESI)) {
					savereg(ESI); esiwassaved = 1;
				} else regsinusetmp[ESI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != ECX && r2 != ECX && r3 != ECX) {
				// Save register value
				// if it is being used.
				if (isreginuse(ECX)) {
					savereg(ECX); ecxwassaved = 1;
				} else regsinusetmp[ECX] = 1; // Mark unused register as temporarily used.
			}
			
			// Set EDI, ESI, ECX
			// respectively
			// to r1, r2, r3 by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r3 != ECX) push(r3);
			if (r2 != ESI) push(r2);
			if (r1 != EDI) {
				push(r1);
				pop(EDI);
			}
			if (r2 != ESI) pop(ESI);
			if (r3 != ECX) pop(ECX);
			
			// Clear direction flag to
			// select incremental copy.
			
			// Specification from the Intel manual.
			// FC	CLD	#Clear DF Flag.
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xfc;
			
			// Repeat the instruction movsd
			// until ECX become null.
			
			// Specification from the Intel manual.
			// F3 A5	MOVSD		#Move 32bits data from address ESI to address EDI.
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0xf3;
			*arrayu8append1(&b->binary) = 0xa5;
			
			// Set r1, r2
			// respectively
			// to ESI, EDI by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r2 != ESI) push(ESI);
			if (r1 != EDI) {
				push(EDI);
				pop(r1);
			}
			if (r2 != ESI) pop(r2);
			
			// If r3 != ECX, the value
			// of r3 is set null because
			// ECX is certainly null.
			if (r3 != ECX) xor(r3, r3);
			
			if (ecxwassaved) {
				// Restore register value.
				restorereg(ECX);
			// Unmark register as temporarily used.
			} else regsinusetmp[ECX] = 0;
			
			if (esiwassaved) {
				// Restore register value.
				restorereg(ESI);
			// Unmark register as temporarily used.
			} else regsinusetmp[ESI] = 0;
			
			if (ediwassaved) {
				// Restore register value.
				restorereg(EDI);
			// Unmark register as temporarily used.
			} else regsinusetmp[EDI] = 0;
		}
		
		void mem32cpy2 (uint r1, uint r2, uint r3) {
			// r1, r2, r3 are assumed
			// different registers.
			
			uint ediwassaved = 0;
			uint esiwassaved = 0;
			uint ecxwassaved = 0;
			
			if (r1 != EDI && r2 != EDI && r3 != EDI) {
				// Save register value
				// if it is being used.
				if (isreginuse(EDI)) {
					savereg(EDI); ediwassaved = 1;
				} else regsinusetmp[EDI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != ESI && r2 != ESI && r3 != ESI) {
				// Save register value
				// if it is being used.
				if (isreginuse(ESI)) {
					savereg(ESI); esiwassaved = 1;
				} else regsinusetmp[ESI] = 1; // Mark unused register as temporarily used.
			}
			
			if (r1 != ECX && r2 != ECX && r3 != ECX) {
				// Save register value
				// if it is being used.
				if (isreginuse(ECX)) {
					savereg(ECX); ecxwassaved = 1;
				} else regsinusetmp[ECX] = 1; // Mark unused register as temporarily used.
			}
			
			// Set EDI, ESI, ECX
			// respectively
			// to r1, r2, r3 by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r3 != ECX) push(r3);
			if (r2 != ESI) push(r2);
			if (r1 != EDI) {
				push(r1);
				pop(EDI);
			}
			if (r2 != ESI) pop(ESI);
			if (r3 != ECX) pop(ECX);
			
			// Set direction flag to
			// select decremental copy.
			
			// Specification from the Intel manual.
			// FD	STD	#Set DF Flag.
			// Append the opcode byte.
			*arrayu8append1(&b->binary) = 0xfd;
			
			// Repeat the instruction movsd
			// until ECX become null.
			
			// Specification from the Intel manual.
			// F3 A5	MOVSD		#Move 32bits data from address ESI to address EDI.
			// Append the opcode bytes.
			*arrayu8append1(&b->binary) = 0xf3;
			*arrayu8append1(&b->binary) = 0xa5;
			
			// Set r1, r2
			// respectively
			// to ESI, EDI by
			// first pushing them
			// in the stack, and
			// poping them in the
			// order they were pushed.
			if (r2 != ESI) push(ESI);
			if (r1 != EDI) {
				push(EDI);
				pop(r1);
			}
			if (r2 != ESI) pop(r2);
			
			// If r3 != ECX, the value
			// of r3 is set null because
			// ECX is certainly null.
			if (r3 != ECX) xor(r3, r3);
			
			if (ecxwassaved) {
				// Restore register value.
				restorereg(ECX);
			// Unmark register as temporarily used.
			} else regsinusetmp[ECX] = 0;
			
			if (esiwassaved) {
				// Restore register value.
				restorereg(ESI);
			// Unmark register as temporarily used.
			} else regsinusetmp[ESI] = 0;
			
			if (ediwassaved) {
				// Restore register value.
				restorereg(EDI);
			// Unmark register as temporarily used.
			} else regsinusetmp[EDI] = 0;
		}
		
		void pushi (u32 imm) {
			
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
			else {
				// Specification from the Intel manual.
				// 68		PUSH imm32	#Push imm32.
				
				// Append the opcode byte.
				*arrayu8append1(&b->binary) = 0x68;
				
				// Append the 32bits immediate value.
				*(u32*)arrayu8append2(&b->binary, sizeof(u32)) = imm;
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
						uint unusedreg = findunusedreg(EAX);
						
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
						uint unusedreg = findunusedreg(EAX);
						
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
						uint unusedreg = findunusedreg(EAX);
						
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
						uint unusedreg = findunusedreg(EAX);
						
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
						uint unusedreg = findunusedreg(ECX);
						
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
						uint unusedreg = findunusedreg(ECX);
						
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
						uint unusedreg = findunusedreg(ECX);
						
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
					
				case LYRICALOPMEM8CPY:
					
					mem8cpy(i->r1, i->r2, i->r3);
					
					break;
					
				case LYRICALOPMEM8CPYI: {
					// Look for an unused register.
					uint unusedreg = findunusedreg(ECX);
					
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
					uint unusedreg = findunusedreg(ECX);
					
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
					uint unusedreg = findunusedreg(ECX);
					
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
					uint unusedreg = findunusedreg(ECX);
					
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
				
				case LYRICALOPMEM32CPY:
					
					mem32cpy(i->r1, i->r2, i->r3);
					
					break;
					
				case LYRICALOPMEM32CPYI: {
					// Look for an unused register.
					uint unusedreg = findunusedreg(ECX);
					
					// The compiler should have insured
					// that there are enough unused
					// registers available.
					if (!unusedreg) throwerror();
					
					// Mark unused register as temporarily used.
					regsinusetmp[unusedreg] = 1;
					
					ldi(unusedreg, 0);
					
					mem32cpy(i->r1, i->r2, unusedreg);
					
					// Unmark register as temporarily used.
					regsinusetmp[unusedreg] = 0;
				
					break;
				}
				
				case LYRICALOPMEM32CPY2:
					
					mem32cpy2(i->r1, i->r2, i->r3);
					
					break;
					
				case LYRICALOPMEM32CPYI2: {
					// Look for an unused register.
					uint unusedreg = findunusedreg(ECX);
					
					// The compiler should have insured
					// that there are enough unused
					// registers available.
					if (!unusedreg) throwerror();
					
					// Mark unused register as temporarily used.
					regsinusetmp[unusedreg] = 1;
					
					ldi(unusedreg, 0);
					
					mem32cpy2(i->r1, i->r2, unusedreg);
					
					// Unmark register as temporarily used.
					regsinusetmp[unusedreg] = 0;
				
					break;
				}
				
				#if defined(LYRICALX86LINUX) && defined(__linux__)
				
				case LYRICALOPPAGEALLOC: {
					// Generate the instructions
					// to use the syscall mmap():
					// void* mmap(void* addr, uint length, uint prot,
					//      uint flags, uint fd, uint offset);
					// Note that there must be enough
					// stack space (6*sizeof(u32) == 24 bytes)
					// to push the 6 arguments
					// needed by the syscall.
					
					uint r1 = i->r1;
					uint r2 = i->r2;
					
					uint eaxwassaved = 0;
					uint ebxwassaved = 0;
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(EAX) && r1 != EAX) {
						savereg(EAX); eaxwassaved = 1;
					} else regsinusetmp[EAX] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(EBX) && r1 != EBX) {
						savereg(EBX); ebxwassaved = 1;
					} else regsinusetmp[EBX] = 1; // Mark unused register as temporarily used.
					
					// Compute in EAX or EBX the byte count
					// equivalent of the page count.
					if (r2 == EBX) sllimm(EBX, clog2(PAGESIZE));
					else {
						if (r2 != EAX) cpy(EAX, r2);
						
						sllimm(EAX, clog2(PAGESIZE));
					}
					
					// Arguments to mmap() get
					// pushed in the stack from
					// the last to first argument,
					// and the pointer to the first
					// argument in the stack is
					// to be set in EBX.
					// The following push are done in order:
					// offset:      0
					// fd:          0
					// flags:       MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED
					// prot:        PROT_READ|PROT_WRITE
					// length:      EAX
					// start:       0
					pushi(0);
					pushi(0);
					pushi(MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED);
					pushi(PROT_READ|PROT_WRITE);
					if (r2 == EBX) push(EBX); else push(EAX);
					pushi(0);
					
					cpy(EBX, ESP);
					
					// I restore the stack pointer register,
					// incrementing it by the byte count
					// of all the arguments pushed.
					
					// Specification from the Intel manual.
					// 83 /0 ib     ADD r/m32, imm8         #Add sign-extended imm8 to r/m32.
					
					// Append the opcode byte.
					*arrayu8append1(&b->binary) = 0x83;
					
					// Append the ModR/M byte.
					appendmodrmfor1reg(0, ESP);
					
					// Append the 8bits immediate value.
					*arrayu8append1(&b->binary) = (6*sizeof(u32));
					
					// Set EAX to 90 which
					// is the value for
					// the mmap() syscall.
					
					xor(EAX, EAX);
					
					// Specification from the Intel manual.
					// B0+rb        MOV r8, imm8            #Move imm8 to r8.
					
					// Append the opcode byte.
					*arrayu8append1(&b->binary) = (0xb0+lookupreg(EAX));
					
					// Append the immediate value.
					*arrayu8append1(&b->binary) = 90;
					
					// Append the instruction INT80.
					*arrayu8append1(&b->binary) = 0xcd;
					*arrayu8append1(&b->binary) = 0x80;
					
					if (r1 != EAX) cpy(r1, EAX);
					
					if (ebxwassaved) {
						// Restore register value.
						restorereg(EBX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[EBX] = 0;
					
					if (eaxwassaved) {
						// Restore register value.
						restorereg(EAX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[EAX] = 0;
					
					break;
				}
				
				case LYRICALOPPAGEALLOCI:
				case LYRICALOPSTACKPAGEALLOC: {
					// Generate the instructions
					// to use the syscall mmap():
					// void* mmap(void* addr, uint length, uint prot,
					//      uint flags, uint fd, uint offset);
					// Note that there must be enough
					// stack space (6*sizeof(u32) == 24 bytes)
					// to push the 6 arguments
					// needed by the syscall.
					
					uint r1 = i->r1;
					
					uint eaxwassaved = 0;
					uint ebxwassaved = 0;
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(EAX) && r1 != EAX) {
						savereg(EAX); eaxwassaved = 1;
					} else regsinusetmp[EAX] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used
					// and it is not r1
					// which is to contain
					// the result.
					if (isreginuse(EBX) && r1 != EBX) {
						savereg(EBX); ebxwassaved = 1;
					} else regsinusetmp[EBX] = 1; // Mark unused register as temporarily used.
					
					// Arguments to mmap() get
					// pushed in the stack from
					// the last to first argument,
					// and the pointer to the first
					// argument in the stack is
					// to be set in EBX.
					// The following push are done in order:
					// offset:      0
					// fd:          0
					// flags:       MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED|((i->op == LYRICALOPSTACKPAGEALLOC) ? MAP_STACK : 0)
					// prot:        PROT_READ|PROT_WRITE
					// length:      (i->op == LYRICALOPSTACKPAGEALLOC) ? PAGESIZE : (i->imm->n*PAGESIZE)
					// start:       0
					pushi(0);
					pushi(0);
					pushi(MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED|((i->op == LYRICALOPSTACKPAGEALLOC) ? MAP_STACK : 0));
					pushi(PROT_READ|PROT_WRITE);
					// With LYRICALOPPAGEALLOCI, a single lyricalimmval
					// of type LYRICALIMMVALUE is guaranteed to have been used.
					pushi((i->op == LYRICALOPSTACKPAGEALLOC) ? PAGESIZE : ((*(u32*)&i->imm->n)*PAGESIZE));
					pushi(0);
					
					cpy(EBX, ESP);
					
					// I restore the stack pointer register,
					// incrementing it by the byte count
					// of all the arguments pushed.
					
					// Specification from the Intel manual.
					// 83 /0 ib     ADD r/m32, imm8         #Add sign-extended imm8 to r/m32.
					
					// Append the opcode byte.
					*arrayu8append1(&b->binary) = 0x83;
					
					// Append the ModR/M byte.
					appendmodrmfor1reg(0, ESP);
					
					// Append the 8bits immediate value.
					*arrayu8append1(&b->binary) = (6*sizeof(u32));
					
					// Set EAX to 90 which
					// is the value for
					// the mmap() syscall.
					
					xor(EAX, EAX);
					
					// Specification from the Intel manual.
					// B0+rb        MOV r8, imm8            #Move imm8 to r8.
					
					// Append the opcode byte.
					*arrayu8append1(&b->binary) = (0xb0+lookupreg(EAX));
					
					// Append the immediate value.
					*arrayu8append1(&b->binary) = 90;
					
					// Append the instruction INT80.
					*arrayu8append1(&b->binary) = 0xcd;
					*arrayu8append1(&b->binary) = 0x80;
					
					if (r1 != EAX) cpy(r1, EAX);
					
					if (ebxwassaved) {
						// Restore register value.
						restorereg(EBX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[EBX] = 0;
					
					if (eaxwassaved) {
						// Restore register value.
						restorereg(EAX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[EAX] = 0;
					
					break;
				}
				
				case LYRICALOPPAGEFREE: {
					// Generate the instructions
					// to use the syscall munmap():
					// uint munmap(void* addr, uint length);
					
					uint r1 = i->r1;
					uint r2 = i->r2;
					
					uint r1r2wasxchged = 0;
					
					if (r1 != EBX && r2 == EBX) {
						// If I get here, I xchg r1 and r2
						// so as to generate the least
						// amount of intructions while
						// setting up the page address
						// in EBX.
						
						xchg(r1, r2);
						
						// r2 is made to refer
						// to the register that
						// contain its value.
						r1 ^= r2; r2 ^= r1; r1 ^= r2;
						// If r1 was ECX, then
						// the instruction cpy(ECX, r2)
						// will not get generated.
						// so exchanging r1 and r2
						// values here has the potential
						// of saving instructions.
						
						r1r2wasxchged = 1;
					}
					
					uint eaxwassaved = 0;
					uint ebxwassaved = 0;
					uint ecxwassaved = 0;
					
					// Save register value
					// if it is being used.
					if (isreginuse(EAX)) {
						savereg(EAX); eaxwassaved = 1;
					} else regsinusetmp[EAX] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used.
					if (isreginuse(EBX)) {
						savereg(EBX); ebxwassaved = 1;
					} else regsinusetmp[EBX] = 1; // Mark unused register as temporarily used.
					
					if (r1 != EBX) cpy(EBX, r1);
					
					// RoundDown to pagesize 0x1000.
					andimm(EBX, -0x1000);
					
					// Save register value
					// if it is being used.
					if (isreginuse(ECX)) {
						savereg(ECX); ecxwassaved = 1;
					} else regsinusetmp[ECX] = 1; // Mark unused register as temporarily used.
					
					// Compute in ECX the byte count
					// equivalent of the page count.
					if (r2 != ECX) cpy(ECX, r2);
					sllimm(ECX, clog2(PAGESIZE));
					
					// Set EAX to 91 which
					// is the value for
					// the munmap() syscall.
					
					xor(EAX, EAX);
					
					// Specification from the Intel manual.
					// B0+rb        MOV r8, imm8            #Move imm8 to r8.
					
					// Append the opcode byte.
					*arrayu8append1(&b->binary) = (0xb0+lookupreg(EAX));
					
					// Append the immediate value.
					*arrayu8append1(&b->binary) = 91;
					
					// Append the instruction INT80.
					*arrayu8append1(&b->binary) = 0xcd;
					*arrayu8append1(&b->binary) = 0x80;
					
					if (ecxwassaved) {
						// Restore register value.
						restorereg(ECX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[ECX] = 0;
					
					if (ebxwassaved) {
						// Restore register value.
						restorereg(EBX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[EBX] = 0;
					
					if (eaxwassaved) {
						// Restore register value.
						restorereg(EAX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[EAX] = 0;
					
					if (r1r2wasxchged) xchg(r1, r2); // Restore registers value.
					
					break;
				}
				
				case LYRICALOPPAGEFREEI:
				case LYRICALOPSTACKPAGEFREE: {
					// Generate the instructions
					// to use the syscall munmap():
					// uint munmap(void* addr, uint length);
					
					uint r1 = i->r1;
					
					uint eaxwassaved = 0;
					uint ebxwassaved = 0;
					uint ecxwassaved = 0;
					
					// Save register value
					// if it is being used.
					if (isreginuse(EAX)) {
						savereg(EAX); eaxwassaved = 1;
					} else regsinusetmp[EAX] = 1; // Mark unused register as temporarily used.
					
					// Save register value
					// if it is being used.
					if (isreginuse(EBX)) {
						savereg(EBX); ebxwassaved = 1;
					} else regsinusetmp[EBX] = 1; // Mark unused register as temporarily used.
					
					if (r1 != EBX) cpy(EBX, r1);
					
					// RoundDown to pagesize 0x1000.
					andimm(EBX, -0x1000);
					
					// Save register value
					// if it is being used.
					if (isreginuse(ECX)) {
						savereg(ECX); ecxwassaved = 1;
					} else regsinusetmp[ECX] = 1; // Mark unused register as temporarily used.
					
					// Compute in ECX the byte count
					// equivalent of the page count.
					loadimm(ECX, (i->op == LYRICALOPSTACKPAGEFREE) ? PAGESIZE : ((*(u32*)&i->imm->n)*PAGESIZE));
					
					// Set EAX to 91 which
					// is the value for
					// the munmap() syscall.
					
					xor(EAX, EAX);
					
					// Specification from the Intel manual.
					// B0+rb        MOV r8, imm8            #Move imm8 to r8.
					
					// Append the opcode byte.
					*arrayu8append1(&b->binary) = (0xb0+lookupreg(EAX));
					
					// Append the immediate value.
					*arrayu8append1(&b->binary) = 91;
					
					// Append the instruction INT80.
					*arrayu8append1(&b->binary) = 0xcd;
					*arrayu8append1(&b->binary) = 0x80;
					
					if (ecxwassaved) {
						// Restore register value.
						restorereg(ECX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[ECX] = 0;
					
					if (ebxwassaved) {
						// Restore register value.
						restorereg(EBX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[EBX] = 0;
					
					if (eaxwassaved) {
						// Restore register value.
						restorereg(EAX);
					// Unmark register as temporarily used if it was marked.
					} else regsinusetmp[EAX] = 0;
					
					break;
				}
				
				#else
				
				case LYRICALOPPAGEALLOC: {
					
					uint r1 = i->r1;
					
					pushusedregs(r1);
					
					uint r2 = i->r2;
					
					// Compute in r2 the byte count
					// equivalent of the page count.
					sllimm(r2, clog2(PAGESIZE));
					
					// The calling convention of
					// the lyrical syscall function
					// is cdecl.
					
					// Arguments to mmap() get
					// pushed in the stack from
					// the last to first argument.
					// The following push are done in order:
					// offset:      0
					// fd:          0
					// flags:       MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED
					// prot:        PROT_READ|PROT_WRITE
					// length:      r2
					// start:       0
					pushi(0);
					pushi(0);
					pushi(MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED);
					pushi(PROT_READ|PROT_WRITE);
					push(r2);
					pushi(0);
					
					pushi(LYRICALSYSCALLMMAP);
					
					// Generate instructions to retrieve
					// the lyrical syscall function address
					// and store it in r1.
					afip2(r1);
					ld32r(r1, r1);
					
					jpushr(r1);
					
					// Retrieve result from register EAX.
					if (r1 != EAX) cpy(r1, EAX);
					
					// I restore the stack pointer register,
					// incrementing it by the byte count
					// of all the arguments pushed.
					
					// Specification from the Intel manual.
					// 83 /0 ib     ADD r/m32, imm8         #Add sign-extended imm8 to r/m32.
					
					// Append the opcode byte.
					*arrayu8append1(&b->binary) = 0x83;
					
					// Append the ModR/M byte.
					appendmodrmfor1reg(0, ESP);
					
					// Append the 8bits immediate value.
					*arrayu8append1(&b->binary) = ((6+1)*sizeof(u32));
					
					popusedregs(r1);
					
					break;
				}
				
				case LYRICALOPPAGEALLOCI:
				case LYRICALOPSTACKPAGEALLOC: {
					
					uint r1 = i->r1;
					
					pushusedregs(r1);
					
					// The calling convention of
					// the lyrical syscall function
					// is cdecl.
					
					// Arguments to mmap() get
					// pushed in the stack from
					// the last to first argument.
					// The following push are done in order:
					// offset:      0
					// fd:          0
					// flags:       MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED|((i->op == LYRICALOPSTACKPAGEALLOC) ? MAP_STACK : 0)
					// prot:        PROT_READ|PROT_WRITE
					// length:      (i->op == LYRICALOPSTACKPAGEALLOC) ? PAGESIZE : (i->imm->n*PAGESIZE)
					// start:       0
					pushi(0);
					pushi(0);
					pushi(MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED|((i->op == LYRICALOPSTACKPAGEALLOC) ? MAP_STACK : 0));
					pushi(PROT_READ|PROT_WRITE);
					// With LYRICALOPPAGEALLOCI, a single lyricalimmval
					// of type LYRICALIMMVALUE is guaranteed to have been used.
					pushi((i->op == LYRICALOPSTACKPAGEALLOC) ? PAGESIZE : ((*(u32*)&i->imm->n)*PAGESIZE));
					pushi(0);
					
					pushi(LYRICALSYSCALLMMAP);
					
					// Generate instructions to retrieve
					// the lyrical syscall function address
					// and store it in r1.
					afip2(r1);
					ld32r(r1, r1);
					
					jpushr(r1);
					
					// Retrieve result from register EAX.
					if (r1 != EAX) cpy(r1, EAX);
					
					// I restore the stack pointer register,
					// incrementing it by the byte count
					// of all the arguments pushed.
					
					// Specification from the Intel manual.
					// 83 /0 ib     ADD r/m32, imm8         #Add sign-extended imm8 to r/m32.
					
					// Append the opcode byte.
					*arrayu8append1(&b->binary) = 0x83;
					
					// Append the ModR/M byte.
					appendmodrmfor1reg(0, ESP);
					
					// Append the 8bits immediate value.
					*arrayu8append1(&b->binary) = ((6+1)*sizeof(u32));
					
					popusedregs(r1);
					
					break;
				}
				
				case LYRICALOPPAGEFREE: {
					
					pushusedregs(0);
					
					uint r2 = i->r2;
					
					// Compute in r2 the byte count
					// equivalent of the page count.
					sllimm(r2, clog2(PAGESIZE));
					
					// The calling convention of
					// the lyrical syscall function
					// is cdecl.
					
					// Arguments to munmap() get
					// pushed in the stack from
					// the last to first argument.
					// The following push are done in order:
					// length:	r2
					// addr:	i->r1
					push(r2);
					push(i->r1);
					
					pushi(LYRICALSYSCALLMUNMAP);
					
					// Generate instructions to retrieve
					// the lyrical syscall function address
					// and store it in r2.
					afip2(r2);
					ld32r(r2, r2);
					
					jpushr(r2);
					
					// I restore the stack pointer register,
					// incrementing it by the byte count
					// of all the arguments pushed.
					
					// Specification from the Intel manual.
					// 83 /0 ib     ADD r/m32, imm8         #Add sign-extended imm8 to r/m32.
					
					// Append the opcode byte.
					*arrayu8append1(&b->binary) = 0x83;
					
					// Append the ModR/M byte.
					appendmodrmfor1reg(0, ESP);
					
					// Append the 8bits immediate value.
					*arrayu8append1(&b->binary) = ((2+1)*sizeof(u32));
					
					popusedregs(0);
					
					break;
				}
				
				case LYRICALOPPAGEFREEI:
				case LYRICALOPSTACKPAGEFREE: {
					
					pushusedregs(0);
					
					uint r1 = i->r1;
					
					// The calling convention of
					// the lyrical syscall function
					// is cdecl.
					
					// Arguments to munmap() get
					// pushed in the stack from
					// the last to first argument.
					// The following push are done in order:
					// length:	(i->op == LYRICALOPSTACKPAGEFREE) ? PAGESIZE : (i->imm->n*PAGESIZE)
					// addr:	r1
					pushi((i->op == LYRICALOPSTACKPAGEFREE) ? PAGESIZE : ((*(u32*)&i->imm->n)*PAGESIZE));
					push(r1);
					
					pushi(LYRICALSYSCALLMUNMAP);
					
					// Generate instructions to retrieve
					// the lyrical syscall function address
					// and store it in r1.
					afip2(r1);
					ld32r(r1, r1);
					
					jpushr(r1);
					
					// I restore the stack pointer register,
					// incrementing it by the byte count
					// of all the arguments pushed.
					
					// Specification from the Intel manual.
					// 83 /0 ib     ADD r/m32, imm8         #Add sign-extended imm8 to r/m32.
					
					// Append the opcode byte.
					*arrayu8append1(&b->binary) = 0x83;
					
					// Append the ModR/M byte.
					appendmodrmfor1reg(0, ESP);
					
					// Append the 8bits immediate value.
					*arrayu8append1(&b->binary) = ((2+1)*sizeof(u32));
					
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
	lyricalbackendx86result* retvar = mmalloc(sizeof(lyricalbackendx86result));
	
	uint compileresultstringregionsz = arrayu8sz(compileresult.stringregion);
	
	// Constant strings region size
	// which will be aligned and used
	// to generate the executable binary.
	uint constantstringssz = compileresultstringregionsz;
	
	retvar->executableinstrsz = executableinstrsz;
	retvar->constantstringssz = constantstringssz;
	retvar->globalvarregionsz = compileresult.globalregionsz;
	
	if (flag&LYRICALBACKENDX86PAGEALIGNED) {
		
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
	
	if (flag&LYRICALBACKENDX86COMPACTPAGEALIGNED) {
		
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
			do *arrayu8append1(&retvar->exportinfo) = (n>>sa); while ((sa += 8) < (sizeof(u32)*8));
			
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
			do *arrayu8append1(&retvar->importinfo) = (n>>sa); while ((sa += 8) < (sizeof(u32)*8));
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
				u32 imm2value = b->imm2miscvalue;
				
				// Calculate the relative address
				// from the next instruction.
				// The sys field is located right after
				// the fields arg and env which are at
				// the top of the global variable region.
				imm2value += ((executableinstrsz + constantstringssz + (2*sizeof(u32))) -
					(b->binaryoffset + b->imm2fieldoffset +
						((b->isimm2used == IMM32) ? sizeof(u32) : sizeof(u8))));
				
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
						// A 32bits offset is what would fit.
						
						// I set the backenddata
						// so that a 32bits offset
						// get tried instead when
						// the lyricalinstruction
						// will be redone.
						b->isimm2used = IMM32;
						
						// I save the lyricalinstruction* to redo.
						bintreeadd(&toredo, (uint)i, i);
					}
					
				} else if (b->isimm2used == IMM32) {
					// Write the computed offset value
					// in the appropriate location within
					// the binary of the instruction.
					// A 32 bits value is written.
					
					*(u32*)(b->binary.ptr + b->imm2fieldoffset) = imm2value;
					
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
				u32 immvalue = b->immmiscvalue;
				
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
						
						immvalue += *(u32*)&imm->n;
						
						break;
						
					case LYRICALIMMOFFSETTOINSTRUCTION:
						// Calculate the relative address
						// from the next instruction.
						immvalue += (((backenddata*)imm->i->backenddata)->binaryoffset -
							(b->binaryoffset + b->immfieldoffset +
								((b->isimmused == IMM32) ? sizeof(u32) : sizeof(u8))));
						
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
								((b->isimmused == IMM32) ? sizeof(u32) : sizeof(u8))));
						
						break;
						
					case LYRICALIMMOFFSETTOGLOBALREGION:
						// Calculate the relative address
						// from the next instruction.
						immvalue += ((executableinstrsz + constantstringssz) -
							(b->binaryoffset + b->immfieldoffset +
								((b->isimmused == IMM32) ? sizeof(u32) : sizeof(u8))));
						
						break;
						
					case LYRICALIMMOFFSETTOSTRINGREGION:
						// Calculate the relative address
						// from the next instruction.
						immvalue += (executableinstrsz -
							(b->binaryoffset + b->immfieldoffset +
								((b->isimmused == IMM32) ? sizeof(u32) : sizeof(u8))));
						
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
						// A 32bits immediate is what would fit.
						
						// I set the backenddata
						// so that a 32bits immediate
						// get tried instead when
						// the lyricalinstruction
						// will be redone.
						b->isimmused = IMM32;
						
						// I save the lyricalinstruction* to redo.
						bintreeadd(&toredo, (uint)i, i);
					}
					
				} else if (b->isimmused == IMM32) {
					// Write the computed immediate value
					// in the appropriate location within
					// the binary of the instruction.
					// A 32 bits value is written.
					
					*(u32*)(b->binary.ptr + b->immfieldoffset) = immvalue;
					
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
							
							uint dbginfosection2sz = arrayu32sz(dbginfosection2);
							
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
							*arrayu32append1(&dbginfosection2) = (uint)filepath;
							
							// The string null-terminating byte
							// will be included in the size.
							dbginfosection2len += mmsz(filepath);
							
							return retvar;
						}
						
						*arrayu32append1(&dbginfosection1) = b->binaryoffset; // binoffset.
						*arrayu32append1(&dbginfosection1) = dbginfofilepathoffset(); // filepath.
						*arrayu32append1(&dbginfosection1) = i->dbginfo.linenumber; // linenumber.
						*arrayu32append1(&dbginfosection1) = i->dbginfo.lineoffset; // lineoffset.
						
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
		*arrayu32append1(&dbginfosection1) = b->binaryoffset + arrayu8sz(b->binary); // binoffset.
		*arrayu32append1(&dbginfosection1) = 0; // filepath.
		*arrayu32append1(&dbginfosection1) = 0; // linenumber.
		*arrayu32append1(&dbginfosection1) = 0; // lineoffset.
		
		retvar->dbginfo.ptr = (u8*)dbginfosection1.ptr;
		
		uint retvardbginfosz = arrayu32sz(dbginfosection1) * sizeof(u32);
		
		// Save the current value of retvardbginfosz.
		uint n = retvardbginfosz;
		
		// Write the byte usage of section1
		// debug information in the first u32
		// that was reserved for that purpose.
		*(u32*)retvar->dbginfo.ptr = retvardbginfosz - sizeof(u32);
		
		// Increment the size of debug information
		// data to return by the amount to be used
		// by section2 debug information.
		// sizeof(u32) account for the space
		// needed to store the usage of section2
		// debug information.
		retvardbginfosz += dbginfosection2len + sizeof(u32);
		
		// Resize the debug information data
		// to return, by the amount needed
		// for section2 debug information.
		retvar->dbginfo.ptr = mmrealloc(retvar->dbginfo.ptr, retvardbginfosz);
		
		// Write section2 debug information.
		
		void* dbginfo = retvar->dbginfo.ptr + n;
		
		// Write the byte usage of
		// section2 debug information.
		*(u32*)dbginfo = dbginfosection2len;
		
		dbginfo += sizeof(u32);
		
		n = 0;
		
		uint dbginfosection2sz = arrayu32sz(dbginfosection2);
		
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
// returned by lyricalbackendx86().
void lyricalbackendx86resultfree (lyricalbackendx86result* o) {
	
	if (o->execbin.ptr) mmfree(o->execbin.ptr);
	
	if (o->exportinfo.ptr) mmfree(o->exportinfo.ptr);
	
	if (o->importinfo.ptr) mmfree(o->importinfo.ptr);
	
	if (o->dbginfo.ptr) mmfree(o->dbginfo.ptr);
	
	mmfree(o);
}
