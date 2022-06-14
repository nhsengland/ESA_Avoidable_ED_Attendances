.onLoad <- function(libname,pkgname){
  if (is.null(getOption('ESAAvoidableAtt_nthreads'))){
    maxThreads <- cpp_get_max_threads();
    if (maxThreads==0){
      options("ESAAvoidableAtt_nthreads"=1)
    } else {
      options("ESAAvoidableAtt_nthreads"=maxThreads/2)
    }
  }
}
.onAttach=function(libname,pkgname){
  message(paste0("Using ",getOption("ESAAvoidableAtt_nthreads")," threads. You can change this using setESAAvoidableAttThreads(n)."))
}
