#include <stdlib.h>
#include <stdio.h>

void * dgt_malloc(
    size_t const bytes)
{
  void * ptr;
  int success = posix_memalign(&ptr, 64, bytes);
  if(success != 0) {
    fprintf(stderr, "ERROR: posix_memalign() returned %d.\n", success);
    exit(1);
  }
  return ptr;
}

void dgt_free(
    void * ptr)
{
  free(ptr);
}

int less_than(
    double a,
    double b)
{
  return (a<b);
}

int greater_than(
    double a,
    double b)
{
  return (a>b);
}
