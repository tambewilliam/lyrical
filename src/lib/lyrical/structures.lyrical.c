
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


// The order in which structure members are declared
// should not be changed because they should match
// their declaration in lyrical.h;


// Structure used to specify predeclared
// variables to be used by the compiler.
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


// I declare without defining the type lyricalfunction
// because I need it in struct lyricalvariable.
struct lyricalfunction;

// Structure representing a variable.
typedef struct lyricalvariable {
	// The field prev of a lyricalvariable is only set
	// for variables of the linkedlist pointed by
	// the field vlocal of a lyricalfunction and
	// the field v of a lyricaltype.
	// For variables of the linkedlist pointed by
	// the field vlocal of a lyricalfunction, the field prev
	// is useful to varalloc() and varfree();
	// For variable of the linkedlist pointed by
	// the field v of a lyricaltype, the field prev is useful
	// to sizeofvarlinkedlist() in order to determine
	// the size that the members of the type will occupy in memory.
	struct lyricalvariable* prev;
	struct lyricalvariable* next;
	
	// Point to the lyricalfunction to which this variable is attached.
	// It will be null for a variable member of struct/pstruct/union.
	// It is used within isregoverlapping() to determine
	// if variables belong to the same region.
	// It is also used within generateloadinstr() and flushreg()
	// to determine whether the variable reside in the global
	// variable region by checking if it is equal to rootfunc.
	// It is also used within flushanddiscardallreg() to determine
	// if a variable is local to currentfunc.
	// It is also used within varfree();
	// It is also used within propagatevarchange().
	struct lyricalfunction* funcowner;
	
	// This field is set by varalloc();
	// This field will be 0 for variables which do not take
	// memory space in the stack or the global variable region,
	// such as address variable, dereference variable, etc...
	// Basically it will be null for variables which do not have
	// their field type set; with the exception of constant string
	// which do not have a type but only a cast and for which
	// this field is set to the number of characters that it contain.
	uint size;
	
	// Byte offset of the variable within the stack
	// or global variable region.
	// It will be used during register manipulation
	// to compute the location of the variable in the stack
	// or global variable region where a register value
	// will be saved or loaded.
	// It will also be used with a lyricalvariable member of
	// a lyricaltype to determine the offset from the first byte
	// of the type when in memory. If the lyricalvariable is
	// a member of an anonymous struct/pstruct/union, the value of
	// this field will be set with respect to the first byte
	// memory offset of the type containing the anonymous
	// struct/pstruct/union.
	uint offset;
	
	// The different type of lyricalvariable are:
	// 
	// Variable created for member of a struct/pstruct/union:
	// 
	// - Explicitely declared variable member; their name is
	// 	the symbol read from the source code that
	// 	the compiler is parsing.
	// - Hidden variable member.
	// 	Their name is the null terminated string "."
	// 	which an explicitly declared variable member
	// 	can never be, effectively making them hidden.
	// - Variable member created for anonymous struct/union;
	// 	their name is an empty null terminated string.
	// - Variable member created to hold the offset for
	// 	the next variable member; they are used when
	// 	creating the first member of a bitfield.
	// 	Their name is an empty null terminated string
	// 	and their field size has the same value as
	// 	the field size of the lyricalvariable member
	// 	for which it is holding the offset.
	// 	It always has a native type.
	// 
	// Variable for which the value come from memory:
	// 
	// - Explicitely declared variable; use space in the stack
	// 	or global variable region.
	// 	Its name is the symbol read from the source code
	// 	that the compiler is parsing.
	// 	A static variable declared inside a function
	// 	has its name prefixed with the string equivalent
	// 	of the address in decimal of the lyricalfunction it belong to.
	// 	The prefix is surrounded with '#' and '_'.
	// - Predeclared variable; they are global, meaning created within
	// 	the root lyricalfunction, and do not use any space in
	// 	the global variable region. Their field size is zero.
	// 	Their name come from the associated lyricalpredeclaredvar.
	// - Status variable used with static variables.
	// 	It allow to determine whether the initializing code has
	// 	already been run; it use space in the global variable region.
	// 	Its name is the address in decimal of the static variable
	// 	for which it is created, converted to a string and to which
	// 	is applied normal prefixing done on static variables
	// 	if declared inside a function.
	// - Tempvar; use space in the stack or global variable region.
	// 	It can be the duplicate of a pushed argument,
	// 	the result of a function or operator.
	// 	Its name is formed using the string equivalent
	// 	of the address in decimal of their struct lyricalvariable
	// 	to which '$' is prefixed and suffixed; ei: "$8761144$".
	// - Dereferenced variables; do not use space in the stack or
	// 	global variable region. Their field size is zero.
	// 	They are always local to a lyricalfunction.
	// 	Their memory address is held by the variable which name is
	// 	in between "(*(cast)" and ")" of their name, where "cast" is
	// 	the name of the type or cast used when dereferencing; hence
	// 	as an example their name can have the following forms:
	// 	"(*(uint*)myvar)", "(*(u8*)myvar.16)";
	// - Variables which have their name suffixed with a string offset
	// 	which is their location within the variable which name is
	// 	the string on the left of the string offset.
	// 	ei: "(*(u8*)myvar).8", "var.8";
	// 	lyricalvariables which have their name suffixed with
	// 	a string offset are always local to a lyricalfunction;
	// 	they represent a variable and do not use space in the stack
	// 	or global variable region. Their field size is zero.
	// - Variable used to save the value of reserved registers;
	// 	use space in the stack or global variable region.
	// 	Its name is an empty null terminated string.
	// 	
	// Variable for which the value is generated by the compiler or during runtime.
	// 
	// - Address of variables; its lyricalvariable is local, meaning created
	// 	within the lyricalfunction for the function where it originated.
	// 	The value of a variable address of variable do not use space
	// 	in the stack or global variable region, and is determined at runtime.
	// 	The field size of its lyricalvariable is zero.
	// 	The string in the field name of its lyricalvariable has
	// 	the following form: "(&myvar)";
	// 	another example: "(&myvar.16)";
	// - Variable enum element; its lyricalvariable is local, meaning created
	// 	within the lyricalfunction for the function where it was defined.
	// 	The value of a variable enum element do not use space in
	// 	the stack or global variable region.
	// 	The field size of its lyricalvariable is zero.
	// 	The string in the field name of its lyricalvariable is the symbol
	// 	read from the source code that the compiler is parsing, and
	// 	that symbol is always uppercase.
	// 	Note that the lyricalvariable for a variable enum element is
	// 	never directly manipulated, instead a corresponding lyricalvariable
	// 	for a variable number is used; the function readvarorfunc()
	// 	can be read to better understand.
	// - Variable numbers; its lyricalvariable is global,
	// 	meaning created within the root lyricalfunction.
	// 	The value of a variable number do not use space in
	// 	the global variable region.
	// 	The field size of its lyricalvariable is zero.
	// 	The string in the field name of its lyricalvariable is formed
	// 	using the string equivalent of the address in decimal
	// 	of its lyricalvariable to which '0' is prefixed.
	// - Variable string constant; its lyricalvariable is global,
	// 	meaning created within the root lyricalfunction.
	// 	The value of a variable string constant do not use space
	// 	in the global variable region, but instead represent
	// 	an offset within the string region.
	// 	The value of a variable string constant is the address
	// 	of the string constant which is determined at runtime.
	// 	The field size of its lyricalvariable is zero.
	// 	The string in the field name of its lyricalvariable is formed
	// 	using the string equivalent of the address of its lyricalvariable
	// 	to which '0' is prefixed.
	// - Variable address of function; its lyricalvariable is global,
	// 	meaning created within the root lyricalfunction.
	// 	The value of a variable address of function do not use space in
	// 	the global variable region. Its value is determined at runtime.
	// 	The field size of its lyricalvariable is zero.
	// 	The string in the field name of its lyricalvariable is formed
	// 	using the string equivalent of the address in decimal
	// 	of its lyricalvariable to which '0' is prefixed.
	// 	
	// This field is never set to stringnull;
	// hence it is never necessary to check whether
	// the field name.ptr of a lyricalvariable
	// is non-null before using it.
	string name;
	
	// The type of a variable refer to its real size in the stack or
	// global variable region; while its cast is just a way to request
	// that the variable be treated in a different way; hence, only variables
	// explicitly declared by the programmer and variables which are results
	// of functions or operators have a type set; other variables only
	// have their field cast set.
	// During processing, the type of a variable is used only if
	// their cast is not set.
	// Here below are different example of type as used when
	// declaring a variable and the type string that they yield.
	// "u8*[4][3]" yield "u8*[4][3]";
	// "uint&" yield "uint*"; a variable declared using '&' is a byref variable;
	// "void()" yield "void()";
	// "uint*[4](uint(uint)(uint)[7] argname, u8)" yield "uint*[4](uint(uint)(uint)[7],u8)";
	// "void*[4](uint, u8, ...)" yield "void*[4](uint,u8,...)";
	// "void(uint arg1, u8& arg2)" yield "void(uint,u8&)";
	// "void(uint, u8)(uint, void(u8))" yield "void(uint,u8)(uint,void(u8))";
	// 
	// When writting the specification of a type for
	// a pointer to function, I also allow for symbol name
	// to be written after the type of the argument.
	// It allow for pointer to function type to be more clearer.
	// ei: Here below I am declaring a pointer to function:
	// void(u8* filename, uint offset, u8* msg) ptrtofunc;
	// the type string that the above will yield will be:
	// "void(u8*,uint,u8*)"
	
	string type;
	
	// Note that this field is a temporary attribute
	// protected by the field preservetempattr when
	// this lyricalvariable is used with pushargument().
	string cast;
	
	// The fields id, funcowner and argorlocal are not set
	// for variables which are members of struct/pstruct/union.
	
	// This field is used by lookupvarbyid().
	// It is set only for variables
	// explicitly declared by the programmer.
	uint id;
	
	// Contain the address of the field varg or vlocal of
	// the lyricalfunction pointed by the field funcowner.
	// It is used within isregoverlapping() to determine
	// if variables belong to the same region.
	// It is also used within generateloadinstr() and flushreg()
	// to determine whether I have an argument or local variable.
	struct lyricalvariable** argorlocal;
	
	// The 2 fields below are used to implement scope
	// and are set by scopesnapshotforvar().
	uint scopedepth;
	uint* scope;
	
	// This field is set if the lyricalvariable
	// is used for a predeclared variable.
	lyricalpredeclaredvar* predeclared;
	
	// This field is 1 if the lyricalvariable
	// is created for a static variable.
	// Note that a static variable can
	// also be declared outside of any
	// function, meaning in rootfunc.
	uint isstatic;
	
	// When set to 1, it mean that the variable
	// should be used as a reference to another variable;
	// in other words, the variable contain the address
	// of another variable and wherever it is used in
	// the source code, it should be automatically dereferenced.
	// To modify a byref variable, an expression such as
	// the following must be used: &var = somevar;
	// In fact since var is dereferenced everytime it is used,
	// using the operator '&' allow to obtain the byref variable itself.
	uint isbyref;
	
	// If the lyricalvariable represent a function address,
	// this field is set to the address of the lyricalfunction
	// for the function.
	struct lyricalfunction* isfuncaddr;
	
	// If the lyricalvariable represent a number,
	// this field is set to 1 and the field numbervalue
	// is set to the value of the number.
	uint isnumber;
	u64 numbervalue;
	
	// If the lyricalvariable represent a string constant,
	// this field is set to the offset that the string
	// constant will have in the string region.
	// The offset value is increment by 1 before it is
	// set in this field so as to be able to store an
	// offset of 0 and insure that this field is non-null.
	uint isstring;
	
	// This field is only used with a lyricalvariable member
	// of a struct declared as a bitfield.
	// It is an integer value representing the bitselect
	// generated when using the operator "." or "->".
	// When creating a structure such as:
	// struct mystruct {
	// 	uint a: 1;
	// 	uint b: 1;
	// 	uint c: 3;
	// 	u8 d: 1;
	// };
	// The member lyricalvariables a, b, c and d of
	// the type mystruct will each have their field bitselect
	// set to a non-null integer value representing the bitselect.
	// The members a, b and c will have the same offset
	// within the struct, while the member d, because
	// it has a different type will have a different offset.
	// Bitfield are declared using only integer native types.
	// Bitfields are allocated within an integer from
	// the most-significant to the least-significant bit,
	// hence in the above example the field 'a' will be
	// the most significant bit of the uint field.
	// The field bitselect is only set for a lyricalvariable
	// having an offset suffixed to its name.
	// Note that this field is a temporary attribute
	// protected by the field preservetempattr when
	// this lyricalvariable is used with pushargument().
	u64 bitselect;
	
	// This field point to the field isalwaysvolatile of
	// this variable or of some other variable and is shared
	// by a main variable and variables having their name
	// suffixed with an offset which refer to that main variable.
	// It is used to mark a variable as volatile.
	// When a variable is volatile, its value is never cached in registers.
	// To prevent variables from aliasing and to keep them in sync,
	// obtaining the address of a variable make it an always volatile
	// variable because the compiler cannot track where its pointer
	// will be used, and dereferenced variables are always volatile
	// because their address can be the address of another variable.
	uint* alwaysvolatile;
	
	// The address of this field is used
	// to set the field alwaysvolatile.
	uint isalwaysvolatile;
	
	// This field is set to non-null before
	// using the lyricalvariable with pushargument(),
	// in order to preserve temporary attributes
	// that are the field cast and bitselect.
	// When this field is null, pushargument()
	// clear temporary attributes to prevent them
	// from being used anywhere else until set again.
	uint preservetempattr;
	
} lyricalvariable;


// This structure represent a register.
// When a register get used, I move it to the bottom of
// the linkedlist, hence the first register of
// the linkedlist is the less used register and will be
// the one used when allocating a register if it is not locked.
// When a register is locked the next register is tried.
typedef struct lyricalreg {
	// Identification number of the register.
	// ei: For register %3, its id is 3.
	uint id;
	
	// Only one of the fields returnaddr, funclevel,
	// globalregionaddr, stringregionaddr, thisaddr, retvaraddr
	// and v can be non-null at a time; if all those fields
	// are null and the field lock and reserved are not set,
	// then the register is free.
	
	// When set to 1, it mean that this register has
	// the return address from the current function.
	uint returnaddr;
	
	// When this field is set, it hold the nth parent value
	// from the current function for which this register has
	// the stackframe address.
	// ei: When this field is 1, this register has
	// the stackframe address of the immediate parent
	// of currentfunc.
	uint funclevel;
	
	// When set to 1, it mean that this register has
	// the address of the global variable region where
	// all global variables are located.
	uint globalregionaddr;
	
	// When set to 1, it mean that this register
	// has the address of the string region where
	// all string constants are located.
	uint stringregionaddr;
	
	// When set to 1, it mean that this register
	// has the address of "this".
	uint thisaddr;
	
	// When set to 1, it mean that this register has
	// the address of retvar.
	uint retvaraddr;
	
	// Point to the variable for which this register is used.
	// This field will never point to a variable which has a name
	// suffixed with an offset, but instead will always point
	// to its main variable, and the field offset is set accordingly;
	// this way it can be insured that there is only one register
	// allocated for a specific memory region, so as to reuse it
	// if it has already been allocated to a specific memory region
	// for which a register is needed.
	lyricalvariable* v;
	
	// Bit select value; where
	// 1 is a meaningful bit
	// and 0 is not.
	u64 bitselect;
	
	// Set to 1 when the register value has changed since
	// it was last loaded and need to be flushed to memory.
	// When it is set, either the field returnaddr, funclevel
	// or v is set.
	uint dirty;
	
	// This field is used as an offset
	// within the memory region of the
	// variable pointed by the field v.
	uint offset;
	
	// When this register is associated with
	// a variable residing in memory, this field
	// determine how many bytes were loaded from
	// the variable in memory and its value
	// can only be a powerof2: ei: 1 or 2 which
	// would correspond to 8 or 16 bits respectively.
	// When this register is associated with
	// a variable generated by the compiler
	// as tested by isvarreadonly(),
	// the value of this field is 0.
	uint size;
	
	// When this field is non-null, allocreg() do not
	// consider this register when allocating a register.
	uint lock;
	
	// This field is set non-null when the register
	// has been reserved for use within an asm block;
	// and allocreg() do not consider this register
	// when allocating a register.
	uint reserved;
	
	// These fields determine whether
	// this register hold a value that
	// was zero or sign extended.
	// They are set wherever an
	// lyricalinstruction get created
	// through newinstruction() and
	// this lyricalreg is the result
	// of the newly created lyricalinstruction.
	// Instructions generated to apply
	// sign or zero extension is done
	// only within getregforvar() for
	// variables having a native type
	// or cast; and sign or zero extending
	// the register is based on the signedness
	// of the type or cast of the variable
	// associated with the register. ei:
	// s16 get sign extended;
	// u8 get zero extended.
	// Note that when the field size is equal
	// to sizeofgpr, it do not matter whether
	// these fields are set, because
	// sign or zero extension do not
	// need to be applied.
	uint waszeroextended;
	uint wassignextended;
	
	// Point to the lyricalfunction owning this register.
	struct lyricalfunction* funcowner;
	
	// The fields prev and next are used
	// to create a circular linkedlist.
	struct lyricalreg* prev;
	struct lyricalreg* next;
	
} lyricalreg;


// This structure represents a type.
typedef struct lyricaltype {
	// Fields linking the lyricaltype
	// in a circular a list.
	struct lyricaltype* prev;
	struct lyricaltype* next;
	
	// This field is set if the type is anonymous.
	// When set, it point to the non-anonymous containing type.
	// This field is only used for struct and union types.
	struct lyricaltype* ownerofanonymoustype;
	
	// The 2 fields below are used to implement scope
	// and are set by scopesnapshotfortype();
	uint scopedepth;
	uint* scope;
	
	// When the lyricaltype is created for a named
	// type, this field is set using the symbol read
	// from the source code that the compiler is parsing;
	// when the lyricaltype is created for an unnamed
	// type, this field is set to a string generated
	// from the value of curpos converted to a string.
	// The character '#' get prefixed to the string
	// set in this field if the lyricaltype is
	// created for an enum.
	string name;
	
	// Size in bytes that the type will occupy in memory.
	// An incompletely defined type, or a type that has been
	// declared without being defined has this field set to null.
	uint size;
	
	// Pointer to a linkedlist of variable which
	// are members of this type. Note that this field point
	// to the last variable of the linkedlist.
	lyricalvariable* v;
	
	// When non-null, it point
	// to the type that was used
	// as a basis for this type;
	// in other words, the first
	// member of this type is anonymous
	// and its type is the base type.
	struct lyricaltype* basetype;
	
} lyricaltype;


// Here I simply declare without defining
// the types lyricalfunction and lyricalsharedregion
// because I need them in struct lyricalimmval.
struct lyricalinstruction;
struct lyricalsharedregion;


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
		LYRICALIMMOFFSETTOSTRINGREGION,
		
		// The following enum values of lyricalimmtype
		// are never found in the resulting tree returned
		// by lyricalcompile(); as they are resolved
		// by reviewimmediatevalues().
		
		// Used to represent the maximum size that local variables
		// of the function pointed by the field f will require and
		// that maximum size value is saved in f->vlocalmaxsize.
		LYRICALIMMLOCALVARSSIZE,
		
		// Same as LYRICALIMMLOCALVARSSIZE but the value
		// in f->vlocalmaxsize is to be negated.
		LYRICALIMMNEGATIVELOCALVARSSIZE,
		
		// Used to represent the size that the stackframe pointers
		// cache of the function pointed by the field f will require and
		// that size value is saved in f->stackframepointerscachesize.
		LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE,
		
		// Same as LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE but the value
		// in f->stackframepointerscachesize is to be negated.
		LYRICALIMMNEGATIVESTACKFRAMEPOINTERSCACHESIZE,
		
		// Used to represent the size that the shared region
		// of the function pointed by the field f will require and
		// that size value is saved in f->sharedregionsize.
		LYRICALIMMSHAREDREGIONSIZE,
		
		// Same as LYRICALIMMSHAREDREGIONSIZE but the value
		// in f->sharedregionsize is to be negated.
		LYRICALIMMNEGATIVESHAREDREGIONSIZE,
		
		// When used, the integer value in the field
		// sharedregion->offset is to be used.
		LYRICALIMMOFFSETWITHINSHAREDREGION,
		
	} type;
	
	union {
		u64 n;
		
		struct lyricalinstruction* i;
		
		struct lyricalfunction* f;
		
		struct lyricalsharedregion* sharedregion;
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
	// of branching lyricalop.
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
	// of branching lyricalop.
	
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


// This structure represent a labeled instruction to resolve.
typedef struct lyricallabeledinstructiontoresolve {
	// Name of the label to which the field
	// pointed by i should be resolved to.
	string name;
	
	lyricalinstruction** i;
	
	// This field get set to the value of curpos
	// at the time of its creation by resolvelabellater().
	// It is used to select the closest label
	// (from different scope), when they have the same name.
	u8* pos;
	
	// The 2 fields below are used to implement scope
	// and are set by scopesnapshotforlabeledinstructiontoresolve();
	uint scopedepth;
	uint* scope;
	
	// A pointer to the previous lyricallabeledinstructiontoresolve
	// is not needed; the last lyricallabeledinstructiontoresolve created
	// will have its field next pointing to the first
	// lyricallabeledinstructiontoresolve of the linkedlist.
	struct lyricallabeledinstructiontoresolve* next;
	
} lyricallabeledinstructiontoresolve;


// This structure represent a label.
typedef struct lyricallabel {
	// Name of the label.
	string name;
	
	// Point to a lyricalinstruction of
	// type LYRICALOPNOP for the label.
	lyricalinstruction* i;
	
	// This field get set to the value of curpos
	// at the time of its creation by newlabel().
	// It is used to determine which label
	// (from different scope), is the closest
	// when they have the same name.
	u8* pos;
	
	// The 2 fields below are used to implement scope
	// and are set by scopesnapshotforlabel();
	uint scopedepth;
	uint* scope;
	
	// A pointer to the previous label is not needed.
	// The last label created will have its field next
	// pointing to the first label of the linkedlist.
	struct lyricallabel* next;
	
} lyricallabel;


// This structure represent a catchable-labelled instruction to resolve.
typedef struct lyricalcatchablelabelledinstructiontoresolve {
	
	lyricalinstruction** i;
	
	// A pointer to the previous lyricalcatchablelabelledinstructiontoresolve
	// is not needed; the last lyricalcatchablelabelledinstructiontoresolve created
	// will have its field next pointing to the first
	// lyricalcatchablelabelledinstructiontoresolve of the linkedlist.
	struct lyricalcatchablelabelledinstructiontoresolve* next;
	
} lyricalcatchablelabelledinstructiontoresolve;


// This structure represent a catchable-label.
typedef struct lyricalcatchablelabel {
	// Name of the catchable-label. Will also be used
	// when checking if a symbol is already used.
	string name;
	
	// Linked list of catchable-labelled instructions to resolve.
	// Each lyricalcatchablelabelledinstructiontoresolve contain
	// a pointer to lyricalinstruction* which will be set
	// to the instruction of the corresponding label
	// within the function where the catchable-label is used.
	lyricalcatchablelabelledinstructiontoresolve* linkedlist;
	
	// The 2 fields below are used to implement scope
	// and are set by scopesnapshotforcatchablelabel();
	uint scopedepth;
	uint* scope;
	
	// A pointer to the previous lyricalcatchablelabel is not needed.
	// The last lyricalcatchablelabel created will have its field next
	// pointing to the first lyricalcatchablelabel of the linkedlist.
	struct lyricalcatchablelabel* next;
	
} lyricalcatchablelabel;


// This struct represent the flags used by each pushed argument.
// The fields of this struct are set only in the firstpass
// and used in the secondpass.
typedef struct lyricalargumentflag {
	// A pointer to the previous lyricalargumentflag
	// is not needed. The last lyricalargumentflag created
	// will have its field next pointing to the first
	// lyricalargumentflag of the linkedlist; hence
	// it make a circular linkedlist.
	struct lyricalargumentflag* next;
	
	// This field is set only in the firstpass
	// and used in the secondpass by searchargflagbyid()
	// in order to retrieve the lyricalargumentflag
	// of an argument created in the firstpass.
	uint id;
	
	// The field istobepassedbyref is set if
	// the argument is to be passed byref and
	// is used only for arguments that are not used
	// with native operators or assembly instructions
	// within an asm block.
	// The field istobeoutput is set and used only
	// for arguments used with native operators or
	// assembly instructions within an asm block.
	// Hence the fields istobepassedbyref and
	// istobeoutput cannot be both set at the same time.
	uint istobepassedbyref;
	uint istobeoutput;
	
} lyricalargumentflag;


// This structure is used to represent
// an argument to operators or functions.
typedef struct lyricalargument {
	// A pointer to the previous argument is not needed.
	// The last argument created will have its field next
	// pointing to the first argument of the linkedlist;
	// hence it make a circular linkedlist.
	struct lyricalargument* next;
	
	// The fields below are used when this argument
	// is in the linkedlist pointed by the variable
	// registeredargs; the first element of the linkedlist
	// has its field prevregisteredarg set to null, and
	// the last element of the linkedlist has its field
	// nextregisteredarg set to null.
	// I register argument to keep track of all pushed
	// arguments between recursive calls of evaluateexpression()
	// when evaluating an expression; funcarg which is declared
	// within evaluateexpression() only allow for seeing
	// pushed arguments for the current instance of
	// evaluateexpression() and to see all pushed arguments
	// accross several recursion of evaluateexpression(),
	// I go through all registered arguments.
	// I cannot have registered arguments when not processing
	// an expression; arguments are registered by pushargument()
	// and propagatevarchange() use the linkedlist.
	struct lyricalargument* prevregisteredarg;
	struct lyricalargument* nextregisteredarg;
	
	// This field point to the variable
	// to use for the argument.
	// If during the pushing, the variable is going
	// to be used as the left argument of the native
	// operator '=' or is a readonly variable, or
	// is a non-volatile variable, this field is set to
	// the variable pushed, otherwise it is set to
	// the duplicate of the variable pushed.
	// If during pushing, the variable is to be
	// passed byref, and the address variable of
	// the variable pushed is a readonly variable,
	// or is a non-volatile variable, this field is set
	// to the address of the pushed variable, otherwise
	// it is set to the duplicate of the address
	// of the pushed variable.
	// The variable pointed by this field is freed
	// by freefuncarg() if it is a tempvar.
	lyricalvariable* v;
	
	// This field point to the variable pushed.
	lyricalvariable* varpushed;
	
	// This field contain the cast or type with which
	// the variable pointed by the field varpushed was pushed.
	string typepushed;
	
	// Bit select value to use
	// with the argument; where
	// 1 is a meaningful bit
	// and 0 is not.
	u64 bitselect;
	
	// The fields below are flags used with the arguments.
	
	// This field point to a lyricalargumentflag which
	// is in the linkedlist pointed by the field pushedargflags
	// of the function within which it was pushed.
	// Note that only the field pushedargflags of
	// the lyricalfunction created in the firstpass
	// is used in the first and secondpass.
	lyricalargumentflag* flag;
	
	// This field get set to 1 within callfunctionnow()
	// for the first argument of a function which
	// do not return a value.
	// When freefuncarg() see it set to 1, it will not
	// attempt to check whether the variable in the field v
	// is a tempvar in order to free it.
	uint tobeusedasreturnvariable;
	
} lyricalargument;


// Structure describing a propagation element which
// is used by propagatevarchange(), propagatevarschangedbyfunc()
// and resolvepropagations().
typedef struct lyricalpropagation {
	// A pointer to the previous
	// lyricalpropagation is not needed.
	// The last element of the linkedlist
	// has its field next set to null.
	struct lyricalpropagation* next;
	
	// Enum for valid values of the field type.
	enum {
		// When used, this propagation element
		// is used with a variable.
		VARIABLETOPROPAGATE,
		
		// When used, this propagation element
		// is used with a function.
		FUNCTIONTOPROPAGATE
		
	} type;
	
	// When the field type == VARIABLETOPROPAGATE,
	// this field point to the lyricalfunction for
	// which the lyricalvariable associated with
	// this propagation element is local.
	// When the field type == FUNCTIONTOPROPAGATE,
	// this field point to the lyricalfunction
	// associated with this propagation element.
	struct lyricalfunction* f;
	
	// The fields below are used only when
	// the field type == VARIABLETOPROPAGATE.
	
	// This field will hold the id of a lyricalvariable
	// for a variable explicitly declared by the programmer
	// which would have been used with processvaroffsetifany()
	// to insure that there is no offset suffixed to their name;
	// hence it will never be a tempvar, readonly variable
	// or a dereference variable.
	uint id;
	
	uint offset;
	
	uint size;
	
} lyricalpropagation;


// Structure representing a cached stackframe.
// It is created in the firstpass by cachestackframe()
// and used in the secondpass by cachestackframepointers()
// to determine which stackframe address need to be cached
// in the stack of the current function.
typedef struct lyricalcachedstackframe {
	// Level at which the stackframe is located
	// from the stackframe pointed by the stack
	// pointer register. ei: the stackframe level for
	// the immediate parent function would be 1.
	uint level;
	
	// This field link all lyricalcachedstackframe
	// of a lyricalfunction in a list ordered from
	// the lowest level value to the highest level value.
	// The last element of the linkedlist has
	// its field next null.
	struct lyricalcachedstackframe* next;
	
} lyricalcachedstackframe;


// Structure used to represent
// a calling to a function.
typedef struct lyricalcalledfunction {
	
	struct lyricalfunction* f;
	
	// This field keep track
	// of the number of times
	// a call to the function
	// was made.
	// Note that this field is null
	// if the lyricalcalledfunction
	// was created for a lyricalfunction
	// which indirectly called
	// the lyricalfunction pointed
	// by the field f. ei: calling
	// a function through another
	// function.
	uint count;
	
	// This field link the lyricalcalledfunction
	// in a list where the last element of
	// the linkedlist has its field next null.
	struct lyricalcalledfunction* next;
	
} lyricalcalledfunction;


// Structure representing a function
// that use a shared region.
typedef struct lyricalsharedregionelement {
	
	struct lyricalfunction* f;
	
	// This field link the lyricalsharedregionelement
	// in a list where the last element of
	// the linkedlist has its field next null.
	struct lyricalsharedregionelement* next;
	
} lyricalsharedregionelement;


// Structure reprensenting a region within
// the shared region of a regular stackframe.
typedef struct lyricalsharedregion {
	// This field point to the list
	// of functions sharing this region.
	lyricalsharedregionelement* e;
	
	// Offset where the region is located
	// within the shared region.
	uint offset;
	
	// This field link the lyricalsharedregion
	// in a list where the last element of
	// the linkedlist has its field next null.
	struct lyricalsharedregion* next;
	
} lyricalsharedregion;


// Structure representing a function.
typedef struct lyricalfunction {
	// The fields prev and next are used to make
	// all created lyricalfunction form a circular linkedlist
	// where the variable rootfunc point to the first
	// created lyricalfunction of the circular linkedlist
	// with its field prev pointing to the last created
	// lyricalfunction and its field next pointing to the
	// second created lyricalfunction.
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
	// This field is set only in the secondpass.
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
	
	// Fields below are not part of
	// lyricalfunction within lyrical.h .
	
	// This field point to the circular linkedlist of
	// general purpose registers used by this function.
	// Each function has to have its own linkedlist
	// of GPRs because register allocation is a process
	// unique to each function.
	// GPRs are created only and used in the secondpass.
	lyricalreg* gpr;
	
	// Point to the last created element of a circular
	// linkedlist of lyricalvariable which represent
	// the local variables of this function.
	lyricalvariable* vlocal;
	
	// Point to the last created element of a circular
	// linkedlist of lyricalvariable which represent
	// the arguments of this function.
	lyricalvariable* varg;
	
	// Point to the last created element of a circular
	// linkedlist of lyricaltype created within this function.
	lyricaltype* t;
	
	// Point to the last created element of a linkedlist
	// of lyricalpropagation for this function.
	// This field is set in the firstpass by propagatevarchange()
	// and propagatevarschangedbyfunc() and used in the secondpass
	// by propagatevarschangedbyfunc() through the field firstpass.
	lyricalpropagation* p;
	
	// All lyricallabel and lyricalcatchablelabel of a lyricalfunction
	// are freed (once I am done parsing the function)
	// by resolvelabelsnow() and resolvecatchablelabelsnow() respectively.
	
	// Point to a circular linkedlist
	// for labels created within this function.
	lyricallabel* l;
	
	// Point to a circular linkedlist
	// for catchable-labels created within this function.
	lyricalcatchablelabel* cl;
	
	// This field point to the first element of
	// a lyricalcachedstackframe linkedlist which represent
	// the parent functions stackframes used by this function.
	// This field is never set for the root function
	// or for immediate children of the root function.
	// This field is set in the firstpass and used in
	// the secondpass through the field firstpass.
	lyricalcachedstackframe* cachedstackframes;
	
	// This field is set by cachestackframepointers()
	// once it is done caching the stackframe pointers
	// used by a function.
	// This field is set and used only in the secondpass.
	uint stackframepointercachingdone;
	
	// This field is set only in the firstpass and used
	// in the secondpass through the field firstpass.
	uint id;
	
	// This field is used to save the location
	// where the function was declared.
	// If the function is defined, it is set
	// to where the function was defined.
	u8* startofdeclaration;
	
	// The 2 fields below are used to implement scope
	// and are set by scopesnapshotforfunc().
	uint scopedepth;
	uint* scope;
	
	// Name of the function.
	string name;
	
	// This field is used to save the
	// return type string of the function.
	string type;
	
	// This field is used to save the type that will be used
	// by the variable address to this function.
	// There is only one of such variable per function, and
	// it is a constant variable which is created using newvarglobal();
	// An example of such string is: "uint(u8,void*)" which
	// represent a pointer to a function which return a uint and
	// which take 2 arguments, a u8 and a void* .
	// Here is another example: "void(u8, u16)" which is a pointer
	// to a function which do not return a value and take 2 arguments
	// which are a u8 and a u16.
	// Here is another example: "uint(void(uint,uint),void*)" which
	// is a pointer to a function which return a uint and take
	// 2 arguments: a pointer to function and a void*; the pointer to
	// function do not return a value and take two uint arguments.
	string typeofaddr;
	
	// This field is used to store this
	// lyricalfunction call signature which
	// get matched against pamsyntokenized which
	// get are set in the field fcall of lyricalfunction.
	// ei: "funcname|uint|void()|", "++|uint*|", "+|uint|";
	// Note that in the signature string, the name of
	// the operator or function and its arguments
	// are separated by '|' instead of commas,
	// opening and closing paranthesis which would
	// create conflict since those same characters
	// are used in a pointer to function type.
	string callsignature;
	
	// The field fcall of a function is used by searchfunc()
	// when searching for a matching function.
	// 
	// Here below are examples of pattern used to generate
	// the pamsyntokenized set in the field fcall:
	// 
	// "<fname|uint|uint|>": Will match a function called fname
	// 	taking 2 arguments of type uint.
	// "<fname|[\#a-z]+0-*{[\,\.()&\*a-z0-9],\[[1-9]+0-*{[0-9]}\]}[)\*]|>":
	// 	Will match a function called fname taking 1 argument.
	// 	The argument is a void* type which can accept
	// 	all pointer types, function pointer included.
	// "<fname|uint|+0-*{|[\#a-z]+0-*{[\,\.()&\*a-z0-9],\[[1-9]+0-*{[0-9]}\]}}|>":
	// 	Will match a variadic function called fname
	// 	which take a uint as its first argument.
	// 
	// Note that in the pattern, the name of the operator or
	// function and its arguments are separated by '|' instead
	// of commas, opening and closing paranthesis, which would create
	// conflict since those same characters can be used in the portion
	// of the pattern used to match a type, ei: pointer to function type.
	// 
	// Note that the return type of a function is not part of
	// the pattern; the appropriate function to call is determined by
	// the type of its arguments.
	// In fact if the return type of a function was used to call
	// a function, I would need to find a way to determine
	// the return type before calling a function and I don't know it until
	// I actually receive the result variable from the function I am calling.
	// So it is natural to just call a function and use whatever type
	// is associated with the result variable for subsequent computation.
	pamsyntokenized fcall;
	
	// This field is only set in the secondpass (for
	// a lyricalfunction created in the secondpass) within
	// funcdeclaration() and is only used in the secondpass
	// to get informations about a function created in
	// the firstpass from its equivalent function created
	// in the secondpass.
	struct lyricalfunction* firstpass;
	
	// This field is only set in the secondpass (for
	// a lyricalfunction created in the firstpass) within
	// funcdeclaration() and is only used in the secondpass
	// to get informations about a function created in
	// the secondpass from its equivalent function created
	// in the firstpass.
	struct lyricalfunction* secondpass;
	
	// This field point to a circular linkedlist of
	// lyricalargumentflag created in the firstpass by pushargument().
	// That same linkedlist is used in the secondpass by pushargument().
	// This field is set only for lyricalfunction created
	// during the firstpass.
	lyricalargumentflag* pushedargflags;
	
	// This field point to a linkedlist of lyricalcalledfunction
	// where each element of the linkedlist is created
	// for each function that this function is calling.
	// This field is never set for the root function.
	// This field is set and used only in the firstpass.
	lyricalcalledfunction* calledfunctions;
	
	// Point to the lyricalfunction for the function holding
	// the stackframe used by this function.
	// It is null for functions which create their own
	// stackframe when called.
	// This field is set in the firstpass and used in
	// the secondpass through the field firstpass.
	// This field is never set for the root function.
	struct lyricalfunction* stackframeholder;
	
	// This field point to the lyricalsharedregion
	// which represent the shared region that
	// will hold the stackframe of this function.
	// This field is set in the firstpass and used in
	// the secondpass through the field firstpass.
	// This field is set only when the field stackframeholder is set.
	// This field is never set for the root function.
	lyricalsharedregion* sharedregiontouse;
	
	// This field point to a linkedlist of lyricalsharedregion
	// where each element of the linkedlist represent a region
	// aligned to a uint within the shared region of a regular stackframe.
	// This field is set in the firstpass and used in the secondpass
	// through the field firstpass.
	// This field is set only when the field stackframeholder is null.
	lyricalsharedregion* sharedregions;
	
	// This field hold the size of the shared region and
	// is set only when the field stackframeholder is null.
	// This field is set and used only in the secondpass.
	uint sharedregionsize;
	
	// This field hold the size of the stackframe pointers cache
	// and is set only when the field stackframeholder is null.
	// This field is set and used only in the secondpass.
	uint stackframepointerscachesize;
	
	// Used to represent the maximum size that local variables
	// of the function will require.
	// For functions other than the root function, the actual size
	// required for local variables change dynamically as variables
	// get allocated and freed with varalloc() and varfree(); and
	// this field hold the maximum size that it will ever grow to be.
	uint vlocalmaxsize;
	
	// This field hold the function stack memory usage.
	// If the function is a stackframeholder, its value
	// do not include the shared region memory usage.
	// This field is set only in the secondpass.
	uint stackusage;
	
	// This field get incremented
	// each time this function is called.
	// It is set only in the firstpass,
	// and used in the secondpass.
	uint wasused;
	
	// This field is set if the function associated with
	// this lyricalfunction get called recursively.
	// This field is set and used only in the firstpass.
	uint isrecursive;
	
	// This flag is set if the function is a variadic function.
	uint isvariadic;
	
	// This field is set in the firstpass within readvarorfunc()
	// when the name of this function is used by the programmer
	// for the purpose of obtaining its address or when this function
	// is set for import/export, or when the keyword "this"
	// is used within this function.
	// In the secondpass, this field is used through the field firstpass.
	uint itspointerisobtained;
	
	// This field is set in the firstpass when
	// this function use the keyword "this".
	uint usethisvar;
	
	// This field is set only in the firstpass.
	// This field is set within funcdeclaration()
	// when the lyricalfunction could have had its field
	// stackframeholder set but was found to have a large
	// stack usage in the secondpass of the previous
	// compilation attempt within reviewdatafromsecondpass().
	// This field is set within reviewdatafromfirstpass()
	// when a lyricalfunction must be a stackframe holder
	// because it called a function that is a stackframe holder
	// while being called by a function that is a stackframe holder.
	// This field is set within callfunctionnow() and
	// assembly.evaluateexpression.parsestatement.lyrical.c
	// when calling a function through a pointer.
	uint couldnotgetastackframeholder;
	
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
