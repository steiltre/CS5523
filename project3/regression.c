
/* Gives us high resolution timers. */
#define _POSIX_C_SOURCE 200809L
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

double validation_time = 0;
double classification_time = 0;

/*******************************************************************
 *
 * Structs
 *
 ******************************************************************/

/*
 * @brief Storage for a dense matrix
 */
typedef struct {
  /* Number of rows */
  int nrows;

  /* Number of columns */
  int ncols;

  /* Matrix values */
  double * val;

} reg_mat;

/******************************************************************
 *
 * Code
 *
 *****************************************************************/

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
 * @brief Compute dot product of two vectors
 *
 * @param vec1 First vector
 * @param vec2 Second vector
 * @param dim Number of entries in vectors
 *
 * @return Dot product of vectors
 */
double reg_dot_prod(
    double * vec1,
    double * vec2,
    int dim)
{
  double sum = 0;
  for (int i=0; i<dim; i++) {
    sum += vec1[i] * vec2[i];
  }
  return sum;
}

/*
 * @brief Compute dot product of vector of doubles with vector of ints
 *
 * @param vec1 First vector (double)
 * @param vec2 Second vector (int)
 * @param dim Number of entries in vectors
 *
 * @return Dot product of vectors
 */
double reg_dot_prod_dbl_int(
    double * vec1,
    int * vec2,
    int dim)
{
  double sum = 0;
  for (int i=0; i<dim; i++) {
    sum += vec1[i] * ( (double) vec2[i] );
  }
  return sum;
}

/*
 * @brief Allocate space for matrix
 *
 * @param nrows Number of rows in matrix
 * @param ncols Number of columns in matrix
 *
 * @return Allocated matrix
 */
reg_mat * reg_mat_alloc(
    int nrows,
    int ncols)
{
  reg_mat * mat = malloc(sizeof(*mat));

  mat->nrows = nrows;
  mat->ncols = ncols;

  mat->val = malloc(nrows*ncols*sizeof(*mat->val));

  return mat;
}

/*
 * @brief Free matrix
 *
 * @param mat Matrix to free
 */
void reg_mat_free(
    reg_mat * mat)
{
  free(mat->val);
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

/*
 * @brief Transpose a matrix
 *
 * @param mat Matrix to transpose
 *
 * @return Transposed matrix
 */
reg_mat * reg_mat_transpose(
    reg_mat * mat)
{
  reg_mat * trans_mat = reg_mat_alloc(mat->ncols, mat->nrows);

  for (int i=0; i<mat->nrows; i++) {
    for (int j=0; j<mat->ncols; j++) {
      trans_mat->val[j*mat->nrows + i] = mat->val[i*mat->ncols + j];
    }
  }

  return trans_mat;
}

/*
 * @brief Multiply two matrices
 *    This is not needed very often, so it is being written to require the
 *    second matrix to be transposed (for faster dot products)
 *
 * @param mat1 Matrix 1
 * @param mat2_trans Transpose of matrix 2
 *
 * @return Product matrix
 */
reg_mat * reg_mat_mult_trans(
    reg_mat * mat1,
    reg_mat * mat2_trans)
{
  reg_mat * prod = reg_mat_alloc(mat1->nrows, mat2_trans->nrows);

  if (mat1->ncols != mat2_trans->ncols) {
    printf("Dimensions do not agree for matrix multiply");
  }

  for (int i=0; i<mat1->nrows; i++) {
    for (int j=0; j<mat2_trans->nrows; j++) {
      prod->val[i*prod->ncols + j] = reg_dot_prod(mat1->val + i*mat1->ncols, mat2_trans->val + j*mat2_trans->ncols, mat1->ncols);
    }
  }
  return prod;
}

/*
 * @brief Compute the objective function
 *
 * @param mat Matrix of data points
 * @param lbl Matrix of binary labels
 * @param lambda Regularization parameter
 * @param w Vector of weights
 *
 * @return Value of ridge regression objective function
 */
double reg_obj_func(
    reg_mat * mat,
    double * lbl,
    double lambda,
    double * w)
{
  double sum = 0;

  for (int i=0; i<mat->nrows; i++) {
    double prod = reg_dot_prod(mat->val + i*mat->ncols, w, mat->ncols);
    sum += (prod - lbl[i]) * (prod - lbl[i]);
  }
  sum += lambda * reg_dot_prod(w, w, mat->ncols);
  return sum;
}

double reg_accuracy(
    int * truth,
    int * pred,
    int npred)
{
  int corr = 0;

  for (int i=0; i<npred; i++) {
    /*
    if (truth[i] != 1) {
      truth[i] = 0;
    }
    else {
      truth[i] = 1;
    }
    */
    if (truth[i] == pred[i]) {
      corr++;
    }
  }

  return ( (double) corr ) / npred;
}

/*
 * @brief Turn vector of class labels into array of binary labels
 *
 * @param lbl Vector of labels
 * @param nlbl Number of labels
 * @param nsmpl Number of samples
 */
reg_mat * reg_binarize(
    int * lbl,
    int nlbl,
    int nsmpl)
{
  /* Store binary labels in dense matrix (can try sparse matrix as well) */
  reg_mat * bin_lbl = reg_mat_alloc(nlbl, nsmpl);

  for (int i=0; i<bin_lbl->nrows; i++) {
    for (int j=0; j<bin_lbl->ncols; j++) {
      bin_lbl->val[i*bin_lbl->ncols + j] = -1;
    }
  }

  for (int i=0; i<nsmpl; i++) {
    bin_lbl->val[ lbl[i]*nsmpl + i ] = 1;
  }

  return bin_lbl;
}

/*
 * @brief Predict label for new sample after training
 *
 * @param smpl Array for sample
 * @param w Weights from training
 * @param dim Dimension of sample
 * @param nlbls Number of possible labels
 *
 * @return Predicted label
 */
int reg_pred(
    double * smpl,
    double * w,
    int dim,
    int nlbls)
{
  double score;
  int lbl;

  score = reg_dot_prod(smpl, w, dim);
  lbl = 0;

  for (int i=1; i<nlbls; i++) {
    double new_score = reg_dot_prod(smpl, w + i*dim, dim);
    if (new_score > score) {
      score = new_score;
      lbl = i;
    }
  }
  return lbl;
}

/*
 * @brief Update weight for ridge regression
 *
 * @param data_prod Product of transpose of data matrix with data matrix
 * @param data_dot_lbl Product of transpose of data matrix with label vector
 * @param lambda Regularization parameter
 * @param w Vector of weights for regression model
 * @param i Entry in weight vector to update
 *
 * @return New weight
 */
double reg_update_wgt(
    reg_mat * data_prod,
    double * data_dot_lbl,
    double lambda,
    double * w,
    int i)
{
  double num = ( (double) data_dot_lbl[i] ) - reg_dot_prod(data_prod->val + i*data_prod->nrows, w, data_prod->nrows) + data_prod->val[i*data_prod->nrows + i] * w[i];
  double den = data_prod->val[i*data_prod->nrows + i] + lambda;

  if (num == 0) {
    return 0;
  }
  else {
    return num/den;
  }
}

/*
 * @brief Train ridge regression model
 *
 * @param train_mat Matrix of data to train from
 * @param train_mat_trans Transposed matrix of data to train from
 * @param train_lbl Matrix of binary labels for training data
 * @param data_prod Product of transpose of data matrix with data matrix
 * @param lambda Regularization parameter
 * @param w Vector of weights for regression model
 */
void reg_train(
    reg_mat * train_mat,
    reg_mat * train_mat_trans,
    reg_mat * train_lbl,
    reg_mat * data_prod,
    double lambda,
    double * w)
{
  double * data_dot_lbl = malloc( train_mat_trans->nrows * sizeof(*data_dot_lbl) );

  double * w_new = malloc( train_mat->ncols * sizeof(*w_new) );

  double old_obj, new_obj, obj_func_ratio;

  for (int lbl=0; lbl<train_lbl->nrows; lbl++) {
    old_obj = reg_obj_func(train_mat, train_lbl->val + lbl*train_lbl->ncols, lambda, w + lbl*train_mat->ncols);
    for (int i=0; i<train_mat_trans->nrows; i++) {
      data_dot_lbl[i] = reg_dot_prod(train_mat_trans->val + i*train_mat_trans->ncols, train_lbl->val + lbl*train_lbl->ncols, train_mat_trans->ncols);
    }
    do {
      for (int i=0; i<train_mat_trans->nrows; i++) {
        w[i + lbl*train_mat->ncols] = reg_update_wgt(data_prod, data_dot_lbl, lambda, w + lbl*train_mat->ncols, i);
      }

      new_obj = reg_obj_func(train_mat, train_lbl->val + lbl*train_lbl->ncols, lambda, w + lbl*train_mat->ncols);
      obj_func_ratio = (old_obj - new_obj) / old_obj;
      old_obj = new_obj;
    } while( obj_func_ratio > 0.0001 );
  }

  free(data_dot_lbl);
  free(w_new);
}

/*
 * @brief Train and validate models to determine best lambda
 *
 * @param train_mat Matrix of training data
 * @param train_lbl Matrix of training labels
 * @param valid_mat Matrix of validation data
 * @param valid_lbl Matrix of validation labels
 * @param lambda Array of regularization parameters to use
 * @param nlambda Number of lambdas to test
 * @param nlbl Number of labels
 *
 * @return "Optimal" lambda
 */
double reg_validate(
    reg_mat * train_mat,
    int * train_lbl,
    reg_mat * valid_mat,
    int * valid_lbl,
    double * lambda,
    int nlambda,
    int nlbl)
{

  double opt_lambda, opt_acc;

  double * w = calloc( train_mat->ncols * nlbl, sizeof(*w) );
  int * valid_pred = malloc(valid_mat->nrows * sizeof(*valid_pred));

  /* Create binary labels to classify with */
  reg_mat * train_bin_lbl = reg_binarize(train_lbl, nlbl, train_mat->nrows);

  reg_mat * train_mat_trans = reg_mat_transpose(train_mat);

  /* Multiply data matrix by its transpose */
  reg_mat * data_prod = reg_mat_mult_trans(train_mat_trans, train_mat_trans);

  reg_train(train_mat, train_mat_trans, train_bin_lbl, data_prod, lambda[0], w);

  for (int i=0; i<valid_mat->nrows; i++) {
    valid_pred[i] = reg_pred(valid_mat->val + i*valid_mat->ncols, w, valid_mat->ncols, nlbl);
  }

  double acc = reg_accuracy(valid_lbl, valid_pred, valid_mat->nrows);
  opt_acc = acc;
  opt_lambda = lambda[0];

  for (int j=1; j<nlambda; j++) {
    memset(w, 0, nlbl * train_mat->ncols * sizeof(*w));

    reg_train(train_mat, train_mat_trans, train_bin_lbl, data_prod, lambda[j], w);

    for (int i=0; i<valid_mat->nrows; i++) {
      valid_pred[i] = reg_pred(valid_mat->val + i*valid_mat->ncols, w, valid_mat->ncols, nlbl);
    }

    double acc = reg_accuracy(valid_lbl, valid_pred, valid_mat->nrows);

    if (acc > opt_acc) {
      opt_acc = acc;
      opt_lambda = lambda[j];
    }

  }

  reg_mat_free(data_prod);
  reg_mat_free(train_mat_trans);
  reg_mat_free(train_bin_lbl);

  return opt_lambda;
}

/*
 * @brief Perform classification using ridge regression
 *
 * @param train_mat Matrix of training data
 * @param train_lbl Matrix of training labels
 * @param valid_mat Matrix of validation data
 * @param valid_lbl Matrix of validation labels
 * @param test_mat Matrix of test data
 * @param test_lbl Matrix of test labels
 * @param lambda Regularization parameter
 * @param nlambda Number of lambda to use for validation
 * @param nlbl Number of labels
 * @param w Array to hold weights for classification
 */
int * reg_ridge_regression(
    reg_mat * train_mat,
    int * train_lbl,
    reg_mat * valid_mat,
    int * valid_lbl,
    reg_mat * test_mat,
    int * test_lbl,
    double * lambda,
    int nlambda,
    int nlbl,
    double * w)
{
  double start = monotonic_seconds();
  double opt_lambda = reg_validate(train_mat, train_lbl, valid_mat, valid_lbl, lambda, nlambda, nlbl);
  validation_time += monotonic_seconds()- start;

  /* Zero out weights before beginning */
  memset(w, 0, nlbl * train_mat->ncols * sizeof(*w));

  /* Create array to hold predictions of test classification */
  int * pred = malloc(test_mat->nrows * sizeof(*pred));

  /* Combine training and validation sets */
  reg_mat * train_valid_mat = reg_mat_alloc(train_mat->nrows + valid_mat->nrows, train_mat->ncols);
  int * train_valid_lbl = malloc( (train_mat->nrows + valid_mat->nrows) * sizeof(*train_valid_lbl) );

  memcpy(train_valid_mat->val, train_mat->val, train_mat->nrows * train_mat->ncols * sizeof(*train_valid_mat->val));
  memcpy(train_valid_mat->val + train_mat->nrows*train_mat->ncols, valid_mat->val, valid_mat->nrows * valid_mat->ncols * sizeof(*train_valid_mat->val));
  memcpy(train_valid_lbl, train_lbl, train_mat->nrows * sizeof(*train_valid_lbl));
  memcpy(train_valid_lbl + train_mat->nrows, valid_lbl, valid_mat->nrows * sizeof(*train_valid_lbl));

  start = monotonic_seconds();
  /* Create binary labels to classify with */
  reg_mat * train_valid_bin_lbl = reg_binarize(train_valid_lbl, nlbl, train_valid_mat->nrows);

  reg_mat * train_valid_mat_trans = reg_mat_transpose(train_valid_mat);

  /* Multiply data matrix by its transpose */
  reg_mat * data_prod = reg_mat_mult_trans(train_valid_mat_trans, train_valid_mat_trans);

  reg_train(train_valid_mat, train_valid_mat_trans, train_valid_bin_lbl, data_prod, opt_lambda, w);

  for (int i=0; i<test_mat->nrows; i++) {
    pred[i] = reg_pred(test_mat->val + i*test_mat->ncols, w, test_mat->ncols, nlbl);
  }
  classification_time += monotonic_seconds() - start;

  double acc = reg_accuracy(test_lbl, pred, test_mat->nrows);

  printf( "ACCURACY: %0.04f\n", acc );

  free(train_valid_lbl);
  reg_mat_free(train_valid_mat);
  reg_mat_free(train_valid_mat_trans);
  reg_mat_free(data_prod);

  return pred;
}

/*
 * @brief Read input file
 *
 * @param fname Name of file to read
 * @param lbl Array to hold labels (need to allocate memory while reading)
 */
reg_mat * reg_read_file(
    char const * const fname,
    int ** lbl)
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

    reg_mat * mat = reg_mat_alloc(nrows, ncols);

    *lbl = malloc(nrows * sizeof(**lbl));

    /* Read file to fill in matrix and labels */
    fin = fopen(fname, "r");

    read = getline(&line, &len, fin);

    for (int i=0; i<nrows; i++) {
      ptr = strtok(line, ",");
      char * end = NULL;

      *(*lbl + i) = strtoull(ptr, &end, 10);

      for (int j=0; j<ncols; j++) {
        ptr = strtok(NULL, ",");
        char * end = NULL;
        mat->val[i*ncols + j] = strtod(ptr, &end);
      }

      read = getline(&line, &len, fin);
    }

    free(line);

    return mat;
}

/*
 * @brief Write predictions to file
 *
 * @param ofname Name of output file to write
 * @param pred Classification predictions
 * @param nrows Number of predictions
 */
void reg_write_output(
    char * ofname,
    int * pred,
    int nrows)
{
  FILE * fout = fopen(ofname, "w");

  for (int i=0; i<nrows; i++) {
    fprintf(fout, "%d\n", pred[i]);
  }
  fclose(fout);
}

/*
 * @brief Write weights to file
 *
 * @param wgtfname Name of weights file to write
 * @param w Array of weights
 * @param nrows Number of rows in w
 * @param ncols Number of columns in w
 */
void reg_write_weights(
    char * wgtfname,
    double * w,
    int nrows,
    int ncols)
{
  FILE * fout = fopen(wgtfname, "w");

  for (int i=0; i<nrows; i++) {
    for (int j=0; j<ncols; j++) {
      if (j == ncols-1) {
        fprintf(fout, "%7.04f\n", w[i*ncols + j]);
      }
      else {
        fprintf(fout, "%7.04f, ", w[i*ncols + j]);
      }
    }
  }
}

int main(
    int argc,
    char ** argv)
{
  char * train_fname = argv[1];
  char * valid_fname = argv[2];
  char * test_fname = argv[3];
  char * out_fname = argv[4];
  char * wgt_fname = argv[5];

  int * train_lbl, * valid_lbl, * test_lbl;
  double lambda[7] = {0.01, 0.05, 0.1, 0.5, 1, 2, 5};

  reg_mat * train_mat = reg_read_file(train_fname, &train_lbl);
  reg_mat * valid_mat = reg_read_file(valid_fname, &valid_lbl);
  reg_mat * test_mat = reg_read_file(test_fname, &test_lbl);

  double * w = malloc(10 * train_mat->ncols * sizeof(*w));

  int * pred = reg_ridge_regression(train_mat, train_lbl, valid_mat, valid_lbl, test_mat, test_lbl, lambda, 7, 10, w);

  printf("Validation time: %0.04f\nClassification Time: %0.04f\n", validation_time, classification_time);

  reg_write_output(out_fname, pred, test_mat->nrows);
  reg_write_weights(wgt_fname, w, 10, train_mat->ncols);

  free(train_lbl);
  free(valid_lbl);
  free(test_lbl);
  free(pred);

  reg_mat_free(train_mat);
  reg_mat_free(valid_mat);
  reg_mat_free(test_mat);
}
