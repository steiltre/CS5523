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
  unsigned long long item;

  /** Number of transactions containing pattern */
  unsigned long long count;
} fpt_node;

/*
 * @brief A CSR matrix
 */
typedef struct
{
  /** The number of transactions. */
  unsigned long long int num_trans;
  /** The number of total items in all transactions */
  unsigned long long int total_items;

  /** Pointer to beginning of items in each transaction */
  unsigned long long * trans_start;

  /** The items of each transaction */
  unsigned long long * items;

  /** The largest unique item ID */
  unsigned long long max_item_ID;
} fpt_csr;


/*****************************************
 * Code
*****************************************/

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
    unsigned long long item)
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
    unsigned long long item)
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

  /* NEED TO FIND RIGHT LOGIC FOR REMOVAL OF NODES */

  if (node->prev_sibling == NULL && node->next_sibling == NULL) {   /* Node has no siblings */
    node->parent->child = node->child;
    if (node->child != NULL) {
      node->child->parent = node->parent;
    }

    fpt_node_free(node);
    return;
  }

  if (node->prev_sibling != NULL) {
    node->prev_sibling->next_sibling = node->next_sibling;
  }
  else {
    node->parent->child = node->next_sibling;
  }

  if (node->next_sibling != NULL) {
    node->next_sibling->prev_sibling = node->prev_sibling;
  }
  else {
    node->

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
  for (unsigned long long i=0; i<trans->max_item_ID; i++) {
    root->item_array[i] = NULL;
  }
  root->root = root;

  fpt_node * current_node;
  fpt_node * child;

  /* Add each transaction */
  for( unsigned long long i=0; i<trans->num_trans; i++ ) {
    current_node = root;

    /* Add each item from current transaction */
    for( unsigned long long j = trans->trans_start[i]; j<trans->trans_start[i+1]; j++ ) {
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

  unsigned long long num_trans = 0;
  unsigned long long num_items = 0;

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

  unsigned long long prev_trans_id = 0;
  trans_csr->trans_start[0] = 0;   /* Pointer to beginning of item array */

  /* DOESN'T HANDLE EMPTY TRANSACTIONS!!!!!! */
  for (unsigned long long i=0; i<trans_csr->total_items; i++) {
    read = getline(&line, &len, fin);

    char * ptr = strtok(line, " ");
    char * end = NULL;

    unsigned long long trans_id = strtoull(ptr, &end, 10);
    if (trans_id > prev_trans_id) {
      trans_csr->trans_start[trans_id] = i;
      prev_trans_id = trans_id;
    }

    ptr = strtok(NULL, " ");
    end = NULL;
    unsigned long long item = strtoull(ptr, &end, 10);
    trans_csr->items[i] = item;
    if (item > trans_csr->max_item_ID) {
      trans_csr->max_item_ID = item;
    }
  }

  trans_csr->trans_start[trans_csr->num_trans] = trans_csr->total_items;   /* Set right edge of array */

  fclose(fin);

  return trans_csr;

}

int main(
    int argc,
    char ** argv)
{
  char * ifname = argv[1];

  fpt_csr * trans_csr = read_file(ifname);

  fpt_node * fp_tree = fpt_create_fp_tree(trans_csr);

  return EXIT_SUCCESS;
}
