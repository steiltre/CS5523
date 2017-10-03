
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "digit_utility.h"
#include "digit.h"
#include "digit_distance.h"

dgt_img_arr * dgt_img_alloc(
    int const img_hgt,
    int const img_wid,
    int const num_imgs)
{
  dgt_img_arr * dgt_arr = dgt_malloc(sizeof(*dgt_arr));

  dgt_arr->img_hgt = img_hgt;
  dgt_arr->img_wid = img_wid;
  dgt_arr->num_imgs = num_imgs;
  dgt_arr->dgts = dgt_malloc( num_imgs * sizeof(*dgt_arr->dgts) );
  dgt_arr->img_arr = dgt_malloc( num_imgs * img_hgt * img_wid * sizeof(*dgt_arr->img_arr) );

  return dgt_arr;
}

void dgt_img_free(
    dgt_img_arr * dgt_arr)
{
  dgt_free( dgt_arr->dgts );
  dgt_free( dgt_arr->img_arr );
}

dgt_img_arr * read_images(
    char const * const fname,
    int img_hgt,
    int img_wid,
    int num_imgs)
{
  /* Open file */
  FILE * fin;
  if((fin = fopen(fname, "r")) == NULL) {
    fprintf(stderr, "Unable to open '%s' for reading.\n", fname);
    exit(EXIT_FAILURE);
  }

  dgt_img_arr * arr = dgt_img_alloc( img_hgt, img_wid, num_imgs );

  char * line = malloc(1024 * 1024);
  size_t len = 0;

  /* Read in array one image at a time. */
  for(int img=0; img < arr->num_imgs; ++img) {
    ssize_t read = getline(&line, &len, fin);
    if(read == -1) {
      fprintf(stderr, "ERROR: premature EOF at line %d\n", img+1);
      dgt_img_free(arr);
      return NULL;
    }

    char * ptr = strtok(line, ",");
    char * end = NULL;
    int const digit = strtoull(ptr, &end, 10);
    arr->dgts[img] = digit;
    ptr = strtok(NULL, ",");

    for(int j=0; j < arr->img_hgt * arr->img_wid; ++j) {
      char * end = NULL;
      double const gray_val = strtod(ptr, &end);
      /* end of line */
      if(ptr == end) {
        break;
      }
      //assert(gray_val >= 0 && gray_val <= 255);

      arr->img_arr[img * arr->img_hgt * arr->img_wid + j] = gray_val;
      ptr = strtok(NULL, ",");
    }
  }

  free(line);

  fclose(fin);

  return arr;
}

int count_closest_digit_matches(
    dgt_img_arr * arr,
    double (*distance_function)(int, double *, double *),
    int (*comp_op)(double, double))
{
  int matches = 0;

  for(int i=0; i<arr->num_imgs; ++i) {
    int closest_dgt = closest_digit(arr, i, distance_function, comp_op);

    if(closest_dgt == arr->dgts[i]) {
      matches += 1;
    }
  }

  return matches;
}

int closest_digit(
    dgt_img_arr * arr,
    int index,
    double (*distance_function)(int, double *, double *),
    int (*comp_op)(double, double))
{

  double optimal_dist;
  int closest_dgt;

  if(index==0) {
    optimal_dist = distance_function(arr->img_hgt * arr->img_wid, &arr->img_arr[0], &arr->img_arr[ arr->img_hgt * arr->img_wid ]);
    closest_dgt = arr->dgts[1];
  }
  else {
    optimal_dist = distance_function(arr->img_hgt * arr->img_wid, &arr->img_arr[index * arr->img_hgt * arr->img_wid], &arr->img_arr[0]);
    closest_dgt = arr->dgts[0];
  }

  for(int i=0; i<arr->num_imgs; ++i) {
    if (i != index) {
      double dist = distance_function(arr->img_hgt * arr->img_wid, &arr->img_arr[index * arr->img_hgt * arr->img_wid], &arr->img_arr[i * arr->img_hgt * arr->img_wid]);

      if (comp_op(dist, optimal_dist)) {
        optimal_dist = dist;
        closest_dgt = arr->dgts[i];
      }
    }
  }

  return closest_dgt;

}

int main(
    int argc,
    char ** argv)
{
  if(argc < 5) {
    printf("Usage: %s <data> <img_hgt> <img_wid> <num_imgs> <proximity_measure>\n", argv[0]);
    return EXIT_FAILURE;
  }

  char * data_fname = argv[1];
  int img_hgt = atoi(argv[2]);
  int img_wid = atoi(argv[3]);
  int num_imgs = atoi(argv[4]);
  int prox_meas_label = atoi(argv[5]);

  double (*prox_meas)(int, double *, double *);
  int (*comp_op)(double, double);
  char * prox_str;
  if (prox_meas_label == 1) {
    prox_meas = Euclidean_dist;
    prox_str = "Euclidean distance";
    comp_op = less_than;
  }
  else if(prox_meas_label == 2) {
    prox_meas = cosine_sim;
    prox_str = "cosine similarity";
    comp_op = greater_than;
  }
  else if(prox_meas_label == 3) {
    prox_meas = Jaccard_sim;
    prox_str = "Jaccard similarity";
    comp_op = greater_than;
  }
  else {
    printf("Invalid proximity identifier: %s\n", argv[5]);
    return EXIT_FAILURE;
  }


  dgt_img_arr * arr = read_images(data_fname, img_hgt, img_wid, num_imgs);

  int num_matches = count_closest_digit_matches(arr, prox_meas, comp_op);

  printf("There are %d closest digit matches in %s using %s\n", num_matches, data_fname, prox_str);

}

