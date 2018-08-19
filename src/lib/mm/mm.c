
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


// Memory manager which support up to 64bits CPUs.
// The current limitation for 64bits can be lifted
// by increasing the value of MMSMALLESTBUDDYCLOG2.
// 
// -------------------------------------------------------
// Through a syscall, the kernel make memory
// available to a thread in pagesize of 4096 bytes.
// A memory manager wouldn't be needed if memory
// requests made by a thread were amounts that were
// all multiples of a pagesize; but in reality
// a thread will need smaller amounts or amounts
// that are not multiples of a pagesize.
// ei: If only 14 bytes is needed, it would be
// a waist to allocate 4096 bytes everytime that
// a small amount of memory is needed. Hence the need
// for a memory manager which will allow a thread to
// judiciously and efficiently partition memory pages
// that it receives from the kernel.
// ei: Two allocations of 14 bytes each will be
// allocated from the same 4096 bytes that
// was requested from the kernel.
// 
// To perform its task, the memory manager will use
// the buddy system algorithm to partition the memory
// pages that it receives from the kernel when possible.
// In fact the buddy system algorithm will partition
// the memory pages that it receive from the kernel
// in blocks having sizes that are powers of two.
// If using the buddy system algorithm will result in
// a waist of memory, the memory manager do not use
// the buddy system algorithm, but instead find the best
// multiple of a pagesize that satisfy the memory request.
// 
// 
// ==== Description of the buddy system algorithm ====
// 
// Allocation is done in blocks of sizes that are powers
// of two; in a 32bits system, the minimum block size
// being 2^6 and maximum block size being 2^31;
// a linkedlist of free blocks for each block size
// is maintained, in other words, there is a linkedlist
// of free blocks for each block size: 2^6, 2^7, 2^8,
// 2^9, 2^10,...,2^31; hence a total of 26 linkedlists.
// 
// Block size are chosen as powers of two so that they can
// easily be splitted in two. ei: a block of size 2^10 can
// be splitted into two blocks of size 2^9; the two blocks
// resulting from the split are called buddies.
// By the same token they are chosen as power of two so
// that buddies can easily be merged together to form
// a single block. ei: Two buddies of size 2^11 are merged
// into a single block of size 2^12.
// 
// When a memory allocation of a particular size
// is requested, the size is rounded up to a valid
// block size which is a power of two. The power of
// two size is then used to obtain the desired block
// from the appropriate linkedlist of free blocks.
// ei: Requesting 10kb of memory will get a block from
// the linkedlist of free blocks of size 16kb or 2^14;
// if the linkedlist for that block size is empty,
// a block from a larger block size is splitted down to
// the needed block size.
// 
// When a memory block is freed, to prevent fragmentation,
// buddies are merged together if both of them are not
// being used. The result of the merge is added to the
// appropriate linkedlist of free blocks.
// 
// The main advantage of the buddy memory allocation system
// is that allocation is very fast and merging of blocks,
// to prevent fragmentation, is cheap because the buddy
// of any block can be calculated from its address.
// ----------------------------------------------------------


// This check is needed because this library
// used non-standard C language extension
// such as nested functions.
#ifndef __GNUC__
#error GCC compatible compiler needed.
#endif


#include <sys/mman.h>
#ifndef MAP_STACK
#define MAP_STACK 0x20000
#endif
#ifndef MAP_UNINITIALIZED
#define MAP_UNINITIALIZED 0x4000000
#endif

// Not all platforms support execinfo.h .
#if defined(__linux__)
#include <execinfo.h>
#endif

#include <stdtypes.h>
#include <macros.h>
#include <byt.h>

// Uncomment this macro to enable
// threadsafety, when threads
// share the same address space.
//#define MMTHREADSAFE

#ifdef MMTHREADSAFE
#include <mutex.h>
#endif

// When this macro is defined,
// additional checks are performed
// to insure the integrity of
// the memory manager.
//#define MMCHECKSIGNATURE

// The following platform dependent macros
// are set depending on sizeof(mmblock) and
// are set so to handle up to a 64bits system.
// MMSMALLESTBUDDYCLOG2	clog2(sizeof(mmblock))			# 8
// MMSMALLESTBUDDY	(1<<MMSMALLESTBUDDYCLOG2)		# 256
// MMINDEXOF4096BUDDY	(clog2(4096)-MMSMALLESTBUDDYCLOG2)	# 4
#define MMSMALLESTBUDDYCLOG2	8
#define MMSMALLESTBUDDY		(1<<MMSMALLESTBUDDYCLOG2)
#define MMINDEXOF4096BUDDY	4

// I determine how many linkedlist I would need.
// The first linkedlist represent the list of the smallest
// allocatable block which is 2^8 == 256.
// The last linkedlist represent the largest allocatable
// block which is 2^31 in a 32bits system.
// Note that the maximum amount of memory in a 32bits system
// is 2^32 and it do not make sens to have a linkedlist for
// free blocks which have the size of the total memory,
// because memory will always be used. Hence the number of
// linkedlists needed in a 32bits system would be ((31-8)+1)
// or 32-8 == 24; (MMNBROFLINKEDLIST-1) should be less
// than 2^8 == 256 (minimum block size), because
// mmsizetoindexforblockselection() will also return values
// which are greater than (MMNBROFLINKEDLIST-1) and which
// are actual size in bytes as oppose to an index to
// mmarrayofpointerstolinkedlistoffreeblocks.
#define MMNBROFLINKEDLIST ((sizeof(uint)*8)-MMSMALLESTBUDDYCLOG2)


// Structure representing a session.
struct mmsessioninternal;

// Structure representing the header of a memory block.
// The minimum buddy system block size is calculated
// so as to be the smallest size that has enough bytes
// for the struct mmblock.
typedef struct mmblock {
	// Pointers to the previous and next block
	// within the same session.
	struct mmblock* prevsessionblock;
	struct mmblock* nextsessionblock;
	
	#ifdef MMTHREADSAFE
	// This field is used when I need to reach
	// the field mutex of the session
	// to which this block belong to.
	struct mmsessioninternal* session;
	#endif
	
	#ifdef MMCHECKSIGNATURE
	// I set this field using the address of
	// mmarrayofpointerstolinkedlistoffreeblocks which
	// guaranty that this block belong to a particular
	// instance of the memory manager, since
	// mmarrayofpointerstolinkedlistoffreeblocks is unique
	// to an instance of the memory manager and will have
	// a different address for a different instance
	// of the memory manager.
	// This field is set wherever the field refcnt is non-null
	// and it is checked only in locations where the programmer
	// provide the memory manager with an address.
	uint signature;
	#endif
	
	#ifdef MMDEBUG
	// Maximum number of addresses that can be
	// stored in mmblock.debug.backtrace;
	// MMSMALLESTBUDDYCLOG2 must be large enough
	// to accomodate the size of mmblock.debug.backtrace.
	#define MMDEBUGCALLBACKCOUNT 16
	struct {
		// These fields are set by mmdebugalloc(), mmdebugallocz()
		// and mmdebugrealloc(); and are used by mmdebugsession().
		// The field changecount keep the number of time the block
		// was reallocated.
		// The field lastchangefilename keep the name of the file
		// within which the block was allocated or last reallocated.
		// The field lastchangelinenumber keep the line number within
		// the file specified by lastchangefilename.
		// The string set in the field lastchangefilename will always
		// be a constant string generated by the compiler since it will
		// come from the preprocessor macro FILE; hence it will never
		// be a string allocated by the memory manager.
		// The field lastchangelinenumber is valid only if the field
		// lastchangefilename is set.
		// The field backtrace is an array of pointers, which are
		// return addresses from their corresponding stackframes;
		// it is a snapshot of what the backtrace was at the time
		// the memory block was allocated.
		uint changecount;
		u8* lastchangefilename;
		uint lastchangelinenumber;
		void* backtrace[MMDEBUGCALLBACKCOUNT];
		
	} debug;
	#endif
	
	// Address where the memory region being managed (to which
	// this block belong) start and its size.
	void* startofregion;
	uint sizeofregion;
	
	// Reference count of this memory block.
	// When null, the memory block is not in use.
	uint refcnt;
	
	// Represent how much of the block that can
	// be copied as oppose to copy the whole block
	// during memory reallocation.
	// It is set using the requested size given
	// to mmalloc() or mmrealloc().
	uint size;
	
	// Index in mmarrayofpointerstolinkedlistoffreeblocks
	// which translate to the size of the block as well,
	// since a particular index correspond to
	// a linkedlist of a specific size.
	uint index;
	
	// Destructor callback pointer which when non-null
	// get called before the memory block is freed.
	// The destructor receive the memory block address
	// as argument.
	void(* dtor)(void*);
	
	// The fields below point to the previous
	// and next free block. These fields are used only
	// when the block is free, because when the block
	// is used, these fields are within the data area
	// of the block.
	struct mmblock* prev;
	struct mmblock* next;
	
} mmblock;

// Array of pointers to linkedlists of free blocks.
static mmblock* mmarrayofpointerstolinkedlistoffreeblocks[MMNBROFLINKEDLIST];

#ifdef MMTHREADSAFE
// Mutex for each linkedlist to protect
// in arrayofpointerstolinkedlistoffreeblocks.
static mutex mmfreeblocksmutexes[MMNBROFLINKEDLIST];
#endif

// Function which will be automatically
// called by GCC before main().
__attribute__ ((__constructor__))
static void mmctor () {
	
	uint i = 0;
	
	// This loop initialize
	// mmarrayofpointerstolinkedlistoffreeblocks
	// and mmfreeblocksmutexes.
	do {
		mmarrayofpointerstolinkedlistoffreeblocks[i] = 0;
		
		#ifdef MMTHREADSAFE
		mmfreeblocksmutexes[i] = mutexnull;
		#endif
		
	} while (++i < MMNBROFLINKEDLIST);
}


// There is no check on the validity of the index,
// or whether the linkedlist at that index has
// any blocks, so it has to be a good index.
// This function do not manipulate session fields.
static mmblock* mmgetblockfromfreeblocks (uint index) {
	#ifdef MMTHREADSAFE
	mutexlock(&mmfreeblocksmutexes[index]);
	#endif
	
	// Get the block at the top of the linkedlist.
	mmblock* b = mmarrayofpointerstolinkedlistoffreeblocks[index];
	
	if (b) {
		// Remove the block from the linkedlist.
		LINKEDLISTCIRCULARREMOVE(prev, next, b, mmarrayofpointerstolinkedlistoffreeblocks[index]);
		
		b->refcnt = 1;
		
		#ifdef MMCHECKSIGNATURE
		b->signature = (uint)&mmarrayofpointerstolinkedlistoffreeblocks;
		#endif
		
		#ifdef MMDEBUG
		b->debug.changecount = 0;
		b->debug.lastchangefilename = 0;
		#endif
		
		b->dtor = (void(*)(void*))0;
	}
	
	#ifdef MMTHREADSAFE
	mutexunlock(&mmfreeblocksmutexes[index]);
	#endif
	
	// No need to set the field index of the block because
	// it would have already been set by any functions
	// adding the block to the linkedlist.
	
	return b;
}


// There is no check on the validity of index,
// and b has to hold a valid address well aligned
// to have a block buddy.
// This function do not manipulate session fields.
static void mmaddblocktofreeblocks (mmblock* b, uint index) {
	#ifdef MMCHECKSIGNATURE
	// Set the signature of the block back to zero
	// to maximize its efficiency. I don't want to have
	// that signature lying around everywhere in memory
	// because it increase the likelyhood to have that
	// signature value right where it would cause
	// an errouneous pointer to be mistakenly seen
	// as a corrrect block.
	b->signature = 0;
	#endif
	
	// If the block that I am adding to the linkedlist
	// of free blocks has the size of the region to which
	// it belong that was previously obtained from the kernel,
	// I return that memory to the kernel by unmapping it.
	if (b->sizeofregion == (MMSMALLESTBUDDY<<index)) {
		munmap(b, b->sizeofregion);
		return;
	}
	
	#ifdef MMTHREADSAFE
	mutexlock(&mmfreeblocksmutexes[index]);
	#endif
	
	LINKEDLISTCIRCULARADDTOBOTTOM(prev, next, b, mmarrayofpointerstolinkedlistoffreeblocks[index]);
	
	b->index = index;
	
	b->refcnt = 0;
	
	#ifdef MMTHREADSAFE
	mutexunlock(&mmfreeblocksmutexes[index]);
	#endif
}


// For speed, there is no check on the validity of the index.
// That work is to be done before calling this function.
static mmblock* mmallocateblock (uint index) {
	// This function is essentially the same as
	// mmaddblocktofreeblocks() with the exception that
	// in this function I do not check whether the block
	// to add(that was previously obtained from the kernel)
	// has the size of its memory region to which it belong;
	// otherwise I would have a infinite loop because
	// the block would get allocated by getmemory() but
	// a call to mmaddblocktofreeblocks() would give it back
	// to the kernel before getmemory() return successful;
	// the code would then think that memory was successfully
	// allocated and try to look for a free block, find that
	// there is no block free and call again getmemory(),
	// causing an infinite loop.
	void addnewblocktofreeblocks (mmblock* b, uint index) {
		#ifdef MMCHECKSIGNATURE
		// Set the signature of the block back to zero
		// to maximize its efficiency. I don't want to have
		// that signature lying around everywhere in memory
		// because it increase the likelyhood to have that
		// signature value right where it would cause
		// an errouneous pointer to be mistakenly seen
		// as a corrrect block.
		b->signature = 0;
		#endif
		
		#ifdef MMTHREADSAFE
		mutexlock(&mmfreeblocksmutexes[index]);
		#endif
		
		LINKEDLISTCIRCULARADDTOBOTTOM(prev, next, b, mmarrayofpointerstolinkedlistoffreeblocks[index]);
		
		b->index = index;
		
		b->refcnt = 0;
		
		#ifdef MMTHREADSAFE
		mutexunlock(&mmfreeblocksmutexes[index]);
		#endif
	}
	
	// This function will directly attach to the array
	// of pointers to linkedlist of free blocks the memory
	// it get from the kernel. The kernel give out memory
	// in multiples of a pagesize.
	void getmemory (uint index) {
		// Index MMINDEXOF4096BUDDY is for blocks having
		// the size of a single page which is 4096 bytes.
		// Memory is allocated by the kernel in pagesize,
		// hence I can only request from the kernel, memory
		// which have a size multiple of a pagesize.
		if (index < MMINDEXOF4096BUDDY) index = MMINDEXOF4096BUDDY;
		
		void* memfromkernel;
		
		// I request memory from the kernel; retrying on failure.
		do memfromkernel = mmap(0, (MMSMALLESTBUDDY<<index), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED, 0, 0);
			while (memfromkernel == (void*)-1);
		
		// Note that the use of the buddy system algorithm
		// is localized to a region that was obtained from the kernel.
		// So any missuse of a block within that region will not affect
		// any other block from other region obtained from the kernel.
		
		((mmblock*)memfromkernel)->startofregion = memfromkernel;
		((mmblock*)memfromkernel)->sizeofregion = (MMSMALLESTBUDDY<<index);
		
		// The block of size (MMSMALLESTBUDDY<<index) obtained from the kernel
		// can now be added to its linkedlist of free block.
		addnewblocktofreeblocks((mmblock*)memfromkernel, index);
	}
	
	mmblock* b;
	
	allocblk:
	b = mmgetblockfromfreeblocks(index);
	
	if (b) return b;
	else {
		// If I get here, then I need to look for
		// a larger block to break down and use.
		uint i = index;
		
		while (++i < MMNBROFLINKEDLIST) {
			
			if (b = mmgetblockfromfreeblocks(i)) {
				// If I get here, I was able to get a block
				// that I can break down.
				
				// I check whether the index in i is greater
				// than the variable index only at the end since
				// I previously incremented i.
				do {
					--i;
					
					// MMSMALLESTBUDDY is the minimum size of a block,
					// so adding to (void*)b the size of the blocks indexed
					// by i will get me half way the size of the block in b.
					// One of the half is returned to the free blocks and
					// its fields startofregion and sizeofregion are set.
					mmblock* buddy = (void*)b + (MMSMALLESTBUDDY<<i); // This variable will point to the buddy block.
					
					buddy->startofregion = b->startofregion;
					buddy->sizeofregion = b->sizeofregion;
					
					addnewblocktofreeblocks(buddy, i);
					
				} while (i > index);
				
				// When I get here, I am sure that the block
				// has been broken down to the size that I wanted.
				// I can now prepare and return it.
				
				// Set the index.
				b->index = i;
				
				// The block was already marked as used
				// by mmgetblockfromfreeblocks() so no need
				// to mark it again.
				
				// The field size is not set by this function,
				// but by the function which called it.
				
				// Return the address of the block.
				return b;
			}
		}
		
		// If I get here I reached the max index and
		// there was no linkedlists with useable memory,
		// so I need to get more memory.
		getmemory(index);
		
		goto allocblk;
	}
}


// This function convert a size in byte to an index
// in mmarrayofpointerstolinkedlistoffreeblocks.
// If the index computed is greater than (MMNBROFLINKEDLIST-1)
// or if the power of two block size indexed in
// mmarrayofpointerstolinkedlistoffreeblocks is larger
// by a pagesize than the size given as argument,
// there will be a waist if the buddy system algorithm
// is used; so the size given as argument rounded up
// to a pagesize is returned and will be greater
// than (MMNBROFLINKEDLIST-1), signalling that
// the returned value should not be used to index
// mmarrayofpointerstolinkedlistoffreeblocks.
static uint mmsizetoindexforblockselection (uint size) {
	
	uint i = 0, j = MMSMALLESTBUDDY; // MMSMALLESTBUDDY == 2^MMSMALLESTBUDDYCLOG2 is the minimum size of a block.
	
	while (j < size) {
		++i;
		j <<= 1; // Multiply j by 2 everytime.
	}
	
	if (i == MMNBROFLINKEDLIST || (j - size) >= 4096) {
		return ROUNDUPTOPOWEROFTWO(size, 4096);
	}
	
	return i;
}


// This function return a block large enough to contain
// the amount of bytes given by the argument size.
// mmallocblock(0) will return a valid block.
// The returned block is not part of any session.
static mmblock* mmallocblock (uint size) {
	
	mmblock* b;
	
	// This variable hold the actual size of the block
	// that get set in the field size of a struct mmblock.
	// (sizeof(mmblock) - 2*sizeof(mmblock*)) is the space
	// occupied by the mmblock header where (2*sizeof(mmblock*))
	// account for the fields prev and next of a struct mmblock
	// which are not part of the mmblock header but are used
	// to link free blocks together.
	uint blocksize = size + (sizeof(mmblock) - 2*sizeof(mmblock*));
	
	// Get the index within the array of free blocks.
	uint i = mmsizetoindexforblockselection(blocksize);
	
	if (i >= MMNBROFLINKEDLIST) {
		// If I get here, allocation should be done without using
		// the buddy system algorithm as it would yield waist; instead,
		// the best multiple of a pagesize that satisfy the memory request
		// is allocated.
		
		// I request memory from the kernel; retrying on failure.
		do b = (mmblock*)mmap(0, i, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED, 0, 0);
			while (b == (mmblock*)-1);
		
		#ifdef MMCHECKSIGNATURE
		b->signature = (uint)&mmarrayofpointerstolinkedlistoffreeblocks;
		#endif
		
		#ifdef MMDEBUG
		b->debug.changecount = 0;
		b->debug.lastchangefilename = 0;
		#endif
		
		b->refcnt = 1;
		
		b->index = i;
		
		// I set the useable size of the block.
		b->size = blocksize;
		
		b->dtor = (void(*)(void*))0;
		
		return b;
		
		// The fields startofregion and sizeofregion
		// of the block are not set since it was not
		// allocated using the buddy system algorithm.
	}
	
	// I call mmallocateblock() to allocate a block.
	b = mmallocateblock(i);
	
	// I set the useable size of the block.
	b->size = blocksize;
	
	return b;
}


// Structure representing a session.
typedef struct mmsessioninternal {
	// Pointers to the previous and next block
	// within the same session.
	// I need to remember that the struct session
	// also act as a dumb struct mmblock so that I can
	// create a circular linkedlist of blocks belonging
	// to the same session and always have a pointer to
	// at least one element of the circular linkedlist
	// when I need to free all blocks of a session.
	struct mmblock* prevsessionblock;
	struct mmblock* nextsessionblock;
	
	// Pointers to the previous and next session.
	// In fact, sessions form a linkedlist.
	// The first session of the linkedlist always has
	// its field prevsession set to null, while the last
	// session of the linkedlist always has its field
	// nextsession set to null.
	struct mmsessioninternal* prevsession;
	struct mmsessioninternal* nextsession;
	
	#ifdef MMTHREADSAFE
	// Mutex used to protect
	// this session linkedlist
	// of blocks.
	mutex mutex;
	#endif
	
} mmsessioninternal;


// This function will free a previously allocated
// block and perform defragmentation if necessary.
// If flag is null, no attempt is done to detach
// the block from its session linkedlist.
static void mmfreeblock (mmblock* b, uint flag) {
	// Call the destructor if it is set.
	if (b->dtor) {
		// Adding (sizeof(mmblock) - 2*sizeof(mmblock*)) to
		// the address in b set it where the block data begin.
		b->dtor((void*)b + (sizeof(mmblock) - 2*sizeof(mmblock*)));
	}
	
	if (flag) {
		#ifdef MMTHREADSAFE
		mutexlock(&b->session->mutex);
		#endif
		
		// I detach the block from its session linkedlist.
		LINKEDLISTCIRCULARREMOVE(prevsessionblock, nextsessionblock, b, ((mmblock*){(mmblock*)0}));
		
		#ifdef MMTHREADSAFE
		mutexunlock(&b->session->mutex);
		#endif
	}
	
	// Get the index of the block.
	uint i = b->index;
	
	if (i >= MMNBROFLINKEDLIST) {
		// If I get here, the block to free was not
		// allocated using the buddy system algorithm,
		// and should be returned to the kernel
		// by unmapping it.
		
		#ifdef MMCHECKSIGNATURE
		// Set the signature of the block back to zero
		// to maximize its efficiency. I don't want to have
		// that signature lying around everywhere in memory
		// because it increase the likelyhood to have that
		// signature value right where it would cause
		// an errouneous pointer to be mistakenly seen
		// as a corrrect block.
		b->signature = 0;
		#endif
		
		munmap(b, i);
		
		return;
	}
	
	// I get here if the block to free was allocated
	// using the buddy system algorithm. I do the merging
	// of buddy blocks.
	
	findbuddy:;
	
	// This variable will point to the buddy block.
	mmblock* buddy;
	
	// The check below test whether I am at an even
	// or odd addressing with respect to the type of blocks
	// indexed by i. The test below is a faster version of:
	// ((((uint)b - (uint)b->startofregion)/(MMSMALLESTBUDDY<<i)) & 1)
	if ((((uint)b - (uint)b->startofregion) >> (MMSMALLESTBUDDYCLOG2+i)) & 1) {
		// If I get here, it mean that the block pointed
		// by b is at the right of its buddy, and the buddy
		// is at the left of the block pointed by b.
		
		// A block found to be at the right of its buddy
		// will always have a buddy at its left, because
		// (((uint)b - (uint)b->startofregion) >> (MMSMALLESTBUDDYCLOG2+i))
		// used in the test above was certainly not null which mean
		// that there was certainly a space at the left of the
		// block within the region where the block belong.
		// So there is no need to check if I can have
		// a legitimate buddy on the left, just like I need
		// to do for blocks having their buddy at their right.
		
		buddy = (mmblock*)((void*)b - (MMSMALLESTBUDDY<<i));
		
		#ifdef MMTHREADSAFE
		// I lock the linkedlist with index i
		// to safely check and modify it.
		mutexlock(&mmfreeblocksmutexes[i]);
		#endif
		
		// Check whether the buddy is not inuse
		// and that it was not splitted.
		if (!buddy->refcnt && buddy->index == i) {
			// If I get here I need to combine both block.
			
			// I detach the buddy from the linkedlist indexed by i.
			LINKEDLISTCIRCULARREMOVE(prev, next, buddy, mmarrayofpointerstolinkedlistoffreeblocks[i]);
			
			// I can update b to the address of the left buddy
			// since it will be the head of the combined block.
			b = buddy;
			
			#ifdef MMTHREADSAFE
			mutexunlock(&mmfreeblocksmutexes[i]);
			#endif
			
			// I increase i without setting the fields index
			// and refcnt of the newly combined block yet;
			// it will be done at the end.
			++i;
			
			// I try to find another buddy.
			goto findbuddy;
			
		} else {
			#ifdef MMTHREADSAFE
			mutexunlock(&mmfreeblocksmutexes[i]);
			#endif
		}
		
	} else {
		// If I get here the buddy is at the right
		// of the block pointed by b.
		
		buddy = (mmblock*)((void*)b + (MMSMALLESTBUDDY<<i));
		
		// I first check that I can have a legitimate
		// buddy on the right, by making sure that the address
		// of the first byte within the buddy at the right
		// is not beyond the end boundary of the memory region
		// to which the block belong.
		if ((void*)buddy < (b->startofregion + b->sizeofregion)) {
			#ifdef MMTHREADSAFE
			// I lock the linkedlist with index i
			// to safely check and modify it.
			mutexlock(&mmfreeblocksmutexes[i]);
			#endif
			
			// Check whether the buddy is not inuse
			// and that it was not splitted.
			if (!buddy->refcnt && buddy->index == i) {
				// If I get here I need to combine both block.
				
				// I detach the buddy from the linkedlist indexed by i.
				LINKEDLISTCIRCULARREMOVE(prev, next, buddy, mmarrayofpointerstolinkedlistoffreeblocks[i]);
				
				// b is set to the address where the head
				// of the combined block is located.
				
				#ifdef MMTHREADSAFE
				mutexunlock(&mmfreeblocksmutexes[i]);
				#endif
				
				// I increase i without setting the fields index
				// and refcnt of the newly combined block yet;
				// it will be done at the end.
				++i;
				
				// I try to find another buddy.
				goto findbuddy;
				
			} else {
				#ifdef MMTHREADSAFE
				mutexunlock(&mmfreeblocksmutexes[i]);
				#endif
			}
		}
	}
	
	// Getting here mean that I did all the possible mergings.
	
	// I add to the linkedlist indexed by i,
	// the block pointed by b.
	mmaddblocktofreeblocks(b, i);
	
	// Note that during merging of blocks,
	// I did not modify any of the fields
	// startofregion or sizeofregion of blocks
	// because I am working only within
	// the region of the block being freed.
}


// Garbage collection is inherently implemented
// through the use of memory sessions.
// 
// mmsessionnew() create a new memory session and
// return a handler for the session; the newly created
// session is a subsession and become the current session.
// Any subsequent call to mmalloc() or mmallocz()
// allocate blocks of memory within the newly created
// session; similarly any subsequent call to mmsessionnew()
// create subsessions.
// 
// mmsessionfree() free the session given as argument;
// any memory allocated within that session is freed.
// The samething is done to subsessions if the argument
// flag == MMDOSUBSESSIONS.
// Allocated memory block within a session can be given
// as arguments to mmsessionfree() so that they do
// not get freed when their session get freed
// by mmsessionfree(). Those memory blocks will be moved
// to the parent session of the session being freed.
// If a block which do not belong to any of the session
// being freed is given as argument to mmsessionfree(),
// it will be moved to the parent session of
// the session being freed.
// If the session being freed is the current session,
// the parent session of the session being freed
// become the current session.
// 
// mmsessioncancel() simply cancel the session given
// as argument without freeing any memory allocated in
// in the session; memory blocks allocated within
// the session given as argument, get moved to the parent
// session of the session being canceled; the samething
// is done to subsessions if the argument flag == MMDOSUBSESSIONS.
// If the session being canceld is the current session,
// the parent session of the session being canceld
// become the current session.
// 
// mmrealloc() keep a block within its session
// when reallocating it.


#ifdef MMTHREADSAFE
// Mutex used to protect
// the linkedlist of sessions.
static mutex mmsessionmutex = mutexnull;
#endif

// Note that session information which are accessed
// through struct mmsessioninternal are found within the body
// of a memory block, not the header part of the memory block;
// and the same memory block has the fields prevsessionblock
// and nextsessionblock which link it to the session it belong to.

// I need to have at least one session,
// since mmalloc() cannot be used to allocate
// the first session, since it will need at least
// an existing session; so the first session is
// declared and mmcurrentsession later get
// initialized with its address.
static mmsessioninternal mmfirstsession = {
	// The two fields below are set to mmfirstsession
	// because the linkedlist of blocks belonging
	// to the same session is circular.
	// I need to remember that struct session also act
	// as a dumb block so that I can create a circular
	// linkedlist of blocks belonging to the same session
	// and always have a pointer to at least one element
	// of the circular linkedlist.
	.prevsessionblock = (mmblock*)&mmfirstsession,
	.nextsessionblock = (mmblock*)&mmfirstsession,
	
	.prevsession = 0,
	.nextsession = 0
	
	#ifdef MMTHREADSAFE
	,.mutex = mutexnull
	#endif
};

// This variable point to the session currently being used.
static mmsessioninternal* mmcurrentsession = &mmfirstsession;


typedef struct {
	
	mmsessioninternal* s;
	
} mmsession;


typedef enum {
	// When used, subsessions are ignored.
	MMIGNORESUBSESSIONS,
	
	// When used, subsessions are processed.
	MMDOSUBSESSIONS
	
} mmsessionflag;


#ifdef MMCHECKSIGNATURE
// Function which get called when
// a block bad signature is detected.
static void mmbadsignature () {
	(*(uint*)0) = 0; // To generate a pagefault.
}
#endif


#include "alloc.mm.c"
#include "sz.mm.c"
#include "realloc.mm.c"
#include "free.mm.c"

#include "setdtor.mm.c"

#include "refup.mm.c"
#include "refdown.mm.c"

#include "newsession.mm.c"
#include "sessionextract.mm.c"
#include "freesession.mm.c"
#include "cancelsession.mm.c"
#include "sessionusage.mm.c"

#ifdef MMDEBUG
#include "debug.mm.c"
#endif

#include "usage.mm.c"
