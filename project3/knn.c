
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_K 20

/*******************************************************************
 *
 * Structs
 *
 ******************************************************************/

/*
 * @brief Storage for a dense matrix with labels for each row
 */
typedef struct {
  /* Number of rows */
  int nrows;

  /* Number of columns */
  int ncols;

  /* Matrix values */
  double * val;

  /* Labels associated to rows */
  int * lbl;
} knn_lbl_mat;

/********************************************************************
 *
 * Distance / Similarity Functions
 *
 *******************************************************************/

double Euclidean_dist(
    int dim,
    double * vec1,
    double * vec2)
{
  double dist = 0;

  for (int i=0; i<dim; i++) {
    dist += (vec2[i] - vec1[i]) * (vec2[i] - vec1[i]);
  }

  /* Returns distance squared */
  return dist;
}

double cosine_sim(
    int dim,
    double * vec1,
    double * vec2)
{
  double sim = 0;
  double norm1 = 0;
  double norm2 = 0;

  /* Compute norm of each vector as we go instead of using Euclidean_dist to reduce memory fetches */
  for (int i=0; i<dim; i++) {
    sim += vec1[i] * vec2[i];
    norm1 += vec1[i] * vec1[i];
    norm2 += vec2[i] * vec2[i];
  }

  norm1 = sqrt(norm1);
  norm2 = sqrt(norm2);

  return sim / (norm1 * norm2);
}

double jaccard_sim(
    int dim,
    double * vec1,
    double * vec2)
{
  double sim = 0;
  double norm1 = 0;
  double norm2 = 0;

  /* Compute norm of each vector as we go instead of using Euclidean_dist to reduce memory fetches */
  /* Norms are actually norms squared */
  for (int i=0; i<dim; i++) {
    sim += vec1[i] * vec2[i];
    norm1 += vec1[i] * vec1[i];
    norm2 += vec2[i] * vec2[i];
  }

  return sim / (norm1 + norm2 - sim);
}

/******************************************************************
 *
 * Comparison Operators
 *
 *****************************************************************/

int less_than(
    const void * a,
    const void * b)
{
  return ( *(double*) a < *(double*) b);
}

int greater_than(
    const void * a,
    const void * b)
{
  return ( *(double*) a> *(double*) b);
}

/******************************************************************
 *
 * Code
 *
 *****************************************************************/

/*
 * @brief Allocate space for matrix
 *
 * @param nrows Number of rows in matrix
 * @param ncols Number of columns in matrix
 *
 * @return Allocated matrix
 */
knn_lbl_mat * knn_mat_alloc(
    int nrows,
    int ncols)
{
  knn_lbl_mat * mat = malloc(sizeof(*mat));

  mat->nrows = nrows;
  mat->ncols = ncols;

  mat->val = malloc(nrows*ncols*sizeof(*mat->val));
  mat->lbl = malloc(nrows*sizeof(*mat->lbl));

  return mat;
}

/*
 * @brief Free matrix
 *
 * @param mat Matrix to free
 */
void knn_mat_free(
    knn_lbl_mat * mat)
{
  free(mat->val);
  free(mat->lbl);
  free(mat);
}

/*
 * @brief Compute accuracy of classification
 *
 * @param truth True labels
 * @param pred Predicted labels
 * @param npred Number of predictions
 *
 * @return Accuracy of classification
 */
double knn_accuracy(
    int * truth,
    int * pred,
    int npred)
{
  int corr = 0;

  for (int i=0; i<npred; i++) {
    if (truth[i] == pred[i]) {
      corr++;
    }
  }

  return ( (double) corr ) / npred;
}

/*
 * @brief Predict labels based on nearest neighbors found
 *
 * @param nn_dist Array with distances to nearest neighbors
 * @param nn_lbl Array with labels of nearest neighbors
 * @param max_k Number of nearest neighbors in nn_dist and nn_lbl
 * @param k Number of nearest neighbors to use in classification
 * @param nrows Number of rows in arrays
 * @param pred Array to hold predictions
 */
void knn_pred(
    double * nn_dist,
    int * nn_lbl,
    int max_k,
    int k,
    int nrows,
    int * pred)
{
  for (int i=0; i<nrows; i++) {
    /* Copy labels into array and sort */
    int * lbls = malloc(k * sizeof(*lbls) );
    memcpy(lbls, nn_lbl + max_k*i, k * sizeof(*lbls));
    qsort(lbls, k, sizeof(*lbls), greater_than);

    /* Find most frequent label among nearest neighbors */
    int freq_lbl = lbls[0];
    int max_freq = 1;
    int curr_lbl = lbls[0];
    int curr_freq = 1;
    for (int j=1; j<k; j++) {
      if (lbls[j] == curr_lbl) {
        curr_freq++;
      }
      else {
        curr_lbl = lbls[j];
        curr_freq = 1;
      }

      if (curr_freq > max_freq) {   /* New most frequent label */
        max_freq = curr_freq;
        freq_lbl = curr_lbl;
      }
      else if (curr_freq == max_freq) {     /* Tied for most frequent */
        if ( ( (double) rand() ) / RAND_MAX < 0.5 ) {
          max_freq = curr_freq;
          freq_lbl = curr_lbl;
        }
      }
    }
    pred[i] = freq_lbl;
  }
}

/*
 * @brief Compute k-nearest neighbors for largest allowable value of k and sort
 *
 * @param train_mat Matrix to pick nearest neighbors from
 * @param test_set Matrix to pick nearest neighbors for
 * @param k Number of neighbors to find
 * @param dist_func Distance function to use for finding nearest neighbors
 * @param comp_op Comparison operator (allows minimizing distance or maximizing similarity)
 * @param nn_dist Array to hold distances to nearest neighbors
 * @param nn_lbl Array to hold labels of nearest neighbors
 */
void knn_find_nn(
    knn_lbl_mat * train_mat,
    knn_lbl_mat * test_mat,
    int k,
    double (*dist_func)(int, double *, double *),
    int (*comp_op)(const void *, const void *),
    double * nn_dist,
    int * nn_lbl)
{
  for (int i=0; i<test_mat->nrows; i++) {
    for (int j=0; j<train_mat->nrows; j++) {
      double dist = (*dist_func)(train_mat->ncols, test_mat->val+i*test_mat->ncols, train_mat->val+j*train_mat->ncols);

      int l=0;
      while ( (comp_op(&dist, &nn_dist[(i+1)*k-l-1]) || (nn_dist[(i+1)*k-l-1] < 0) ) && l<k) {    /* Current point is closer than previous close point, or no previous close point */
        l++;
      }

      if (l>0) {   /* Current point replaces a previous close point */
        /* Shift distances and labels down */
        memcpy( nn_dist + ( (i+1)*k-l+1 ), nn_dist + ( (i+1)*k-l ), (l-1)*sizeof(*nn_dist) );
        memcpy( nn_lbl + ( (i+1)*k-l+1 ), nn_lbl + ( (i+1)*k-l ), (l-1)*sizeof(*nn_lbl) );

        nn_dist[ (i+1)*k-l ] = dist;
        nn_lbl[ (i+1)*k-l ] = train_mat->lbl[j];
      }
    }
  }
}

/*
 * @brief Use validation set to determine number of neighbors to use
 *
 * @param train_set Matrix of training data
 * @param valid_set Matrix of validation data
 * @param max_k Max number of nearest neighbors to use
 * @param dist_func Distance function to use
 * @param comp_op Comparison operator to use
 *
 * @return Number of neighbors to use based on validation
 */
int knn_num_neighbors_validate(
    knn_lbl_mat * train_mat,
    knn_lbl_mat * valid_mat,
    int max_k,
    double (*dist_func)(int, double *, double *),
    int (comp_op)(const void *, const void *))
{
  double * nn_dist = malloc( valid_mat->nrows*MAX_K*sizeof(*nn_dist) );
  int * nn_lbl = malloc( valid_mat->nrows*MAX_K*sizeof(*nn_lbl) );

  for (int i=0; i<valid_mat->nrows*MAX_K; i++) {
    nn_dist[i] = -1;
  }

  knn_find_nn( train_mat, valid_mat, max_k, dist_func, comp_op, nn_dist, nn_lbl );

  int optimal_k = 0;
  double optimal_acc = 0;

  int * pred = malloc(valid_mat->nrows * sizeof(*pred));
  for (int k=1; k<=max_k; k++) {
    knn_pred(nn_dist, nn_lbl, max_k, k, valid_mat->nrows, pred);
    double acc = knn_accuracy(valid_mat->lbl, pred, valid_mat->nrows);

    printf( "k: %d Accuracy: %0.04f\n", k, acc );

    if (acc > optimal_acc) {
      optimal_k = k;
      optimal_acc = acc;
    }
  }

  return optimal_k;

}

/*
 * @brief Perform classification using k-nearest neighbors
 *
 * @param train_mat Matrix of training data
 * @param valid_mat Matrix of validation data
 * @param test_mat Matrix of test data
 * @param max_k Max number of nearest neighbors to use
 * @param dist_func Distance function
 * @param comp_op Comparison operator
 *
 * @return Labels assigned to test set
 */
int * knn_classification(
    knn_lbl_mat * train_mat,
    knn_lbl_mat * valid_mat,
    knn_lbl_mat * test_mat,
    int max_k,
    double (*dist_func)(int, double *, double *),
    int (*comp_op)(const void *, const void *))
{
  int k = knn_num_neighbors_validate(train_mat, valid_mat, max_k, dist_func, comp_op);

  int * pred = malloc(test_mat->nrows*sizeof(*pred));

  knn_lbl_mat * train_valid_mat = knn_mat_alloc(train_mat->nrows + valid_mat->nrows, train_mat->ncols);

  /* Combine training and validation sets the lazy way */
  memcpy( train_valid_mat->val, train_mat->val, train_mat->nrows*train_mat->ncols*sizeof(*train_valid_mat->val) );
  memcpy( train_valid_mat->val + train_mat->nrows*train_mat->ncols, valid_mat->val, valid_mat->nrows*valid_mat->ncols*sizeof(*train_valid_mat->val) );
  memcpy( train_valid_mat->lbl, train_mat->lbl, train_mat->nrows*sizeof(*train_valid_mat->lbl) );
  memcpy( train_valid_mat->lbl + train_mat->nrows, valid_mat->lbl, valid_mat->nrows*sizeof(*train_valid_mat->lbl) );

  double * nn_dist = malloc( test_mat->nrows*k*sizeof(*nn_dist) );
  int * nn_lbl = malloc( test_mat->nrows*k*sizeof(*nn_lbl) );

  knn_find_nn(train_valid_mat, test_mat, k, dist_func, comp_op, nn_dist, nn_lbl);

  knn_pred(nn_dist, nn_lbl, k, k, test_mat->nrows, pred);

  double acc = knn_accuracy(test_mat->lbl, pred, test_mat->nrows);

  printf("ACCURACY: %0.04f\n", acc);
  return pred;
}

/*
 * @brief Read input file
 *
 * @param fname Name of file to read
 */
knn_lbl_mat * knn_read_file(
    char const * const fname)
{
  /* Open file */
    FILE * fin;
    if ((fin = fopen(fname, "r")) == NULL) {
      fprintf(stderr, "unable to open '%s' for reading.\n", fname);
      exit(EXIT_FAILURE);
    }

    char * line = malloc(1024 * 1024);
    size_t len = 0;

    int nrows = 0;
    int ncols = 0;

    /* Read through file once to determine number of rows and columns */
    ssize_t read = getline(&line, &len, fin);

    /* First column is label */
    char * ptr = strtok(line, ",");
    ptr = strtok(NULL, ",");

    while (ptr != NULL) {
      ncols++;

      ptr = strtok(NULL, ", ");
    }

    while (read >= 0) {
      nrows++;
      read = getline(&line, &len, fin);
    }

    knn_lbl_mat * mat = knn_mat_alloc(nrows, ncols);

    /* Read file to fill in matrix and labels */
    fin = fopen(fname, "r");

    read = getline(&line, &len, fin);

    for (int i=0; i<nrows; i++) {
      ptr = strtok(line, ",");
      char * end = NULL;

      mat->lbl[i] = strtoull(ptr, &end, 10);

      for (int j=0; j<ncols; j++) {
        ptr = strtok(NULL, ",");
        char * end = NULL;
        mat->val[i*ncols + j] = strtod(ptr, &end);
      }

      read = getline(&line, &len, fin);
    }

    return mat;
}

int main(
    int argc,
    char ** argv)
{
  srand(1);

  char * train_fname = argv[1];
  char * valid_fname = argv[2];
  char * test_fname = argv[3];
  char * out_fname = argv[4];

  knn_lbl_mat * train_mat = knn_read_file(train_fname);
  knn_lbl_mat * valid_mat = knn_read_file(valid_fname);
  knn_lbl_mat * test_mat = knn_read_file(test_fname);

  double (*dist_func)(int, double * , double *);
  int (*comp_op)(const void *, const void *);

  dist_func = jaccard_sim;
  comp_op = greater_than;

  knn_classification( train_mat, valid_mat, test_mat, MAX_K, dist_func, comp_op );
}
