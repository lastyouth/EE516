#include <stdio.h>
#include <stdlib.h>
#include <queue>

using namespace std;

// colors
#define BLACK 0x106
#define RED	0x107

// tree type
#define NIL	0xA08 // null node
#define ALLOCATED 0xA09 // allocated

#define PID_LEN 9

struct rbtnode // represent redblack tree
{
	struct rbtnode *parent; //parent node
	struct rbtnode *right; // right child
	struct rbtnode *left; // left child
	int pid; // pid
	int value; // value (virtual execution time)
	int color; // color
	int type; // type
};

struct rbtnode *root; // tree's root

// prototypes
struct rbtnode* insertNode(int v, int pid); // insertNode. it uses the scheme that original binary search tree
void verifyInsertion(struct rbtnode *target); // verify the inserted node, tree structure is modified if necessary to preserve feature of redblack tree.
struct rbtnode* leftmostNode(struct rbtnode *target); // get LeftMostNode, min virtual execution time in tree
void deleteNode(struct rbtnode *target); // delete target node with preserving feature of redblack tree

struct rbtnode* grandparent(struct rbtnode* n); // retrieve the grandparent of node n
struct rbtnode* uncle(struct rbtnode* n); // retrieve the uncle of node n, 
struct rbtnode* sibling(struct rbtnode* n); // // retrieve the sibling of node n

void rotateLeft(struct rbtnode *n); // perform left rotate of node n
void rotateRight(struct rbtnode *n); // perform right rotate of node n
int IsEmpty(); // check whether tree is empty or not.

// internally used function in deleteNode function
void delete_case1(struct rbtnode *n);
void delete_case2(struct rbtnode *n);
void delete_case3(struct rbtnode *n);
void delete_case4(struct rbtnode *n);
void delete_case5(struct rbtnode *n);
void delete_case6(struct rbtnode *n);

// overall knowledge of redblack tree is referred from korean wikipedia https://ko.wikipedia.org/wiki/%EB%A0%88%EB%93%9C-%EB%B8%94%EB%9E%99_%ED%8A%B8%EB%A6%AC

// Especially, verifyInsertion and delete_case1~6 functions' logic is totally referred from above link

void printTree(struct rbtnode *current); // this function used for printing tree structure.

void printTree(struct rbtnode *current)
{
	/*
	it is only for debug purpose
	this function uses STL queue which requires g++ compilation
	
	if node's level is same, print it at same line.*/
	queue<pair<struct rbtnode*,int> > ret;
	int line = 0;

	if (current != NULL)
	{
		ret.push(make_pair(current,0));

		while (!ret.empty())
		{
			pair<struct rbtnode*, int> p = ret.front();
			struct rbtnode* c = p.first;
			ret.pop();

			if (line != p.second)
			{
				line = p.second;
				printf("\n\n");
			}
			if (p.first->type != NIL)
			{
				printf("(ADDR : %x PARENT : %x) : %d %d, COLOR : %s\t",p.first,p.first->parent, p.first->pid, p.first->value, p.first->color == RED ? "  RED" : "BLACK");
			}

			if (c->left != NULL)
			{
				ret.push(make_pair(c->left,p.second+1));
			}
			if (c->right != NULL)
			{
				ret.push(make_pair(c->right, p.second + 1));
			}
		}
	}
	printf("\n");
}

struct rbtnode* grandparent(struct rbtnode* n)
{
	/*
	retrieve grandparent of node n*/
	if ((n != NULL) && (n->parent != NULL))
	{
		return n->parent->parent;
	}
	else
	{
		return NULL;
	}
}

struct rbtnode* uncle(struct rbtnode *n)
{
	struct rbtnode *g = grandparent(n);
	/*
	retrieve uncle of node n*/
	if (g == NULL)
	{
		return NULL; // No grandparent means no uncle
	}
	if (n->parent == g->left)
	{
		return g->right;
	}
	else
	{
		return g->left;
	}
}

struct rbtnode* sibling(struct rbtnode *n)
{
	struct rbtnode *parent;
	/*
	retrieve sibling of node n
	*/
	if (n == NULL)
	{
		return NULL;
	}
	parent = n->parent;
	if (parent == NULL)
	{
		return NULL;
	}
	if (n == parent->left)
	{
		return parent->right;
	}
	else
	{
		return parent->left;
	}
}

void rotateLeft(struct rbtnode *n)
{
	struct rbtnode *c = n->right;
	struct rbtnode *p = n->parent;

	/*
	perform rotate left of node n to preserve redblack tree's feature
	*/

	if (c->left != NULL)
	{
		c->left->parent = n;
	}
	n->right = c->left;
	n->parent = c;
	c->left = n;
	c->parent = p;

	if (p != NULL)
	{
		if (p->left == n)
		{
			p->left = c;
		}
		else
		{
			p->right = c;
		}
	}
}

void rotateRight(struct rbtnode *n)
{
	struct rbtnode *c = n->left;
	struct rbtnode *p = n->parent;

	/*
	perform rotate right of node n to preserve redblack tree's feature
	*/

	if (c->right != NULL)
	{
		c->right->parent = n;
	}
	n->left = c->right;
	n->parent = c;
	c->right = n;
	c->parent = p;

	if (p != NULL) {
		if (p->right == n)
		{
			p->right = c;
		}
		else
		{
			p->left = c;
		}
	}
}

struct rbtnode* insertNode(int v,int pid)
{
	struct rbtnode *ret;
	struct rbtnode *parent;
	/*
	Insert node with virtual execution time and pid*/

	if (root == NULL)
	{
		root = (struct rbtnode*)calloc(1, sizeof(struct rbtnode));

		// allocate value
		root->value = v;
		root->pid = pid;
		root->color = BLACK; // root node should be black
		root->type = ALLOCATED;
		
		// allocate for nil-nodes
		root->left = (struct rbtnode*)calloc(1, sizeof(struct rbtnode));
		root->right = (struct rbtnode*)calloc(1, sizeof(struct rbtnode));

		root->left->color = BLACK;
		root->right->color = BLACK;
		root->left->type = NIL;
		root->right->type = NIL;
		return root;
	}
	
	ret = root;
	parent = root;

	// find proper position for insertion.
	while (ret->type != NIL)
	{
		parent = ret;
		if (ret->value > v)
		{
			ret = ret->left;
		}
		else if (ret->value < v)
		{
			ret = ret->right;
		}
		else
		{
			return NULL; // duplicated, not supported
		}
	}
	// allocate
	ret->color = RED;
	ret->value = v;
	ret->pid = pid;
	ret->type = ALLOCATED;
	ret->left = (struct rbtnode*)calloc(1, sizeof(struct rbtnode));
	ret->right = (struct rbtnode*)calloc(1, sizeof(struct rbtnode));
	ret->left->color = BLACK;
	ret->right->color = BLACK;
	ret->parent = parent;
	ret->left->parent = ret->right->parent = ret;
	ret->left->type = NIL;
	ret->right->type = NIL;
	return ret;
}

void verifyInsertion(struct rbtnode *target)
{
	struct rbtnode *u, *g;
	/*
	re-organize tree structure to satisfy redblack tree's feature*/

	if (target->parent == NULL)
	{
		// root should be black
		target->color = BLACK;
	}
	else
	{
		if (target->parent->color == BLACK)
		{
			// if parent is black, initially allocated node is red, So feature is satisfied.
			return;
		}
		u = uncle(target);

		// parent color is RED. So RED children should be black and there are same count of black node from root to leaf node
		// below routine is performing the tree re-organization.
		if (u != NULL && u->color == RED)
		{
			target->parent->color = BLACK;
			u->color = BLACK;
			g = grandparent(target);

			g->color = RED;

			verifyInsertion(g);
		}
		else
		{
			g = grandparent(target);

			if ((target == target->parent->right)&& (target->parent == g->left))
			{
				rotateLeft(target->parent);
				target = target->left;
			}
			else if ((target == target->parent->left)
				&& (target->parent == g->right))
			{
				rotateRight(target->parent);
				target = target->right;
			}
			target->parent->color = BLACK;
			g->color = RED;
			if (target == target->parent->left)
			{
				rotateRight(g);
			}
			else
			{
				rotateLeft(g);
			}

		}
	}
}

struct rbtnode* leftmostNode(struct rbtnode *target)
{
	struct rbtnode *ret = target;
	/*
	retrieve the leftmostnode
	
	it should be min value in the tree which is designated by target*/

	if (ret == NULL)
	{
		return NULL;
	}

	while (ret->type != NIL)
	{
		ret = ret->left;
	}

	ret = ret->parent;
	return ret;
}


struct rbtnode* getMaximum(struct rbtnode* target)
{
	/*
	for find the predecessor from subtree which is designated by target */
	struct rbtnode* ret;

	if (target == NULL)
	{
		return NULL;
	}
	ret = target;

	while (ret->type != NIL)
	{
		ret = ret->right;
	}
	ret = ret->parent;
	// return right most node

	return ret;
}

int isLeaf(struct rbtnode *target)
{
	// check whether the target is nil or not
	if (target->type == NIL)
	{
		return 1;
	}
	return 0;
}

// delete_case1~6 functions are used to re-organize the tree structure for redblack tree's feature
// each function will change the node's color and apply rotate operation to target node to satisfy redblack tree's feature.
void delete_case1(struct rbtnode *n)
{
	if (n->parent != NULL)
		delete_case2(n);
}

void delete_case2(struct rbtnode *n)
{
	struct rbtnode *s = sibling(n);

	if (s->color == RED) {
		n->parent->color = RED;
		s->color = BLACK;
		if (n == n->parent->left)
		{
			rotateLeft(n->parent);
		}
		else
		{
			rotateRight(n->parent);
		}
	}
	delete_case3(n);
}


void delete_case3(struct rbtnode *n)
{
	struct rbtnode *s = sibling(n);

	if ((n->parent->color == BLACK) && (s->color == BLACK) && (s->left->color == BLACK) && (s->right->color == BLACK)) 
	{
		s->color = RED;
		delete_case1(n->parent);
	}
	else
	{
		delete_case4(n);
	}
}

void delete_case4(struct rbtnode *n)
{
	struct rbtnode *s = sibling(n);

	if ((n->parent->color == RED) && (s->color == BLACK) &&	(s->left->color == BLACK) && (s->right->color == BLACK)) 
	{
		s->color = RED;
		n->parent->color = BLACK;
	}
	else
	{
		delete_case5(n);
	}
}

void delete_case5(struct rbtnode *n)
{
	struct rbtnode *s = sibling(n);

	if (s->color == BLACK) 
	{ 
		if ((n == n->parent->left) && (s->right->color == BLACK) && (s->left->color == RED)) 
		{ 
			s->color = RED;
			s->left->color = BLACK;
			rotateRight(s);
		}
		else if ((n == n->parent->right) && (s->left->color == BLACK) && (s->right->color == RED)) 
		{
			s->color = RED;
			s->right->color = BLACK;
			rotateLeft(s);
		}
	}
	delete_case6(n);
}

void delete_case6(struct rbtnode *n)
{
	struct rbtnode *s = sibling(n);

	s->color = n->parent->color;
	n->parent->color = BLACK;

	if (n == n->parent->left) 
	{
		s->right->color = BLACK;
		rotateLeft(n->parent);
	}
	else 
	{
		s->left->color = BLACK;
		rotateRight(n->parent);
	}
}

void deleteNode(struct rbtnode *target)
{
	struct rbtnode *deleteTarget = target;
	struct rbtnode *child;
	
	if (target->right->type == NIL && target->left->type == NIL)
	{
		// target node is leaf node, just remove it from the tree

		// free target left and right child
		free(target->right);
		free(target->left);
		target->right = NULL;
		target->left = NULL;

		// target node will not be released. it just changes type from ALLOCATED to NIL
		target->type = NIL;

		if (target == root)
		{
			// root node that should be deleted

			root = NULL;
		}

		return;
	}
	if (target->right->type == ALLOCATED && target->left->type == ALLOCATED)
	{
		// there are two children
		int v, pid;
		deleteTarget = getMaximum(target->left); // find predecessor in the left subtree.
		
		// exchange the value of target and deleteTarget (pid and value(execution time))
		v = deleteTarget->value;
		pid = deleteTarget->pid;
		deleteTarget->value = target->value;
		deleteTarget->value = target->pid;
		target->value = v;
		target->pid = pid;
	}
	// in this point, deleteTarget should have only one child. retrieve it.
	child = isLeaf(deleteTarget->right) ? deleteTarget->left : deleteTarget->right;

	if (deleteTarget->parent == NULL)
	{
		// deleteTarget is root, so child should be root
		child->parent = NULL;
		// new root
		root = child;
	}
	else
	{

		// replace deleteTarget to child
		if (deleteTarget->parent->left == deleteTarget)
		{
			deleteTarget->parent->left = child;
		}
		else
		{
			deleteTarget->parent->right = child;
		}
		child->parent = deleteTarget->parent;
	}
	
	// now ready to delete, perform the delete_case1~6 properly

	if (deleteTarget->color == BLACK)
	{
		if (child->color == RED)
		{
			child->color = BLACK;
		}
		else
		{
			delete_case1(child);
		}
	}
	// change the type to NIL
	deleteTarget->right = deleteTarget->left = NULL;
	deleteTarget->type = NIL;
}

int isEmpty()
{
	/*
	check whether tree is empty or not*/
	if (root == NULL)
	{
		return 1;
	}
	return 0;
}

int main()
{
	struct rbtnode *p;
	int i;
	int vrt[PID_LEN] = { 27, 19, 34, 65, 37, 7, 49, 2, 98 };

	// insert virtual execution time and pid to the redblack tree
	for (i = 0; i < PID_LEN; i++)
	{
		verifyInsertion(insertNode(vrt[i], i));
		
	}
	printTree(root);
	while (!isEmpty())
	{
		// find the leftmostNode and print it and delete that node while tree is not empty
		p = leftmostNode(root);

		printf("PID : %d, Virtual Time : %d\n", p->pid,p->value);

		deleteNode(p);
		printTree(root);
	}


	return 0;
}
