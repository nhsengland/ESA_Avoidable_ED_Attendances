#' Derive avoidable ED attendances as per Uni of Sheffield ScHARR avoidable attendances metric
#'
#' This returns a data.frame with logical vector (TRUE/FALSE) corresponding to
#' each of the input rows, stating whether the (patient level) attendance is
#' 'avoidable' as per the University of Sheffield's School of Health and Related
#' Research (ScHARR)'s avoidable attendance metric. [insert reference here].
#' The code will derive this for either SNOMED or HES codes, and is based upon
#' codes published by NHS Digital.
#'
#' @param data data.frame/data.table: attendance-level data.frame containing ED data
#' @param colTypeED string: column name of the type of ED attendance
#' @param colAttDischarge string: column name of the attendance discharge/disposal
#' @param colAttCategory string: column name of the  attendance category
#' @param colArrivalMode string: column name of the arrival mode
#' @param colsInvestigations character vector: column names of all the ED investigations
#' @param colsTreatments character vector: column names of all the ED treatments
#' @param clinicalCodeStandard string; clinical coding standard to use - either snomed or hes
#' @param nthreads integer; number of threads to use
#' @return logical vector containing binary TRUE/FALSE whether each row is an avoidable attendance
#' @examples
#' calculateAvoidableEDAtt(dt,'Department_Type','Discharge_Status','AttendanceCategory','Arrival_Mode',paste0('Investigation_',1:30),paste0('Treatment_',1:30),'snomed',30)
#' calculateAvoidableEDAtt(dt,'Department_Type','Discharge_Status','AttendanceCategory','Arrival_Mode',paste0('Investigation_',1:30),paste0('Treatment_',1:30),'hes',6)
calculateAvoidableEDAtt <- function(data,colTypeED,colAttDischarge,
                                    colAttCategory,colArrivalMode,colsInvestigations,
                                    colsTreatments,clinicalCodeStandard='snomed',
                                    nthreads=getOption("ESAAvoidableAtt_nthreads")){
  # get number of threads set as option since it checks whether its feasible amount
  if (!is.null(nthreads)){
    setESAAvoidableAttThreads(nthreads)
  }
  nThr <- getOption("ESAAvoidableAtt_nthreads")
  if (is.null(nThr)) {
    nThr <- cpp_get_max_threads()/2
  }
  # call c++ function with same arguments
  output <- cpp_derive_avoidable_att(df=data,colTypeED=colTypeED,
                                     colAttDischarge=colAttDischarge,colAttCategory=colAttCategory,
                                     colArrivalMode=colArrivalMode,
                                     colsInvestigations=colsInvestigations,
                                     colsTreatments=colsTreatments,
                                     clinicalCodeStandard=clinicalCodeStandard,
                                     nthreads=nThr);
  return(output)
}
