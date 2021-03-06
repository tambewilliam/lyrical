
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


#ifndef LIBLYRICAL
#define LIBLYRICAL

#include <string.h>

// Structures describing the data generated by lyricalcompile().
// Some of them have a size smaller than their similar structures
// defined within the implementation of lyricalcompile().
// 
// The order in which structure members are declared
// should not be changed because they match their declaration
// in the implementation of lyricalcompile().


struct lyricalinstruction;
struct lyricalfunction;

// This structure represent the immediate value
// of an instruction. ei: addi, ld, st, etc...
// An immediate value is not always known during compilation,
// in which case the backend is to determine the immediate
// value while processing the result of the compilation.
typedef struct lyricalimmval {
	// Point to the next lyricalimmval in the linkedlist.
	// The last element of the linkedlist
	// has its field next set to null.
	struct lyricalimmval* next;
	
	// Enum values for the different types of immediate values.
	enum {
		// Used to represent the value in the field n.
		LYRICALIMMVALUE,
		
		// Used to represent a relative address to
		// the instruction pointed by the field i.
		LYRICALIMMOFFSETTOINSTRUCTION,
		
		// Used to represent a relative address to
		// the function pointed by the field f.
		LYRICALIMMOFFSETTOFUNCTION,
		
		// Used to represent a relative address to
		// the region containing the global variables.
		LYRICALIMMOFFSETTOGLOBALREGION,
		
		// Used to represent a relative address to
		// the region containing the string constants.
		LYRICALIMMOFFSETTOSTRINGREGION
		
	} type;
	
	union {
		u64 n;
		
		struct lyricalinstruction* i;
		
		struct lyricalfunction* f;
	};
	
} lyricalimmval;


// Enumerated op constants which are set
// in the field op of a lyricalinstruction.
// The backend implementation of these lyricalop
// should not make function-calls, because
// the stack pointer register may not point
// to the top of the callstack, as it could
// have been backtracked to a stackframe
// holding the tiny-stackframe of
// the currently executing function, and
// function-calls would overwrite stackframes
// above the stackframe to which the stack
// pointer register was backtracked to.
// Exceptions to the above rule are:
// LYRICALOPSTACKPAGEALLOC and LYRICALOPSTACKPAGEFREE
// which are generated by the compiler only when allocating
// a new stackframe and the stack pointer register
// is always pointing to the top of the callstack.
// If a lyricalop backend implementation beside
// LYRICALOPSTACKPAGEALLOC and LYRICALOPSTACKPAGEFREE
// make function-calls, the function where it is
// used must not use a tiny-stackframe such that
// the stack pointer register always point to
// the top of the callstack.
typedef enum {
	// Arithmetic integer.
	LYRICALOPADD,	// r1 = r2 + r3;
	LYRICALOPADDI,	// r1 = r2 + imm;
	LYRICALOPSUB,	// r1 = r2 - r3;
	LYRICALOPNEG,	// r1 = -r2;
	LYRICALOPMUL,	// r1 = r2 * r3;	#Signed multiplication.
	LYRICALOPMULH,	// r1 = r2 * r3;	#Signed high multiplication.
	LYRICALOPDIV,	// r1 = r2 / r3;	#Signed division.
	LYRICALOPMOD,	// r1 = r2 % r3;	#Signed modulo.
	LYRICALOPMULHU,	// r1 = r2 * r3;	#Unsigned high multiplication.
	LYRICALOPDIVU,	// r1 = r2 / r3;	#Unsigned division.
	LYRICALOPMODU,	// r1 = r2 % r3;	#Unsigned modulo.
	LYRICALOPMULI,	// r1 = r2 * imm;	#Signed multiplication.
	LYRICALOPMULHI,	// r1 = r2 * imm;	#Signed high multiplication.
	LYRICALOPDIVI,	// r1 = r2 / imm;	#Signed division.
	LYRICALOPMODI,	// r1 = r2 % imm;	#Signed modulo.
	LYRICALOPDIVI2,	// r1 = imm / r2;	#Signed division.
	LYRICALOPMODI2,	// r1 = imm % r2;	#Signed modulo.
	LYRICALOPMULHUI,	// r1 = r2 * imm;	#Unsigned high multiplication.
	LYRICALOPDIVUI,	// r1 = r2 / imm;	#Unsigned division.
	LYRICALOPMODUI,	// r1 = r2 % imm;	#Unsigned modulo.
	LYRICALOPDIVUI2, // r1 = imm / r2;	#Unsigned division.
	LYRICALOPMODUI2, // r1 = imm % r2;	#Unsigned modulo.
	
	// Bitwise.
	LYRICALOPAND,	// r1 = r2 & r3;
	LYRICALOPANDI,	// r1 = r2 & imm;
	LYRICALOPOR,	// r1 = r2 | r3;
	LYRICALOPORI,	// r1 = r2 | imm;
	LYRICALOPXOR,	// r1 = r2 ^ r3;
	LYRICALOPXORI,	// r1 = r2 ^ imm;
	LYRICALOPNOT,	// r1 = ~r2;
	LYRICALOPCPY,	// r1 = r2;
	LYRICALOPSLL,	// r1 = r2 << r3;	#Logical shift.
	LYRICALOPSLLI,	// r1 = r2 << imm;	#Logical shift.
	LYRICALOPSLLI2,	// r1 = imm << r2;	#Logical shift.
	LYRICALOPSRL,	// r1 = r2 >> r3;	#Logical shift.
	LYRICALOPSRLI,	// r1 = r2 >> imm;	#Logical shift.
	LYRICALOPSRLI2,	// r1 = imm >> r2;	#Logical shift.
	LYRICALOPSRA,	// r1 = r2 >> r3;	#Arithmetic shift.
	LYRICALOPSRAI,	// r1 = r2 >> imm;	#Arithmetic shift.
	LYRICALOPSRAI2,	// r1 = imm >> r2;	#Arithmetic shift.
	LYRICALOPZXT,	// Zero extended the value in r2 and store it r1; imm hold the count of least significant bits to zero extend; no operation if imm is 0.
	LYRICALOPSXT,	// Sign extended the value in r2 and store it r1; imm hold the count of least significant bits to sign extend; no operation if imm is 0.
	// For LYRICALOPZXT and LYRICALOPSXT,
	// the backends can assume that
	// a single lyricalimmval of type
	// LYRICALIMMVALUE has been used.
	
	// Test.
	LYRICALOPSEQ,	// if(r2 == r3) r1 = 1; else r1 = 0;
	LYRICALOPSNE,	// if(r2 != r3) r1 = 1; else r1 = 0;
	LYRICALOPSEQI,	// if(r2 == imm) r1 = 1; else r1 = 0;
	LYRICALOPSNEI,	// if(r2 != imm) r1 = 1; else r1 = 0;
	LYRICALOPSLT,	// if(r2 < r3) r1 = 1; else r1 = 0;	#Signed comparison.
	LYRICALOPSLTE,	// if(r2 <= r3) r1 = 1; else r1 = 0;	#Signed comparison.
	LYRICALOPSLTU,	// if(r2 < r3) r1 = 1; else r1 = 0;	#Unsigned comparison.
	LYRICALOPSLTEU,	// if(r2 <= r3) r1 = 1; else r1 = 0;	#Unsigned comparison.
	LYRICALOPSLTI,	// if(r2 < imm) r1 = 1; else r1 = 0;	#Signed comparison.
	LYRICALOPSLTEI,	// if(r2 <= imm) r1 = 1; else r1 = 0;	#Signed comparison.
	LYRICALOPSLTUI,	// if(r2 < imm) r1 = 1; else r1 = 0;	#Unsigned comparison.
	LYRICALOPSLTEUI,	// if(r2 <= imm) r1 = 1; else r1 = 0;	#Unsigned comparison.
	LYRICALOPSGTI,	// if(r2 > imm) r1 = 1; else r1 = 0;	#Signed comparison.
	LYRICALOPSGTEI,	// if(r2 >= imm) r1 = 1; else r1 = 0;	#Signed comparison.
	LYRICALOPSGTUI,	// if(r2 > imm) r1 = 1; else r1 = 0;	#Unsigned comparison.
	LYRICALOPSGTEUI,	// if(r2 >= imm) r1 = 1; else r1 = 0;	#Unsigned comparison.
	LYRICALOPSZ,	// if(!r2) r1 = 1; else r1 = 0;
	LYRICALOPSNZ,	// if(r2) r1 = 1; else r1 = 0;
	
	// Branching.
	// LYRICALOPJEQ must always start
	// the range of branching instructions,
	// as it is used to compute the range
	// of lyricalop used for branching.
	LYRICALOPJEQ,	// if (r1 == r2) goto imm;	#imm is a relative address.
	LYRICALOPJEQI,	// if (r1 == r2) goto imm;	#imm is an address.
	LYRICALOPJEQR,	// if (r1 == r2) goto r3;	#r3 contain an address.
	LYRICALOPJNE,	// if (r1 != r2) goto imm;	#imm is a relative address.
	LYRICALOPJNEI,	// if (r1 != r2) goto imm;	#imm is an address.
	LYRICALOPJNER,	// if (r1 != r2) goto r3;	#r3 contain an address.
	LYRICALOPJLT,	// if (r1 < r2) goto imm;	#imm is a relative address; signed comparison.
	LYRICALOPJLTI,	// if (r1 < r2) goto imm;	#imm is an address; signed comparison.
	LYRICALOPJLTR,	// if (r1 < r2) goto r3;	#r3 contain an address; signed comparison.
	LYRICALOPJLTE,	// if (r1 <= r2) goto imm;	#imm is a relative address; signed comparison.
	LYRICALOPJLTEI,	// if (r1 <= r2) goto imm;	#imm is an address; signed comparison.
	LYRICALOPJLTER,	// if (r1 <= r2) goto r3;	#r3 contain an address; signed comparison.
	LYRICALOPJLTU,	// if (r1 < r2) goto imm;	#imm is a relative address; unsigned comparison.
	LYRICALOPJLTUI,	// if (r1 < r2) goto imm;	#imm is an address; unsigned comparison.
	LYRICALOPJLTUR,	// if (r1 < r2) goto r3;	#r3 contain an address; unsigned comparison.
	LYRICALOPJLTEU,	// if (r1 <= r2) goto imm;	#imm is a relative address; unsigned comparison.
	LYRICALOPJLTEUI,	// if (r1 <= r2) goto imm;	#imm is an address; unsigned comparison.
	LYRICALOPJLTEUR,	// if (r1 <= r2) goto r3;	#r3 contain an address; unsigned comparison.
	LYRICALOPJZ,	// if (!r1) goto imm;	#imm is a relative address.
	LYRICALOPJZI,	// if (!r1) goto imm;	#imm is an address.
	LYRICALOPJZR,	// if (!r1) goto r2;	#r2 contain an address.
	LYRICALOPJNZ,	// if (r1) goto imm;	#imm is a relative address.
	LYRICALOPJNZI,	// if (r1) goto imm;	#imm is an address.
	LYRICALOPJNZR,	// if (r1) goto r2;	#r2 contain an address.
	LYRICALOPJ,	// goto imm;		#imm is a relative address.
	LYRICALOPJI,	// goto imm;		#imm is an address.
	LYRICALOPJR,	// goto r1;		#r1 contain an address.
	LYRICALOPJL,	// Store address of the next instruction in r1; goto imm; imm is a relative address.
	LYRICALOPJLI,	// Store address of the next instruction in r1; goto imm; imm is an address.
	LYRICALOPJLR,	// Store address of the next instruction in r1; goto r2; r2 contain an address.
	LYRICALOPJPUSH,	// Stack push address of the next instruction; goto imm; imm is a relative address.
	LYRICALOPJPUSHI,	// Stack push address of the next instruction; goto imm; imm is an address.
	LYRICALOPJPUSHR,	// Stack push address of the next instruction; goto r1; r1 contain an address.
	LYRICALOPJPOP,	// Stack pop address and continue execution from that address.
	// LYRICALOPJPOP must always terminate
	// the range of branching instructions,
	// as it is used to compute the range
	// of lyricalop used for branching.
	
	LYRICALOPAFIP,	// Store in r1 the relative address imm.
	
	LYRICALOPLI,	// r1 = imm;
	
	// Memory access load.
	LYRICALOPLD8,	// Load in r1, an 8bits value from the address (r2 + imm);
	LYRICALOPLD8R,	// Load in r1, an 8bits value from the address r2;
	LYRICALOPLD8I,	// Load in r1, an 8bits value from the address imm;
	LYRICALOPLD16,	// Load in r1, a 16bits value from the address (r2 + imm);
	LYRICALOPLD16R,	// Load in r1, a 16bits value from the address r2;
	LYRICALOPLD16I,	// Load in r1, a 16bits value from the address imm;
	LYRICALOPLD32,	// Load in r1, a 32bits value from the address (r2 + imm);
	LYRICALOPLD32R,	// Load in r1, a 32bits value from the address r2;
	LYRICALOPLD32I,	// Load in r1, a 32bits value from the address imm;
	LYRICALOPLD64,	// Load in r1, a 64bits value from the address (r2 + imm);
	LYRICALOPLD64R,	// Load in r1, a 64bits value from the address r2;
	LYRICALOPLD64I,	// Load in r1, a 64bits value from the address imm;
	
	// Memory access store.
	LYRICALOPST8,	// Store the 8 least significant bits of r1 at the address (r2 + imm);
	LYRICALOPST8R,	// Store the 8 least significant bits of r1 at the address r2;
	LYRICALOPST8I,	// Store the 8 least significant bits of r1 at the address imm;
	LYRICALOPST16,	// Store the 16 least significant bits of r1 at the address (r2 + imm);
	LYRICALOPST16R,	// Store the 16 least significant bits of r1 at the address r2;
	LYRICALOPST16I,	// Store the 16 least significant bits of r1 at the address imm;
	LYRICALOPST32,	// Store the 32 bits of r1 at the address (r2 + imm);
	LYRICALOPST32R,	// Store the 32 bits of r1 at the address r2;
	LYRICALOPST32I,	// Store the 32 bits of r1 at the address imm;
	LYRICALOPST64,	// Store the 64 bits of r1 at the address (r2 + imm);
	LYRICALOPST64R,	// Store the 64 bits of r1 at the address r2;
	LYRICALOPST64I,	// Store the 64 bits of r1 at the address imm;
	
	// Memory access atomic load and store.
	LYRICALOPLDST8,	// Atomically swap the value of the 8 least significant bits of r1 and the 8bits value at the address (r2 + imm);
	LYRICALOPLDST8R,	// Atomically swap the value of the 8 least significant bits of r1 and the 8bits value at the address r2;
	LYRICALOPLDST8I,	// Atomically swap the value of the 8 least significant bits of r1 and the 8bits value at the address imm;
	LYRICALOPLDST16,	// Atomically swap the value of the 16 least significant bits of r1 and the 16bits value at the address (r2 + imm);
	LYRICALOPLDST16R,// Atomically swap the value of the 16 least significant bits of r1 and the 16bits value at the address r2;
	LYRICALOPLDST16I,// Atomically swap the value of the 16 least significant bits of r1 and the 16bits value at the address imm;
	LYRICALOPLDST32,	// Atomically swap the value of the 32 bits of r1 and the 32bits value at the address (r2 + imm);
	LYRICALOPLDST32R,// Atomically swap the value of the 32 bits of r1 and the 32bits value at the address r2;
	LYRICALOPLDST32I,// Atomically swap the value of the 32 bits of r1 and the 32bits value at the address imm;
	LYRICALOPLDST64,	// Atomically swap the value of the 64 bits of r1 and the 64bits value at the address (r2 + imm);
	LYRICALOPLDST64R,// Atomically swap the value of the 64 bits of r1 and the 64bits value at the address r2;
	LYRICALOPLDST64I,// Atomically swap the value of the 64 bits of r1 and the 64bits value at the address imm;
	
	// Memory copy.
	// These instructions copy data
	// from the memory location in r2
	// to the memory location in r1;
	// the amount to copy is given by r3
	// or by an immediate value.
	// The value of r3 or the immediate
	// value will never be null because
	// a check is generated to insure it.
	// At the end of the copy, r1 and r2
	// hold the address of the next memory
	// locations that would have been
	// used if there was more data to copy.
	LYRICALOPMEM8CPY,	// Copy each u8 incrementing the values in r1 and r2 by sizeof(u8), while decrementing the value of r3; r1, r2, r3 are never to be the same register.
	LYRICALOPMEM8CPYI,	// Copy each u8 incrementing the values in r1 and r2 by sizeof(u8); r1, r2 are never to be the same register.
	LYRICALOPMEM8CPY2,	// Copy each u8 decrementing the values in r1 and r2 by sizeof(u8), while decrementing the value of r3; r1, r2, r3 are never to be the same register.
	LYRICALOPMEM8CPYI2,	// Copy each u8 decrementing the values in r1 and r2 by sizeof(u8); r1, r2 are never to be the same register.
	LYRICALOPMEM16CPY,	// Copy each u16 incrementing the values in r1 and r2 by sizeof(u16), while decrementing the value of r3; r1, r2, r3 are never to be the same register.
	LYRICALOPMEM16CPYI,	// Copy each u16 incrementing the values in r1 and r2 by sizeof(u16); r1, r2 are never to be the same register.
	LYRICALOPMEM16CPY2,	// Copy each u16 decrementing the values in r1 and r2 by sizeof(u16), while decrementing the value of r3; r1, r2, r3 are never to be the same register.
	LYRICALOPMEM16CPYI2,	// Copy each u16 decrementing the values in r1 and r2 by sizeof(u16); r1, r2 are never to be the same register.
	LYRICALOPMEM32CPY,	// Copy each u32 incrementing the values in r1 and r2 by sizeof(u32), while decrementing the value of r3; r1, r2, r3 are never to be the same register.
	LYRICALOPMEM32CPYI,	// Copy each u32 incrementing the values in r1 and r2 by sizeof(u32); r1, r2 are never to be the same register.
	LYRICALOPMEM32CPY2,	// Copy each u32 decrementing the values in r1 and r2 by sizeof(u32), while decrementing the value of r3; r1, r2, r3 are never to be the same register.
	LYRICALOPMEM32CPYI2,	// Copy each u32 decrementing the values in r1 and r2 by sizeof(u32); r1, r2 are never to be the same register.
	LYRICALOPMEM64CPY,	// Copy each u64 incrementing the values in r1 and r2 by sizeof(u64), while decrementing the value of r3; r1, r2, r3 are never to be the same register.
	LYRICALOPMEM64CPYI,	// Copy each u64 incrementing the values in r1 and r2 by sizeof(u64); r1, r2 are never to be the same register.
	LYRICALOPMEM64CPY2,	// Copy each u64 decrementing the values in r1 and r2 by sizeof(u64), while decrementing the value of r3; r1, r2, r3 are never to be the same register.
	LYRICALOPMEM64CPYI2,	// Copy each u64 decrementing the values in r1 and r2 by sizeof(u64); r1, r2 are never to be the same register.
	
	// These lyricalinstruction respectively allocate
	// and free pages of memory to and from the program address space.
	// The pages allocated should be readable and writable.
	// These lyricalinstruction are generated by the asm statements:
	// pagealloc, pagealloci, pagefree, pagefreei.
	LYRICALOPPAGEALLOC,	// r1 get set to the start address of the first page allocated; r2 hold the count of pages. r1 get set to -1 on failure.
	LYRICALOPPAGEALLOCI,	// r1 get set to the start address of the first page allocated; imm is the count of pages. r1 get set to -1 on failure.
	LYRICALOPPAGEFREE,	// r1 hold an address anywhere within the first page to free; r2 hold the count of pages.
	LYRICALOPPAGEFREEI,	// r1 hold an address anywhere within the first page to free; imm is the count of pages.
	// For LYRICALOPPAGE*I, the backends
	// can assume that a single lyricalimmval
	// of type LYRICALIMMVALUE has been used.
	
	// These lyricalinstruction respectively allocate
	// and free a page of memory to be used as stack
	// to and from the program address space.
	// The pages allocated should be readable and writable.
	// These lyricalinstruction are generated
	// by the compiler to dynamically allocate
	// stack memory when needed and free it
	// when no longer needed.
	LYRICALOPSTACKPAGEALLOC,	// r1 get set to the start address of the page allocated. r1 get set to -1 on failure.
	LYRICALOPSTACKPAGEFREE,		// r1 hold an address anywhere within the page to free.
	
	// This lyricalinstruction is
	// generated by the compiler
	// for a machine code string
	// used within an asm block.
	// The field opmachinecode of
	// the lyricalinstruction contain
	// the machine code string.
	// The machine code string most
	// likely contain non-printable
	// characters.
	LYRICALOPMACHINECODE,
	
	// No Operation instruction;
	// it can be safely ignored
	// by a backend.
	// It is there to give a cue
	// to the backend that branching
	// occur to the instruction
	// following the LYRICALOPNOP.
	// It is also guaranteed that
	// among instructions between
	// two LYRICALOPNOP, the first
	// instruction is the only
	// one that is a branch target;
	// hence a block of instructions
	// between two LYRICALOPNOP can
	// be treated as a block where
	// the entry point is the first
	// instruction of the block.
	// All registers have been flushed
	// at the start of a block.
	// Note that the first instruction
	// of a lyricalfunction is never
	// preceded by an LYRICALOPNOP,
	// but it is a branch target;
	// and the last instruction of
	// a lyricalfunction, although
	// never followed by an LYRICALOPNOP,
	// is the last instruction of
	// a block as previously described.
	// This enum is used as the size of the field
	// lyricalcompilearg.minunusedregcountforop,
	// and must be after the last enum used
	// for a lyricalop that use a register.
	LYRICALOPNOP,
	
	// This lyricalinstruction is used
	// by the compiler to generate
	// a comment of what it is doing.
	// The field comment of the
	// lyricalinstruction contain
	// the comment string.
	// This lyricalinstruction is
	// generated by the compiler
	// only if the compileflag
	// argument of lyricalcompile()
	// is set with LYRICALCOMPILECOMMENT.
	LYRICALOPCOMMENT,
	
} lyricalop;


// This structure represent an instruction.
typedef struct lyricalinstruction {
	// Fields linking the lyricalinstruction
	// in a circular a list.
	struct lyricalinstruction* prev;
	struct lyricalinstruction* next;
	
	// Instruction id.
	lyricalop op;
	
	struct {
		// These fields are used depending
		// on the number of registers
		// that the instruction use.
		// They contain the register id;
		// ei: the register id of the stackframe
		// pointer register is 0.
		uint r1, r2, r3;
		
		// This field is set if
		// the instruction use
		// an immediate value.
		lyricalimmval* imm;
	};
	
	union {
		// This field is used when op == LYRICALOPMACHINECODE;
		// it hold the machine code string
		// which most likely contain
		// non-printable characters.
		string opmachinecode;
		
		// This field is used when op == LYRICALOPCOMMENT;
		// it hold the string containing a comment.
		string comment;
	};
	
	// When non-null, this field is the byte size
	// that the binary executable equivalent must have.
	// It must be a multiple of the target hardware NOP
	// instruction, as padding in the backend is done
	// using NOP instructions.
	uint binsz;
	
	// When non-null, this field
	// is set to a null terminated
	// array of ids for registers
	// which were un-allocated when
	// this instruction was generated.
	// Note that register id 0
	// is the stack pointer register
	// which is always in use, hence
	// the reason why it is used
	// to terminate the array.
	uint* unusedregs;
	
	// Struture holding debug information.
	struct {
		// Absolute path to the file
		// from which the instruction
		// was generated.
		string filepath;
		
		// Line number from
		// which the instruction
		// was generated.
		uint linenumber;
		
		// Offset of the line
		// within the file.
		uint lineoffset;
		
		// This field is not set during compilation,
		// and is to be used by a backend to report
		// the offset of the binary generated in
		// the field lyricalinstruction.backenddata;
		// the reported value can subsquently
		// be used by another backend.
		uint binoffset;
		
	} dbginfo;
	
	// This field is not set during compilation and is
	// to be used by a backend to easily associate data with
	// this instruction when converting the lyricalinstruction
	// to its binary executable equivalent.
	void* backenddata;
	
} lyricalinstruction;


// Structure representing a function.
typedef struct lyricalfunction {
	// Fields linking the lyricalfunction
	// in a circular a list.
	struct lyricalfunction* prev;
	struct lyricalfunction* next;
	
	// Point to the parent function.
	struct lyricalfunction* parent;
	
	// Point to a linkedlist of functions which have been
	// previously declared inside the function which is directly
	// parent to this function. The field sibling point to
	// the function which was created last before this function.
	// The first element of the linkedlist has its field sibling
	// set to null; this field can be seen as a pointer to
	// an older sibling function created before this function.
	struct lyricalfunction* sibling;
	
	// Point to the last child created within this function,
	// so as to be able to go through all the children using
	// the field sibling of the each of the children.
	struct lyricalfunction* children;
	
	// This field contain a signature string
	// that is used to identify a function.
	// Its format is similar to the way a pointer
	// to function type is specified, with the
	// difference that the name of the function
	// is used instead of its return type. ei:
	// "functionname(uint&,uint(void*),...)" This is a signature string
	// 	for a variadic function called functionname; its first argument
	// 	is a byref uint and its second argument is a pointer to function
	// 	which return a uint and take a void* as argument.
	// "+(uint&,uint(void*))" This is a signature string for the operator + .
	// 	its first argument is a byref uint and its second argument is
	// 	a pointer to function which return a uint and take a void* as argument.
	// This field is only set in the secondpass.
	string linkingsignature;
	
	// Point to the last created element of a circular
	// linkedlist of lyricalinstruction which represent
	// the instructions of this function.
	lyricalinstruction* i;
	
	// When this field is set to 1, it mean that the function
	// or operator was defined within the root function using
	// the keyword export and is to be exported.
	uint toexport;
	
	// When this field is non-null, it mean that the function
	// or operator was declared within the root function but
	// not defined; and its definition is to be imported.
	// Functions having this field set, only have instructions
	// to retrieve from the string region the offset
	// to the function to import and jump to it.
	// When non-null, the value of this field is the offset+1
	// within the string region, from where the offset
	// to the function to import is to be retrieved.
	uint toimport;
	
} lyricalfunction;


// Enum used with the argument compileflag of lyricalcompile().
// More than one enum value can be used by using bitwise "or".
typedef enum {
	// Should be used when no other
	// compile flag is to be used.
	LYRICALCOMPILENOFLAG = 0,
	
	// When used, lyricalinstruction having
	// their field comment set are generated.
	LYRICALCOMPILECOMMENT = 1,
	
	// When used, debugging information are generated.
	LYRICALCOMPILEGENERATEDEBUGINFO = 1<<1,
	
	// When used, all variables are made volatile,
	// hence their value are never cached in a register.
	LYRICALCOMPILEALLVARVOLATILE = 1<<2,
	
	// When used, functions never share a stackframe.
	// Stackframe sharing allow for faster function calling
	// because there is no stack setup needed.
	LYRICALCOMPILENOSTACKFRAMESHARING = 1<<3,
	
	// When used, an undefined non-nested function/operator
	// trigger an error instead of becoming an import; effectively
	// forcing the definition of declared function/operator.
	LYRICALCOMPILENOFUNCTIONIMPORT = 1<<4,
	
	// When used, the use of the keyword "export"
	// on non-nested function/operator throw an error,
	// effectively disabling runtime exporting.
	LYRICALCOMPILENOFUNCTIONEXPORT = 1<<5,
	
} lyricalcompileflag;

// Structure used to specify predeclared
// variables to be used by lyricalcompile().
typedef struct {
	// Pointer to the string to use for
	// the name of the predeclared variable;
	// the name should only use digits and
	// lowercase characters and cannot start
	// by a digit. Undefined behaviors will
	// occur if the name is invalid.
	u8* name;
	
	// Pointer to the string to use for
	// the cast of the predeclared variable.
	// Only native type name can be used for
	// the cast of the predeclared variable. ei:
	// "u8*(uint)"; #Pointer to function type.
	// "uint[10]"; #Array of 10 uint elements.
	// If the predeclared variable is to be
	// a byref variable, its cast must be
	// terminated by an asterix since
	// it will be automatically dereferenced
	// by the compiler wherever it is used.
	// Note that '&' in the cast must only
	// be used with the type of an argument
	// within a pointer to function.
	// Undefined behaviors will occur
	// if the cast is invalid.
	u8* type;
	
	// When set to 1, it mean that
	// the predeclared variable should be
	// used as a reference to another variable;
	// in other words, the predeclared variable
	// contain the address of another variable
	// and wherever it is used in the source code,
	// it is automatically dereferenced.
	// To modify a byref variable, an expression
	// such as the following must be used: &var = somevar;
	// In fact since var is dereferenced everytime
	// it is used, using the operator '&' allow
	// to obtain the byref variable itself.
	uint isbyref;
	
	// Address of the predeclared variable.
	// The predeclared variable must have
	// a size that match sizeof() of the cast
	// string used, otherwise incorrect memory
	// accesses will occur.
	void* varaddr;
	
	// Function to be called when the value
	// of the predeclared variable change.
	// The callback take no arguments because
	// the function called may not use the same
	// stack layout convention as Lyrical;
	// furthermore it is faster to call.
	// The callback can access its parent functions
	// variables only if was using the same
	// stack layout convention as Lyrical.
	void (*callback)();
	
} lyricalpredeclaredvar;

// Structure used to specify predeclared
// macro to be used by lyricalcompile().
typedef struct {
	// Pointer to the string to use for
	// the name of the predeclared macro;
	// the name should only use digits and
	// uppercase characters, cannot start by
	// a digit and cannot be "FILE" or "LINE".
	// Undefined behaviors will occur if
	// the macro name is invalid.
	u8* name;
	
	// Pointer to the string that will
	// substite the name of the macro when
	// used in the source code to compile.
	// The string must not contain any newline
	// character otherwise the line number
	// can be reported incorrectly when
	// an error occur while processing
	// the string.
	u8* content;
	
} lyricalpredeclaredmacro;

// Structure used to describe the compilation
// results returned by lyricalcompile().
typedef struct {
	// Pointer to a circular linkedlist of lyricalfunction.
	// The lyricalfunction pointed to by this field
	// is the one for the root func.
	// If lyricalcompile() encounter an error,
	// this field is null.
	lyricalfunction* rootfunc;
	
	// Data which should be used to
	// initialize the string region.
	arrayu8 stringregion;
	
	// Memory size that must be
	// available for the global
	// variable region.
	uint globalregionsz;
	
	// String that contain the list
	// of absolute paths to the source
	// files used to compile the executable.
	// Each path is separated by '\n';
	// the last path is terminated by 0.
	string srcfilepaths;
	
} lyricalcompileresult;

// lyricalcompile() argument.
typedef struct {
	// Pointer to a null terminating string
	// containing the source code to compile.
	// lyricalcompile() will never modify
	// the source code to compile.
	u8* source;
	
	// Size in bytes of the target processor gpr.
	// It must be non-null and a powerof2.
	uint sizeofgpr;
	
	// Number of general purpose registers
	// to use, beside %0 which is the stack
	// pointer register; an error occur if
	// the value of this field is less than 3.
	uint nbrofgpr;
	
	// Array indexed using a lyricalop enum,
	// and where each individual element specify
	// the minimum count of unused registers
	// that must be available prior to generating
	// the corresponding lyricalinstruction.
	uint minunusedregcountforop[LYRICALOPNOP];
	
	// Additional stack space needed
	// by LYRICALOPSTACKPAGEALLOC when
	// allocating a new stack page;
	// in fact, a new stack page is allocated
	// when there is not enough stack space
	// left in the current stack page,
	// and when that is the case,
	// there must be at least enough space
	// to be used by LYRICALOPSTACKPAGEALLOC
	// if it need to push data in the stack
	// when allocating a new stack page.
	uint stackpageallocprovision;
	
	// When the stack of a function being called
	// is created above the stack pointed by
	// the stack pointer register, it is possible
	// for the data at the bottom of the stack
	// being created to be corrupted on architecture
	// such as x86 where the instruction CALL
	// is used to compute an address from
	// the instruction pointer register address;
	// indeed, using the instruction CALL push
	// the instruction pointer register address
	// in the stack, and in doing so, corrupt
	// data at the bottom of the stack being
	// created if a guard space is not used.
	// This field is the number of sizeofgpr
	// to use as guard space.
	uint functioncallargsguardspace;
	
	// Field used to compute (1<<lyricalcompilearg->jumpcaseclog2sz)
	// which is the powerof2 byte size of each LYRICALOPJ in
	// a jumpcase array when generating a switch() jumptable;
	// each entry in a jumpcase array hold the instructions
	// used to jump to the instructions of a switch() case.
	uint jumpcaseclog2sz;
	
	// When non-null, this field should
	// point to an array of lyricalpredeclaredvar
	// that is to be terminated
	// by a lyricalpredeclaredvar
	// with its field name null.
	lyricalpredeclaredvar* predeclaredvars;
	
	// When non-null, this field should
	// point to an array of lyricalpredeclaredmacro
	// that is to be terminated
	// by a lyricalpredeclaredmacro
	// with its field name null.
	lyricalpredeclaredmacro* predeclaredmacros;
	
	// When non-null, this field should
	// point to an array of pointer
	// to null terminated strings that
	// is to be terminated by a null pointer;
	// those null terminated strings should
	// be absolute paths to directories
	// from where files to include will be
	// searched if their paths do not start
	// with either "/", "./" or "../"
	// when used with the preprocessor
	// directive "include".
	// When this field is null, an error
	// is thrown if a filepath which do not
	// start with either "/", "./" or "../"
	// is used with the directive "include".
	u8** standardpaths;
	
	// When non-null, this field is to be set
	// to a callback function which is called
	// when an `include could not be found.
	// It must return null on failure,
	// and non-null otherwise.
	uint (*installmissingmodule)(u8* modulename);
	
	// When parsing an .lyx file, the null terminated
	// string pointed by this field get appended
	// to every double quoted string generated
	// from the text outside of the code block
	// between "<%" and "%>" .
	u8* lyxappend;
	
	// This field must be non-null and
	// is to be set to a callback function
	// which is called when a compilation
	// error occur; the callback function
	// is given as argument a pointer to
	// a null terminated string containing
	// the error message; the callback function
	// should not attempt to free the pointer
	// that it receive as argument.
	void (*error)(u8* errormsg);
	
	lyricalcompileflag compileflag;
	
} lyricalcompilearg;

// This function perform the compilation
// and return a lyricalcompileresult;
// if an error occur, the field rootfunc
// of the lyricalcompileresult returned is null.
lyricalcompileresult lyricalcompile (lyricalcompilearg* compilearg);


// This function free the memory used by
// the compile result returned by lyricalcompile().
// The field rootfunc of the compileresult should
// be non-null to be used with this function.
void lyricalfree (lyricalcompileresult compileresult);

// Structure which standardize
// the Lyrical Resource page format.
typedef struct {
	void***	arg;		// Address of the array of arguments.
	void***	env;		// Address of the array of environment variables.
	void**	lye;		// Address where the Lyrical compile executable file was mapped in memory.
	void**	syscall;	// Address where the Lyrical syscall was installed.
} lyricalrsc;

// Enum standardizing the value
// of Lyrical syscalls.
typedef enum {
	// See "Linux man 2 exit".
	// arg0: status.
	// Do not return a value.
	LYRICALSYSCALLEXIT,
	
	// See "Linux man 2 creat".
	// arg0: pathname.
	// arg1: mode.
	// Return a value.
	LYRICALSYSCALLCREAT,
	
	// See "Linux man 2 open".
	// arg0: pathname.
	// arg1: flags.
	// Return a value.
	LYRICALSYSCALLOPEN,
	
	// See "Linux man 2 close".
	// arg0: fd.
	// Return a value.
	LYRICALSYSCALLCLOSE,
	
	// See "Linux man 2 read".
	// arg0: fd.
	// arg1: buf.
	// arg2: count.
	// Return a value.
	LYRICALSYSCALLREAD,
	
	// See "Linux man 2 write".
	// arg0: fd.
	// arg1: buf.
	// arg2: count.
	// Return a value.
	LYRICALSYSCALLWRITE,
	
	// See "Linux man 2 lseek".
	// arg0: fd.
	// arg1: offset.
	// arg2: whence.
	// Return a value.
	LYRICALSYSCALLSEEK,
	
	// See "Linux man 2 mmap".
	// arg0: addr.
	// arg1: length.
	// arg2: prot.
	// arg3: flags.
	// arg4: fd.
	// arg5: offset.
	// Return a value.
	LYRICALSYSCALLMMAP,
	
	// See "Linux man 2 munmap".
	// arg0: addr.
	// arg1: length.
	// Return a value.
	LYRICALSYSCALLMUNMAP,
	
	// See "Linux man 2 mprotect".
	// arg0: addr.
	// arg1: len.
	// arg2: prot.
	// Return a value.
	LYRICALSYSCALLMPROTECT,
	
	// See "Linux man 3 getcwd".
	// arg0: buf.
	// arg1: size.
	// Return a value.
	LYRICALSYSCALLGETCWD,
	
	// See "Linux man 2 mkdir".
	// arg0: pathname.
	// arg1: mode.
	// Return a value.
	LYRICALSYSCALLMKDIR,
	
	// See "Linux man 2 chdir".
	// arg0: path.
	// Return a value.
	LYRICALSYSCALLCHDIR,
	
	// See "Linux man 2 fchdir".
	// arg0: path.
	// Return a value.
	LYRICALSYSCALLFCHDIR,
	
	// See "Linux man 2 rename".
	// arg0: oldpath.
	// arg1: newpath.
	// Return a value.
	LYRICALSYSCALLRENAME,
	
	// See "Linux man 2 link".
	// arg0: oldpath.
	// arg1: newpath.
	// Return a value.
	LYRICALSYSCALLLINK,
	
	// See "Linux man 2 symlink".
	// arg0: oldpath.
	// arg1: newpath.
	// Return a value.
	LYRICALSYSCALLSYMLINK,
	
	// See "Linux man 2 readlink".
	// arg0: path.
	// arg1: buf.
	// arg2: bufsiz.
	// Return a value.
	LYRICALSYSCALLREADLINK,
	
	// See "Linux man 2 fork".
	// Return a value.
	LYRICALSYSCALLFORK,
	
	// See "Linux man 2 vfork".
	// Return a value.
	LYRICALSYSCALLVFORK,
	
	// See "Linux man 2 unlink".
	// arg0: pathname.
	// Return a value.
	LYRICALSYSCALLUNLINK,
	
	// See "Linux man 2 rmdir".
	// arg0: pathname.
	// Return a value.
	LYRICALSYSCALLRMDIR,
	
	// See "Linux man 2 stat".
	// arg0: path.
	// arg1: buf.
	// Return a value.
	LYRICALSYSCALLSTAT,
	
	// See "Linux man 2 fstat".
	// arg0: fd.
	// arg1: buf.
	// Return a value.
	LYRICALSYSCALLFSTAT,
	
	// See "Linux man 2 lstat".
	// arg0: path.
	// arg1: buf.
	// Return a value.
	LYRICALSYSCALLLSTAT,
	
	// See "Linux man 2 access".
	// arg0: pathname.
	// arg1: mode.
	// Return a value.
	LYRICALSYSCALLACCESS,
	
	// See "Linux man 2 chown".
	// arg0: path.
	// arg1: owner.
	// arg2: group.
	// Return a value.
	LYRICALSYSCALLCHOWN,
	
	// See "Linux man 2 fchown".
	// arg0: fd.
	// arg1: owner.
	// arg2: group.
	// Return a value.
	LYRICALSYSCALLFCHOWN,
	
	// See "Linux man 2 lchown".
	// arg0: path.
	// arg1: owner.
	// arg2: group.
	// Return a value.
	LYRICALSYSCALLLCHOWN,
	
	// See "Linux man 2 chmod".
	// arg0: path.
	// arg1: mode.
	// Return a value.
	LYRICALSYSCALLCHMOD,
	
	// See "Linux man 2 fchmod".
	// arg0: fd.
	// arg1: mode.
	// Return a value.
	LYRICALSYSCALLFCHMOD,
	
	// See "Linux man 2 truncate".
	// arg0: path.
	// arg1: length.
	// Return a value.
	LYRICALSYSCALLTRUNCATE,
	
	// See "Linux man 2 ftruncate".
	// arg0: fd.
	// arg1: length.
	// Return a value.
	LYRICALSYSCALLFTRUNCATE
	
} lyricalsyscall;

#endif
