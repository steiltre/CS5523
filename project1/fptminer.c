/*
 * @author Trevor Steil
 *
 * @date 10/4/17
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>


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
  int num_trans;
  /** The number of total items in all transactions */
  int total_items;

  /** Pointer to beginning of items in each transaction */
  int * trans_start;

  /** The items of each transaction */
  int * items;

  /** The largest unique item ID */
  int max_item_ID;
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


/*****************************************
 * Code
*****************************************/

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
    const fpt_csr * trans_csr,
    const int * item_counts,
    const int * forward_map,
    const int min_sup)
{

  /* Determine number of unique frequent items */
  int frequent_items = 0;
  for (int i=0; i<trans_csr->max_item_ID; i++) {
    if(item_counts[i] >= min_sup) {
      frequent_items += 1;
    }
  }

  /* Determine number of total frequent items */
  int total_items = 0;
  for (int i=0; i<trans_csr->total_items; i++) {
    if(item_counts[trans_csr->items[i]-1] >= min_sup) {      /* Need to subtract 1 because items are 1-indexed */
      total_items += 1;
    }
  }

  fpt_csr * relabeled_trans_csr = malloc(sizeof(*relabeled_trans_csr));
  relabeled_trans_csr->total_items = total_items;
  relabeled_trans_csr->num_trans = trans_csr->num_trans;
  relabeled_trans_csr->max_item_ID = frequent_items;
  relabeled_trans_csr->trans_start = malloc( (relabeled_trans_csr->num_trans+1) * sizeof(*relabeled_trans_csr->trans_start) );
  relabeled_trans_csr->items = malloc( total_items * sizeof(*relabeled_trans_csr->items) );

  /* Add transactions to new dataset */
  int added_items = 0;
  relabeled_trans_csr->trans_start[0] = 0;
  for (int i=0; i<trans_csr->num_trans; i++) {
    for (int j=trans_csr->trans_start[i]; j<trans_csr->trans_start[i+1]; j++) {
      if(item_counts[trans_csr->items[j]-1] >= min_sup) {
        relabeled_trans_csr->items[added_items] = forward_map[trans_csr->items[j]-1];   /* Need to subtract 1 because items are 1-indexed */
        added_items += 1;
      }
    }
    relabeled_trans_csr->trans_start[i+1] = added_items;
  }

  /* Sort each transaction so item IDs are in ascending order */
  for (int i=0; i<relabeled_trans_csr->num_trans; i++) {
    qsort(relabeled_trans_csr->items + relabeled_trans_csr->trans_start[i], relabeled_trans_csr->trans_start[i+1] - relabeled_trans_csr->trans_start[i], sizeof(*relabeled_trans_csr->items), fpt_lt);
  }

  return relabeled_trans_csr;

}

/*
 * @brief Free memory from csr matrix
 *
 * @param mat Pointer to csr matrix
 */
void fpt_free_csr(
    fpt_csr * mat)
{
  free(mat->trans_start);
  free(mat->items);
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

  /* SHOULD BE ABLE TO WRITE THIS MORE CLEANLY!!!!!!!!!!! */
  /* MAY BE FASTER IF TAIL OF CHILD LIST IS STORED!!!!!!!!!!!!! */
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
    fpt_csr * mat)
{
  int * counts = malloc(mat->max_item_ID * sizeof(*counts));

  for (int i=0; i<mat->max_item_ID; i++) {
    counts[i] = 0;
  }

  for (int i=0; i<mat->total_items; i++) {
    counts[mat->items[i]-1] += 1;
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
  root->item_array = malloc(trans->max_item_ID * sizeof(*root->item_array));
  for (int i=0; i<trans->max_item_ID; i++) {
    root->item_array[i] = NULL;
  }
  root->root = root;
  root->max_item_ID = trans->max_item_ID;

  fpt_node * current_node;
  fpt_node * child;

  /* Add each transaction */
  for( int i=0; i<trans->num_trans; i++ ) {
    current_node = root;

    /* Add each item from current transaction */
    for( int j = trans->trans_start[i]; j<trans->trans_start[i+1]; j++ ) {
      child = current_node->child;

      /* Search for existing path with same item */
      while (child != NULL) {
        if (child->item == trans->items[j]) {
          break;
        }
        child = child->next_sibling;
      }

      if (child == NULL) {                /* No path with current item found */
        child = fpt_add_child_node( current_node, trans->items[j] );
        child->count = 1;
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
  fpt_node * prefix_tree = fpt_new_node();
  prefix_tree->item_array = malloc(item * sizeof(*prefix_tree->item_array));
  for (int i=0; i<item; i++) {
    prefix_tree->item_array[i] = NULL;
  }

  prefix_tree->root = prefix_tree;
  prefix_tree->max_item_ID = item;

  /* Array of pointers to current node with each item. Will use to prevent copying nodes multiple times */
  fpt_node ** current_nodes_orig = malloc(item * sizeof(*current_nodes_orig) );
  /* Array of pointers to current node in prefix_tree */
  fpt_node ** current_nodes_pref = malloc(item * sizeof(*current_nodes_pref) );
  fpt_node ** dummy_nodes = malloc(item * sizeof(*current_nodes_pref) );
  for (int i=0; i<item; i++) {
    dummy_nodes[i] = fpt_new_node();
    dummy_nodes[i]->ngbr = tree->item_array[i];
    current_nodes_orig[i] = dummy_nodes[i];
    current_nodes_pref[i] = NULL;
  }
  current_nodes_orig[item-1] = tree->item_array[item-1];

  fpt_node * new_node;

  /* Walk along list of desired item */
  while( current_nodes_orig[item-1] != NULL ) {
    /* Initialize new leaf node and add to tree */
    new_node = fpt_new_node();
    new_node->root = prefix_tree;
    new_node->count = current_nodes_orig[item-1]->count;
    new_node->item = item;

    fpt_node * parent_orig = current_nodes_orig[item-1]->parent;
    int add_to_root = 1;    /* Boolean to tell if path already led to root. If so, don't add redundant nodes as children of root */
    /* Walk up tree and add parents */
    /* This process relies on the fact that the list of pointers is ordered across all items
     * to ensure no node us duplicated multiple times. This is done by walking up paths from
     * leaves to the root while checking item pointers to see if a node in the original tree
     * has already been visited. */
    while(parent_orig != tree->root) {
      if(parent_orig != current_nodes_orig[parent_orig->item-1]) {    /* Node has not been seen in original tree */
        new_node = fpt_add_parent_node(new_node, parent_orig->item);  /* Add new parent node */

        current_nodes_pref[parent_orig->item-1] = new_node;

        while(parent_orig != current_nodes_orig[parent_orig->item-1]) {   /* Walk along list until corresponding node in original tree has been found */
          current_nodes_orig[parent_orig->item-1] = current_nodes_orig[parent_orig->item-1]->ngbr;
        }
      }
      else {    /* Parent has already been seen in original */
        /* Add new node to child list of parent */
        new_node->next_sibling = current_nodes_pref[parent_orig->item-1]->child;
        current_nodes_pref[parent_orig->item-1]->child->prev_sibling = new_node;
        current_nodes_pref[parent_orig->item-1]->child = new_node;
        new_node->parent = current_nodes_pref[parent_orig->item-1];

        add_to_root = 0;
        break;    /* Once an old path is reached, move to next leaf */
      }

      parent_orig = parent_orig->parent;  /* Should use commented out "break" above to exit loop when old node is found, but this would add unwanted nodes as children of root */
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

    current_nodes_orig[item-1] = current_nodes_orig[item-1]->ngbr;  /* Move to next node with desired item */
  }

  fpt_create_item_pointers(prefix_tree);

  fpt_propagate_counts_up(prefix_tree, item);

  for(int i=0; i<item; i++) {
    free(dummy_nodes[i]);
  }
  free(current_nodes_orig);
  free(current_nodes_pref);
  free(dummy_nodes);

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

  for (int i=0; i<cond_tree->max_item_ID-1; i++) {
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

/*****************************************
 * @brief Find frequent itemsets
 *
 * @param tree Pointer to root of FP tree
 * @param item Next item to project onto
 * @param min_freq Minimum frequency for frequent pattern
 * @param suffix Current suffix
 * @param suff_len Current suffix length
 */
void fpt_find_frequent_itemsets(
    fpt_node * tree,
    int item,
    int min_freq,
    int * suffix,
    int suff_len)
{

  for (int k=suff_len-1; k>=0; k--) {
    printf("%d ", *(suffix+k));
  }
  printf("\n");

  fpt_node * cond_tree = fpt_create_conditional_tree(tree, item, min_freq);
  for (int j=item-1; j>0; j--) {
    if(cond_tree->item_array[j-1] != NULL) {
      *(suffix+suff_len) = j;
      suff_len += 1;
      fpt_find_frequent_itemsets(cond_tree, j, min_freq, suffix, suff_len);
      suff_len -= 1;
    }
  }

  fpt_delete_tree(cond_tree);

  /*
  for (int i=tree->max_item_ID; i>0; i--) {
    fpt_node * cond_tree = fpt_create_conditional_tree(tree, i, min_freq);
    for (int j=i-1; j>0; j--) {
      if(cond_tree->item_array[j-1] != NULL) {
        *(suffix+suff_len) = j;
        suff_len += 1;
        for (int k=suff_len-1; k>=0; k--) {
          printf("%llu ", *(suffix+k));
        }
        printf("\n");
        fpt_find_frequent_itemsets(cond_tree, min_freq, suffix, suff_len);
        suff_len -= 1;
      }
    }
  }
  */
}

/*****************************************
 * @brief Read datafile
 *
 * @param fname Name of file to read
 * **************************************/
fpt_csr * read_file(
    char const * const fname)
{
  /* Open file */
  FILE * fin;
  if((fin = fopen(fname, "r")) == NULL) {
    fprintf(stderr, "unable to open '%s' for reading.\n", fname);
    exit(EXIT_FAILURE);
  }

  int num_trans = 0;
  int num_items = 0;

  char * line = malloc(1024 * 1024);
  size_t len = 0;
  ssize_t read = getline(&line, &len, fin);

  while (read >= 0) {

    num_items += 1;

    char * ptr = strtok(line, " ");
    char * end = NULL;

    if ( strtoull(ptr, &end, 10)  > num_trans ) {
      num_trans = strtoull(ptr, &end, 10) + 1;      /* Transaction IDs are 0-indexed */
    }

    read = getline(&line, &len, fin);
  }

  fclose(fin);

  /* Create CSR matrix to temporarily store transactions */
  fpt_csr * trans_csr = malloc(sizeof(*trans_csr));

  trans_csr->num_trans = num_trans;
  trans_csr->total_items = num_items;
  trans_csr->trans_start = malloc((trans_csr->num_trans + 1) * sizeof(*trans_csr->trans_start));
  trans_csr->items = malloc(trans_csr->total_items * sizeof(*trans_csr->items));
  trans_csr->max_item_ID = 0;

  fin = fopen(fname, "r");

  int prev_trans_id = 0;
  trans_csr->trans_start[0] = 0;   /* Pointer to beginning of item array */

  /* DOESN'T HANDLE EMPTY TRANSACTIONS!!!!!! */
  for (int i=0; i<trans_csr->total_items; i++) {
    read = getline(&line, &len, fin);

    char * ptr = strtok(line, " ");
    char * end = NULL;

    int trans_id = strtoull(ptr, &end, 10);
    if (trans_id > prev_trans_id) {
      trans_csr->trans_start[trans_id] = i;
      prev_trans_id = trans_id;
    }

    ptr = strtok(NULL, " ");
    end = NULL;
    int item = strtoull(ptr, &end, 10);
    trans_csr->items[i] = item;
    if (item > trans_csr->max_item_ID) {
      trans_csr->max_item_ID = item;
    }
  }

  trans_csr->trans_start[trans_csr->num_trans] = trans_csr->total_items;   /* Set right edge of array */

  fclose(fin);

  free(line);

  return trans_csr;

}

int main(
    int argc,
    char ** argv)
{
  int min_sup = atoi(argv[1]);
  int min_conf = atoi(argv[2]);
  char * ifname = argv[3];
  char * ofname = argv[4];

  fpt_csr * trans_csr = read_file(ifname);

  int * item_counts = count_items(trans_csr);

  int * forward_map = malloc(trans_csr->max_item_ID * sizeof(*forward_map));
  int * backward_map = malloc(trans_csr->max_item_ID * sizeof(*backward_map));

  fpt_sort_item_IDs(item_counts, trans_csr->max_item_ID, forward_map, backward_map);

  fpt_csr * sorted_trans_csr = fpt_relabel_item_IDs(trans_csr, item_counts, forward_map, min_sup);

  fpt_free_csr(trans_csr);

  fpt_node * fp_tree = fpt_create_fp_tree(sorted_trans_csr);

  int * suffix = malloc((fp_tree->max_item_ID) * sizeof(*suffix));

  /*
  fpt_node * cond_tree = fpt_create_conditional_tree(fp_tree, 3, 1);
  cond_tree = fpt_create_conditional_tree(fp_tree, 2, 1);
  cond_tree = fpt_create_conditional_tree(fp_tree, 1, 1);
  */

  //fpt_node * tree2 = fpt_create_conditional_tree(fp_tree, 3, 1);

  //fpt_find_frequent_itemsets(fp_tree, 1, suffix, 0);

  for (int i=fp_tree->max_item_ID; i>0; i--) {
    *suffix = i;
    fpt_find_frequent_itemsets(fp_tree, i, min_sup, suffix, 1);
  }

  fpt_delete_tree(fp_tree);

  free(suffix);
  free(item_counts);
  free(forward_map);
  free(backward_map);
  fpt_free_csr(sorted_trans_csr);

  return EXIT_SUCCESS;
}
