#' Set the number of threads to use by default
#' @param n integer of number of threads to use
#' @example
#' setESAAvoidableAttThreads(16)
#' @export
setESAAvoidableAttThreads <- function(n){
  # get max threads
  maxThreads <- cpp_get_max_threads();
  # parse n
  if (n == 0){
    n = max(maxThreads,TRUE)
  } else if (n<1){
    n = max(ceiling(maxThreads*n),TRUE)
  } else if (n>1){
    if (maxThreads==0){
      warning("OpenMP not detected. operating in single threaded mode.")
      n = 1
    } else if (n>maxThreads){
      warning(paste0("Threads exceeded maximum threads. Using ",maxThreads," instead."))
      n = maxThreads
    }
  }
  options("ESAAvoidableAtt_nthreads"=n)
}

#' Get the number of threads set in options
#' @example
#' getESAAvoidableAttThreads()
#' @export
getESAAvoidableAttThreads <- function(){
  return(getOption("ESAAvoidableAtt_nthreads"))
}
