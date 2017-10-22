/*
 * @author Trevor Steil
 *
 * @date 10/4/17
 */

/* Gives us high-resolution timers. */
#define _POSIX_C_SOURCE 200809L
#include <time.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stddef.h>


static int const DYN_ARRAY_INIT_CAPACITY = 32;

/******************************************
 * Structs
******************************************/

/*
 * @brief A structure for a node of an FP tree
 */
typedef struct fpt_node {
  /** Pointer to first child */
  struct fpt_node * child;

  /** Array of item pointers (only used by root) */
  struct fpt_node ** item_array;

  /** Pointer to parent of node */
  struct fpt_node * parent;

  /** Pointer to next node with same item */
  struct fpt_node * ngbr;

  /* Store siblings in doubly-linked list to make node deletion easier */
  /** Pointer to previous sibling */
  struct fpt_node * prev_sibling;

  /** Pointer to next sibling */
  struct fpt_node * next_sibling;

  /** Pointer to root of tree */
  struct fpt_node * root;

  /** ID of item stored at node */
  int item;

  /** Number of transactions containing pattern */
  int count;

  /** The largest unique item ID */
  int max_item_ID;
} fpt_node;

/*
 * @brief A CSR matrix
 */
typedef struct
{
  /** The number of transactions. */
  int nrows;
  /** The number of total items in all transactions */
  int nnz;

  /** Pointer to beginning of items in each transaction */
  int * row_idx;

  /** The items of each transaction */
  int * val;

  /** The largest unique item ID */
  int max_val;
} fpt_csr;

/*
 * @brief A pair of integers used to sort item indices based on frequency
 */
typedef struct
{
  /* Item frequency */
  int  count;

  /* Item ID */
  int item;
} fpt_item_freq;

/*
 * @brief A dynamic array of integers
 */
typedef struct
{
  /* Maximum number of elements */
  int capacity;

  /* Current number of elements */
  int num_elements;

  /* Array of elements */
  int * array;
} fpt_dyn_array;

/*
 * @brief A dynamic array of doubles
 */
typedef struct
{
  /* Maximum number of elements */
  int capacity;

  /* Current number of elements */
  int num_elements;

  /* Array of elements */
  double * array;
} fpt_dyn_array_dbl;

/*
 * @brief A dynamic CSR structure
 */
typedef struct
{
  /* Array of values */
  fpt_dyn_array * val;

  /* Pointer to beginning of rows */
  fpt_dyn_array * row_idx;

  /* Largest value in matrix */
  int max_val;
} fpt_dyn_csr;

/*
 * @brief A set of dynamic arrays to hold frequent itemsets
 */
typedef struct
{
  /* Dynamic array for holding frequent itemsets */
  fpt_dyn_array * itemsets;

  /* Dynamic array for holding pointers to beginning of frequent itemsets */
  fpt_dyn_array * itemset_ind;

  /* Dynamic array for holding counts of frequent itemsets */
  fpt_dyn_array * supports;
} fpt_freq_itemsets;

/*
 * @brief A set of dynamic arrays to hold generated rules
 */
typedef struct
{
  /* Dynamic array for holding left-hand sides */
  fpt_dyn_array * lhs;

  /* Dynamic array for holding pointers to beginnings of left-hand sides */
  fpt_dyn_array * lhs_idx;

  /* Dynamic array for holding right-hand sides */
  fpt_dyn_array * rhs;

  /* Dynamic array for holding pointers to beginnings of right-hand sides */
  fpt_dyn_array * rhs_idx;

  /* Dynamic array for holding supports */
  fpt_dyn_array * supp;

  /* Dynamic array for holding confidences */
  fpt_dyn_array_dbl * conf;
} fpt_rules;

/*****************************************
 * Code
*****************************************/

/**
 * * @brief Return the number of seconds since an unspecified time (e.g., Unix
 * *        epoch). This is accomplished with a high-resolution monotonic timer,
 * *        suitable for performance timing.
 * *
 * * @return The number of seconds.
 * */
static inline double monotonic_seconds()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/*
 * @brief Comparison operator for sorting item IDs based on frequency
 *
 * @param a Pointer to first fpt_item_freq
 * @param b Pointer to second fpt_item_freq
 */
int fpt_item_pair_comp(
    const void *a,
    const void *b)
{
  fpt_item_freq * a_freq = (fpt_item_freq *) a;
  fpt_item_freq * b_freq = (fpt_item_freq *) b;

  return (b_freq->count - a_freq->count);
}

/*
 * @brief Standard comparison operator for ascending order with qsort
 *
 * @param a Pointer to first int
 * @param b Pointer to second int
 *
 * return Value signifying order
 */
int fpt_lt(
    const void *a,
    const void *b)
{
  int * a_ptr = (int *) a;
  int * b_ptr = (int *) b;

  return *a_ptr - *b_ptr;
}

/*
 * @brief Initialize dynamic array
 *
 * @return Allocated dynamic array
 */
fpt_dyn_array * fpt_dyn_array_malloc()
{
  fpt_dyn_array * arr = malloc(sizeof(*arr));

  arr->capacity = DYN_ARRAY_INIT_CAPACITY;
  arr->num_elements = 0;

  arr->array = malloc(DYN_ARRAY_INIT_CAPACITY * sizeof(*arr->array));

  return arr;
}

/*
 * @brief Initialize dynamic array
 *
 * @return Allocated dynamic array
 */
fpt_dyn_array_dbl * fpt_dyn_array_dbl_malloc()
{
  fpt_dyn_array_dbl * arr = malloc(sizeof(*arr));

  arr->capacity = DYN_ARRAY_INIT_CAPACITY;
  arr->num_elements = 0;

  arr->array = malloc(DYN_ARRAY_INIT_CAPACITY * sizeof(*arr->array));

  return arr;
}

/*
 * @brief Double size of dynamic array
 *
 * @param arr Dynamic array
 */
void fpt_dyn_array_double(
    fpt_dyn_array * arr)
{
  arr->array = (int *) realloc(arr->array, 2 * arr->capacity * sizeof(*arr->array));
  arr->capacity = 2 * arr->capacity;
}

/*
 * @brief Double size of dynamic array
 *
 * @param arr Dynamic array
 */
void fpt_dyn_array_dbl_double(
    fpt_dyn_array_dbl * arr)
{
  arr->array = (double *) realloc(arr->array, 2 * arr->capacity * sizeof(*arr->array));
  arr->capacity = 2 * arr->capacity;
}

/*
 * @brief Add element to dynamic array
 *
 * @param arr Dynamic array
 * @param val Value to add to array
 */
void fpt_dyn_array_add(
    fpt_dyn_array * arr,
    int val)
{
  if(arr->num_elements == arr->capacity) {
    fpt_dyn_array_double(arr);
  }

  arr->array[arr->num_elements++] = val;
}

/*
 * @brief Add multiple elements to dynamic array
 *
 * @param arr Dynamic array
 * @param vals Array of values to add
 * @param len Number of values being added
 */
void fpt_dyn_array_add_values(
    fpt_dyn_array * arr,
    int * val,
    int len)
{
  arr->num_elements += len;

  while(arr->num_elements > arr->capacity) {
    fpt_dyn_array_double(arr);
  }

  memcpy(&arr->array[arr->num_elements-len], val, len*sizeof(*arr->array));
}

/*
 * @brief Add element to dynamic array
 *
 * @param arr Dynamic array
 * @param val Value to add to array
 */
void fpt_dyn_array_dbl_add(
    fpt_dyn_array_dbl * arr,
    double val)
{
  if(arr->num_elements == arr->capacity) {
    fpt_dyn_array_dbl_double(arr);
  }

  arr->array[arr->num_elements++] = val;
}

/*
 * @brief Free dynamic array
 *
 * @param arr Pointer to dynamic array
 */
void fpt_dyn_array_free(
    fpt_dyn_array * arr)
{
  free(arr->array);
  free(arr);
}

/*
 * @brief Free dynamic array
 *
 * @param arr Pointer to dynamic array
 */
void fpt_dyn_array_dbl_free(
    fpt_dyn_array_dbl * arr)
{
  free(arr->array);
  free(arr);
}

/*
 * @brief Initialize dynamic CSR structure
 *
 * @return Dynamic CSR with allocated arrays and default values
 */
fpt_dyn_csr * fpt_dyn_csr_init()
{
  fpt_dyn_csr * csr = malloc(sizeof(*csr));

  csr->val = fpt_dyn_array_malloc();
  csr->row_idx = fpt_dyn_array_malloc();

  /* Add initial pointer to beginning of array */
  fpt_dyn_array_add(csr->row_idx, 0);

  csr->max_val = 0;

  return csr;
}

/*
 * @brief Free dynamic CSR structure
 *
 * @param csr Dynamic CSR to free
 */
void fpt_dyn_csr_free(
    fpt_dyn_csr * csr)
{
  fpt_dyn_array_free(csr->val);
  fpt_dyn_array_free(csr->row_idx);
  free(csr);
}

/*
 * @brief Initialize frequent itemset
 *
 * @return Frequent itemset with allocated arrays and default values
 */
fpt_freq_itemsets * fpt_freq_itemsets_init()
{
  fpt_freq_itemsets * freq_itemsets = malloc(sizeof(*freq_itemsets));

  freq_itemsets->itemsets = fpt_dyn_array_malloc();
  freq_itemsets->itemset_ind = fpt_dyn_array_malloc();
  freq_itemsets->supports = fpt_dyn_array_malloc();

  fpt_dyn_array_add(freq_itemsets->itemset_ind, 0);

  return freq_itemsets;
}

/*
 * @brief Free memory for frequent itemsets
 *
 * @param freq_itemsets Set of frequent itemsets to free
 */
void fpt_freq_itemsets_free(
    fpt_freq_itemsets * freq_itemsets)
{
  fpt_dyn_array_free(freq_itemsets->itemsets);
  fpt_dyn_array_free(freq_itemsets->itemset_ind);
  fpt_dyn_array_free(freq_itemsets->supports);
  free(freq_itemsets);
}

/*
 * @brief Initialize rules
 *
 * @return Rules with allocated arrays and default values
 */
fpt_rules * fpt_rules_init()
{
  fpt_rules * rules = malloc(sizeof(*rules));

  rules->lhs = fpt_dyn_array_malloc();
  rules->lhs_idx = fpt_dyn_array_malloc();
  rules->rhs = fpt_dyn_array_malloc();
  rules->rhs_idx = fpt_dyn_array_malloc();
  rules->supp = fpt_dyn_array_malloc();
  rules->conf = fpt_dyn_array_dbl_malloc();

  fpt_dyn_array_add(rules->lhs_idx, 0);
  fpt_dyn_array_add(rules->rhs_idx, 0);

  return rules;
}

/*
 * @brief Free memory for rules
 *
 * @param rules Set of rules to free
 */
void fpt_rules_free(
    fpt_rules * rules)
{
  fpt_dyn_array_free(rules->lhs);
  fpt_dyn_array_free(rules->lhs_idx);
  fpt_dyn_array_free(rules->rhs);
  fpt_dyn_array_free(rules->rhs_idx);
  fpt_dyn_array_free(rules->supp);
  fpt_dyn_array_dbl_free(rules->conf);
  free(rules);
}

/*
 * @brief Look up support of frequent itemset
 *
 * @param itemset Array holding itemset
 * @param itemset_len Length of itemset
 * @param freq_itemsets Set of frequent itemsets
 *
 * @return Support count of itemset
 */
int fpt_lookup_support(
    int * itemset,
    int itemset_len,
    fpt_freq_itemsets * freq_itemsets)
{
  /* Items are inserted into array of frequent itemsets in order found by FP-growth algorithm. This orders
   * itemsets based on suffixes. This ordering allows a binary search. */

  int left = 0;
  int right = freq_itemsets->supports->num_elements;
  int mid;
  int len;

  while( left <= right ) {
    mid = left + (right-left)/2;
    len = (itemset_len < (freq_itemsets->itemset_ind->array[mid+1] - freq_itemsets->itemset_ind->array[mid])) ? itemset_len : (freq_itemsets->itemset_ind->array[mid+1] - freq_itemsets->itemset_ind->array[mid]);
    int itemset_match = 1;

    int start_ind = freq_itemsets->itemset_ind->array[mid+1];
    for (int i=1; i<=len; i++) {        /* Need to start checking from back of both itemsets */
      if (freq_itemsets->itemsets->array[start_ind-i] > itemset[itemset_len-i]) {
        left = mid+1;
        itemset_match = 0;
        break;
      }
      else if (freq_itemsets->itemsets->array[start_ind-i] < itemset[itemset_len-i]) {
        right = mid-1;
        itemset_match = 0;
        break;
      }
    }

    if (itemset_match) {
      if (itemset_len < (freq_itemsets->itemset_ind->array[mid+1] - freq_itemsets->itemset_ind->array[mid])) {
        right = mid-1;
      }
      else if(itemset_len > (freq_itemsets->itemset_ind->array[mid+1] - freq_itemsets->itemset_ind->array[mid])) {
        left = mid+1;
      }
      else {        /* Matches items and length */
        return freq_itemsets->supports->array[mid];
      }
    }
  }

  printf("Itemset not found:");
  for (int i=0; i<itemset_len; i++) {
    printf("%d ", itemset[i]);
  }
  printf("\n");
  return -1;
}

/*
 * @brief Compute LHS of rule given itemset and RHS
 *
 * @param itemset
 * @param itemset_len
 * @param rhs
 * @param rhs_len
 * @param lhs Variable to store left-hand side of rule
 */
void fpt_rule_lhs(
    int * itemset,
    int itemset_len,
    int * rhs,
    int rhs_len,
    int * lhs)
{
  int rhs_pos = 0;
  int lhs_pos = 0;
  int subset = 0;
  for (int i=0; i<itemset_len; i++) {
    while (rhs[rhs_pos] < itemset[i] && rhs_pos < rhs_len-1) {
      rhs_pos++;
    }
    if (rhs[rhs_pos] != itemset[i]) {
      lhs[lhs_pos++] = itemset[i];
    }
    else {
      subset = 1;
    }
  }
  if (subset == 0) {
    printf("RHS not a subset of itemset\n");
  }
}

/*
 * @brief Sort item IDs based on frequency
 *
 * @param counts Array holding counts of items
 * @param max_item_ID Largest item ID
 * @param forward_map Array to give indices of new item IDs
 * @param backward_map Array to return new item IDs to old IDs
 */
void fpt_sort_item_IDs(
    const int * counts,
    const int max_item_ID,
    int * forward_map,
    int * backward_map)
{

  fpt_item_freq * item_pairs = malloc(max_item_ID * sizeof(*item_pairs));

  for (int i=0; i<max_item_ID; i++) {
    item_pairs[i].item = i+1;
    item_pairs[i].count = counts[i];
  }

  qsort( item_pairs, max_item_ID, sizeof(*item_pairs), fpt_item_pair_comp );

  /* Create maps between new and old item IDs */
  for (int i=0; i<max_item_ID; i++) {
    backward_map[i] = item_pairs[i].item;

    int j = 0;
    while (item_pairs[j].item != i+1) {
      j += 1;
    }
    forward_map[i] = j+1;
  }

  free(item_pairs);

}

/*
 * @brief Relabel item IDs and remove infrequent items
 *
 * @param trans_csr Matrix of transactions stored in csr format
 * @param item_counts Array of counts for each item (in terms of old indices)
 * @param forward_map Array allowing mapping of item IDs to new item IDs
 * @param min_sup Minimum support count for frequent item
 *
 * @return relabeled_trans_csr Transactions with new item labels and infrequent items removed
 */
fpt_csr * fpt_relabel_item_IDs(
    const fpt_dyn_csr * trans_csr,
    const int * item_counts,
    const int * forward_map,
    const int min_sup)
{

  /* Determine number of unique frequent items */
  int frequent_items = 0;
  for (int i=0; i<trans_csr->max_val; i++) {
    if(item_counts[i] >= min_sup) {
      frequent_items += 1;
    }
  }

  /* Determine number of total frequent items */
  int total_items = 0;
  for (int i=0; i<trans_csr->val->num_elements; i++) {
    if(item_counts[trans_csr->val->array[i]-1] >= min_sup) {      /* Need to subtract 1 because items are 1-indexed */
      total_items += 1;
    }
  }

  fpt_csr * relabeled_trans_csr = malloc(sizeof(*relabeled_trans_csr));
  relabeled_trans_csr->nnz = total_items;
  relabeled_trans_csr->nrows = trans_csr->row_idx->num_elements-1;
  relabeled_trans_csr->max_val = frequent_items;
  relabeled_trans_csr->row_idx = malloc( (relabeled_trans_csr->nrows+1) * sizeof(*relabeled_trans_csr->row_idx) );
  relabeled_trans_csr->val = malloc( total_items * sizeof(*relabeled_trans_csr->val) );

  /* Add transactions to new dataset */
  int added_items = 0;
  relabeled_trans_csr->row_idx[0] = 0;
  for (int i=0; i<trans_csr->row_idx->num_elements-1; i++) {
    for (int j=trans_csr->row_idx->array[i]; j<trans_csr->row_idx->array[i+1]; j++) {
      if(item_counts[trans_csr->val->array[j]-1] >= min_sup) {
        relabeled_trans_csr->val[added_items] = forward_map[trans_csr->val->array[j]-1];   /* Need to subtract 1 because items are 1-indexed */
        added_items += 1;
      }
    }
    relabeled_trans_csr->row_idx[i+1] = added_items;
  }

  /* Sort each transaction so item IDs are in ascending order */
  for (int i=0; i<relabeled_trans_csr->nrows; i++) {
    qsort(relabeled_trans_csr->val + relabeled_trans_csr->row_idx[i], relabeled_trans_csr->row_idx[i+1] - relabeled_trans_csr->row_idx[i], sizeof(*relabeled_trans_csr->val), fpt_lt);
  }

  return relabeled_trans_csr;

}

/*
 * @brief Initialize fpt_csr structure
 *
 * @param nrows Number of rows
 * @param nnz Number of nonzeroes
 *
 * @return Allocated CSR matrix
 */
fpt_csr * fpt_malloc_csr(
    const int nrows,
    const int nnz)
{
  fpt_csr * mat = malloc(sizeof(*mat));

  mat->row_idx = malloc((nrows+1) * sizeof(*mat->row_idx));
  mat->val = malloc(nnz * sizeof(*mat->val));

  mat->nrows = nrows;
  mat->nnz = nnz;

  return mat;
}

/*
 * @brief Free memory from csr matrix
 *
 * @param mat Pointer to csr matrix
 */
void fpt_free_csr(
    fpt_csr * mat)
{
  free(mat->row_idx);
  free(mat->val);
  free(mat);
}

/*
 * @brief Creates a new node with NULL pointers
 */
fpt_node * fpt_new_node()
{
  fpt_node * node = malloc(sizeof(*node));

  node->child = NULL;
  node->item_array = NULL;
  node->parent = NULL;
  node->ngbr = NULL;
  node->prev_sibling = NULL;
  node->next_sibling = NULL;
  node->root = NULL;

  node->item = 0;
  node->count = 0;

  return node;
}

/*
 * @brief Add a child node to an existing node in the FP tree
 *
 * @param parent Pointer to parent node
 * @param item Item stored at node
 *
 * @return child Pointer to new child node
 */
fpt_node * fpt_add_child_node(
    fpt_node * parent,
    int item)
{
  fpt_node * new_node = fpt_new_node();

  new_node->item = item;
  new_node->parent = parent;
  new_node->root = parent->root;

  /* Add new node to beginning of parent's child list */
  new_node->next_sibling = parent->child;
  new_node->prev_sibling = NULL;

  if (new_node->next_sibling != NULL) {
    new_node->next_sibling->prev_sibling = new_node;    /* Make new node previous sibling of next sibling */
  }

  parent->child = new_node;

  return new_node;
}

/*
 * @brief Add a new node that is the parent of an existing node
 *
 * @param child Pointer to child node
 * @param item Item stored at node
 *
 * @return parent Pointer to new parent node
 */
fpt_node * fpt_add_parent_node(
    fpt_node * child,
    int item)
{
  fpt_node * new_node = fpt_new_node();

  new_node->item = item;
  new_node->child = child;
  new_node->root = child->root;

  child->parent = new_node;

  return new_node;
}

/*
 * @brief Delete node from FP tree
 *
 * @param node Node to delete
 */
void fpt_delete_node(
    fpt_node * node)
{

  /* Do not need to change item pointers because algorithm removes all nodes with a given item, not individual nodes */

  if (node == node->root) {
    free(node->item_array);
    free(node);
    return;
  }

  if (node->prev_sibling != NULL) {      /* Node has a previous sibling */
    node->prev_sibling->next_sibling = node->next_sibling;
  }
  else {          /* Node is first child of parent */
    node->parent->child = node->next_sibling;
  }

  if (node->next_sibling != NULL) {      /* Node has a next sibling */
    node->next_sibling->prev_sibling = node->prev_sibling;
  }

  /* Add node's children to parent's list of children */
  fpt_node * current = node->child;

  if (current != NULL) {    /* Node has children */
    while (current->next_sibling != NULL) {
      current->parent = node->parent;
      current = current->next_sibling;
    }
    current->parent = node->parent;

    /* Add children to beginning of parent's child list */
    current->next_sibling = node->parent->child;
    if(current->next_sibling != NULL) {
      current->next_sibling->prev_sibling = current;
    }
    current->parent->child = node->child;
  }

  /* Free memory */
  if (node->item_array != NULL) {
    free(node->item_array);
  }
  free(node);

}

/*
 * @brief Delete a tree starting at the root
 *
 * @param tree Pointer to root of tree
 */
void fpt_delete_tree(
    fpt_node * tree)
{
  fpt_node * current = tree->next_sibling;

  if(current != NULL) {
    fpt_delete_tree(current);
  }

  current = tree->child;

  if(current != NULL) {
    fpt_delete_tree(current);
  }

  fpt_delete_node(tree);

}

/*
 * @brief Count occurrences of each individual item
 *
 * @param mat Matrix of transactions in csr format
 *
 * @return counts Array of counts for each item
 */
int * count_items(
    fpt_dyn_csr * mat)
{
  int * counts = calloc(mat->max_val, sizeof(*counts));

  for (int i=0; i<mat->val->num_elements; i++) {
    counts[mat->val->array[i]-1] += 1;
  }

  return counts;
}

/*
 * @brief Create lists of nodes containing same item
 *
 * @param node FP-tree node being added
 */
void fpt_create_item_pointers(
    fpt_node * node)
{

  /* Recursively find nodes and add them to appropriate lists */
  if (node->root != node) {    /* Node is not root node */
    node->ngbr = node->root->item_array[node->item - 1];       /* Item IDs are 1-indexed */
    node->root->item_array[node->item - 1] = node;
  }

  fpt_node * child = node->child;

  while (child != NULL) {      /* Do until current node has no more children */
    fpt_create_item_pointers(child);
    child = child->next_sibling;
  }
}

/*
 * @brief Propagate counts from leaves up to root
 *
 * @param tree Pointer to root of FP tree
 * @param item Index of item at leaves
 */
void fpt_propagate_counts_up(
    fpt_node * tree,
    int item)
{
  for (int i=item; i>0; i--) {
    fpt_node * current = tree->item_array[i-1];

    while(current != NULL) {
      current->parent->count += current->count;
      current = current->ngbr;     /* Move through item pointers */
    }
  }
}

/*
 * @brief Travel item pointers to count specific item in tree
 *
 * @param node Pointer to beginning of list
 *
 * @return count Total count of items along item list
 */
int fpt_count_item(
    fpt_node * node)
{

  int count = 0;

  while( node != NULL ) {
    count += node->count;
    node = node->ngbr;
  }

  return count;
}

/*
 * @brief Construct FP tree
 *
 * @param trans CSR array of transactions
 *
 * @return tree Pointer to root node of FP tree
 */
fpt_node * fpt_create_fp_tree(
    fpt_csr * trans)
{
  fpt_node * root = fpt_new_node();
  root->item_array = malloc(trans->max_val * sizeof(*root->item_array));
  for (int i=0; i<trans->max_val; i++) {
    root->item_array[i] = NULL;
  }
  root->root = root;
  root->max_item_ID = trans->max_val;

  fpt_node * current_node;
  fpt_node * child;

  /* Add each transaction */
  for( int i=0; i<trans->nrows; i++ ) {
    current_node = root;

    /* Add each item from current transaction */
    for( int j = trans->row_idx[i]; j<trans->row_idx[i+1]; j++ ) {
      child = current_node->child;

      /* Search for existing path with same item */
      while (child != NULL) {
        if (child->item == trans->val[j]) {
          break;
        }
        child = child->next_sibling;
      }

      if (child == NULL) {                /* No path with current item found */
        child = fpt_add_child_node( current_node, trans->val[j] );
        child->count = 1;
        if (child->item == child->parent->item) {
          printf("Child and parent have same item\n");
        }
      }
      else {                              /* Path with matching item found */
        child->count += 1;
      }

      current_node = child;               /* Prepare to add next item */
    }
  }

  fpt_create_item_pointers(root);

  return root;

}

/*
 * @brief Create tree of prefix paths on a given item
 *
 * @param tree Pointer to root of tree to build prefix paths from
 * @param item Item prefix paths will be built on
 *
 * @return prefix_tree Pointer to root of tree of prefix paths
 */
fpt_node * fpt_create_prefix_tree(
    fpt_node * tree,
    int item)
{
  fpt_node * node_to_copy;

  fpt_node * prefix_tree = fpt_new_node();
  prefix_tree->item_array = calloc(item, sizeof(*prefix_tree->item_array));

  prefix_tree->root = prefix_tree;
  prefix_tree->max_item_ID = item;

  /* Array of pointers to current node with each item. Will use to prevent copying nodes multiple times */
  fpt_node ** current_nodes_orig = calloc((item-1), sizeof(*current_nodes_orig) );
  /* Array of pointers to current node in prefix_tree */
  fpt_node ** current_nodes_pref = calloc(item, sizeof(*current_nodes_pref) );

  node_to_copy = tree->item_array[item-1];

  fpt_node * new_node;

  /* Walk along list of desired item */
  while( node_to_copy != NULL ) {
    /* Initialize new leaf node and add to tree */
    new_node = fpt_new_node();
    new_node->root = prefix_tree;
    new_node->count = node_to_copy->count;
    new_node->item = item;
    new_node->ngbr = prefix_tree->item_array[item-1];
    prefix_tree->item_array[item-1] = new_node;

    fpt_node * parent_orig = node_to_copy->parent;
    int add_to_root = 1;    /* Boolean to tell if path already led to root. If so, don't add redundant nodes as children of root */
    /* Walk up tree and add parents */
    /* This process relies on the fact that the list of pointers is ordered across all items
     * to ensure no node is duplicated multiple times. This is done by walking up paths from
     * leaves to the root while checking item pointers to see if a node in the original tree
     * has already been visited. */
    while(parent_orig != tree->root) {
      int parent_item = parent_orig->item-1;       /* Subtract 1 to give index into arrays */
      if(parent_orig != current_nodes_orig[parent_item]) {    /* Node has not been seen in original tree */
        new_node = fpt_add_parent_node(new_node, parent_item+1);  /* Add new parent node */

        /* Store original node and new node in array for if we reach the same node in our tree later */
        current_nodes_pref[parent_item] = new_node;
        current_nodes_orig[parent_item] = parent_orig;

        /* Update pointers between items */
        new_node->ngbr = prefix_tree->item_array[parent_item];
        prefix_tree->item_array[parent_item] = new_node;
      }
      else {    /* Parent has already been seen in original */
        /* Add new node to child list of parent */
        new_node->next_sibling = current_nodes_pref[parent_item]->child;
        current_nodes_pref[parent_item]->child->prev_sibling = new_node;
        current_nodes_pref[parent_item]->child = new_node;
        new_node->parent = current_nodes_pref[parent_item];

        add_to_root = 0;
        break;    /* Once an old path is reached, move to next leaf */
      }

      parent_orig = parent_orig->parent;
    }

    if(add_to_root) {
      /* Add top node as child of root */
      new_node->parent = prefix_tree;
      new_node->next_sibling = prefix_tree->child;
      if(prefix_tree->child != NULL) {
        prefix_tree->child->prev_sibling = new_node;
      }
      prefix_tree->child = new_node;
    }

    node_to_copy = node_to_copy->ngbr;  /* Move to next node with desired item */
  }

  fpt_propagate_counts_up(prefix_tree, item);

  free(current_nodes_orig);
  free(current_nodes_pref);

  return prefix_tree;

}

/*
 * @brief Create conditional FP tree from prefix paths tree
 *
 * @param tree Pointer to root of FP tree
 * @param item ID of item to project on
 * @param min_freq Minimum frequency for inclusion in conditional tree
 *
 * @return cond_tree Pointer to root of conditional FP tree
 */
fpt_node * fpt_create_conditional_tree(
    fpt_node * tree,
    int item,
    int min_freq)
{

  int count = 0;

  fpt_node * cond_tree = fpt_create_prefix_tree(tree, item);
  cond_tree->max_item_ID = item-1;

  for (int i=0; i<cond_tree->max_item_ID; i++) {
    count = fpt_count_item(cond_tree->item_array[i]);
    fpt_node * current;
    fpt_node * next;

    if(count < min_freq) {
      current = cond_tree->item_array[i];
      while(current != NULL) {
        next = current->ngbr;
        fpt_delete_node(current);
        current = next;
      }
      cond_tree->item_array[i] = NULL;
    }
  }

  /* Remove leaf nodes from tree */
  fpt_node * current = cond_tree->item_array[cond_tree->max_item_ID];
  fpt_node * next;
  while(current != NULL) {
    next = current->ngbr;
    fpt_delete_node(current);
    current = next;
  }
  cond_tree->item_array[cond_tree->max_item_ID] = NULL;

  return cond_tree;
}

/*
 * @brief Check if a tree is linear
 *
 * @param tree Pointer to root of tree
 *
 * @return Boolean signifying if tree is linear or not
 */
int fpt_check_tree_linearity(
    fpt_node * tree)
{
  fpt_node * current = tree->child;

  while(current != NULL) {
    if (current->next_sibling != NULL) {    /* Node has a sibling */
      return 0;
    }
    current = current->child;
  }
  return 1;
}

/*
 * @brief Find frequent itemsets
 *
 * @param tree Pointer to root of FP tree
 * @param min_freq Minimum frequency for frequent pattern
 * @param suffix Current suffix
 * @param suff_len Current suffix length
 * @param freq_itemsets Container for holding frequent itemsets
 */
void fpt_find_frequent_itemsets(
    fpt_node * tree,
    int min_freq,
    int * suffix,
    int suff_len,
    fpt_freq_itemsets * freq_itemsets)
{
    for (int i=tree->max_item_ID; i>0; i--) {
      if (tree->item_array[i-1] != NULL) {
        *(suffix-suff_len-1) = i;
        suff_len += 1;

        int count = fpt_count_item(tree->item_array[i-1]);
        fpt_dyn_array_add(freq_itemsets->supports, count);

        fpt_dyn_array_add_values(freq_itemsets->itemsets, &suffix[-1 * suff_len], suff_len);
        fpt_dyn_array_add(freq_itemsets->itemset_ind, freq_itemsets->itemset_ind->array[freq_itemsets->itemset_ind->num_elements-1] + suff_len);

        fpt_node * cond_tree = fpt_create_conditional_tree(tree, i, min_freq);
        fpt_find_frequent_itemsets(cond_tree, min_freq, suffix, suff_len, freq_itemsets);

        suff_len -= 1;
        fpt_delete_tree(cond_tree);
      }
    }
}

/*
 * @brief Generate rules from an itemset using right-hand sides of rules at previous level in tree
 *
 * @param itemset Frequent itemset to generate rules from
 * @param itemset_len Length of frequent itemset
 * @param itemset_supp Support of frequent itemset
 * @param num_rules Number of rules generated at previous level
 * @param min_conf Minimum confidence for valid rule
 * @param freq_itemsets Set of frequent itemsets discovered
 * @param rules Struct for holding rules as they are generated
 * @param rule_len Length of previously generated rules
 * @param prev_rules Pointer to beginning of rules generated at previous level of lattice
 */
void fpt_gen_rules(
    int * itemset,
    int itemset_len,
    int itemset_supp,
    double min_conf,
    fpt_freq_itemsets * freq_itemsets,
    fpt_rules * rules,
    int rule_len,
    int num_rules,
    int * prev_rules)
{
  fpt_dyn_array * cand_rules = fpt_dyn_array_malloc();

  if (itemset_len > rule_len + 1) {

    /* Generate candidate rules */
    /* First handle generation of rules of length 1 */
    if (rule_len == 0) {
      fpt_dyn_array_add_values(cand_rules, itemset, itemset_len);
    }

    for (int i=0; i<num_rules; i++) {
      int j = i+1;
      while (j<num_rules) {
        if ( memcmp(&prev_rules[i*rule_len], &prev_rules[j*rule_len], (rule_len-1)*sizeof(*prev_rules)) == 0 ) {  /* Prefixes match */
          fpt_dyn_array_add_values(cand_rules, &prev_rules[i*rule_len], rule_len);
          fpt_dyn_array_add(cand_rules, prev_rules[(j+1)*rule_len - 1]);
        }
        j += 1;
      }
    }

    /* Prune candidates */
    int * marker = malloc(cand_rules->num_elements/(rule_len+1) * sizeof(*marker) );
    for (int i=0; i<cand_rules->num_elements/(rule_len+1); i++) {
      marker[i] = 1;
      /* Handle new rules of length 1 */
      if (rule_len == 0) {
        marker[i] = 1;
      }
      else {
        int match = 0;
        for (int j=0; j<rule_len; j++) {
          for (int k=0; k<num_rules; k++) {
              if ( (memcmp( &cand_rules->array[i*(rule_len+1)], &prev_rules[k*rule_len], j) == 0) &&
                  (memcmp( &cand_rules->array[i*(rule_len+1) + j + 1], &prev_rules[k*rule_len+j], rule_len-j) == 0) ) {    /* Subset is high confidence */
                match = 1;
                break;    /* Stop checking this subset of rule */
              }
            }
          if (match == 0) {    /* Subrule did not have high confidence */
            break;
          }
        }
        if (match == 1) {   /* All subrules have high confidence */
          marker[i] = 1;
        }
      }
    }

    /* Check confidence of remaining rules */
    int * lhs = malloc((itemset_len-(rule_len+1)) * sizeof(*lhs));
    int num_new_rules = 0;
    int total_prev_elements = rules->rhs->num_elements;    /* Need to keep the number of rules instead of a pointer in case dynamic array is expanded */
    for (int i=0; i<cand_rules->num_elements/(rule_len+1); i++) {
      if (marker[i] == 1) {

        fpt_rule_lhs(itemset, itemset_len, &cand_rules->array[i * (rule_len+1)], rule_len+1, lhs);

        int supp = fpt_lookup_support(lhs, itemset_len-(rule_len+1), freq_itemsets);

        double conf = itemset_supp / ( (double) supp);

        if (conf > min_conf) {
          /* Add LHS of rule to set of rules */
          fpt_dyn_array_add_values(rules->lhs, lhs, itemset_len-(rule_len+1));
          fpt_dyn_array_add(rules->lhs_idx, rules->lhs_idx->array[rules->lhs_idx->num_elements-1] + itemset_len-(rule_len+1));

          /* Add RHS of rule to set of rules */
          fpt_dyn_array_add_values(rules->rhs, &cand_rules->array[i*(rule_len+1)], rule_len+1);
          fpt_dyn_array_add(rules->rhs_idx, rules->rhs_idx->array[rules->rhs_idx->num_elements-1] + rule_len+1);

          /* Add support and confidence to set of rules */
          fpt_dyn_array_add(rules->supp, itemset_supp);
          fpt_dyn_array_dbl_add(rules->conf, conf);

          num_new_rules++;
        }
      }
    }

    free(marker);
    free(lhs);
    fpt_gen_rules(itemset, itemset_len, itemset_supp, min_conf, freq_itemsets, rules, rule_len+1, num_new_rules, &rules->rhs->array[total_prev_elements]);
  }
  fpt_dyn_array_free(cand_rules);
}

/*
 * @brief Generate rules from all frequent itemsets
 *
 * @param freq_itemsets Set of all frequent itemsets
 * @param rules Struct to hold rules as they are generated
 * @param min_conf Minimum confidence level for valid rules
 */
void fpt_gen_all_rules(
    fpt_freq_itemsets * freq_itemsets,
    fpt_rules * rules,
    double min_conf)
{
  for (int i=0; i<freq_itemsets->supports->num_elements; i++) {
    int * itemset = &freq_itemsets->itemsets->array[freq_itemsets->itemset_ind->array[i]];
    int itemset_len = freq_itemsets->itemset_ind->array[i+1] - freq_itemsets->itemset_ind->array[i];
    int itemset_supp = freq_itemsets->supports->array[i];
    fpt_gen_rules(itemset, itemset_len, itemset_supp, min_conf, freq_itemsets, rules, 0, 0, rules->rhs->array);
  }
}

/*
 * @brief Create rules with empty RHS's (for use with small min_supp values)
 *
 * @param freq_itemsets Set of all frequent itemsets
 * @param rules Struct to hold rules
 */
void fpt_create_empty_rules(
    fpt_freq_itemsets * freq_itemsets,
    fpt_rules * rules)
{
  for (int i=0; i<freq_itemsets->supports->num_elements; i++) {
    fpt_dyn_array_add_values(rules->lhs, &freq_itemsets->itemsets->array[freq_itemsets->itemset_ind->array[i]], freq_itemsets->itemset_ind->array[i+1] - freq_itemsets->itemset_ind->array[i]);
    fpt_dyn_array_add(rules->lhs_idx, rules->lhs_idx->array[rules->lhs_idx->num_elements-1] + freq_itemsets->itemset_ind->array[i+1] - freq_itemsets->itemset_ind->array[i]);

    fpt_dyn_array_add(rules->rhs_idx, 0);

    fpt_dyn_array_add(rules->supp, freq_itemsets->supports->array[i]);
    fpt_dyn_array_dbl_add(rules->conf, -1);
  }
}

/*
 * @brief Read datafile
 *
 * @param fname Name of file to read
 */
fpt_dyn_csr * read_file(
    char const * const fname)
{
  /* Open file */
  FILE * fin;
  if((fin = fopen(fname, "r")) == NULL) {
    fprintf(stderr, "unable to open '%s' for reading.\n", fname);
    exit(EXIT_FAILURE);
  }

  fpt_dyn_csr * csr = fpt_dyn_csr_init();

  int num_items = 0;
  int prev_trans_id = 0;

  char * line = malloc(1024 * 1024);
  size_t len = 0;
  ssize_t read = getline(&line, &len, fin);

  while (read >= 0) {

    char * ptr = strtok(line, " ");
    char * end = NULL;

    int trans_id = strtoull(ptr, &end, 10);
    if (trans_id > prev_trans_id) {
      prev_trans_id = trans_id;
      fpt_dyn_array_add(csr->row_idx, num_items);
    }

    ptr = strtok(NULL, " ");
    end = NULL;
    int item = strtoull(ptr, &end, 10);
    if (item > csr->max_val) {
      csr->max_val = item;
    }
    fpt_dyn_array_add(csr->val, item);

    num_items++;

    read = getline(&line, &len, fin);
  }

  fpt_dyn_array_add(csr->row_idx, num_items);

  fclose(fin);

  free(line);

  return csr;

}

/*
 * @brief Write rules to output file
 *
 * @param rules Struct holding rules generated
 * @param ofname Name of output file
 * @param map Transforms item IDs back to original IDs
 */
void fpt_write_rules_to_file(
    fpt_rules * rules,
    char * ofname,
    int * map)
{
  FILE * fout = fopen(ofname, "w");

  for (int i=0; i<rules->supp->num_elements; i++) {
    for (int j=rules->lhs_idx->array[i]; j<rules->lhs_idx->array[i+1]; j++) {
      fprintf(fout, "%d ", map[rules->lhs->array[j]-1]);
    }
    fprintf(fout, "| ");
    if ((rules->rhs_idx->array[i+1] - rules->rhs_idx->array[i]) == 0) {
      fprintf(fout, "{} ");
    }
    else {
      for (int j=rules->rhs_idx->array[i]; j<rules->rhs_idx->array[i+1]; j++) {
        fprintf(fout, "%d ", map[rules->rhs->array[j]-1]);
      }
    }
    if (rules->conf->array[i] == -1) {
      fprintf(fout, "| %d | %0.0f\n", rules->supp->array[i], rules->conf->array[i]);
    }
    else {
      fprintf(fout, "| %d | %0.04f\n", rules->supp->array[i], rules->conf->array[i]);
    }
  }
}

int main(
    int argc,
    char ** argv)
{
  int min_supp = atoi(argv[1]);
  double min_conf = atof(argv[2]);
  char * ifname = argv[3];
  char * ofname = NULL;
  if (argc > 3) {
    ofname = argv[4];
  }

  fpt_dyn_csr * trans_csr = read_file(ifname);

  int * item_counts = count_items(trans_csr);

  int * forward_map = malloc(trans_csr->max_val * sizeof(*forward_map));
  int * backward_map = malloc(trans_csr->max_val * sizeof(*backward_map));

  fpt_sort_item_IDs(item_counts, trans_csr->max_val, forward_map, backward_map);

  fpt_csr * sorted_trans_csr = fpt_relabel_item_IDs(trans_csr, item_counts, forward_map, min_supp);

  fpt_dyn_csr_free(trans_csr);

  fpt_node * fp_tree = fpt_create_fp_tree(sorted_trans_csr);

  int * suffix = malloc((fp_tree->max_item_ID) * sizeof(*suffix));
  suffix = suffix + fp_tree->max_item_ID;

  fpt_freq_itemsets * freq_itemsets = fpt_freq_itemsets_init();

  double start = monotonic_seconds();

  fpt_find_frequent_itemsets(fp_tree, min_supp, suffix, 0, freq_itemsets);

  printf("Frequent itemset generation: %0.04f seconds\n", monotonic_seconds()-start);
  printf("Number of frequent itemsets found: %d\n", freq_itemsets->supports->num_elements);

  suffix = suffix - fp_tree->max_item_ID;
  free(suffix);

  fpt_rules * rules = fpt_rules_init();

  if (min_supp > 20) {
    start = monotonic_seconds();
    fpt_gen_all_rules(freq_itemsets, rules, min_conf);
    printf("Rule generation: %0.04f seconds\n", monotonic_seconds()-start);
    printf("Number of rules generated: %d\n", rules->supp->num_elements);
  }
  else {
    fpt_create_empty_rules(freq_itemsets, rules);
  }

  if (ofname != NULL) {
    fpt_write_rules_to_file(rules, ofname, backward_map);
  }

  fpt_delete_tree(fp_tree);

  free(item_counts);
  free(forward_map);
  free(backward_map);
  fpt_freq_itemsets_free(freq_itemsets);
  fpt_rules_free(rules);
  fpt_free_csr(sorted_trans_csr);

  return EXIT_SUCCESS;
}
