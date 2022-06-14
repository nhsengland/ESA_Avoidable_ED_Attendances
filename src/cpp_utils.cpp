#include <Rcpp.h>
#ifdef _OPENMP
  #include <omp.h>
  #include <pthread.h>
#else
  #define omp_get_max_threads() 0
#endif

using namespace Rcpp;

// [[Rcpp::plugins(openmp)]]

// wrapper around omp function and export for use in R code.
// [[Rcpp::export]]
int cpp_get_max_threads(){
  return omp_get_max_threads();
}
