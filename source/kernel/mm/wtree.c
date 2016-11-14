/*
 * wtree.c
 *
 *  Created on: Jun 1, 2016
 *      Author: innocentevil
 */

#include "wtree.h"
#include "cdsl_slist.h"


typedef struct {
	slistNode_t    clr_lhead;
	wtreeNode_t    node;
} clr_node_t;

typedef struct {
	slistEntry_t   clr_list;
	wtreeRoot_t    *root;
} clr_argument;

static DECLARE_WTREE_TRAVERSE_CALLBACK(for_each_node_destroy);


static wtreeNode_t* insert_rc(wtreeRoot_t* root, wtreeNode_t* parent, wtreeNode_t* item, BOOL compact, int* depth);
static wtreeNode_t* grows_node(wtreeRoot_t* root, wtreeNode_t* parent, wtreeNode_t** grown, uint32_t nsz);
static BOOL new_cacheNode(wtreeRoot_t* root,size_t sz,BOOL compact);
static wtreeNode_t* purge_rc(wtreeRoot_t* root, wtreeNode_t* node);
static void traverse_base_rc(wtreeNode_t* node, wt_callback_t callback, void* arg);
static size_t size_rc(wtreeNode_t* node);
static size_t fsize_rc(wtreeNode_t* node);
static size_t count_rc(wtreeNode_t* node);
static uint32_t level_rc(wtreeNode_t* node);
static wtreeNode_t* rotate_left(wtreeNode_t* parent);
static wtreeNode_t* rotate_right(wtreeNode_t* parent);
static wtreeNode_t* resolve(wtreeRoot_t* root, wtreeNode_t* parent, BOOL compact);
static wtreeNode_t* merge_next(wtreeRoot_t* root, wtreeNode_t* merger);
static wtreeNode_t* merge_prev(wtreeRoot_t* root, wtreeNode_t* merger);
static wtreeNode_t* merge_from_leftend(wtreeRoot_t* root, wtreeNode_t* left, wtreeNode_t* merger);
static wtreeNode_t* merge_from_rightend(wtreeRoot_t* root, wtreeNode_t* right,wtreeNode_t* merger);
static void print_rc(wtreeNode_t* parent, int depth);
static void print_tab(int times);

void wtree_rootInit(wtreeRoot_t* root, void* ext_ctx, const wt_adapter* adapter, size_t cust_hdr_sz) {
	if(!root || !adapter) {
		return;
	}

	root->entry = NULL;
	root->total_sz = root->used_sz = 0;
	root->adapter = adapter;
	root->ext_ctx = ext_ctx;
	root->hdr_sz = cust_hdr_sz < sizeof(wtreeNode_t)? sizeof(wtreeNode_t) : cust_hdr_sz;
}

wtreeNode_t* wtree_nodeInit(wtreeRoot_t* root, uaddr_t addr, uint32_t sz, void* preserve) {
	if(!addr || !sz) return NULL;
	uint8_t* chunk = (uint8_t*) addr;
	wtreeNode_t* node = (wtreeNode_t*) &chunk[sz - root->hdr_sz];
	if(preserve) mcpy(preserve, node, root->hdr_sz);
	node->size = sz;
	node->base_size = 0;
	node->left = node->right = NULL;
	node->top = (uaddr_t) &chunk[sz];
	return node;
}

void wtree_restorePreserved(wtreeRoot_t* root, uaddr_t addr, uint32_t sz, void* preserved) {
	if(!addr || !sz || !preserved) return;
	uint8_t* chunk = (uint8_t*) addr;
	mcpy(&chunk[sz - root->hdr_sz], preserved, root->hdr_sz);
}


wtreeNode_t* wtree_baseNodeInit(wtreeRoot_t* root, uaddr_t addr, uint32_t sz) {
	if(!addr || !sz) return NULL;
	uint8_t* chunk = (uint8_t*) addr;
	wtreeNode_t* node = (wtreeNode_t*) &chunk[sz - root->hdr_sz];
	node->size = node->base_size = sz;
	node->left = node->right = NULL;
	node->top = (uaddr_t) &chunk[sz];
	return node;
}

void wtree_purge(wtreeRoot_t* root) {
	if(!root) return;
	root->entry = purge_rc(root,root->entry);
}

void wtree_cleanup(wtreeRoot_t* root) {
	if(!root) return;
	slistEntry_t clr_list;
	clr_node_t* clrn;
	cdsl_slistEntryInit(&clr_list);
	wtree_traverseBaseNode(root, for_each_node_destroy, &clr_list);
	while((clrn = (clr_node_t*)cdsl_slistRemoveHead(&clr_list))) {
		clrn = container_of(clrn, clr_node_t, clr_lhead);
		if(root->adapter->onremoved) {
			root->adapter->onremoved(&clrn->node, root->ext_ctx);
		}
		if(root->adapter->onfree) {
			root->adapter->onfree(clrn->node.top - clrn->node.base_size,  clrn->node.base_size, &clrn->node, root->ext_ctx);
		}
	}
	root->entry = NULL;
	root->total_sz = root->used_sz = 0;
}

void wtree_traverseBaseNode(wtreeRoot_t* root, wt_callback_t callback, void* arg) {
	if(!root) return;
	traverse_base_rc(root->entry, callback, arg);
}

void wtree_addNode(wtreeRoot_t* root, wtreeNode_t* node, BOOL compact,int* idepth) {
	if(!root || !node) return;
	root->entry = insert_rc(root,root->entry,node,compact, idepth);
}


void* wtree_reclaim_chunk(wtreeRoot_t* root, size_t sz,BOOL compact) {

	if (!root || (sz < sizeof(wtreeNode_t)))	return NULL;

	if (!root->entry)		return NULL;

	wtreeNode_t* largest = root->entry;
	uint8_t* chunk = (uint8_t*) largest->top;
	if((largest->size - root->hdr_sz) < sz) {
		if(new_cacheNode(root, sz, TRUE)){
			largest = root->entry;
			chunk = largest->top;
		} else return NULL;
	}
	chunk = chunk - largest->size;
	largest->size -= sz;
	root->entry = resolve(root, root->entry, compact);
	return chunk;
}


void* wtree_reclaim_chunk_from_node(wtreeNode_t* node, size_t sz) {

	if(!node || !sz) return NULL;

	uint8_t* chunk = (uint8_t*) node->top;
	chunk = chunk  - node->size;
	node->size -= sz;
	return chunk;
}


void* wtree_grow_chunk(wtreeRoot_t* root, wtreeNode_t** node, size_t nsz) {
	if(!root || !(*node) || !nsz)
		return NULL;
	if(!root->entry)
		return NULL;
	if(!(nsz > (*node)->size))
		return (void*) ((size_t) (*node)->top - (*node)->size);
	root->entry = grows_node(root, root->entry, node, nsz);
	root->entry = resolve(root, root->entry, TRUE);
	if(*node) {
		return (void*) ((size_t) (*node)->top - (*node)->size);
	}
	return wtree_reclaim_chunk(root, nsz,TRUE);
}

void wtree_print(wtreeRoot_t* root) {
	if(!root)
		return;
	if(!root->entry)
		return;
	PRINT("\n");
	print_rc(root->entry, 0);
	PRINT("\n");
}

uint32_t wtree_level(wtreeRoot_t* root) {
	if(!root)
		return 0;
	if(!root->entry)
		return 0;
	return level_rc(root->entry);
}

size_t wtree_nodeCount(wtreeRoot_t* root) {
	if(!root)
		return 0;
	if(!root->entry)
		return 0;
	return count_rc(root->entry);
}

size_t wtree_totalSize(wtreeRoot_t* root) {
	if(!root)
		return 0;
	if(!root->entry)
		return 0;
	return size_rc(root->entry);
}

size_t wtree_freeSize(wtreeRoot_t* root) {
	if(!root)
		return 0;
	if(!root->entry)
		return 0;
	return fsize_rc(root->entry);
}

static wtreeNode_t* insert_rc(wtreeRoot_t* root, wtreeNode_t* parent, wtreeNode_t* item, BOOL compact, int* depth) {
	if(!root || !item)
		return parent;
	if(!parent) {
		if(root->adapter->onadded) root->adapter->onadded(item, root->ext_ctx);
		return item;
	}
	if(depth) {
		(*depth)++;
	}

	if(parent->top < item->top) {
		if((item->top - item->size) == parent->top) {
			if(item->base_size) {
				item->base_size += parent->base_size;
			}
			else if(parent->base_size) {
				parent->right = insert_rc(root, parent->right, item, compact,depth);
				parent = merge_next(root,parent);
				parent = resolve(root, parent, compact);
				return parent;
			}
			item->size += parent->size;
			item->left = parent->left;
			item->right = parent->right;
			if(root->adapter->onremoved) root->adapter->onremoved(parent, root->ext_ctx);
			if(root->adapter->onadded) root->adapter->onadded(item, root->ext_ctx);
			parent = item;
		} else {
			parent->right = insert_rc(root, parent->right, item, compact,depth);
			parent = merge_next(root, parent);
			parent = resolve(root, parent, compact);
		}
		return parent;
	} else {
		if((parent->top - parent->size) == item->top) {
			if(parent->base_size) {
				parent->base_size += item->base_size;
			}
			else if(item->base_size) {
				parent->left = insert_rc(root, parent->left, item, compact,depth);
				parent = merge_prev(root, parent);
				parent = resolve(root, parent, compact);
				return parent;
			}
			parent->size += item->size;
		} else {
			parent->left = insert_rc(root, parent->left, item, compact,depth);
			parent = merge_prev(root, parent);
			parent = resolve(root, parent, compact);
			return parent;
		}
		return parent;
	}
}
static wtreeNode_t* grows_node(wtreeRoot_t* root, wtreeNode_t* parent, wtreeNode_t** grown, uint32_t nsz) {
	if(!parent) {
		parent = *grown;
		*grown = NULL;
		if(root->adapter->onadded) root->adapter->onadded(parent, root->ext_ctx);
		return parent;
	}
	if((*grown)->top == (parent->top - parent->size)) {
		if((parent->size + root->hdr_sz) > (nsz - (*grown)->size)) {
			parent->size -= (nsz - (*grown)->size);
			return parent;
		}
		// return requested node to its base node or neighbor
		parent->size += (*grown)->size;
		grown = NULL;
		return parent;
	} else if((*grown)->top > parent->top) {
		parent->right = grows_node(root, parent->right, grown, nsz);
		parent = merge_next(root, parent);
		parent = resolve(root, parent, FALSE);
	} else {
		parent->left = grows_node(root, parent->left, grown, nsz);
		parent = merge_prev(root, parent);
		parent = resolve(root, parent, FALSE);
	}
	return parent;
}

static BOOL new_cacheNode(wtreeRoot_t* root,size_t sz, BOOL compact) {
	if(!root->adapter->onallocate)
		return FALSE;
	size_t rsz;
	wtreeNode_t* node = root->adapter->onallocate(sz, &rsz, root->ext_ctx);
	if(!node)
		return FALSE;
	node = wtree_baseNodeInit(root, node, rsz);
	int depth = 0;
	wtree_addNode(root, node, compact, &depth);

	return TRUE;
}



static wtreeNode_t* purge_rc(wtreeRoot_t* root, wtreeNode_t* node) {
	if(!root)
		return NULL;
	if(node->right) {
		node->right = purge_rc(root, node->right);
		node = merge_next(root, node);
	}
	if(node->left) {
		node->left = purge_rc(root, node->left);
		node = merge_prev(root, node);
	}
	if (node->size == node->base_size) {
		if(node->left && (node->left->base_size != node->left->size)) {
			node = rotate_right(node);
			node->right = purge_rc(root, node->right);
		} else if(node->right && (node->right->base_size != node->right->size)) {
			node = rotate_left(node);
			node->left = purge_rc(root, node->left);
		} else if(!node->left && !node->right) {
			if(root->adapter->onfree) {
				if(root->adapter->onremoved) root->adapter->onremoved(node, root->ext_ctx);
				root->adapter->onfree(node->top - node->base_size, node->base_size,node, root->ext_ctx);
				return NULL;
			}
		}
	}
	return node;
}

static void traverse_base_rc(wtreeNode_t* node, wt_callback_t callback, void* arg) {
	if (!node)
		return;
	traverse_base_rc(node->right, callback, arg);
	if (node->base_size)
		callback(node, arg);
	traverse_base_rc(node->left, callback, arg);
}

static size_t size_rc(wtreeNode_t* node) {
	if (!node)
		return 0;
	size_t sz = size_rc(node->left);
	sz += node->base_size;
	sz += size_rc(node->right);
	return sz;
}

static size_t fsize_rc(wtreeNode_t* node) {
	if (!node)
		return 0;
	size_t sz = fsize_rc(node->left);
	sz += node->size;
	sz += fsize_rc(node->right);
	return sz;
}

static size_t count_rc(wtreeNode_t* node) {
	if (!node)
		return 0;
	if (!node->right && !node->left)
		return 1;
	if (node->right && node->left)
		return count_rc(node->left) + count_rc(node->right) + 1;
	if (!node->right)
		return count_rc(node->left) + 1;
	if (!node->left)
		return count_rc(node->right) + 1;
	return 0;
}

static uint32_t level_rc(wtreeNode_t* node) {
	if (!node)
		return 0;
	uint32_t l, r = level_rc(node->right) + 1;
	l = level_rc(node->left) + 1;
	return r > l ? r : l;
}

static wtreeNode_t* rotate_left(wtreeNode_t* parent) {
	wtreeNode_t* nparent = parent->right;
	parent->right = nparent->left;
	nparent->left = parent;
	return nparent;
}

static wtreeNode_t* rotate_right(wtreeNode_t* parent) {
	wtreeNode_t* nparent = parent->left;
	parent->left = nparent->right;
	nparent->right = parent;
	return nparent;
}

static wtreeNode_t* resolve(wtreeRoot_t* root, wtreeNode_t* parent, BOOL compact) {
	if (!parent)
		return NULL;
	if (parent->right && parent->left) {
		if (!(parent->right->size > parent->size)
				&& !(parent->left->size > parent->size))
			return parent;
		if (parent->right->size > parent->left->size) {
			parent = rotate_left(parent);
			parent->left = resolve(root,parent->left,compact);
			parent = merge_prev(root, parent);
		} else {
			parent = rotate_right(parent);
			parent->right = resolve(root,parent->right,compact);
			parent = merge_next(root, parent);
		}
		return parent;
	} else if (parent->right) {
		if (parent->right->size > parent->size) {
			parent = rotate_left(parent);
			parent->left = resolve(root,parent->left,compact);
			parent = merge_prev(root, parent);
		}
		return parent;
	} else if (parent->left) {
		if (parent->left->size > parent->size) {
			parent = rotate_right(parent);
			parent->right = resolve(root, parent->right,compact);
			parent = merge_next(root, parent);
		}
		return parent;
	} else {
		if (!parent->size && !parent->base_size){
			return NULL;
		}else if(parent->size == parent->base_size) {
			if(root->adapter->onfree && compact) {
				if(root->adapter->onremoved)  root->adapter->onremoved(parent,root->ext_ctx);
				root->adapter->onfree(parent->top - parent->base_size, parent->base_size, parent, root->ext_ctx);
				return NULL;
			}
		}
	}
	return parent;
}

static wtreeNode_t* merge_next(wtreeRoot_t* root, wtreeNode_t* merger) {
	/*
	 *    merger                merger                   merger
	 *        \                     \                       \
	 *         r (nm)     -->        r (nm)     ->           r (nm)   ->    stop
	 *        /                     /                       /
	 *      rl (nm)               rl (nm)                 rl (m)
	 *      /                     /                       /  \
	 *    rll (m)               rllr (m)           rllrr(nm)  rlr
	 *    / \                  /    \
	 *   0  rllr              0    rllrr

	 *
	 */
	if(!merger)
		return NULL;
	if(!merger->right)
		return merger;
	merger->right = merge_next(root, merger->right);
	merger->right = merge_from_leftend(root, merger->right, merger);
	wtreeNode_t* node = (wtreeNode_t*) (merger->top - root->hdr_sz);
	node->top = merger->top;
	node->base_size = merger->base_size;
	node->size = merger->size;
	node->left = merger->left;
	node->right = merger->right;
	if(root->adapter->onremoved) root->adapter->onremoved(merger, root->ext_ctx);
	if(root->adapter->onadded) root->adapter->onadded(node, root->ext_ctx);
	return node;
}

static wtreeNode_t* merge_prev(wtreeRoot_t* root, wtreeNode_t* merger) {
	if(!merger)
		return NULL;
	if(!merger->left)
		return merger;
	merger->left = merge_prev(root, merger->left);
	merger->left = merge_from_rightend(root, merger->left, merger);
	return merger;
}


static wtreeNode_t* merge_from_leftend(wtreeRoot_t* root, wtreeNode_t* left, wtreeNode_t* merger) {
	/*
	 *   |--------------|header 1|
	 *                                          |-----------|header 2|
	 *                           |-----|header 3|
	 */
	if(!left) return NULL;
	left->left = merge_from_leftend(root,left->left, merger);
	if(left->left) return left;
	while((left->top - left->size) == merger->top) {
		if(left->base_size) {
			merger->base_size += left->base_size;
		}
		else if(merger->base_size) {
			return left;
		}
		merger->size += left->size;
		merger->top = left->top;
		if(root->adapter->onremoved) root->adapter->onremoved(left, root->ext_ctx);
		if(!left->right)
			return NULL;
		left = left->right;
	}
	return left;
}



static wtreeNode_t* merge_from_rightend(wtreeRoot_t* root, wtreeNode_t* right,wtreeNode_t* merger) {
	/*
	 *                                   |------------|header 1|
	 *    |-----|header 2|
	 *                   |------|header 3|
	 */
	if(!right) return NULL;
	right->right = merge_from_rightend(root,right->right, merger);
	if(right->right) return right;
	while((merger->top - merger->size) == right->top) {
		if(merger->base_size){
			merger->base_size += right->base_size;
		}
		else if(right->base_size) {
			return right;
		}
		merger->size += right->size;
		if(root->adapter->onremoved) root->adapter->onremoved(right, root->ext_ctx);
		if(!right->left)
			return NULL;
		right = right->left;
	}
	return right;
}

static DECLARE_WTREE_TRAVERSE_CALLBACK(for_each_node_destroy) {
	slistEntry_t* clr_list = (slistEntry_t*) arg;
	clr_node_t* clr_node = container_of(clr_node, clr_node_t, node);
	cdsl_slistNodeInit(&clr_node->clr_lhead);
	cdsl_slistPutHead(clr_list, &clr_node->clr_lhead);
	return TRUE;
}

static void print_rc(wtreeNode_t* parent, int depth) {
	if (!parent)
		return;
	print_rc(parent->right, depth + 1);
	print_tab(depth);
	PRINT("Node@%lx (bottom : %lx / base :%lx) {size : %u / base_size : %u / top : %lx} (%d)\n",
			(uint64_t) parent, (uint64_t)parent->top - parent->size, (uint64_t)parent->top - parent->base_size, parent->size, parent->base_size, (uint64_t) parent->top, depth);
	print_rc(parent->left, depth + 1);
}

static void print_tab(int times) {
	while (times--)
		PRINT("\t");
}

