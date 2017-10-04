/*
 * @author Trevor Steil
 *
 * @date 10/4/17
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>


/******************************************
 * Structs
******************************************/

/*
 * @brief A structure for a node of an FP tree
 *
 * @param children Array for children of a node
 * @param parent Pointer to node's parent
 * @param ngbr Pointer to next node with same item
 * @param count Number of transactions containing pattern
 */
typedef struct fpt_node {
  struct fpt_node ** children;
  struct fpt_node * parent;
  struct fpt_node * ngbr;
  size_t count;
} fpt_node;


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
}

int main(
    int argc,
    char ** argv)
{
  return EXIT_SUCCESS;
}
