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
  /** Array of children */
  struct fpt_node ** children;

  /** Array of item pointers (only used by root) */
  struct fpt_node ** item_array;

  /** Pointer to parent of node */
  struct fpt_node * parent;

  /** POinter to next node with same item */
  struct fpt_node * ngbr;

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
} fpt_csr;


/*****************************************
 * Code
*****************************************/

/*****************************************
 * @brief Read datafile
 *
 * @param fname Name of file to read
 * **************************************/
fpt_node * read_file(
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
      num_trans = strtoull(ptr, &end, 10);
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

  fin = fopen(fname, "r");

  unsigned long long prev_trans_id = 0;

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
  }

  fclose(fin);

  return NULL;

}

int main(
    int argc,
    char ** argv)
{
  char * ifname = argv[1];

  fpt_node * tree = read_file(ifname);

  return EXIT_SUCCESS;
}
