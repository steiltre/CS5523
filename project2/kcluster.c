/*
 * @author Trevor Steil
 *
 * @date 11/12/17
 */

/* Gives us high resolution timers. */
#define _POSIX_C_SOURCE 200809L
#include <time.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>

double dot_prod_time = 0;
double add_dense_time = 0;
double add_sparse_dense_time = 0;
double total_time = 0;

/***********************************************
 *
 * Structs
 *
 **********************************************/

typedef struct {

  /* Number of rows in array */
  int num_rows;

  /* Number of columns in array */
  int num_cols;

  /* Number of nonzeroes */
  int nnz;

  /* Pointer to beginning of row's values in array */
  int * row_ptr;

  /* Array of indices for columns corresponding values */
  int * row_ind;

  /* Array of values */
  double * val;
} kc_csr;

typedef struct {

  /* Number of strings in array */
  int num_rows;

  /* Number of total characters */
  int nnz;

  /* Pointer to beginning of each string */
  int * row_ptr;

  /* Array of characters */
  char * val;
} kc_csr_char;

/*
 * @brief Struct to hold clustering state info
 */
typedef struct kc_state {

  /* CSR array of data points */
  kc_csr * data;

  /* Array of cluster assignments */
  int * clusters;

  /* Array to store optimal cluster assignments */
  int * opt_clusters;

  /* Array of norms for data points */
  double * data_norms;

  /* Array of dot products of data points with global centroid
   * Prevents recalculation of values during E1 updates */
  double * data_gc_dot;

  /* Array of centroids */
  double * centroids;

  /* Array of centroid norms */
  double * centroid_norms;

  /* Array of dot products of centroids with global centroid
   * Prevents recalculation of values during E1 updates */
  double * centroid_gc_dot;

  /* Array for centroid of whole dataset */
  double * global_centroid;

  /* Array of cluster sizes */
  int * cluster_sizes;

  /* Array of cluster sizes for optimal solution */
  int * opt_cluster_sizes;

  /* Number of clusters */
  int num_clusters;

  /* Number of trials */
  int num_trials;

  /* Pointer to function to update assignment of single point */
  void (*update_func)(int, struct kc_state *);

  /* Pointer to objective function */
  double (*obj_func)(struct kc_state *);

  /* Dimensionality of points */
  int dim;

  /* Flag to determine if objective is being minimized (0) or maximized(1) */
  int opt;

  /* Number of points updated in current iteration */
  int updates;

  /* Number of iterations */
  int iter;

  /* Optimal objective function found so far */
  double opt_obj;
} kc_state;

/***********************************************
 *
 * Utilities
 *
 **********************************************/

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
 * @brief Capitalize all characters in char array
 *
 * @param arr Array of characters
 * @param len Length of character array
 */
void kc_capitalize(
    char * arr,
    int len)
{
  for (int i=0; i<len; i++) {
    arr[i] = toupper(arr[i]);
  }
}

/*
 * @brief Compute the dot product between two dense vectors
 *
 * @param vec1 First vector
 * @param vec2 Second vector
 * @param dim Dimension of vectors
 *
 * @return Dot product
 */
double kc_dot_prod(
    double * vec1,
    double * vec2,
    int dim)
{
  double prod = 0;

  double start = monotonic_seconds();

  #pragma omp parallel reduction(+: prod)
  {
    #pragma omp for schedule(static)
    for (int i=0; i<dim; i++) {
      prod += vec1[i] * vec2[i];
    }
  }

  dot_prod_time += monotonic_seconds() - start;
  return prod;
}

/*
 * @brief Compute dot product between dense vector and sparse vector
 *
 * @param vec1 Dense vector
 * @param vec2_ind Sparse vector indices
 * @param vec2_val Sparse vector values
 * @param vec2_nnz Sparse vector number of nonzeroes
 *
 * @return Dot product of vectors
 */
double kc_dot_prod_den_sp(
    double * vec1,
    int * vec2_ind,
    double * vec2_val,
    int vec2_nnz)
{
  double prod = 0;

  double start = monotonic_seconds();

  #pragma omp parallel reduction(+: prod)
  {
    #pragma omp for schedule(static)
    for (int i=0; i<vec2_nnz; i++) {
      prod += vec2_val[i] * vec1[ vec2_ind[i] ];
    }
  }

  dot_prod_time += monotonic_seconds() - start;
  return prod;
}

/*
 * @brief Add and scale a dense vector and a sparse vector
 *
 * @param vec1 Dense vector
 * @param vec1_dim Dimension of dense vector
 * @param scale1 Scaling factor of dense vector
 * @param vec2_ind Sparse vector indices
 * @param vec2_val Sparse vector values
 * @param vec2_nnz Number of nonzeroes in sparse vector
 * @param scale2 Scaling factor of sparse vector
 * @param res Pointer to allocated vector to hold result
 */
void kc_vec_add_den_sp(
    double * vec1,
    int vec1_dim,
    double scale1,
    int * vec2_ind,
    double * vec2_val,
    int vec2_nnz,
    double scale2,
    double * res)
{
  double start = monotonic_seconds();

  if (scale1 == 1) {
    memcpy(res, vec1, vec1_dim * sizeof(*res) );
  }
  else {
    #pragma omp parallel
    {
      #pragma omp for schedule(static)
      for (int i=0; i<vec1_dim; i++) {
        res[i] = scale1 * vec1[i];
      }
    }
  }

  #pragma omp parallel
  {
    #pragma omp for
    for (int i=0; i<vec2_nnz; i++) {
      res[ vec2_ind[i] ] += scale2 * vec2_val[i];
    }
  }

  add_sparse_dense_time += monotonic_seconds() - start;
}

/*
 * @brief Add and scale two dense vectors
 *
 * @param vec1 First vector
 * @param scale1 Scaling factor of first vector
 * @param vec2 Second vector
 * @param scale2 Scaling factor of second vector
 * @param dim Dimension of vectors
 * @param res Allocated vector to hold result
 */
void kc_vec_add(
    double * vec1,
    double scale1,
    double * vec2,
    double scale2,
    int dim,
    double * res)
{
  double start = monotonic_seconds();

  #pragma omp parallel
  {
    #pragma omp for schedule(static)
    for (int i=0; i<dim; i++) {
      res[i] = scale1*vec1[i] + scale2*vec2[i];
    }
  }

  add_dense_time += monotonic_seconds() - start;
}

/*
 * @brief Store norm for all data points
 *
 * @param state State structure for clustering
 */
void kc_data_norms(
    kc_state * state)
{
  for (int i=0; i<state->data->num_rows; i++) {
    double norm = 0;
    for (int j=state->data->row_ptr[i]; j<state->data->row_ptr[i+1]; j++) {
      norm += state->data->val[j] * state->data->val[j];
    }
    state->data_norms[i] = norm;
  }
}

/*
 * @brief Compute norm of a centroid
 *
 * @param state State structure for clustering
 * @param i ID of centroid
 */
void kc_centroid_norm(
    kc_state * state,
    int i)
{
  double norm = kc_dot_prod( state->centroids + i * state->dim, state->centroids + i * state->dim, state->dim );
  state->centroid_norms[i] = norm;
}

/*
 * @brief Compute dot product of a centroid with global centroid
 *
 * @param state State structure for clustering
 * @param i ID of centroid
 */
void kc_centroid_gc_dot(
    kc_state * state,
    int i)
{
  state->centroid_gc_dot[i] = kc_dot_prod( state->centroids + i*state->dim, state->global_centroid, state->dim);
}

/***********************************************
 *
 * Cluster updates
 *
 **********************************************/

/*
 * @brief Update clusters using SSE criterion
 *
 * @param pt_ID ID of point to update
 * @param state State structure for clustering
 */
void kc_update_sse(
    int pt_ID,
    kc_state * state)
{
  /* Calculate change in SSE from moving pt out of current cluster */
  int sparse_size = state->data->row_ptr[pt_ID+1] - state->data->row_ptr[pt_ID];     /* size of sparse vector of data point */

  double scale_cent, scale_pt, change_obj;
  double * new_cent1, * diff_cents, * diff_cent_pt;

  new_cent1 = malloc(state->dim * sizeof(*new_cent1));
  diff_cents = malloc(state->dim * sizeof(*diff_cents));
  diff_cent_pt = malloc(state->dim * sizeof(*diff_cent_pt));

  if (state->clusters[pt_ID] != -1) {       /* Only calculate change if point is leaving a valid cluster */
    /* Define scaling factors for vectors */
    scale_cent = ( (double) state->cluster_sizes[ state->clusters[pt_ID] ] ) / ( state->cluster_sizes[ state->clusters[pt_ID] ] - 1 );
    scale_pt = ( (double) -1 ) / ( state->cluster_sizes[ state->clusters[pt_ID] ] - 1 );

    kc_vec_add_den_sp( state->centroids + (state->clusters[pt_ID] * state->dim), state->dim, scale_cent,
        state->data->row_ind + state->data->row_ptr[pt_ID], state->data->val + state->data->row_ptr[pt_ID], sparse_size, scale_pt, new_cent1);

    kc_vec_add( state->centroids + (state->clusters[pt_ID] * state->dim), 1, new_cent1, -1, state->dim, diff_cents);
    kc_vec_add_den_sp( state->centroids + (state->clusters[pt_ID] * state->dim), state->dim, 1,
        state->data->row_ind + state->data->row_ptr[pt_ID], state->data->val + state->data->row_ptr[pt_ID], sparse_size, -1, diff_cent_pt);
    /* Set change in objective function to be value from moving current point out of current cluster */
    change_obj = 2 * kc_dot_prod( diff_cents, diff_cent_pt, state->dim ) +
      (2 - state->cluster_sizes[ state->clusters[pt_ID] ]) / ((double) state->cluster_sizes[ state->clusters[pt_ID] ] - 1) * kc_dot_prod(diff_cent_pt, diff_cent_pt, state->dim);
  }
  else {
    change_obj = 0;
  }

  double best_change = 0;
  int curr_best_cluster = state->clusters[pt_ID];
  for (int i=0; i<state->num_clusters; i++) {
    if (i != state->clusters[pt_ID]) {
      double scale = ( (double) state->cluster_sizes[i] ) / ( state->cluster_sizes[i] + 1 );

      double dot_prod = kc_dot_prod_den_sp( state->centroids + i*state->dim, state->data->row_ind + state->data->row_ptr[pt_ID],
          state->data->val + state->data->row_ptr[pt_ID], state->data->row_ptr[pt_ID+1] - state->data->row_ptr[pt_ID] );

      double total_change_obj = change_obj + scale * (state->centroid_norms[i] - 2 * dot_prod + state->data_norms[pt_ID]);

      if ((total_change_obj < best_change) | (curr_best_cluster == -1)) {
        curr_best_cluster = i;
        best_change = total_change_obj;
      }
    }
  }

  /* Update values */
  if (curr_best_cluster != state->clusters[pt_ID]) {
    if (state->clusters[pt_ID] != -1) {        /* Only remove point from old cluster if previously assigned to a cluster */
      memcpy(state->centroids + state->clusters[pt_ID] * state->dim, new_cent1, state->dim * sizeof(*state->centroids));
      state->cluster_sizes[ state->clusters[pt_ID] ]--;
      kc_centroid_norm(state, state->clusters[pt_ID]);
    }
    state->clusters[pt_ID] = curr_best_cluster;
    kc_vec_add_den_sp( state->centroids + curr_best_cluster*state->dim, state->dim, ( (double) state->cluster_sizes[curr_best_cluster] ) / ( state->cluster_sizes[curr_best_cluster] + 1 ),
        state->data->row_ind + state->data->row_ptr[pt_ID], state->data->val + state->data->row_ptr[pt_ID], state->data->row_ptr[pt_ID+1] - state->data->row_ptr[pt_ID],
        ( (double) 1 ) / (state->cluster_sizes[curr_best_cluster] + 1), state->centroids + curr_best_cluster*state->dim);
    kc_centroid_norm(state, curr_best_cluster);
    state->cluster_sizes[curr_best_cluster]++;

    state->updates++;
  }

  free(new_cent1);
  free(diff_cents);
  free(diff_cent_pt);
}

/*
 * @brief Update clusters using I2 criterion
 *
 * @param pt_ID ID of point to update
 * @param state State structure for clustering
 */
void kc_update_i2(
    int pt_ID,
    kc_state * state)
{
  /* Calculate change in I2 from moving pt out of current cluster */
  int sparse_size = state->data->row_ptr[pt_ID+1] - state->data->row_ptr[pt_ID];      /* size of sparse vector of data point */

  double change_obj;
  double * new_cent1;

  new_cent1 = malloc( state->dim * sizeof(*new_cent1) );

  if (state->clusters[pt_ID] != -1) {           /* Only calculate change if point is leaving a valid cluster */
    kc_vec_add_den_sp( state->centroids + (state->clusters[pt_ID] * state->dim), state->dim, 1,
        state->data->row_ind + state->data->row_ptr[pt_ID], state->data->val + state->data->row_ptr[pt_ID], sparse_size, -1, new_cent1);

    change_obj = sqrt( kc_dot_prod( new_cent1, new_cent1, state->dim ) ) -
      sqrt( kc_dot_prod( state->centroids + (state->clusters[pt_ID] * state->dim), state->centroids + (state->clusters[pt_ID] * state->dim), state->dim ) );
  }
  else {
    change_obj = 0;
  }

  double best_change = 0;
  int curr_best_cluster = state->clusters[pt_ID];
  for (int i=0; i<state->num_clusters; i++) {
    if (i != state->clusters[pt_ID]) {
      double dot_prod = kc_dot_prod_den_sp( state->centroids + i*state->dim, state->data->row_ind + state->data->row_ptr[pt_ID],
          state->data->val + state->data->row_ptr[pt_ID], state->data->row_ptr[pt_ID+1] - state->data->row_ptr[pt_ID] );

      double total_change_obj = change_obj + sqrt(state->centroid_norms[i] + 2 * dot_prod + state->data_norms[pt_ID]) - sqrt(state->centroid_norms[i]);

      if ((total_change_obj > best_change) | (curr_best_cluster == -1) ) {
        curr_best_cluster = i;
        best_change = total_change_obj;
      }
    }
  }

  /* Update values */
  if (curr_best_cluster != state->clusters[pt_ID]) {
    if ( state->clusters[pt_ID] != -1) {           /* Only remove point from old cluster if previously assigned to a cluster */
      memcpy(state->centroids + state->clusters[pt_ID] * state->dim, new_cent1, state->dim * sizeof(*state->centroids));
      state->cluster_sizes[ state->clusters[pt_ID] ]--;
      kc_centroid_norm(state, state->clusters[pt_ID]);
    }
    state->clusters[pt_ID] = curr_best_cluster;
    kc_vec_add_den_sp( state->centroids + curr_best_cluster*state->dim, state->dim, 1, state->data->row_ind + state->data->row_ptr[pt_ID],
        state->data->val + state->data->row_ptr[pt_ID], state->data->row_ptr[pt_ID+1] - state->data->row_ptr[pt_ID], 1,
        state->centroids + curr_best_cluster*state->dim);
    kc_centroid_norm(state, curr_best_cluster);
    state->cluster_sizes[curr_best_cluster]++;

    state->updates++;
  }

  free(new_cent1);
}

/*
 * @brief Update clusters using E1 criterion
 *
 * @param pt_ID ID of point to update
 * @param state State structure for clustering
 */
void kc_update_e1(
    int pt_ID,
    kc_state * state)
{
  /* Calculate change in E1 from moving pt out of current cluster */
  int sparse_size = state->data->row_ptr[pt_ID+1] - state->data->row_ptr[pt_ID];       /* size of sparse vector of data point */

  double norm, change_obj;
  double * new_cent1;

  new_cent1 = malloc( state->dim * sizeof(*new_cent1) );

  if (state->clusters[pt_ID] != -1) {   /* Only calculate change if point is leaving a valid cluster */
    kc_vec_add_den_sp( state->centroids + (state->clusters[pt_ID] * state->dim), state->dim, 1,
        state->data->row_ind + state->data->row_ptr[pt_ID], state->data->val + state->data->row_ptr[pt_ID], sparse_size, -1, new_cent1);

    norm = sqrt( kc_dot_prod( state->centroids + (state->clusters[pt_ID] * state->dim), state->centroids + (state->clusters[pt_ID] * state->dim), state->dim) );

    change_obj = (state->cluster_sizes[ state->clusters[pt_ID] ] - 1) * kc_dot_prod( new_cent1, state->global_centroid, state->dim ) / ( sqrt( kc_dot_prod( new_cent1, new_cent1, state->dim ) ) ) -
      state->cluster_sizes[ state->clusters[pt_ID] ] * kc_dot_prod( state->centroids + (state->clusters[pt_ID] * state->dim), state->global_centroid, state->dim ) / norm;
  }
  else {
    change_obj = 0;
  }

  double best_change = 0;
  int curr_best_cluster = state->clusters[pt_ID];
  for (int i=0; i<state->num_clusters; i++) {
    if (i != state->clusters[pt_ID]) {
      double dot_prod = kc_dot_prod_den_sp( state->centroids + i*state->dim, state->data->row_ind + state->data->row_ptr[pt_ID],
          state->data->val + state->data->row_ptr[pt_ID], sparse_size );

      double total_change_obj = change_obj + (state->cluster_sizes[i]+1) * (state->centroid_gc_dot[i] + state->data_gc_dot[pt_ID]) / sqrt( state->centroid_norms[i] + 2 * dot_prod + state->data_norms[pt_ID] )
        - state->cluster_sizes[i] * state->centroid_gc_dot[i] / sqrt( state->centroid_norms[i] );

      if ((total_change_obj < best_change) | (curr_best_cluster == -1) ) {
        curr_best_cluster = i;
        best_change = total_change_obj;
      }
    }
  }

  /* Update values */
  if (curr_best_cluster != state->clusters[pt_ID]) {
    if (state->clusters[pt_ID] != -1) {                /* Only remove from old cluster if previously assigned to a cluster */
      memcpy(state->centroids + state->clusters[pt_ID] * state->dim, new_cent1, state->dim * sizeof(*state->centroids));
      state->cluster_sizes[ state->clusters[pt_ID] ]--;
      kc_centroid_norm(state, state->clusters[pt_ID]);
      kc_centroid_gc_dot(state, state->clusters[pt_ID]);
    }
    state->clusters[pt_ID] = curr_best_cluster;
    kc_vec_add_den_sp( state->centroids + curr_best_cluster*state->dim, state->dim, 1, state->data->row_ind + state->data->row_ptr[pt_ID],
        state->data->val + state->data->row_ptr[pt_ID], state->data->row_ptr[pt_ID+1] - state->data->row_ptr[pt_ID], 1,
        state->centroids + curr_best_cluster*state->dim);
    kc_centroid_norm(state, curr_best_cluster);
    kc_centroid_gc_dot(state, curr_best_cluster);
    state->cluster_sizes[curr_best_cluster]++;

    state->updates++;
  }

  free(new_cent1);
}

/***********************************************
 *
 * Similarity measures
 *
 **********************************************/

/*
 * @brief Compute l_2 distance squared (SSE) between sparse vector and dense vector
 *
 * @param vec1_ind First vector indices of nonzeroes
 * @param vec1_val First vector values
 * @param vec2 Second vector
 * @param nnz Number of nonzeroes in vec1
 *
 * @return Distance
 */
double l2_square(
    int * vec1_ind,
    double * vec1_val,
    double * vec2,
    int nnz)
{
  double sum = 0;

  for (int i=0; i<nnz; i++) {
    sum += (vec1_val[i] - vec2[vec1_ind[i]]) * (vec1_val[i] - vec2[vec1_ind[i]]);
  }

  return sum;
}

/*
 * @brief Compute cosine similarity (assuming unit vectors) between sparse and dense vector
 *
 * @param vec1_ind First vector indices of nonzeroes
 * @param vec1_val First vector values
 * @param vec2 Second vector
 * @param nnz Number of nonzeroes in vec1
 *
 * @return Cosine similarity
 */
double cos_sim(
    int * vec1_ind,
    double * vec1_val,
    double * vec2,
    int nnz)
{
  double cos = 0;

  for (int i=0; i<nnz; i++) {
    cos += vec1_val[i] * vec2[vec1_ind[i]];
  }

  return cos;
}

/***********************************************
 *
 * Objective functions
 *
 **********************************************/

/*
 * @brief Compute SSE objective function
 *
 * @param state State structure for clustering
 *
 * @return SSE
 */
double kc_sse(
    kc_state * state)
{
  double sum = 0;

  for (int i=0; i<state->data->num_rows; i++) {
    sum += l2_square( state->data->row_ind + state->data->row_ptr[i], state->data->val + state->data->row_ptr[i],
        state->centroids + (state->clusters[i] * state->dim), state->data->row_ptr[i+1] - state->data->row_ptr[i] );
  }

  return sum;
}

/*
 * @brief Compute I2 objective function
 *
 * @param state State structure for clustering
 *
 * @return I2
 */
double kc_i2(
    kc_state * state)
{
  double sum = 0;

  for (int i=0; i<state->num_clusters; i++) {
    sum += sqrt( kc_dot_prod( state->centroids + i * state->dim, state->centroids + i * state->dim, state->dim ) );
  }

  return sum;
}

/*
 * @brief Compute E1 objective function
 *
 * @param state State structure for clustering
 *
 * @return E1
 */
double kc_e1(
    kc_state * state)
{
  double sum = 0;

  for (int i=0; i<state->num_clusters; i++) {
    double norm1 = sqrt( kc_dot_prod( state->centroids + i *state->dim, state->centroids + i * state->dim, state->dim ) );
    double norm2 = sqrt( kc_dot_prod( state->global_centroid, state->global_centroid, state->dim) );
    sum += state->cluster_sizes[i] * kc_dot_prod( state->centroids + i * state->dim, state->global_centroid, state->dim ) / ( norm1 * norm2 );
  }

  return sum;
}

/***********************************************
 *
 * Code
 *
 **********************************************/

/*
 * @brief Allocate space for csr array
 *
 * @param nrows Number of rows
 * @param nnz Number of nonzeroes
 *
 * @return Allocated csr array
 */
kc_csr * kc_csr_alloc(
    int num_rows,
    int nnz)
{
  kc_csr * arr = malloc(sizeof(*arr));

  arr->num_rows = num_rows;
  arr->nnz = nnz;

  arr->row_ptr = malloc( (num_rows+1) * sizeof(*arr->row_ptr));
  arr->row_ind = malloc( nnz * sizeof(*arr->row_ind));
  arr->val = malloc( nnz * sizeof(*arr->val));

  return arr;
}

/*
 * @brief Free space for csr array
 *
 * @param arr Array to free
 */
void kc_csr_free(
    kc_csr * arr)
{
  free(arr->row_ptr);
  free(arr->row_ind);
  free(arr->val);
  free(arr);
}

/*
 * @brief Allocate space for csr array of characters
 *
 * @param nrows Number of rows
 * @param nnz Number of nonzeroes
 *
 * @return Allocated csr array
 */
kc_csr_char * kc_csr_char_alloc(
    int num_rows,
    int nnz)
{
  kc_csr_char * arr = malloc(sizeof(*arr));

  /* Setting these to 0 so array can be filled from beginning.
   * A dynamic structure should be used instead */
  arr->num_rows = 0;
  arr->nnz = 0;

  arr->row_ptr = malloc( (num_rows+1) * sizeof(*arr->row_ptr));
  arr->val = malloc( nnz * sizeof(*arr->val));

  arr->row_ptr[0] = 0;

  return arr;
}

/*
 * @brief Free space for csr array
 *
 * @param arr Array to free
 */
void kc_csr_char_free(
    kc_csr_char * arr)
{
  free(arr->row_ptr);
  free(arr->val);
  free(arr);
}

/*
 * @brief Compute global centroid
 *
 * @param state State structure for clustering
 */
void kc_set_global_cent(
    kc_state * state)
{
  for (int i=0; i<state->data->nnz; i++) {
    state->global_centroid[ state->data->row_ind[i] ] += state->data->val[i];
  }
}

/*
 * @brief Store the dot product between data points and the global centroid
 *        This function avoids recalculation of dot products (data and global centroid do not change)
 *
 * @param state State structure for clustering
 */
void kc_data_global_cent_dot_prod(
    kc_state * state)
{
  for (int i=0; i<state->data->num_rows; i++) {
    state->data_gc_dot[i] = kc_dot_prod_den_sp( state->global_centroid, state->data->row_ind + state->data->row_ptr[i],
        state->data->val + state->data->row_ptr[i], state->data->row_ptr[i+1] - state->data->row_ptr[i] );
  }
}

/*
 * @brief Set state parameters after file with data has been loaded
 *
 * @param state State structure
 */
void kc_state_set(
    kc_state * state)
{
  state->dim = state->data->num_cols;

  state->clusters = malloc( state->data->num_rows * sizeof(state->clusters));
  state->opt_clusters = malloc( state->data->num_rows * sizeof(state->clusters));
  state->data_norms = malloc( state->data->num_rows * sizeof(state->data_norms) );
  state->data_gc_dot = malloc( state->data->num_rows * sizeof(state->data_gc_dot) );
  state->centroids = calloc( state->num_clusters * state->dim, sizeof(*state->centroids) );
  state->centroid_norms = malloc( state->num_clusters * sizeof(*state->centroid_norms) );
  state->centroid_gc_dot = malloc( state->num_clusters * sizeof(*state->centroid_gc_dot) );
  state->cluster_sizes = calloc( state->num_clusters, sizeof(*state->cluster_sizes) );
  state->opt_cluster_sizes = calloc( state->num_clusters, sizeof(*state->opt_cluster_sizes) );
  state->updates = 0;
  state->opt_obj = -1;
  state->iter = 0;

  state->global_centroid = calloc( state->dim, sizeof(*state->global_centroid) );
  kc_set_global_cent(state);
  kc_data_global_cent_dot_prod(state);
}

/*
 * @brief Prepare state parameters for next clustering trial
 *
 * @param state State structure for clustering
 */
void kc_state_reset(
    kc_state * state)
{
  for (int i=0; i<state->data->num_rows; i++) {
    state->clusters[i] = -1;
  }

  for (int i=0; i<state->num_clusters * state->dim; i++) {
    state->centroids[i] = 0;
  }

  for (int i=0; i<state->num_clusters; i++) {
    state->cluster_sizes[i] = 0;
    state->centroid_norms[i] = 0;
  }

  state->updates = 0;
  state->iter = 0;
}

/*
 * @brief Free memory for clustering state structure
 *
 * @param state State structure to free
 */
void kc_state_free(
    kc_state * state)
{
  kc_csr_free(state->data);
  free(state->clusters);
  free(state->opt_clusters);
  free(state->data_norms);
  free(state->centroids);
  free(state->centroid_norms);
  free(state->global_centroid);
  free(state->cluster_sizes);
  free(state->opt_cluster_sizes);
  free(state);
}

/*
 * @brief Choose random initial centroids from data points
 *
 * @param state State structure of clustering
 */
void kc_random_init_cents(
    kc_state * state)
{
  int * centroid_IDs = malloc( state->num_clusters * sizeof(*centroid_IDs) );

  /* Choose IDs of points to use as initial centroids */
  int init_failed;
  do {
    init_failed = 0;
    for (int i=0; i<state->num_clusters; i++) {
      centroid_IDs[i] = rand() % state->data->num_rows;
      for (int j=0; j<i; j++) {       /* Make sure all initial centroids are unique */
        if (centroid_IDs[j] == centroid_IDs[i]) {
          //printf("Initial cluster centroids not unique. Reattempting initialization\n");
          init_failed = 1;
          break;
        }
      }
    }
  } while(init_failed);

  /* Assign centroids */
  for (int i=0; i<state->num_clusters; i++) {
    for (int j=state->data->row_ptr[centroid_IDs[i]]; j<state->data->row_ptr[centroid_IDs[i]+1]; j++) {
      state->centroids[ state->data->row_ind[j] + i * state->dim ] = state->data->val[j];
    }
    /* Set point defining centroid as only point originally in cluster */
    state->clusters[ centroid_IDs[i] ] = i;
    state->cluster_sizes[i] = 1;
    state->centroid_norms[i] = state->data_norms[ centroid_IDs[i] ];
    state->centroid_gc_dot[i] = state->data_gc_dot[ centroid_IDs[i] ];
  }

  free(centroid_IDs);

}

/*
 * @brief Perform clustering
 *
 * @param state State structure of clustering
 */
void kc_single_clustering(
    kc_state * state)
{

  do {
    state->updates = 0;
    for (int i=0; i<state->data->num_rows; i++) {
      (*state->update_func)(i, state);
    }
    state->iter++;

    //printf( "Objective: %0.04f\n", (*state->obj_func)(state) );

  } while ((state->updates >= 0.1 * state->data->num_rows) & (state->iter < 30));

  //printf("Iterations: %d\n", state->iter);

}

/*
 * @brief Perform clustering over several trials
 *
 * @param state State structure of clustering
 */
void kc_kcluster(
    kc_state * state)
{
  for (int i=0; i<state->num_trials; i++) {
    srand(2*i + 1);      /* Set random number generator seed */

    kc_state_reset(state);
    kc_random_init_cents(state);

    kc_single_clustering(state);

    double obj = (*state->obj_func)(state);

    if ( state->opt == 0 ) {   /* Objective function is being minimized */
      if ((obj < state->opt_obj) | (state->opt_obj < 0)) {
        state->opt_obj = obj;

        memcpy(state->opt_clusters, state->clusters, state->data->num_rows * sizeof(*state->opt_clusters));
        memcpy(state->opt_cluster_sizes, state->cluster_sizes, state->num_clusters * sizeof(*state->opt_cluster_sizes));
      }
    }
    else if (state->opt == 1 ) {     /* Objective function is being maximized */
      if ((obj > state->opt_obj) | (state->opt_obj < 0)) {
        state->opt_obj = obj;

        memcpy(state->opt_clusters, state->clusters, state->data->num_rows * sizeof(*state->opt_clusters));
        memcpy(state->opt_cluster_sizes, state->cluster_sizes, state->num_clusters * sizeof(*state->opt_cluster_sizes));
      }
    }

    //printf("Trial %d: Objective function: %0.04f\n", i, obj);
  }
}

/* @brief Fill confusion matrix from clustering scaled by cluster size
 *
 * @param state State structure for clustering
 * @param labels Array of true labels
 * @param num_labels Number of unique labels
 * @param conf_mat Pointer to confusion matrix
 */
void kc_fill_scaled_conf_mat(
    kc_state * state,
    int * labels,
    int num_labels,
    double * conf_mat)
{
  /* Fill confusion matrix
   * rows = clusters
   * columns = true labels */
  for (int i=0; i<state->data->num_rows; i++) {
    conf_mat[ labels[i] + state->opt_clusters[i] * num_labels ]++;
  }

  for (int i=0; i<state->num_clusters; i++) {
    for (int j=0; j<num_labels; j++) {
      conf_mat[ j + i * num_labels ] = conf_mat[ j + i * num_labels ] / state->opt_cluster_sizes[i];
    }
  }
}

/*
 * @brief Output cluster validity measures
 *
 * @param state State structure for clustering
 * @param labels Array of true labels
 * @param num_labels Number of unique labels
 */
void kc_output_results(
    kc_state * state,
    int * labels,
    int num_labels)
{
  double * conf_mat = calloc(state->num_clusters * num_labels, sizeof(*conf_mat) );
  kc_fill_scaled_conf_mat(state, labels, num_labels, conf_mat);

  double entropy = 0;
  double purity = 0;

  for (int i=0; i<state->num_clusters; i++) {
    double cluster_entropy = 0;
    for (int j=0; j<num_labels; j++) {
      if (conf_mat[ j + i * num_labels ] != 0) {
        cluster_entropy -= conf_mat[ j + i * num_labels ] * log2( conf_mat[ j + i * num_labels ] );
      }
    }
    entropy += cluster_entropy * state->opt_cluster_sizes[i] / state->data->num_rows;     /* Add scaled entropy for cluster */
  }

  for (int i=0; i<state->num_clusters; i++) {
    double cluster_purity = 0;
    for (int j=0; j<num_labels; j++) {
      if (conf_mat[ j + i * num_labels ] > cluster_purity) {
        cluster_purity = conf_mat[ j + i * num_labels ];
      }
    }
    purity += cluster_purity * state->opt_cluster_sizes[i] / state->data->num_rows;       /* Add scaled purity for cluster */
  }

  free(conf_mat);

  printf( "Best objective function: %0.04f\n", state->opt_obj );
  printf( "Best entropy: %0.04f\n", entropy );
  printf( "Best purity: %0.04f\n", purity );
}

/*
 * @brief Read input data from file
 *
 * @param fname Name of file to read
 *
 * @return Input data in csr array
 */
kc_csr * kc_read_ifile(
    char const * const fname,
    int ** row_IDs)
{
  /* Open file */
  FILE * fin;
  if((fin = fopen(fname, "r")) == NULL) {
    //fprintf(stderr, "unable to open '%s' for reading.\n", fname);
    exit(EXIT_FAILURE);
  }

  int nrows = 0;
  int nnz = 0;

  int prev_row_id = -1;    /* Use to keep track of previous row ID */

  char * line = malloc(1024 * 1024);
  size_t len = 0;
  ssize_t read = getline(&line, &len, fin);

  while (read >= 0) {

    char * ptr = strtok(line, ", ");
    char * end = NULL;

    int row_id = strtoull(ptr, &end, 10);
    if (row_id != prev_row_id) {
      prev_row_id = row_id;
      nrows++;
    }

    nnz++;

    read = getline(&line, &len, fin);
  }

  fclose(fin);

  kc_csr * csr = kc_csr_alloc(nrows, nnz);

  *row_IDs = malloc(nrows * sizeof(**row_IDs));     /* Array to store row IDs present in input file */

  fin = fopen(fname, "r");

  read = getline(&line, &len, fin);

  int row_count = 0;
  int nz_count = 0;

  int num_cols = 0;

  prev_row_id = -1;

  while (read >= 0) {

    char * ptr = strtok(line, ", ");
    char * end = NULL;

    int row_id = strtoull(ptr, &end, 10);
    if (row_id != prev_row_id) {
      if (prev_row_id != -1) {
        (*row_IDs)[row_count++] = prev_row_id;
      }
      csr->row_ptr[row_count] = nz_count;
    }
    prev_row_id = row_id;

    ptr = strtok(NULL, ", ");
    end = NULL;
    int col_num = strtoull(ptr, &end, 10);
    if (col_num + 1 > num_cols) {
      num_cols = col_num+1;
    }
    csr->row_ind[nz_count] = col_num;

    ptr = strtok(NULL, ", ");
    end = NULL;
    double val = atof(ptr);
    csr->val[nz_count++] = val;

    read = getline(&line, &len, fin);
  }

  csr->num_cols = num_cols;
  (*row_IDs)[row_count++] = prev_row_id;
  csr->row_ptr[row_count] = nz_count;

  fclose(fin);

  free(line);

  return csr;
}

/*
 * @brief Read file of topic labels from file and convert to numeric values
 *
 * @param fname Name of file to read
 * @param nrows Number of labels in file
 * @param labels Array to hold numeric value for cluster labels
 *
 * @return Array of unique labels as strings
 */
kc_csr_char * kc_read_classfile(
    char const * const fname,
    int nrows,
    int * labels)
{
  /* Create large csr to store label strings (should use dynamic structure instead of allocating large chunk of memory, but not worth the effort) */
  kc_csr_char * label_str = kc_csr_char_alloc(1000, 10000);

  FILE * fclass;
  if((fclass = fopen(fname, "r")) == NULL) {
    fprintf(stderr, "unable to open '%s' for reading.\n", fname);
    exit(EXIT_FAILURE);
  }

  char * line = malloc(1024 * 1024);
  size_t len = 0;
  ssize_t read = getline(&line, &len, fclass);

  int ID = 0;    /* ID of current article */

  while (read >= 0) {

    int new_str = 1;

    char * ptr = strtok(line, ", ");

    ptr = strtok(NULL, "\n");

    for (int i=0; i<label_str->num_rows; i++) {
      if (!strcmp(label_str->val + label_str->row_ptr[i], ptr)) {    /* String has already been seen */
        new_str = 0;
        labels[ID] = i;
      }
    }

    if (new_str) {      /* String has not been seen before */
      labels[ID] = label_str->num_rows;
      memcpy( label_str->val + label_str->row_ptr[ label_str->num_rows ], ptr, strlen(ptr) + 1 );
      label_str->row_ptr[ label_str->num_rows+1 ] = label_str->row_ptr[ label_str->num_rows ] + strlen(ptr) + 1;
      label_str->num_rows++;
      label_str->nnz += strlen(ptr);
    }

    read = getline(&line, &len, fclass);
    ID++;
  }

  free(line);

  return label_str;

}

/*
 * @brief Write output clusters to file
 *
 * @param fname Name of file to write to
 * @param clusters Array of cluster assignments
 * @param IDs Array of item IDs
 * @param num_rows Number of rows
 */
void kc_write_clusters_file(
    char * fname,
    int * clusters,
    int * IDs,
    int num_rows)
{
  FILE * fout = fopen(fname, "w");

  for (int i=0; i<num_rows; i++) {
    fprintf(fout, "%d, %d\n", IDs[i], clusters[i]);
  }

  fclose(fout);
}

int main(
    int argc,
    char **argv)
{
  char * ifname = argv[1];
  char * crit_func = argv[2];
  char * class_fname = argv[3];
  int num_clusters = atoi(argv[4]);
  int num_trials = atoi(argv[5]);
  char * ofname = argv[6];

  kc_capitalize(crit_func, strlen(crit_func));

  kc_state * state = malloc(sizeof(*state));
  state->num_clusters = num_clusters;
  state->num_trials = num_trials;

  /* Assign criterion function and update function */
  if ( !strcmp(crit_func, "SSE") ) {
    state->update_func = &kc_update_sse;
    state->obj_func = &kc_sse;
    state->opt = 0;
  }
  else if ( !strcmp(crit_func, "I2") ) {
    state->update_func = &kc_update_i2;
    state->obj_func = &kc_i2;
    state->opt = 1;
  }
  else if ( !strcmp(crit_func, "E1") ) {
    state->update_func = &kc_update_e1;
    state->obj_func = &kc_e1;
    state->opt = 0;
  }
  else {
    printf("Invalid criterion function: %s\n", crit_func);
  }

  int * article_IDs;

  state->data = kc_read_ifile(ifname, &article_IDs);

  kc_state_set(state);

  kc_data_norms(state);

  int * class = malloc( state->data->num_rows * sizeof(*class) );
  kc_csr_char * class_str = kc_read_classfile(class_fname, state->data->num_rows, class);

  double start = monotonic_seconds();
  kc_kcluster(state);
  total_time += monotonic_seconds() - start;

  kc_output_results( state, class, class_str->num_rows );

  kc_write_clusters_file( ofname, state->clusters, article_IDs, state->data->num_rows );

  printf( "Clustering time: %0.04f\n", total_time );
  //printf( "Dot product time: %0.04f\n", dot_prod_time );
  //printf( "Dense vector addition time: %0.04f\n", add_dense_time );
  //printf( "Sparse dense vector addition time: %0.04f\n", add_sparse_dense_time );

  free(article_IDs);
  free(class);
  kc_csr_char_free(class_str);
  kc_state_free(state);
}
