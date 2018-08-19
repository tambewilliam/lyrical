
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


// This file implement the functions needed to scan through
// a tree, to retrieve, add or remove a node from a tree.
// 
// As a tree grow, its nodes are naturally arranged in such a way that
// the value of the nodes from left to right goes in order ascending.
// 
// In order to keep the tree balanced, rotations are used.
// An inbalance occur when the balance factor of a node is -1, 1, -2 or 2,
// but a rotation is necessary only when the balance factor is -2 or 2.
// Keeping a tree balanced allow for a fast walk through the tree
// because the height of the tree is kept at its minimum.
// 
// The rotation rules used to do balancing are implemented in balance.bintree.c .
// 
// Adding a node to the tree:
// 
// - Go down the tree until the leaf node, on which the new node
// is to be attached, is reached. The balance of the node inserted is set to 0.
// 
// - From the node on the tree on which the new node was inserted,
// the balance factor of affected nodes is recalculated going up the tree.
// The balance factor of a node is decremented if the new node was inserted
// on its left, and incremented if the new node was inserted on its right.
// 
// - If incrementing or decrementing the balance factor of a node yield 0,
// I stop going up the tree because the added node would not have increased
// the height of the tree; hence no inbalance created.
// If incrementing or decrementing the balance factor of a node yield -1 or 1,
// I keep going up the tree because the height of the tree would have increased by one,
// creating an inbalance that affect the balance factor of parent nodes as well.
// If incrementing or decrementing the balance factor of a node yield -2 or 2,
// I perform a rotation, and depending on whether it yield a balance factor
// of 0, -1 or 1, I stop or keep going up the tree.
// 
// 
// Removing a node from the tree:
// 
// - Go down the tree until the node to remove is found.
// 
// - If the node to remove is a leaf, it is removed from the tree.
// If the node to remove is not a leaf, it is replaced with
// either the node having the largest value in its left subtree,
// or the node having the smallest value in its right subtree.
// The balance factor of the node to remove is used to determine
// whether I want to use its left or right subtree, so as to select
// the deepest subtree. The deepest subtree is selected because
// detaching the replacement node from that subtree will contribute
// in rebalancing the tree and save us a costly rotation which
// occur when the balance factor of a node is -2 or 2.
// The node found as a replacement is detached from the tree
// and replace the node to remove. The node found as a replacement
// can only have at most one subtree, which, if existing
// is re-attached to the tree.
// 
// - The balance factors of affected node is adjusted going up the tree.
// If the node to remove was a leaf, the balance factors adjustments
// start from its parent node if any.
// If the node to remove was replaced, rebalancing start from
// the parent of the node that was used as a replacement.
// The balance factor of a node is incremented if the node removal was done
// from its left subtree, and decremented if the node removal was done from
// its right subtree.
// If incrementing or decrementing the balance factor of a node yield -1 or 1,
// I stop going up the tree because the removed node would not have decreased
// the height of the tree; hence no inbalance created.
// If incrementing or decrementing the balance factor of a node yield 0,
// I keep going up the tree because the height of the tree would have decreased by one
// creating an inbalance that affect the balance factor of parents nodes as well.
// If incrementing or decrementing the balance factor of a node yield -2 or 2,
// I perform a rotation, and depending on whether it yield a balance factor
// of 0, -1 or 1, I stop or keep going up the tree.

#include <stdtypes.h>
#include <macros.h>
#include <mm.h>

// Structure representing a node.
typedef struct bintreenode {
	// Value of the node.
	uint value;
	
	// Balance factor of the node.
	sint balance;
	
	// Pointer to the parent node.
	struct bintreenode* parent;
	
	// Pointers to the left and right child nodes.
	struct bintreenode* children[2];
	
	// Pointer to any data to associate with the node.
	void* data;
	
} bintreenode;

// Structure representing a binary tree.
// A new bintree should be initialized to zero.
typedef struct {
	
	bintreenode* root;
	
} bintree;

#include "find.bintree.c"
#include "scan.bintree.c"
#include "balance.bintree.c"
#include "add.bintree.c"
#include "remove.bintree.c"
#include "empty.bintree.c"
