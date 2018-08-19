
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


// These functions scan a tree up (From the smallest node value to the
// largest node value) or down (From the largest node value to the smallest
// node value); and for each node, the value and a reference to the memory
// location holding the pointer to the data associated with it is passed
// to the callback function; allowing the callback function to modify
// the pointer value to the data associated with the node.
// Scanning through the tree is interrupted if the callback function return 0.

void bintreescanup (bintree tree, uint (*callback)(uint value, void** data)) {
	// Label to return-to when the scanning
	// through the tree is to be aborted.
	__label__ abort;
	
	void processnode (bintreenode* node) {
		
		if (node->children[0]) processnode(node->children[0]);
		
		if (!callback(node->value, &node->data)) goto abort;
		
		if (node->children[1]) processnode(node->children[1]);
	}
	
	if (tree.root) processnode(tree.root);
	
	abort:;
}

void bintreescandown (bintree tree, uint (*callback)(uint value, void** data)) {
	// Label to return-to when the scanning
	// through the tree is to be aborted.
	__label__ abort;
	
	void processnode (bintreenode* node) {
		
		if (node->children[1]) processnode(node->children[1]);
		
		if (!callback(node->value, &node->data)) goto abort;
		
		if (node->children[0]) processnode(node->children[0]);
	}
	
	if (tree.root) processnode(tree.root);
	
	abort:;
}
