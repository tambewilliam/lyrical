
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


// This function add to the tree given as argument, a node with
// the value given as argument; associate it with the pointer
// to data given as argument and return null.
// If a node with the value given as argument already exist in the tree,
// the pointer to data given as argument get associated with the already
// existing node and the old pointer to data that was associated with
// that node is returned.
void* bintreeadd (bintree* tree, uint value, void* data) {
	
	bintreenode* node = tree->root;
	
	if (node) {
		// This loop goes down the tree until it reach
		// the leaf where the new node is to be added.
		while (1) {
			// If a node with the same value as the value
			// to add to the tree is found, the pointer to the data
			// associated with the node found is replaced and returned.
			if (value == node->value) {
				
				XORXCHG(*(uint*)&node->data, *(uint*)&data);
				
				return data;
			}
			
			bintreenode* o = node->children[value > node->value];
			
			if (o) node = o;
			else break;
		}
		
		// I get here when the leaf of the tree is reached.
		
		uint i = value > node->value;
		
		bintreenode* newnode = mmalloc(sizeof(bintreenode));
		
		node->children[i] = newnode;
		
		newnode->value = value;
		newnode->balance = 0;
		newnode->parent = node;
		newnode->children[0] = 0;
		newnode->children[1] = 0;
		newnode->data = data;
		
		// The balance of the node on which the new node is being added
		// can never become -2 or 2; so there is no need to check it.
		if (i) ++node->balance;
		else --node->balance;
		
		// This loop go up the tree and update balance factors.
		while (node->balance && node->parent) {
			
			skiptest:
			
			i = node->value > node->parent->value;
			
			node = node->parent;
			
			// If i is null then the node was added on
			// the left of the node pointed by node,
			// otherwise the node was added on the right.
			if (i) {
				
				if (++node->balance == 2) {
					
					balanceleft(node);
					
					node = node->parent;
					
					// Here the node that was parent to the node which
					// was rotated need to have his children field updated.
					// If the node that was rotated was the root node,
					// tree->root is updated to the new root node.
					if (node->parent) {
						
						node->parent->children[node->value > node->parent->value] = node;
						
						if (node->balance) goto skiptest;
						else return 0;
						
					} else {
						
						tree->root = node;
						
						return 0;
					}
				}
				
			} else {
				
				if (--node->balance == -2) {
					
					balanceright(node);
					
					node = node->parent;
					
					// Here the node that was parent to the node which
					// was rotated needs to have his children field updated.
					// If the node that was rotated was the root node,
					// tree->root is updated to the new root node.
					if (node->parent) {
						
						node->parent->children[node->value > node->parent->value] = node;
						
						if (node->balance) goto skiptest;
						else return 0;
						
					} else {
						
						tree->root = node;
						
						return 0;
					}
				}
			}
		}
		
		return 0;
		
	} else {
		// I get here if there was no node in the tree.
		
		bintreenode* newnode = mmalloc(sizeof(bintreenode));
		
		tree->root = newnode;
		
		newnode->value = value;
		newnode->balance = 0;
		newnode->parent = 0;
		newnode->children[0] = 0;
		newnode->children[1] = 0;
		newnode->data = data;
		
		return 0;
	}
}
