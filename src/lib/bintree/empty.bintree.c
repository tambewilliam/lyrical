
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


// This function remove all nodes of the tree given
// as argument from the leaf nodes going right;
// and set the field root of the bintree to null.
// Before removing each node, their value and the pointer to the data
// associated with them is passed to the callback function.
// The pointer argument for the callback function can be null.
void bintreeempty_ (bintree* tree, void (*callback)(uint value, void* data)) {
	
	void processnode (bintreenode* node) {
		
		if (node->children[0]) processnode(node->children[0]);
		if (node->children[1]) processnode(node->children[1]);
		
		if (callback) callback(node->value, node->data);
		
		mmfree(node);
	}
	
	if (tree->root) {
		
		processnode(tree->root);
		
		tree->root = 0;
	}
}

void bintreeempty (bintree* tree) {
	bintreeempty_(tree, (void(*)(uint,void*))0);
}
