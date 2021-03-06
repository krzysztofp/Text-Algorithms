#include "farach.h"
inline int max(int a,int b) {return a>b?a:b;}
const int min_len = 10;
const int YEN = 2147483647;
/*
Main procedure of the algorithm, returns the suffix tree for a string of given length
*/
suff_tree * compute_suffix_tree(const int * S, const int len) {
	int * SPrim = get_s_prim(S,len);
	int l2 = len/2;
	suff_tree * TSPrim;
	if (l2 < min_len) {
		TSPrim = ad_hoc_suffix_tree(SPrim,l2);
	} else {
		TSPrim = compute_suffix_tree(SPrim,l2);
	}
	int * S_copy = new int[len+1];for(int i=0;i<len;i++) S_copy[i] = S[i]; S_copy[len] = YEN;
	suff_tree * odd_tree = build_odd_tree(S_copy, TSPrim,len);
	suff_tree * even_tree = build_even_tree(S_copy,odd_tree,len);
	return merge(odd_tree,even_tree,len);
}

/*
If the current length of integer string is small enough, it is possible to calculate its suffix tree ad hoc

WARNING:
1. Virually the last character is ¥ - so the character (INT_MAX==2147483647) that is greater than all other integers
*/
suff_tree * ad_hoc_suffix_tree(const int * S, const int len) {
	int 
		i,    // inner counter
		j,    // outer counter
		*x,   // temporary pointer
		k,    // temporay inne counter value
		r,		// temporary rank value
		_lcp,  // temporary longest common prefix value
		**SA; // temporary suffix array

	suff_tree * st // resulted suffix tree
		= new suff_tree();

	tree_node * nd; // temporary node 

	// copy the old integer string
	st->S = new int[len+1]; for(int i=0;i<len;i++) st->S[i] = S[i];st->S[len] = YEN;
	st->len = len+1;
	st->root = new tree_node();
	st->leaves = new tree_node *[st->len];
	SA = new int * [st->len];
	st->ranks = new int [st->len];
	for(int i=0;i<st->len;i++) {
		SA[i] = new int[st->len];
		st->ranks[i] = i;
	}
	
	for(i=0;i<len;i++) {
		for(j=0;j<len-i;j++) SA[i][j] = S[i+j];
		SA[i][j] = YEN;
	}
	SA[i][0] = YEN;

	// insert sort the suffix array and start building the suffix tree
	// void insert(const int * S, const int len) 
	for( j = len;j>=0;j--) {
		for(i = 1;i<st->len;i++) {	
			r = st->ranks[i]; x = SA[i]; k = i - 1;
			while ((k >= 0) && (x[j] < SA[k][j])) {
				SA[k + 1] = SA[k]; 
				st->ranks[k+1] = st->ranks[k];
				k--;
			}
			SA[k + 1] = x;	st->ranks[k+1] = r;
		}
	}

	// When we already do have the sorted suffix array, we may start building the compacted suffix tree
	// We must compact the trie basing both on the common suffix ends and common prefix begining
	// TODO: Compacting prefixes and suffixes
	for(int d=0;d<st->len;d++) {
		if (d==0) _lcp = 0;
		else _lcp = lcp(SA[d-1],SA[d]); // for now the lcp takes totally quadratic time, but will be soon modified into linear
		nd = st->root;
		int e=0, L=0;
		bool split_at_leaf; //determining wheter or not we have splitted a leaf
		while(true) {
			if(e<_lcp) { // if we have still work to do - go to the last child
				if(_lcp-e==1) { // if this is the last common integer character - split the tree and add new node
					// update the node with its rank and integer character
					split_at_leaf = nd->is_leaf();
					tree_node * split = split_at_leaf?nd:nd->last_child->node;
					if(split->L!=_lcp) { // there is something to split
						split->L = _lcp;
						L = st->len - st->ranks[d-1] - _lcp;
						split->add_next_child(SA[d-1][_lcp],split->rank,L);
						// update the suffix link and leaf array
						st->leaves[d-1] = split->last_child->node;
						if(d>1) st->leaves[d-2]->sl = split->last_child->node;
						if (!split_at_leaf) nd=nd->last_child->node;
					} else {
						nd = nd->last_child->node;
					}			
				}
				else {
					nd = nd->last_child->node;
				}			
			} else {
				L = st->len - st->ranks[d] - e;
				nd->add_next_child(SA[d][e],st->ranks[d],L);
				nd = nd->last_child->node;
				st->leaves[d] = nd;
				if(d>0) st->leaves[d-1]->sl = nd;
				break;
			}
			e++;
		}
	}
	print_trie(S,st);
	return st;
}

/*
Recursively builds the odd tree basing on given S and T_S'
Algorithm based on the DFS tree traversal
*/
suff_tree * build_odd_tree(const int * S, suff_tree * TSPrim, const int len) {
	if (len==0) return NULL;
	// Copy the S array
	// Returned tree is the same given, but with doubled L values
	node_list * current_node = new node_list(TSPrim->root);
	node_list * current_child_node = current_node->node->first_child;
	node_list * last_child_node = NULL;
	bool add_node = false;
	do { // DFS - takes the linear time to traverse whole tree
		if (current_node->visited==false) {
			// Are we visiting the node for the first time?
			if(current_node->node->is_root==false) {
				if(current_node->node->odded) {
					current_node->node->L=current_node->node->L*2-2;			
					printf("odd\n");
				}	else {
					current_node->node->L=current_node->node->L*2-1;			
				}
				
			}
			if (current_node->node->is_leaf()) {
				// If it is a leaf - make its rank as odd [[k -> 2k-1]]
				current_node->node->rank *= 2; // l_s = l_{2s-1}
				current_node->node->chr = S[current_node->node->rank];
			} else {
				// If it is not a leaf - double L value for internal node
				int current_first_integer,last_first_integer=-1;;
				int current_index_begin, child_counter = 0;
				while(current_child_node!=NULL) {
					int childL = current_child_node->node->rank*2;
					current_first_integer = S[childL];
					current_child_node->node->chr = current_first_integer;
					if (current_first_integer==last_first_integer) add_node=true;
					if ((current_first_integer!=last_first_integer) // the equal sibling chain has ended 
					|| ((current_first_integer==last_first_integer) && (current_child_node->next==NULL))) { // the last sibling has been met
						if (add_node) {
							tree_node * new_node = new tree_node(current_node->node,last_first_integer,-1,current_node->node->L+1);
							node_list * new_child_node = new node_list(new_node);
							node_list * temp_node = current_child_node;
							int diff = child_counter-current_index_begin;
							if ((current_child_node->next!=NULL)||(current_first_integer!=last_first_integer)) diff--; // the equal sibling chain has ended, but not the last one
							for(int i=0;i<=diff;i++) {
								temp_node = temp_node->prev;
								temp_node->node->parent = new_node;
								if(temp_node->node->L == temp_node->node->parent->L) {
									temp_node->node->L++;
									temp_node->node->odded = true;
								}
							}
							// Set the new node current child pointers
							new_child_node->current_child = temp_node;
							new_child_node->node->first_child = temp_node;
							new_child_node->node->first_child->prev = NULL;
							new_child_node->node->last_child = current_child_node->prev;
							new_child_node->node->last_child->next = NULL;
							new_child_node->node->child_count = (diff+1);

							// If it is node the first child - update the previous sibling pointers
							if (last_child_node!=NULL) last_child_node->next = new_child_node;
							else { // during the first visit in a node - update it pointers for the current and first child, which of course are the same
								current_node->node->first_child = new_child_node;
								current_node->current_child = new_child_node;
							}
							if (current_child_node->next==NULL) current_node->node->last_child = new_child_node;
							new_child_node->prev = last_child_node;

							// Update the next sibling pointers
							new_child_node->next = current_child_node;
							current_child_node->prev = new_child_node;

							// Update the parent child counter
							current_node->node->child_count -= diff;

							// Update the temporary pointers
							add_node = false;
							last_child_node = new_child_node;
						}
						// Update the current child index with the same first starting character
						current_index_begin = child_counter;
					}
					// Take next child of current node
					current_child_node = current_child_node->next;

					// Update the last first integer value
					last_first_integer = current_first_integer;

					// Update current child counter
					child_counter++;
				}
			}
			// Mark current node as visited
			current_node->visited = true;
		}
		// Deselect the temporary last child node ponter - wait for next 2 or more children chain
		last_child_node = NULL;

		if(current_node->current_child == NULL) {	// It was the last child - return to the upper level
			if(current_node->node->is_root) {
				break;
			}
			else {
				current_node = current_node->prev;
				current_node->delete_node();
				continue;
			}
		} else {	// Finally - go to the next node in the list
			current_node->add_node(current_node->current_child->node);
			// update the current node child pointer
			current_node->current_child = current_node->current_child->next;

			// update the current node pointer - perform the DFS
			current_node = current_node->next;
		}
	} while(true) ;
	delete current_node;
	current_node = NULL;

	// !!! TODO : TEST IT WELL

	// Output the odd trie
	print_trie(S,TSPrim);

	// return the complete odd tree
	return TSPrim; 
}


/*
Calculates the even tree basing on the given odd tree
*/
suff_tree * build_even_tree(const int * S, const suff_tree * TOdd, const int len) {
	// If the given input is empty - return
	if (TOdd->len==0) return NULL;
	int even_len = TOdd->len - 1;
	suff_tree * TEven = new suff_tree(even_len);
	pair ** pairs = new pair*[even_len];for(int i=0;i<even_len;i++) pairs[i] = new pair();
	int counter = 0;
	for(int i=0;i<TOdd->len;i++) {
		if(TOdd->leaves[i]->rank!=0) {
			pairs[counter]->first = S[TOdd->leaves[i]->rank-1];
			pairs[counter]->second = TOdd->leaves[i]->rank;
			counter++;
		}
	}
	// Debug session - comparing the even_len with non-zero positions counter
	if (even_len!=counter) printf("[ERROR:] even_len compared to the non-zero leafes counter failed in the function build_even_tree");
	// Debug session - printing the pair collections
	print_pairs(pairs,even_len);
	int * positions = radix_sort_pairs(pairs, even_len);
	int * even_order = new int[even_len];
	printf("Even order:\n[");
	for(int i=0;i<even_len;i++) {
		even_order[i] = pairs[positions[i]]->second - 1;
		printf("%d", even_order[i]);
		if (i != even_len-1) printf(", ");
	}
	printf("]\n");
	/*printf("\nRANKS & L's:\n\n");
	for(int j=0;j<even_len;j++) {
		int a = even_order[j];
		for(int k=0;k<2*even_len - a;k++) printf("%d",S[a+k]); printf("YEN");
		printf("\n");
	}*/
	int * lcpOdd = new int[even_len];
	lcpOdd = get_lcp_lex(TOdd);
	//lcpOdd = get_lcp_lex(TOdd);
	for(int i=0;i<even_len;i++) {
		int a = even_order[i];
		int whole_len = 2*even_len;
		int lcp = 0;
		if(i>0) {
			if(S[a]==S[even_order[i-1]]) {
				lcp = lcpOdd[i-1] + 1;
				tree_node * add_parent_node = TEven->leaves[i-1]->parent;
				if((add_parent_node->is_root) || (add_parent_node->L == lcp)) {
					node_list * newadd_parent_node->add_next_child(S[a],a,whole_len-a);
				} else {
					add_parent_node = add_parent_node->parent;
				}
			} else {
				lcp = 0;
			}
		}
		node_list * new_leaf = TEven->root->add_next_child(S[a],a,whole_len - a);
		TEven->leaves[i] = new_leaf->node;
	}

	return TEven;
}
/*
Merging both odd and even tree
*/
suff_tree * merge(suff_tree *odd_tree, suff_tree * even_tree, const int len) {
 // TODO
	return 0;
}

/*
Radix sorts the given string - basing on pairs, an returns S' - rank of <S[2*i],S[2*i+1]> on the sorted list.
*/
int * get_s_prim(const int * S, const int len) {
	int l2=(len+1)/2;
	int k;
	pair ** pairs = new pair*[l2];for(int i=0;i<l2;i++) pairs[i] = new pair();
	/*Produce pairs - begin*/
	int counter = 0;
	for(k=1;k<len;k+=2) pairs[counter++]->second = S[k];
	counter = 0;
	for(k=0;k<len;k+=2) pairs[counter++]->first = S[k];

	print_pairs(pairs,l2);

	int * positions = radix_sort_pairs(pairs, l2);
	/*Produce pairs - end*/

	// Construct the S' string
	int * SPrim = new int[l2];

	// Count different pairs in the S string
	int countPairs = 0;
	int l3 = (len-2)/2;
	for(k=0;k<l3;k++)  {
		int new_k = positions[k];
		int new_k1 = positions[k+1];
		if( (pairs[new_k]->first != pairs[new_k1]->first ) || (pairs[new_k]->second != pairs[new_k1]->second )) {
			SPrim[new_k] = countPairs++;
		} else {
			SPrim[new_k] = countPairs;
			printf("k=%d | ",k);
		}		
	}
	printf("\n");
	SPrim[positions[l3]]= countPairs;
	
	// Return the S' table
	return SPrim;
}


/*
Returns the lcp array for the lexicographically sorted chain of leaves
*/
int * get_lcp_lex(const suff_tree * st) {
	int * lcp = new int[st->len-1];
	node_list * current_node = new node_list(st->root);
	int current_leaf = -1;
	int max_common = 0;
	do {
		if(current_node->current_child == NULL) {	// It was the last child - return to the upper level
			if(current_node->node->is_root) {
				break;
			}
			else {
				// going up
				if(!current_node->node->is_leaf()) {
					max_common = current_node->prev->node->L;
				}
				current_node = current_node->prev;
				current_node->delete_node();
				continue;
			}
		} else {	// Finally - go to the next node in the list
			current_node->add_node(current_node->current_child->node);
			// update the current node child pointer
			current_node->current_child = current_node->current_child->next;
			
			// update the current node pointer - perform the DFS
			current_node = current_node->next;		
		}
		
		if(current_node->node->is_leaf()) {
			if(current_leaf < 0) {
				current_leaf++;
			} else {
				printf("lcp[%d] = %d\n",current_leaf,max_common);
				lcp[current_leaf++] = max_common;
			}
			max_common = current_node->node->parent->L;
		}
	} while(true);

	// return the lcp chain
	return lcp;
}
/*
Least common prefix
*/
int lcp(const int * A, const int lenA, const int * B, const int lenB) {
	int result = 0;
	int min = (lenA<lenB)?lenA:lenB;
	for(int i=0;i<min;i++) {
		if (A[i] == B[i]) result++; 
		else break;
	}
	return result;
}
/*
Least common prefix when we know only than word ends on YEN integer
*/
int lcp(const int * A, const int * B) {
	int result = 0;
	for(int m=0;;m++) {
		if ((A[m] != B[m]) || (A[m] == YEN) || (B[m] == YEN)) break;
		result++;
	}
	return result;
}

/*
Auxiliary function that prints the trie
*/
void print_trie(const int * S, suff_tree * st) {
	if (st->len==0) {
		printf("The trie is empty\n");
		return;
	} else {
		printf("\nTRIE:\n\n");
	}
	int curr_col = 0;
	// Returned tree is the same given, but with doubled L values
	node_list * current_node = new node_list(st->root);
	do { // DFS - takes the linear time to traverse whole tree
		// print_node:
		if (current_node->visited ==false){
			for(int i=0;i<curr_col;i++) printf(" ");
			printf("|--");
			if (current_node->node->is_root) printf("ROOT");
			else printf("L: %d; rank: %d; chr: %d",current_node->node->L, current_node->node->rank,current_node->node->chr);
			if (current_node->node->is_leaf()) printf("*");
			printf("\n");
			current_node->visited = true;
		}

		if(current_node->current_child == NULL) {
			// return to the upper level
			if(current_node->node->is_root) break;
			else {
				current_node = current_node->prev;
				current_node->delete_node();
				curr_col-=2;
			}
		} else {	// Finally - go to the next node in the list
			current_node->add_node(current_node->current_child->node);
			current_node->current_child = current_node->current_child->next;
			current_node = current_node->next;
			curr_col+=2;
		}
	} while(true) ;

	printf("\nRANKS & L's:\n\n");
	int LEN = st->len;
	for(int i=0;i<LEN;i++) {
		printf("%d\t%d\t",st->leaves[i]->rank,st->leaves[i]->L);
		for(int j=0;j<st->leaves[i]->L-1;j++) printf("%d",S[j+st->leaves[i]->rank ]); printf("YEN");
		printf("\n");
	}

	delete current_node;
	current_node=NULL;
}
/* 
Prints the array collection
*/
void print_pairs(pair ** pairs, const int len) {
	printf("[");
	for(int i=0;i<len;i++) {
		printf("<%d, %d>",pairs[i]->first,pairs[i]->second);
		if (i!=len-1) printf(", ");
	}
	printf("]\n");
}
/*
Radix sort given pairs of a given length, returns an array of positions to sort with
*/
int * radix_sort_pairs(pair ** pairs, const int len) {
	int k;
	int * positions = new int[len];for(int i=0;i<len;i++) positions[i] = i;
	int * last_positions = new int[len];for(int i=0;i<len;i++) last_positions[i] = i;

	// Search for the max
	int max1 = 0; for(k=0;k<len;k++) {
		if(pairs[k]->first > max1) {
			max1 = pairs[k]->first;
		}
	}
	int log_max1 = int(floorf(log10f((float)max1))) + 1;

	// Search for the max
	int max2 = 0; for(k=0;k<len;k++) {
		if(pairs[k]->second > max2) {
			max2 = pairs[k]->second;
		}
	}
	int log_max2 = int(floorf(log10f((float)max2))) + 1;
	
	int ** b = new int*[len];for(int i=0;i<len;i++) b[i] = new int();
	int * c = new int[10];
	// I stage - sort even positions
	for(int i=0;i<log_max2;i++) {
		int powerTen = (int) powl(10,i);
		for(k=0;k<10;k++)   c[k]=0;
		for(k=0;k<len;k++) {
			int s = (pairs[positions[k]]->second/powerTen)%10;
			c[s]++;
		}
		for(k=1;k<10;k++)   c[k]+=c[k-1];
		int s;
		for(k=len-1;k>=0;k--)   { // must be odd
			s = (pairs[positions[k]]->second/powerTen)%10;
			b[--c[s]] = &positions[k];
		}
		// Set the order depending on temporary b table
		for(k=0;k<len;k++) last_positions[k] = (*b[k]);
		for(k=0;k<len;k++) positions[k] = last_positions[k] ;
		
		true;
	}

	// II stage - sort odd positions
	for(int i=0;i<log_max1;i++) {
		int powerTen = (int) powl(10,i);
		for(k=0;k<10;k++)   c[k]=0;
		for(k=0;k<len;k++) {
			int s = (pairs[positions[k]]->first/powerTen)%10;
			c[s]++;
		}
		for(k=1;k<10;k++) c[k]+=c[k-1];
		int s;
		for(k=len-1;k>=0;k--)   { // must be odd
			s = (pairs[positions[k]]->first/powerTen)%10;
			b[--c[s]] = &positions[k];
		}
		// Set the order depending on temporary b table
		for(k=0;k<len;k++) last_positions[k] = (*b[k]);
		for(k=0;k<len;k++) positions[k] = last_positions[k] ;
	}

	// return the required positions array
	return positions;
}



// Node List Constructor 
node_list::node_list(tree_node * n) {
		this->next = NULL;
		this->node = n; 
		this->prev = NULL;
		this->current_child = n->first_child;
		this->visited = false;
}

// Adds a new node to the queue 
void node_list::add_node(tree_node * n) {
		node_list * newOne = new node_list(n);
		next = newOne;
		newOne->prev = this;	
}

// Deletes a node from the queue 
void node_list::delete_node() {
		delete next;
		next = NULL;
}

// Constructor for the root
tree_node::tree_node() {
	this->rank = -1;
	this->child_count =0;
	this->first_child= NULL;
	this->last_child= NULL;
	this->sl = NULL;
	this->parent = NULL;
	this->L = 0;
	this->is_root = true;
	this->odded = false;
}

// Constructor with parent
tree_node::tree_node(tree_node * par, int character, int r, int L) {
	this->rank = r;
	this->is_root = false; 
	this->child_count =0;
	this->first_child= NULL;
	this->last_child= NULL;
	this->sl = NULL;
	this->parent = par;
	this->L = par->L + L;
	this->chr = character;
	this->odded = false;
}

// If the current node has no children - it is alread a leaf
bool tree_node::is_leaf() {
	return ((!is_root)&&(child_count ==0));
}

// When building a suffix tree from the suffix array is is only possible to 
// add a child at the end of the children list
node_list * tree_node::add_next_child(int character, int r, int L) {
	if((this->child_count == 0) || (this->last_child->node->chr != character)) {
		tree_node * new_child_node = new tree_node(this, character, r, L);
		node_list * new_child = new node_list(new_child_node);
		new_child->prev = this->last_child;
		if(this->last_child) this->last_child->next = new_child;
		if(this->child_count==0) this->first_child = new_child;
		
		// Update the current node with newest child 
		this->last_child = new_child;
		this->child_count++;
		return new_child;
	} else {
		printf("[ERROR] Some erro while adding a new child occured");
		return NULL;
	}
}
