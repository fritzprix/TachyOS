/*
 * quantum.c
 *
 *  Created on: Jun 1, 2016
 *      Author: innocentevil
 */

#include "quantum.h"
#include "cdsl_nrbtree.h"
#include "wtree.h"

/**
 *  QUANTUM Size (bytes)
 *  2 / 4 / 6 .... 36 / 38 / 40 / 42 / 44 / 46 / 48
 *
 *  QUANTUM Base Size
 *  2 : 4096
 *  4 : 8192
 *  6 : 12288
 *  .
 *  .
 *  .
 *  48 :  98304
 *
 *  QUANTUM Node Size in 64bit machine
 *  304 byte
 *
 *  INTERNAL FRAGMENTATION RATE for QUANTUM Node
 *  2 : 7.4%
 *  4 : 3.7%
 *  6 : 2.4%
 *  .
 *  .
 *  .
 *  48 : 0.3%
 *
 */

#define QMAP_COUNT		32
#define SEGMENT_SIZE 	(1 << 20)

#define DIR_LEFT		1
#define DIR_RIGHT       2

#ifndef QUANTUM_POOL_PURGE_DEPTH_THRESHOLD
#define QUANTUM_POOL_PURGE_DEPTH_THRESHOLD 16
#endif


typedef struct quantum_node quantumNode_t;
typedef struct quantum quantum_t;

typedef uint64_t qmap_t;


struct quantum {
	nrbtreeNode_t    qtree_node;
	quantumNode_t    *entry;
};

typedef struct {
	slistNode_t	clr_lhead;
	wtreeNode_t node;
} cleanupNode_t;

struct quantum_node {
	quantumNode_t   *left,*right;      // child tree nodes
	quantumNode_t   *parent;           // parent
	uint16_t         free_cnt;         // number of free quantum
	uint16_t         qcnt;             // number of total available quantum
	uint16_t         quantum;          // quantum class
	void*            top;              // top
	nrbtreeNode_t    addr_rbnode;      // rbtree node for address lookup
	union {
		qmap_t           map[QMAP_COUNT];  // bitmap
		slistNode_t		 clr_lhead;
	};
};

struct getsz_arg {
	void*  chunk;
	size_t sz;
};

const size_t QMAP_UNIT_OFFSET = (sizeof(qmap_t) << 3);

static DECLARE_TRAVERSE_CALLBACK(try_purge_for_each_qnode);
static DECLARE_TRAVERSE_CALLBACK(for_each_quantum_print);
static DECLARE_TRAVERSE_CALLBACK(find_chunk_size);
static DECLARE_WTREE_TRAVERSE_CALLBACK(force_purge_for_each_pool);


static void quantum_init(quantum_t* quantum, uint16_t qsz);
static void quantum_add(quantumRoot_t* root, quantum_t* quantum);
static quantum_t* quantum_new(quantumRoot_t* root, size_t quantum_sz);

static void quantum_node_init(quantumNode_t* node, uint16_t qsz,ssize_t hsz, BOOL base);
static void quantum_node_add(quantumRoot_t* root, quantum_t* quantum, quantumNode_t* qnode);
static void* quantum_node_alloc_unit(quantumNode_t* node);
static void quantum_node_free_unit(quantumNode_t* qnode, void* chunk);
static quantumNode_t* quantum_node_insert_rc(quantumNode_t* parent, quantumNode_t* item);
static quantumNode_t* quantum_node_rotate_right(quantumNode_t* qnode);
static quantumNode_t* quantum_node_rotate_left(quantumNode_t* qnode);
static void quantum_node_print_rc(quantumNode_t* qnode, int level);
static void quantum_node_print(quantumNode_t* qnode,int level);


void quantum_root_init(quantumRoot_t* root, wt_map_func_t mapper, wt_unmap_func_t unmapper) {
	if(!root)
		return;
	root->adapter.onfree = unmapper;
	root->adapter.onallocate = mapper;
	root->adapter.onremoved = root->adapter.onadded = NULL;
	cdsl_nrbtreeRootInit(&root->addr_rbroot);
	cdsl_nrbtreeRootInit(&root->quantum_tree);
	wtree_rootInit(&root->quantum_pool,NULL,&root->adapter,0);
	cdsl_slistEntryInit(&root->clr_lentry);

	size_t seg_sz;
	uint8_t* init_seg = mapper(1, &seg_sz,NULL);
	owtreeNode_t* qpool = wtree_baseNodeInit(&root->quantum_pool, init_seg, seg_sz);
	wtree_addNode(&root->quantum_pool,qpool,FALSE,NULL);
}

void* quantum_reclaim_chunk(quantumRoot_t* root, ssize_t sz) {
	// calculate quantum size
	if(!root || !sz) return NULL;
	if(QUANTUM_MAX < sz) return NULL;

	quantumNode_t* qnode;
	sz = (sz + QUANTUM_SPACE) & ~(QUANTUM_SPACE - 1);
	/*
	 *  direct casting is possible, because rbnode is first element in quantum struct
	 */
	quantum_t* quantum = (quantum_t*) cdsl_nrbtreeLookup(&root->quantum_tree, sz);
	if(!quantum) {
		quantum = quantum_new(root, sz);
	}

	qnode = quantum->entry;
	uint8_t *chnk = NULL;
	while(!(chnk = quantum_node_alloc_unit(qnode))) {
		qnode = wtree_reclaim_chunk(&root->quantum_pool, sz * QMAP_UNIT_OFFSET * QMAP_COUNT, TRUE);
		quantum_node_init(qnode, sz, sizeof(quantumNode_t), FALSE);
		quantum_node_add(root, quantum, qnode);
	}
	return chnk;
}


size_t quantum_get_chunk_size(quantumRoot_t* root, void* chunk) {
	if(!root) return 0;
	struct getsz_arg arg;
	arg.chunk = chunk;
	arg.sz = 0;
	cdsl_nrbtreeTraverseTarget(&root->addr_rbroot,find_chunk_size, (trkey_t) chunk, &arg);
	return arg.sz;
}


int quantum_free_chunk(quantumRoot_t* root, void* chunk) {
	if(!chunk)
		return FALSE;
	nrbtreeNode_t* cnode = cdsl_nrbtreeTop(&root->addr_rbroot);
	quantumNode_t* qnode;
	int depth = 0;


	// find original quantum node by looking up address tree (red black tree)
	while(cnode) {
		qnode = container_of(cnode,quantumNode_t, addr_rbnode);
		if((size_t) chunk < (size_t) qnode) {
			cnode = cdsl_nrbtreeGoLeft(cnode);
		} else if((size_t) chunk > (size_t) qnode->top) {
			cnode = cdsl_nrbtreeGoRight(cnode);
		} else {
			/*
			 *  original quantum node found
			 *  free chunk
			 */
			quantum_node_free_unit(qnode, chunk);
			if(qnode->free_cnt == qnode->qcnt) {
				/*
				 * if quantum node is completely full
				 * and leaf node, we can free it to cache
				 */
				if(!qnode->parent) {
					quantum_t* quantum = (quantum_t*) cdsl_nrbtreeLookup(&root->quantum_tree, (trkey_t) qnode->quantum);
					if (!quantum) {
						fprintf(stderr, "Invalid Quantum %d\n", qnode->quantum);
						exit(-1);
					}
					quantum = container_of(quantum, quantum_t, qtree_node);
					quantum->entry = NULL;
					return TRUE;
				}

				quantumNode_t* nchild = NULL;
				if(qnode->left && qnode->right) {
					return TRUE;
				} else if(qnode->right) {
					nchild = qnode->right;
					nchild->parent = qnode->parent;
				} else if(qnode->left) {
					nchild = qnode->left;
					nchild->parent = qnode->parent;
				}

				// free quantum node to quantum pool
				// delete linkage from the parent node
				if(qnode->parent->right == qnode) {
					qnode->parent->right = nchild;
				} else if(qnode->parent->left == qnode){
					qnode->parent->left = nchild;
				} else {
					// unexpected linkage from parent node
					fprintf(stderr, "Quantum Corruption happend : Bad Parent Reference");
					exit(-1);
				}

				// delete quantum node from address lookup tree
				cdsl_nrbtreeDelete(&root->addr_rbroot,(trkey_t) qnode);
				owtreeNode_t* free_q = wtree_nodeInit(&root->quantum_pool,qnode, (size_t) qnode->top - (size_t) qnode, NULL);
				wtree_addNode(&root->quantum_pool,free_q,TRUE, &depth);
			}
			if(depth > QUANTUM_POOL_PURGE_DEPTH_THRESHOLD) {
				wtree_purge(&root->quantum_pool);
			}
			return TRUE;
		}
	}
	fprintf(stderr, "invalid chunk is freed %lx\n", (size_t) chunk);
	quantum_print(root);
	exit(-1);
}

void quantum_try_purge_cache(quantumRoot_t* root) {
	if(!root)
		return;
	cdsl_nrbtreeTraverse(&root->addr_rbroot, try_purge_for_each_qnode, ORDER_INC, root);
	quantumNode_t* qnode;
	owtreeNode_t* pnode;
	while((qnode = (quantumNode_t*) cdsl_slistRemoveHead(&root->clr_lentry))) {
		qnode = container_of(qnode, quantumNode_t, clr_lhead);
		if(!cdsl_nrbtreeDelete(&root->addr_rbroot, (size_t) qnode)) {
			fprintf(stderr, "Null node is detected in purge op.");
			exit(-1);
		}
		pnode = wtree_nodeInit(&root->quantum_pool,qnode, (size_t) qnode->top - (size_t) qnode, NULL);
		wtree_addNode(&root->quantum_pool, pnode, TRUE, NULL);
	}
	wtree_purge(&root->quantum_pool);
}

void quantum_cleanup(quantumRoot_t* root) {
	/*
	 * cleanup force memory mapped to unmap,
	 * after cleanup invoked, referencing memory allocated raise segmentation fault
	 */
	if(!root)
		return;
	slistEntry_t clr_entry;
	wt_unmap_func_t unmapper = root->adapter.onfree;
	cdsl_slistEntryInit(&clr_entry);
	wtree_traverseBaseNode(&root->quantum_pool, force_purge_for_each_pool, &clr_entry);
	cleanupNode_t* clr_node;
	while((clr_node = (cleanupNode_t*) cdsl_slistRemoveHead(&clr_entry))) {
		clr_node = container_of(clr_node, cleanupNode_t, clr_lhead);
		unmapper(clr_node->node.top - clr_node->node.base_size,clr_node->node.base_size,&clr_node->node, NULL);
	}
}



void quantum_print(quantumRoot_t* root) {
	if(!root)
		return;
	printf("\n");
	cdsl_nrbtreeTraverse(&root->quantum_tree, for_each_quantum_print, ORDER_INC, root);
	printf("\n");
}



static void quantum_init(quantum_t* quantum, uint16_t qsz) {
	if(!quantum)
		return;
	quantum->entry = NULL;
	cdsl_nrbtreeNodeInit(&quantum->qtree_node,qsz);
}


static void quantum_node_init(quantumNode_t* node, uint16_t qsz, ssize_t hsz, BOOL base) {
	if(!node)
		return;
	node->quantum = qsz;
	node->parent = NULL;
	node->qcnt = QUANTUM_COUNT_MAX - 1;
	node->left = node->right = NULL;
	node->top = (void*) (((size_t) node) + qsz * QUANTUM_COUNT_MAX);
	cdsl_nrbtreeNodeInit(&node->addr_rbnode, (trkey_t) node);
	cdsl_slistNodeInit(&node->clr_lhead);
	mset(node->map, 0xff, sizeof(qmap_t) *  QMAP_COUNT);
	qmap_t msk = 1;
	qmap_t *map = node->map;

	/*
	 * clear bitmap for occupied area used by header
	 */
	uint16_t shift_cnt = 0;
	size_t boffest = QMAP_UNIT_OFFSET * qsz;
	while(hsz > boffest) {
		hsz -= boffest;
		node->qcnt -= QMAP_UNIT_OFFSET;
		*map = 0;
		map++;
	}


	while((hsz = hsz - qsz) > 0) {
		*map &= ~msk;
		msk <<= 1;
		node->qcnt--;
	}
	if(hsz > 0) {
		fprintf(stderr, "unexpected header sz positive %u /w hsz : %ld\n",qsz,hsz);
		exit(-1);
	}

	node->free_cnt = node->qcnt;
}


static void quantum_add(quantumRoot_t* root, quantum_t* quantum) {
	if(!root || !quantum)
		return;
	cdsl_nrbtreeInsert(&root->quantum_tree, &quantum->qtree_node);
}

static quantum_t* quantum_new(quantumRoot_t* root, size_t quantum_sz) {
	if(!root || !quantum_sz) return NULL;
	quantum_t* quantum = wtree_reclaim_chunk(&root->quantum_pool, sizeof(quantum_t), FALSE);
	quantumNode_t* qnode = wtree_reclaim_chunk(&root->quantum_pool, quantum_sz * QMAP_UNIT_OFFSET * QMAP_COUNT, TRUE);
	quantum_node_init(qnode, quantum_sz, sizeof(quantumNode_t), FALSE);
	quantum_init(quantum, quantum_sz);
	quantum_add(root,quantum);
	quantum_node_add(root, quantum, qnode);
	return quantum;
}


static void quantum_node_add(quantumRoot_t* root, quantum_t* quantum, quantumNode_t* qnode) {
	if(!quantum || !qnode || !root)
		return;
	quantum->entry = quantum_node_insert_rc(quantum->entry, qnode);
	quantum->entry->parent = NULL;
	cdsl_nrbtreeInsert(&root->addr_rbroot, &qnode->addr_rbnode);
}

static quantumNode_t* quantum_node_insert_rc(quantumNode_t* parent, quantumNode_t* item) {
	if(!parent)
		return item;
	if((size_t)parent < (size_t) item) {
		parent->right = quantum_node_insert_rc(parent->right, item);
		if(!parent->right)
			return parent;
		parent->right->parent = parent;
		if(parent->right->free_cnt > parent->free_cnt) {
			parent = quantum_node_rotate_left(parent);
		}
	} else if((size_t) parent > (size_t) item) {
		parent->left = quantum_node_insert_rc(parent->left, item);
		if(!parent->left)
			return parent;
		parent->left->parent = parent;
		if(parent->left->free_cnt > parent->free_cnt) {
			parent = quantum_node_rotate_right(parent);
		}
	} else {
		fprintf(stderr, "Quantum Node Collision\n");
		exit(-1);
	}
	return parent;

}

static quantumNode_t* quantum_node_rotate_right(quantumNode_t* qnode) {
	if(!qnode || !qnode->left)
		return qnode;
	quantumNode_t* top = qnode->left;
	top->parent = qnode->parent;

	qnode->left = top->right;
	if(top->right)
		top->right->parent = qnode;

	top->right = qnode;
	qnode->parent = top;
	return top;
}

static quantumNode_t* quantum_node_rotate_left(quantumNode_t* qnode) {
	if(!qnode || !qnode->right)
		return qnode;
	quantumNode_t* top = qnode->right;
	top->parent = qnode->parent;

	qnode->right = top->left;
	if(top->left)
		top->left->parent = qnode;

	top->left = qnode;
	qnode->parent = top;
	return top;
}


static void* quantum_node_alloc_unit(quantumNode_t* qnode) {
	if(!qnode || !qnode->free_cnt) 	return NULL;

	qmap_t* cmap = qnode->map;
	uint16_t offset = 0;
	qmap_t  vmap = 0;
	qmap_t  imap = 1;
	while(!*cmap && (offset < QUANTUM_COUNT_MAX)) {
		cmap++;
		offset += QMAP_UNIT_OFFSET;
	}
	vmap = *cmap;
	if(!vmap) {
		fprintf(stderr, "bitmap is corrupted\n");
		quantum_node_print(qnode, 0);
		exit(-1);
	}
	while(!(vmap & 1) && (offset < QUANTUM_COUNT_MAX)) {
		vmap >>= 1;
		imap <<= 1;
		offset++;
	}
	*cmap &= ~imap;
	qnode->free_cnt--;
	uint8_t* chnk = &((uint8_t*) qnode)[(offset + 1) * qnode->quantum];
	if((size_t) qnode->top < (size_t) chnk) {
		exit(-1);
	}
	return chnk;
}


static void quantum_node_free_unit(quantumNode_t* qnode, void* chunk) {
	if(!qnode)
		return;
	size_t offset = (size_t)chunk - (size_t)qnode;
	uint16_t bmoffset = offset / qnode->quantum;
	qmap_t* cmap = qnode->map;
	while(!(bmoffset < QMAP_UNIT_OFFSET)) {
		cmap++;
		bmoffset -= QMAP_UNIT_OFFSET;
	}
	*cmap |= ((qmap_t) 1 << bmoffset);
	qnode->free_cnt++;
}


static DECLARE_TRAVERSE_CALLBACK(try_purge_for_each_qnode) {
	if(!node)
		return TRAVERSE_BREAK;
	quantumNode_t* qnode = container_of(node, quantumNode_t, addr_rbnode);
	quantumRoot_t* root = (quantumRoot_t*) arg;

	if(qnode->free_cnt == qnode->qcnt) {
		if(qnode->left && qnode->right) {
			/*
			 * can't be purged
			 */
			return TRAVERSE_OK;
		} else if(qnode->left) {
			if(qnode->parent) {
				if(qnode->parent->right == qnode) {
					qnode->parent->right = qnode->left;
					qnode->left->parent = qnode->parent;
				} else if(qnode->parent->left == qnode) {
					qnode->parent->left = qnode->left;
					qnode->left->parent = qnode->parent;
				} else {
					fprintf(stderr, "Invalid Parent Node in quantum tree purge op.\n");
					exit(-1);
				}
			} else {
				qnode->left->parent = qnode->parent;
			}
		} else if(qnode->right) {
			if(qnode->parent) {
				if(qnode->parent->right == qnode) {
					qnode->parent->right = qnode->right;
					qnode->right->parent = qnode->parent;
				} else if(qnode->parent->left == qnode) {
					qnode->parent->left = qnode->right;
					qnode->right->parent = qnode->parent;
				} else {
					fprintf(stderr, "Invalid Parent Node in quantum tree purge op.\n");
					exit(-1);
				}
			} else {
				qnode->right->parent = qnode->parent;
			}
		} else {
			if(qnode->parent) {
				if(qnode->parent->right == qnode) {
					qnode->parent->right = NULL;
				} else if(qnode->parent->left == qnode) {
					qnode->parent->left = NULL;
				} else {
					fprintf(stderr, "Invalid Parent Node in quantum tree purge op.\n");
					exit(-1);
				}
			}
		}
		cdsl_slistPutHead(&root->clr_lentry, &qnode->clr_lhead);
	}
	return TRAVERSE_OK;
}



static DECLARE_TRAVERSE_CALLBACK(for_each_quantum_print) {
	if(!node)
		return TRAVERSE_BREAK;
	quantum_t* qt = container_of(node, quantum_t, qtree_node);
	quantum_node_print_rc(qt->entry, 0);
	return 0;
}

static DECLARE_TRAVERSE_CALLBACK(find_chunk_size) {
	struct getsz_arg* gsz = (struct getsz_arg*) arg;
	quantumNode_t* qnode = container_of(node, quantumNode_t, addr_rbnode);
	if(((size_t) gsz->chunk < (size_t) qnode->top) && ((size_t) gsz->chunk > (size_t) qnode)) {
		gsz->sz = qnode->quantum;
		return TRAVERSE_BREAK;
	}
	return TRAVERSE_OK;
}


static DECLARE_WTREE_TRAVERSE_CALLBACK(force_purge_for_each_pool) {
	if(!node) return FALSE;
	slistEntry_t* clr_list = (slistEntry_t*) arg;
	cleanupNode_t* clnode =  (cleanupNode_t*) ((size_t) node - offsetof(cleanupNode_t,node));
	cdsl_slistNodeInit(&clnode->clr_lhead);
	cdsl_slistPutHead(clr_list, &clnode->clr_lhead);
	return TRUE;
}


static void quantum_node_print_rc(quantumNode_t* qnode, int level) {
	if(!qnode)
		return;
	quantum_node_print_rc(qnode->right, level + 1);
	quantum_node_print(qnode, level);
	quantum_node_print_rc(qnode->left, level + 1);
}

static void quantum_node_print(quantumNode_t* qnode,int level) {
	int l = level;
	while(l--)
		printf("\t");
	printf("@ %lx ~ %lx {Q : %d | usage : %u /  %u } (%u)\n",(size_t) qnode, (size_t) qnode->top ,qnode->quantum, qnode->free_cnt, qnode->qcnt,level );
}



