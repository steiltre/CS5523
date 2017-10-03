
/**
 * @brief Digit image structure
 */
typedef struct
{
  int img_hgt;
  int img_wid;
  int num_imgs;
  int * dgts;
  double * img_arr;
} dgt_img_arr;

/**
 * @brief Create an array of digit images
 *
 * @param img_hgt The number of rows in the image
 * @param img_wid The number of columns in the image
 *
 * @return The allocated digit array
 */
dgt_img_arr * dgt_img_alloc(
    int const img_hgt,
    int const img_wid,
    int const num_imgs);

void dgt_img_free(
    dgt_img_arr * dgt_arr);

/**
 * @brief Count the number of digits whose closest neighbor is the same digit
 *
 * @param arr Array of images and digits
 * @param distance_function Pointer to function for calculating distances
 *
 * @return matches Number of images whose closest neighbor is same digit
 */
int count_closest_digit_matches(
    dgt_img_arr * arr,
    double (*distance_function)(int, double *, double *),
    int (*comp_op)(double, double));

/**
 * @brief Find the closest digit to a given image in array of images
 *
 * @param arr Array of images and digit labels
 * @param index Index of selected image in arr
 * @param distance_function Pointer to function for calculating distances
 *
 * @return closest_digit Label of nearest digit image
 */
int closest_digit(
    dgt_img_arr * arr,
    int index,
    double (*distance_function)(int, double *, double *),
    int (*comp_op)(double, double));
