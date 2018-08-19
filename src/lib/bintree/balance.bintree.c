
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


// Documentation about the rotation rules used
// to do balancing is found under doc/bintree/

// This function balance a node having a balance factor
// of -2 by rotating it right; and is only called on nodes
// having a balance factor of -2.
static void balanceright (bintreenode* node) {
	
	if (node->children[0]->balance == 1) {
		
		if (node->children[0]->children[1]->balance == 0) {
			
			node->children[0]->balance = 0;
			node->children[0]->children[1]->balance = -1;
			
		} else if (node->children[0]->children[1]->balance == 1) {
			
			node->children[0]->balance = -1;
			node->children[0]->children[1]->balance = -1;
			
		} else if (node->children[0]->children[1]->balance == -1) {
			
			node->children[0]->balance = 0;
			node->children[0]->children[1]->balance = -2;
		}
		
		node->children[0]->children[1]->parent = node;
		bintreenode* o = node->children[0]->children[1]->children[0];
		node->children[0]->children[1]->children[0] = node->children[0];
		node->children[0]->parent = node->children[0]->children[1];
		node->children[0] = node->children[0]->children[1];
		node->children[0]->children[0]->children[1] = o;
		if (o) o->parent = node->children[0]->children[0];
	}
	
	// node->balance is assumed to be -2.
	
	if (node->children[0]->balance == 0) {
		
		node->balance = -1;
		node->children[0]->balance = 1;
		
	} else if (node->children[0]->balance == -1) {
		
		node->balance = 0;
		node->children[0]->balance = 0;
		
	} else if (node->children[0]->balance == -2) {
		
		node->balance = 1;
		node->children[0]->balance = 0;
	}
	
	node->children[0]->parent = node->parent;
	node->parent = node->children[0];
	bintreenode* o = node->children[0]->children[1];
	node->children[0]->children[1] = node;
	node->children[0] = o;
	if (o) o->parent = node;
}


// This function balance a node having a balance factor
// of 2 by rotating it left; and is only called on nodes
// having a balance factor of 2.
static void balanceleft (bintreenode* node) {
	
	if (node->children[1]->balance == -1) {
		
		if (node->children[1]->children[0]->balance == 0) {
			
			node->children[1]->balance = 0;
			node->children[1]->children[0]->balance = 1;
			
		} else if (node->children[1]->children[0]->balance == -1) {
			
			node->children[1]->balance = 1;
			node->children[1]->children[0]->balance = 1;
			
		} else if (node->children[1]->children[0]->balance == 1) {
			
			node->children[1]->balance = 0;
			node->children[1]->children[0]->balance = 2;
		}
		
		node->children[1]->children[0]->parent = node;
		bintreenode* o = node->children[1]->children[0]->children[1];
		node->children[1]->children[0]->children[1] = node->children[1];
		node->children[1]->parent = node->children[1]->children[0];
		node->children[1] = node->children[1]->children[0];
		node->children[1]->children[1]->children[0] = o;
		if (o) o->parent = node->children[1]->children[1];
	}
	
	// node->balance is assumed to be 2.
	
	if (node->children[1]->balance == 0) {
		
		node->balance = 1;
		node->children[1]->balance = -1;
		
	} else if (node->children[1]->balance == 1) {
		
		node->balance = 0;
		node->children[1]->balance = 0;
		
	} else if (node->children[1]->balance == 2) {
		
		node->balance = -1;
		node->children[1]->balance = 0;
	}
	
	node->children[1]->parent = node->parent;
	node->parent = node->children[1];
	bintreenode* o = node->children[1]->children[0];
	node->children[1]->children[0] = node;
	node->children[1] = o;
	if (o) o->parent = node;
}
