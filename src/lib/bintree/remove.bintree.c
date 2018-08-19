
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


// This function remove the node with the value given as argument
// from the tree given as argument.
// If the node to remove from the tree is found, the pointer to the data
// associated with it is passed to the callback function before its removal.
// The pointer argument for the callback function can be null.
void bintreeremove_ (bintree* tree, uint value, void (*callback)(void* data)) {
	
	bintreenode* node;
	
	// Here I look for the node to remove.
	if (node = tree->root) {
		
		while (1) {
			
			if (value == node->value) break;
			else if (!(node = node->children[value > node->value])) return;
		}
		
	} else return;
	
	// When I get here, the variable node is set
	// with the node to remove from the tree.
	
	if (callback) callback(node->data);
	
	// This variable is set to the node from where the rebalancing
	// of the tree going up the tree is to start once the node is removed.
	bintreenode* nodetobalance;
	
	// If node->balance is zero, it mean that
	// the node to remove has either 2 children or no children.
	// If node->balance is less than zero, it mean that
	// the node to remove certainly has a left child.
	// If node->balance is greater than zero, it mean that
	// the node to remove certainly has a right child.
	uint i = node->balance > 0;
	
	// Here I check whether the node to remove was a leaf node.
	// If it is not a leaf node, it is replaced with either
	// the node having the largest value in its left subtree, or
	// the node having the smallest value in its right subtree.
	// The balance factor of the node to remove is used to determine
	// whether I want to use its left or right subtree, so as to select
	// the deepest subtree. The deepest subtree is selected because
	// detaching the replacement node from that subtree will contribute
	// in rebalancing the tree and save us a costly rotation which occur when
	// the balance factor of a node is -2 or 2.
	if (node->children[i]) {
		// Getting here mean that the node
		// to remove has at least one child.
		
		bintreenode* nodetoremove = node->children[i];
		
		// This loop finds the node which is to replace the node
		// to remove; and it do so by finding the node having
		// the largest or smallest value depending on whether
		// the left or right subtree of the node to remove was
		// respectively selected.
		while (nodetoremove->children[!i])
			nodetoremove = nodetoremove->children[!i];
		
		// The replacement node found is removed from the tree and
		// its field value and data are used to set the corresponding
		// fields of the node which was to be removed.
		
		nodetoremove->parent->children[nodetoremove->value > nodetoremove->parent->value] =
			nodetoremove->children[i];
		
		if (nodetoremove->children[i]) {
			
			nodetoremove->children[i]->parent = nodetoremove->parent;
			
			nodetobalance = nodetoremove->children[i];
			
			node->value = nodetoremove->value;
			node->data = nodetoremove->data;
			
			// The memory used by the bintreenode which
			// was removed from the tree is freed.
			mmfree(nodetoremove);
			
			goto skiptest;
			
		} else {
			
			nodetobalance = nodetoremove->parent;
			
			uint i = nodetoremove->value > nodetobalance->value;
			
			node->value = nodetoremove->value;
			node->data = nodetoremove->data;
			
			// The memory used by the bintreenode which
			// was removed from the tree is freed.
			mmfree(nodetoremove);
			
			if (i) goto noderemovedfromright;
			else goto noderemovedfromleft;
		}
		
	} else {
		
		if (node->parent) {
			// Getting here mean that the node to remove
			// didn't have any children, but had a parent.
			
			nodetobalance = node->parent;
			
			uint i = value > nodetobalance->value;
			
			nodetobalance->children[i] = 0;
			
			// The memory used by the bintreenode which
			// was removed from the tree is freed.
			mmfree(node);
			
			// If i is null then the node was removed from
			// the left of the node pointed by nodetoinsert,
			// otherwise the node was removed from its right.
			if (i) goto noderemovedfromright;
			else goto noderemovedfromleft;
			
		// I get to the else case if the node to remove
		// didn't have any children nor did it have any parent.
		} else {
			// The memory used by the bintreenode which
			// was removed from the tree is freed.
			mmfree(node);
			
			tree->root = 0;
			
			return;
		}
	}
	
	// This loop update the balance factor
	// of the parent nodes of the node to remove.
	while (nodetobalance->parent) {
		
		skiptest:;
		
		uint i = nodetobalance->value > nodetobalance->parent->value;
		
		nodetobalance = nodetobalance->parent;
		
		// If i is null then the node was removed from
		// the left of the node pointed by nodetobalance,
		// otherwise the node was removed from its right.
		if (i) {
			
			noderemovedfromright:
			
			if (--nodetobalance->balance == -2) {
				
				balanceright(nodetobalance);
				
				nodetobalance = nodetobalance->parent;
				
				// Here the node that was parent to the node which
				// was rotated needs to have his children field updated.
				// If the node that was rotated was the root node,
				// tree->root is updated to the new root node.
				if (nodetobalance->parent) {
					
					nodetobalance->parent->children[nodetobalance->value > nodetobalance->parent->value] = nodetobalance;
					
					if (nodetobalance->balance) break;
					else goto skiptest;
					
				} else {
					
					tree->root = nodetobalance;
					
					break;
				}
				
			} else if (nodetobalance->balance) break;
			
		} else {
			
			noderemovedfromleft:
			
			if (++nodetobalance->balance == 2) {
				
				balanceleft(nodetobalance);
				
				nodetobalance = nodetobalance->parent;
				
				// Here the node that was parent to the node which
				// was rotated needs to have his children field updated.
				// If the node that was rotated was the root node,
				// tree->root is updated to the new root node.
				if (nodetobalance->parent) {
					
					nodetobalance->parent->children[nodetobalance->value > nodetobalance->parent->value] = nodetobalance;
					
					if (nodetobalance->balance) break;
					else goto skiptest;
					
				} else {
					
					tree->root = nodetobalance;
					
					break;
				}
				
			} else if (nodetobalance->balance) break;
		}
	}
}

void bintreeremove (bintree* tree, uint value) {
	bintreeremove_(tree, value, (void(*)(void*))0);
}
