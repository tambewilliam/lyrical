
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


// Implementation of a binary tree which is kept balanced
// as nodes are inserted or removed from the tree.

#ifndef LIBBINTREE
#define LIBBINTREE

// Structure representing a binary tree.
// A bintree should be zeroed to initialize it.
typedef struct {
	
	void* ptr;
	
} bintree;

// Null value useful for initialization.
#define bintreenull ((bintree){.ptr = 0})

// Find a node from the tree that has the value given
// as argument and return a pointer to the data associated
// with the node. A null pointer is returned if a node having
// the value given as argument could not be found.
void* bintreefind (bintree tree, uint value);

// These functions scan a tree up (From the smallest node value to the
// largest node value) or down (From the largest node value to the smallest
// node value); and for each node, the value and a reference to the memory
// location holding the pointer to the data associated with it is passed
// to the callback function; allowing the callback function to modify
// the pointer value to the data associated with the node.
// Scanning through the tree is interrupted if the callback function return 0.
void bintreescanup (bintree tree, uint (*callback)(uint value, void** data));
void bintreescandown (bintree tree, uint (*callback)(uint value, void** data));

// This function add to the tree given as argument, a node with
// the value given as argument; associate it with the pointer
// to data given as argument and return null.
// If a node with the value given as argument already exist in the tree,
// the pointer to data given as argument get associated with the already
// existing node and the old pointer to data that was associated with
// that node is returned.
void* bintreeadd (bintree* tree, uint value, void* data);

// This function remove the node with the value given as argument
// from the tree given as argument.
// If the node to remove from the tree is found, the pointer to the data
// associated with it is passed to the callback function before its removal.
// The pointer argument for the callback function can be null.
void bintreeremove_ (bintree* tree, uint value, void (*callback)(void* data));
void bintreeremove (bintree* tree, uint value);

// This function remove all nodes of the tree given
// as argument from the leaf nodes going right;
// and set the field root of the bintree to null.
// Before removing each node, their value and the pointer to the data
// associated with them is passed to the callback function.
// The pointer argument for the callback function can be null.
void bintreeempty_ (bintree* tree, void (*callback)(uint value, void* data));
void bintreeempty (bintree* tree);

#endif
