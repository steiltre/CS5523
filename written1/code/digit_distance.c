#include <stdlib.h>
#include <math.h>

#include "digit_distance.h"

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

double Jaccard_sim(
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

