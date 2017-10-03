
/**
 * @brief A wrapper around `posix_memalign()` to get aligned memory.
 *
 * @param bytes How many bytes to allocate
 *
 * @return Allocated memory.
 */
void * dgt_malloc(
    size_t const bytes);

/**
 * @brief Free memory allocated by `digit_malloc()`
 *
 * @param ptr The pointer to free.
 */
void dgt_free(
    void * ptr);

/**
 * @brief Less than operator for passing to functions
 *
 * @param a First number to compare
 * @param b Second number to compare
 *
 * @return Integer indicating relation
 */
int less_than(
    double a,
    double b);

/**
 * @brief Less than operator for passing to functions
 *
 * @param a First number to compare
 * @param b Second number to compare
 *
 * @return Integer indicating relation
 */
int greater_than(
    double a,
    double b);
