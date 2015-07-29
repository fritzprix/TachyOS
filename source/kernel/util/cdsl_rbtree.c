/*
 * cdsl_rbtree.c
 *
 *  Created on: 2015. 5. 14.
 *      Author: innocentevil
 */

#include "cdsl.h"
#include "cdsl_rbtree.h"
#include <stdio.h>




#define BLACK					((unsigned) (1 > 0))
#define RED						((unsigned) (1 < 0))


#define CLEAN					((uint8_t) 0)
#define COLLISION				((uint8_t) 1)
#define DIR_RIGHT				((uint8_t) 2)
#define DIR_LEFT				((uint8_t) 3)


#define RB_NIL 					((rb_treeNode_t*) &NIL_NODE)
#define IS_LEAF_NODE(node) 		((node->right == RB_NIL) || (node->left == RB_NIL))


const static rb_treeNode_t NIL_NODE = {
		.left = NULL,
		.right = NULL,
		.key = 0,
		.color = BLACK
};



static rb_treeNode_t* rotateLeft(rb_treeNode_t* rot_pivot,BOOL paint);
static rb_treeNode_t* rotateRight(rb_treeNode_t* rot_pivot,BOOL paint);
static rb_treeNode_t* insert_r(rb_treeNode_t* parent,rb_treeNode_t* item,uint8_t* context);
static rb_treeNode_t* delete_r(rb_treeNode_t* cur,int key,rb_treeNode_t** del,uint8_t* context,int k);
static rb_treeNode_t* deleteLeft_r(rb_treeNode_t* root,rb_treeNode_t** l_most,uint8_t* context);
static rb_treeNode_t* deleteRight_r(rb_treeNode_t* root,rb_treeNode_t** r_most,uint8_t* context);
static rb_treeNode_t* handleDoubleBlack(rb_treeNode_t* parent,uint8_t* context);
static void print_r(rb_treeNode_t* current,int depth);
static void print_tab(int cnt);

void cdsl_rbtreeNodeInit(rb_treeNode_t* item,int key){
	item->key = key;
	item->left = item->right = RB_NIL;
	item->color = RED;
}


BOOL cdsl_rbtreeInsert(rb_treeNode_t** root,rb_treeNode_t* item){
	if(!root)
		return FALSE;
	if(!*root){
		*root = item;
		item->color = BLACK;
		return TRUE;
	}
	uint8_t info = 0;

	*root = insert_r(*root,item,&info);
	(*root)->color = BLACK;
	return TRUE;
}

rb_treeNode_t* cdsl_rbtreeDelete(rb_treeNode_t** root,int key){
	if(!root || !(*root))
		return NULL;		// red black tree is empty or invalid arg
	uint8_t context = 0;
	rb_treeNode_t* del_node = NULL;
	*root = delete_r(*root,key,&del_node,&context,0);
	return del_node;
}

rb_treeNode_t* cdsl_rbtreeLookup(rb_treeNode_t** root,int key){
	if(!root || !(*root))
		return NULL;
	rb_treeNode_t* current = *root;
	while(current != RB_NIL){
		if(key > current->key)
			current = current->right;
		else if(key < current->key)
			current = current->left;
		else
			return current;
	}
	return NULL;
}


static rb_treeNode_t* deleteLeft_r(rb_treeNode_t* root,rb_treeNode_t** l_most,uint8_t* context){
	if(!root)
		return NULL;
	if(root == RB_NIL)
		return RB_NIL;
	if(root->left == RB_NIL){
		*l_most = root;
		*context = CLEAN;
		if((root->color == BLACK) && (root->right->color == BLACK))
			*context = COLLISION;
		if(root->right != RB_NIL)
			root->right->color = (*l_most)->color;
		return root->right;
	}
	root->left = deleteLeft_r(root->left,l_most,context);
	if((*context) == COLLISION){
		(*context) = DIR_LEFT;
		return handleDoubleBlack(root,context);
	}
	return root;
}

static rb_treeNode_t* deleteRight_r(rb_treeNode_t* root,rb_treeNode_t** r_most,uint8_t* context){
	if(!root)
		return NULL;
	if(root == RB_NIL)
		return RB_NIL;
	if(root->right == RB_NIL){
		*r_most = root;
		*context = CLEAN;
		if((root->color == BLACK) && (root->left->color == BLACK))
			*context = COLLISION;
		if(root->left != RB_NIL)
			root->left->color = (*r_most)->color;
		return root->left;
	}
	root->right = deleteRight_r(root->right,r_most,context);
	if((*context) == COLLISION){
		*context = DIR_RIGHT;
		return handleDoubleBlack(root,context);
	}
	return root;
}

static rb_treeNode_t* delete_r(rb_treeNode_t* cur,int key,rb_treeNode_t** del,uint8_t* context,int k){
	if(!cur) return NULL;
	if(cur == RB_NIL){
		del = NULL;
		context = CLEAN;
		return cur;
	}
	uint8_t direction = *context;
	if(cur->key < key){
		*context = DIR_RIGHT;
		cur->right = delete_r(cur->right,key,del,context,k + 1);
		if((*context) == COLLISION){
			//handle subtree double black condition
			if(cur->right->color != RED){
				*context = DIR_RIGHT;
				return handleDoubleBlack(cur,context);
			}
			cur->right->color = BLACK;
			*context = CLEAN;
		}
		return cur;
	}
	if(cur->key > key){
		*context = DIR_LEFT;
		cur->left = delete_r(cur->left,key,del,context,k + 1);
		if((*context) == COLLISION){
			//handle subtree double black condition
			if(cur->left->color != RED){
				*context = DIR_LEFT;
				return handleDoubleBlack(cur,context);
			}
			cur->left->color = BLACK;
			*context = CLEAN;
		}
		return cur;
	}
	*del = cur;
	if(cur->left != RB_NIL){
		*context = DIR_LEFT;
		(*del)->left = deleteRight_r(cur->left,&cur,context);
		cur->color = (*del)->color;
		cur->right = (*del)->right;
		cur->left = (*del)->left;
		if((*context) == COLLISION){
			*context = DIR_LEFT;
			return handleDoubleBlack(cur,context);
		}
		return cur;
	}
	if(cur->right != RB_NIL){
		*context = DIR_RIGHT;
		(*del)->right = deleteLeft_r(cur->right,&cur,context);
		cur->color = (*del)->color;
		cur->left = (*del)->left;
		cur->right = (*del)->right;
		if((*context) == COLLISION){
			*context = DIR_RIGHT;
			return handleDoubleBlack(cur,context);
		}
		return cur;
	}
	if(cur->color != RED)
		*context = COLLISION;
	else
		*context = CLEAN;
	return RB_NIL;

}


static rb_treeNode_t* handleDoubleBlack(rb_treeNode_t* parent,uint8_t* context){
	if(!parent)
		return parent;
	if((*context) == DIR_RIGHT){
		if(parent->left == RB_NIL)
			return parent;
		if(parent->left->color == BLACK){ // if sibling is black
			if ((parent->left->left->color == RED)
				|| (parent->left->right->color == RED))
			{
				if (parent->left->left->color != RED) {
					parent->left = rotateLeft(parent->left, TRUE);
				}
				parent = rotateRight(parent, FALSE);	//roate right

				// color check
				parent->left->color = RED;
				if(parent->color == RED){
					parent->color = BLACK;
					parent->right->color = RED;
				}
				*context = CLEAN;
				return parent;
			}
			// both childs of sibling is black
			parent->left->color = RED;
			*context = COLLISION;
			return parent;
		} else{
			parent = rotateRight(parent,TRUE);
			*context = DIR_RIGHT;
			parent->right = handleDoubleBlack(parent->right,context);
			if((*context) == COLLISION){
				*context = DIR_RIGHT;
				return handleDoubleBlack(parent,context);
			}
			return parent;
		}
	}else {
		if(parent->right == RB_NIL)
			return parent;
		if(parent->right->color == BLACK){ // if sibling is black
			if ((parent->right->right->color == RED)
					|| (parent->right->left->right))
			{
				if (parent->right->right->color != RED) {
					parent->right = rotateRight(parent->right, TRUE);
				}
				parent = rotateLeft(parent, TRUE);
				parent->right->color = RED;
				if(parent->color == RED){
					parent->color = BLACK;
					parent->left->color = RED;
				}
				*context = CLEAN;
				return parent;
			}
			// both childs of sibling is black
			parent->right->color = RED;
			*context = COLLISION;
		} else{
			parent = rotateLeft(parent,TRUE);
			*context = DIR_LEFT;
			parent->left = handleDoubleBlack(parent->left,context);
			if((*context) == COLLISION){
				*context = DIR_LEFT;
				return handleDoubleBlack(parent,context);
			}
			return parent;
		}
	}
	return parent;
}

int cdsl_rbtreeSize(rb_treeNode_t** root){
	if(!root || !(*root))
		return 0;
	if((*root) == RB_NIL)
		return 0;
	return cdsl_rbtreeSize(&(*root)->left) + 1 + cdsl_rbtreeSize(&(*root)->right);
}


void cdsl_rbtreePrint(rb_treeNode_t** root){
	if(!root)
		return;
//	printf("\n");
	print_r(*root,0);
//	printf("\n");
}

int cdsl_rbtreeMaxDepth(rb_treeNode_t** root){
	if(!root || !(*root))
		return 0;
	if((*root) == RB_NIL)
		return 0;
	int max = 0;
	int temp = 0;
	if(max < (temp = cdsl_rbtreeMaxDepth(&(*root)->left)))
		max = temp;
	if(max < (temp = cdsl_rbtreeMaxDepth(&(*root)->right)))
		max = temp;
	return max + 1;
}

int cdsl_rbtreeIsNIL(rb_treeNode_t* rbn){
	return rbn == RB_NIL;
}



static rb_treeNode_t* insert_r(rb_treeNode_t* parent,rb_treeNode_t* item,uint8_t* context){
	if(!item || !parent)
		return RB_NIL;
	if(parent == RB_NIL){
		*context = CLEAN;
		return item;
	}
	uint8_t direction = *context;
	if(parent->key <= item->key){
		*context = DIR_RIGHT;
		parent->right = insert_r(parent->right,item,context);
		if((*context) == COLLISION)		// if collision
		{
			if(parent->left->color == parent->right->color){
				parent->right->color = !parent->left->color;
				parent->left->color =!parent->left->color;
				if((parent->left->color == BLACK) && (parent->right->color == BLACK))
					parent->color = RED;
				*context = CLEAN;
				return parent;
			}
			parent = rotateLeft(parent,TRUE);
			*context = CLEAN;
			return parent;
		}

		if((parent->color == RED) && (parent->right->color == RED)){
			*context = COLLISION;
			if(direction == DIR_LEFT){
				parent = rotateLeft(parent,FALSE);
			}
		}
	}else{
		*context = DIR_LEFT;
		parent->left = insert_r(parent->left,item,context);
		if((*context) == COLLISION)
		{
			if(parent->left->color == parent->right->color){
				parent->right->color = !parent->left->color;
				parent->left->color =!parent->left->color;
				if((parent->left->color == BLACK) && (parent->right->color == BLACK))
					parent->color = RED;
				*context = CLEAN;
				return parent;
			}
			parent = rotateRight(parent,TRUE);
			*context = CLEAN;
			return parent;
		}

		if((parent->color == RED) && (parent->left->color == RED)){
			*context = COLLISION;
			if(direction == DIR_RIGHT){
				parent = rotateRight(parent,FALSE);
			}
		}
	}
	return parent;
}

static rb_treeNode_t* rotateLeft(rb_treeNode_t* rot_pivot,BOOL paint){
	uint8_t color;
	rb_treeNode_t* nparent = rot_pivot->right;
	rot_pivot->right = nparent->left;
	nparent->left = rot_pivot;
	if(paint){
		color = nparent->color;
		nparent->color = nparent->left->color;
		nparent->left->color = color;
	}
	return nparent;
}

static rb_treeNode_t* rotateRight(rb_treeNode_t* rot_pivot,BOOL paint){
	uint8_t color;
	rb_treeNode_t* nparent = rot_pivot->left;
	rot_pivot->left = nparent->right;
	nparent->right = rot_pivot;
	if(paint){
		color = nparent->color;
		nparent->color = nparent->right->color;
		nparent->right->color = color;
	}
	return nparent;
}

static void print_tab(int cnt){
//	while(cnt--)
//		printf("\t");
}


static void print_r(rb_treeNode_t* current,int depth){

	if(current == RB_NIL){
//		print_tab(depth); printf("NIL {Color : BLACK, Key: %d} @Depth %d \n",current->key,depth);
		return;
	}
//	printf("\n");
	print_r(current->right,depth + 1);
//	print_tab(depth); printf("{Color : %s, Key: %d} @Depth %d \n",current->color == BLACK? "BLACK" : "RED",current->key,depth);
	print_r(current->left,depth + 1);
//	printf("\n");
}
