
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


// This function increment the reference count
// of the memory block given as argument.
void mmrefup (void* ptr) {
	// Substracting (sizeof(mmblock) - 2*sizeof(mmblock*)) from
	// the address in ptr set it at the beginning of the block.
	mmblock* b = (mmblock*)(ptr - (sizeof(mmblock) - 2*sizeof(mmblock*)));
	
	#ifdef MMCHECKSIGNATURE
	// If b point to a memory location that is not a block
	// which belong to this instance of the memory manager,
	// mmbadsignature() is called.
	if (b->signature != (uint)&mmarrayofpointerstolinkedlistoffreeblocks)
		mmbadsignature();
	#endif
	
	// There is no need to check whether
	// b->refcnt == -1, because it will
	// never be the case as it would mean
	// that every single memory location
	// is referencing the memory block.
	++b->refcnt;
}
