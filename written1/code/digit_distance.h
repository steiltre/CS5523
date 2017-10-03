
/**
 * @brief Compute the Euclidean distance between two vectors
 *
 * @param dim Number of dimensions
 * @param vec1 First vector
 * @param vec2 Second vector
 *
 * @return dist Distance between vectors
 */
double Euclidean_dist(
    int dim,
    double * vec1,
    double * vec2);

/**
 * @brief Compute the cosine similarity between two vectors
 *
 * @param dim Number of dimensions
 * @param vec1 First vector
 * @param vec2 Second vector
 *
 * @return sim Similariy between vectors
 */
double cosine_sim(
    int dim,
    double * vec1,
    double * vec2);

/**
 * @brief Compute the Jaccard similarity between two vectors
 *
 * @param dim Number of dimensions
 * @param vec1 First vector
 * @param vec2 Second vector
 *
 * @return sim Similarity between vectors
 */
double Jaccard_sim(
    int dim,
    double * vec1,
    double * vec2);
