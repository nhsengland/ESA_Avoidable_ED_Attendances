#include <Rcpp.h>
#include <vector>
#include <string>
#include <numeric>
#include <string_view>
#include <optional>
#ifdef _OPENMP
  #include <omp.h>
  #include <parallel/algorithm>
#else
  #define omp_get_thread_num() 0
  #include <algorithm>
#endif
using namespace Rcpp;

// [[Rcpp::plugins(openmp)]]
// [[Rcpp::plugins("cpp17")]]

// function which applies either a | or & binary operator to vector of bools
inline bool cpp_reduce_bools(std::string op, std::vector<bool> vals){
  // parse string (either | or &) and apply appropriate binary operation fn
  bool out = std::accumulate(vals.begin()+1,vals.end(),*vals.begin(),
                             [&op](bool a, bool b){
                               // use or operator
                               if (op=="|"){ return a||b; }
                               // default to using & operator
                               return a&&b;
                             });
  return out;
}
/*  function to check whether string is any of a vector of search terms
 *  pass reference to value in vector
 *  pass reference to SNOMED/HES codes searching for
 */
inline bool cpp_string_in_terms(const std::optional<std::string_view> &str,
                                std::vector<std::string> terms,
                                bool incNAs, bool notStr){
  bool out;
  // check if the optional has a value
  if (str){
    int nTerms = terms.size();
    // create empty vector of bools to the size of terms
    std::vector <bool> rowTerms(nTerms);
    // loop through all of the terms
    for (int j = 0; j < nTerms; j++){
      if (str==terms[j]){
        rowTerms[j] = true;
      } else {
        rowTerms[j] = false;
      }
    }
    // return bool whether string matches any of the search terms
    out = cpp_reduce_bools("|",rowTerms);
    // negate whether the string matches first
    if (notStr){
      out = !out;
    }
  } else {
    // string is an NA value
    out = incNAs ? true : false;
  }
  return out;
}
// function which takes a vector by reference, and appends a vector of strings
inline void cpp_insert_to_vec(std::vector<std::string> &vec,std::vector<std::string> vars){
  // iterate through all of elements in vars
  for (auto itr = vars.begin(); itr != vars.end(); itr++){
    // resizes the reference vector and copies content to end of it
    vec.push_back(*itr);
  }
}
// function to extract second element in an std::pair
template<class T1,class T2>
static void extract_second(const std::vector<std::pair<T1,T2>> &vec,std::vector<T2> &out){
  out.resize(vec.size());
  for (size_t i =0; i < out.size(); i++){
    out[i] = vec[i].second;
  }
}
// struct to store a vector of string_views, which represents a row in dataframe
struct avoidableAttRow {
  std::vector<std::optional<std::string_view>> contents;
};
/*
 * Main function to export; takes in an R data.frame, and character vectors
 * for the key columns required to identify ScHARR avoidable attendances
 */
// [[Rcpp::export]]
std::vector<bool> cpp_derive_avoidable_att(const DataFrame &df,
                                           const std::string &colTypeED,
                                           const std::string &colAttDischarge,
                                           const std::string &colAttCategory,
                                           const std::string &colArrivalMode,
                                           const std::vector<std::string> &colsInvestigations,
                                           const std::vector<std::string> &colsTreatments,
                                           std::string clinicalCodeStandard,
                                           int nthreads){
  /*
   * Initial checks, parsing of clinical code standard
   */
  // check whether clinical code standard is valid argument (snomed or hes)
  // set all characters to lowercase incase someone capitalises
  std::transform(clinicalCodeStandard.begin(), clinicalCodeStandard.end(), clinicalCodeStandard.begin(),
                 [](unsigned char c){ return std::tolower(c); });
  if ((clinicalCodeStandard!="snomed")&&(clinicalCodeStandard!="hes")){
    Rcpp::stop("invalid clinical coding standard provided. must be snomed or hes.");
  }
  // Rcpp will flag if one of the columns is invalid
  // find index of each of the column names
  int idxTypeED = df.findName(colTypeED);
  int idxAttDisch = df.findName(colAttDischarge);
  int idxAttCat = df.findName(colAttCategory);
  int idxArrMode = df.findName(colArrivalMode);
  // investigations and treatments should be vectors, initialise empty vec
  // get the number of treatment and investigation columns
  size_t nInvesCols = colsInvestigations.size();
  size_t nTreatCols = colsTreatments.size();
  // allocate interger vectors to store their column indiexes
  std::vector<int> idxesInves(nInvesCols);
  std::vector<int> idxesTreat(nTreatCols);
  // fill the vectors
  for (size_t i=0;i<nInvesCols;i++){
    idxesInves[i] = df.findName(colsInvestigations[i]);
  }
  for (size_t i=0;i<nTreatCols;i++){
    idxesTreat[i] = df.findName(colsTreatments[i]);
  }
  // check the column types for the above columns are string
  // get list of all relevant indexes
  std::vector<int> allIdxes {idxTypeED,idxAttDisch,idxAttCat,idxArrMode};
  allIdxes.insert(allIdxes.end(),idxesInves.begin(),idxesInves.end());
  allIdxes.insert(allIdxes.end(),idxesTreat.begin(),idxesTreat.end());
  size_t nColIdxs = allIdxes.size();
  // loop through relevant indexes
  for (size_t i=0;i<nColIdxs;i++){
    if (TYPEOF(df[allIdxes[i]])!=STRSXP){
      Rcout << "index: " << allIdxes[i] << std::endl;
      Rcpp::stop("Column type must be string type.");
    }
  }
  R_xlen_t dfNRows = df.nrows();
  // generate integer identifier vector - this is length of number of rows,
  // and a sequence between 1 to nrows;
  std::vector<uint64_t> identCol(dfNRows);
  std::iota(identCol.begin(),identCol.end(),1);
  // create string view and place in a pair with ident
  std::vector<std::pair<uint64_t,avoidableAttRow>> dfViewCpp(dfNRows);
  // not going to get around having to make a copy of the data
  // [unless someone has a better suggestion...]
  // given many of the columns are likely NA values, use std::optional<std::string>
  // iterate through all of the rows
  for (R_xlen_t i=0;i<dfNRows;i++){
    // empty vector of string views of length to indexes
    std::vector<std::optional<std::string_view>> vecStrViewRow(nColIdxs);
    // iterate through the column indexes
    for (size_t j=0;j<nColIdxs;j++){
      // reference to column
      CharacterVector col(df[allIdxes[j]]);
      // optional (string) for contents of CharacterVector
      std::optional<std::string_view> s;
      if (col[i]!=NA_STRING){
        s = std::string_view(col[i]);//Rcpp::as<std::string>(col[i]);
      } else {
        s = std::nullopt;
      }
      // add to vec representing row
      vecStrViewRow[j] = s;
    }
    // reference to identifier
    uint64_t rowIdent = identCol[i];
    avoidableAttRow row;
    row.contents = vecStrViewRow;
    // create the pair
    std::pair<uint64_t,avoidableAttRow> rowPair{rowIdent,row};
    // add to data.frame view vector
    dfViewCpp[i] = rowPair;
  }
  /*
   * Declare some string vectors to store search terms for SNOMED/HES
   * These codes are sourced via NHS Digital...
   */
  std::vector<std::string> termsTypeED,termsAttCat,termsAttDisp,termsArrMode,termsInves,termsTreat;
  // derive bool flags for ED type and att category as same across snomed & hes
  // ED Type - Type 1 ED (01 or 1)
  termsTypeED = std::vector<std::string>{"01","1"};
  // ED attendance category - first attendance (1)
  termsAttCat = std::vector<std::string>{"1","01"};
  // HES and SNOMED codes differ, hence add codes to relevant terms vector
  if (clinicalCodeStandard=="snomed"){
    /*
     * Attendance disposal
     * 1077021000000100 = Discharged - follow-up treatment to be provided by GP
     * 182992009 = Discharged - did not require any follow-up treatment
     * 1066321000000107 = Left department before being treated
     */
    cpp_insert_to_vec(termsAttDisp,
                      std::vector<std::string>{"1077021000000100","182992009",
                                               "1066321000000107"});
    /*
     * Arrival mode - non-ambulance arrivals
     * 1048071000000103,1048061000000105,1047991000000102,1048001000000106
     */
    cpp_insert_to_vec(termsArrMode,
                      std::vector<std::string>{"1048071000000103","1048061000000105",
                                               "1047991000000102","1048001000000106"});
    // as arrival mode differs (a not) between snomed and HES, call search terms fn
    //flagArrMode = cpp_search_terms_in_df(arrivalMode,termsArrMode,false,false,nthreads);
    /*
     * Investigations
     * 27171005 = urinanlysis
     * 167252002, 67900009 = pregnancy test
     * 53115007 = dental investigation
     * 1088291000000101, or blank = None
     */
    cpp_insert_to_vec(termsInves,
                      std::vector<std::string>{"27171005","167252002","67900009",
                                               "53115007","1088291000000101"});
    /*
     * 413334001 - guidance - written
     * 81733005 - dental treatment
     * 266712008 - prescription meds
     * 183964008 or none - none
     */
    cpp_insert_to_vec(termsTreat,
                      std::vector<std::string>{"413334001","81733005","266712008",
                                               "183964008"});
  } else if (clinicalCodeStandard=="hes"){
    /*
     * Attendance disposal
     * 02 - discharged - followup GP; 03 - dicharge no followup; 12 - left before treated
     */
    cpp_insert_to_vec(termsAttDisp,
                      std::vector<std::string>{"02","2","03","3","12"});
    // Arrival mode - not 1; hence search for 1 and negate
    cpp_insert_to_vec(termsArrMode,std::vector<std::string>{"1"});
    // as arrival mode differs (a not) between snomed and HES, call search terms fn
    //flagArrMode = cpp_search_terms_in_df(arrivalMode,termsArrMode,false,true,nthreads);
    /*
     * Investigations
     * 06,6 = urinanlysis, 21 = pregnancy test, 22 = dental investigation
     * 24, or blank = None
     */
    cpp_insert_to_vec(termsInves,std::vector<std::string>{"06","6","21","22","24"});
    /*
     * HES treatments
     * 221 - guidance written,222 - guidance verbal, 30 - record vital signs
     * 56 - dental treat, 57 - prescription meds,99 or blank - none,
     * 07 - prescriptions (depricated)
     */
    cpp_insert_to_vec(termsTreat,std::vector<std::string>{"221","222","30","56","57","99","07","7"});
  }
  /*
   *
   */
  // create vector of pairs containing result
  std::vector<std::pair<uint64_t,bool>> resultOut(dfNRows);
  // parallelerise searching for terms
  #pragma omp parallel for num_threads(nthreads) schedule(static)
  // iterate through each row
  for (R_xlen_t i=0;i<dfNRows;i++){
    // get contents of avoidableAttRow struct in each pair
    std::vector<std::optional<std::string_view>> rowvec = dfViewCpp[i].second.contents;
    // index 0 of row is typeED
    bool flagTypeED = cpp_string_in_terms(rowvec[0],termsTypeED,false,false);
    // index 1 of row.contents is attendance discharge(disposal)
    bool flagAttDisch = cpp_string_in_terms(rowvec[1],termsAttDisp,false,false);
    // index 2 of row.contents is attendance category
    bool flagAttCat = cpp_string_in_terms(rowvec[2],termsAttCat,false,false);
    // index 3 of row.contents is arrival mode
    // this is a special case, handle hes and snomed differently
    bool flagArrMode = false;
    if (clinicalCodeStandard=="snomed"){
      flagArrMode = cpp_string_in_terms(rowvec[3],termsArrMode,false,false);
    } else if (clinicalCodeStandard=="hes"){
      flagArrMode = cpp_string_in_terms(rowvec[3],termsArrMode,false,true);
    }
    // index 4 to 4+length(investigations) are inestigation columns
    std::vector<bool> vecFlagsInves(nInvesCols);
    for (size_t i=0;i<nInvesCols;i++){
      int truIdx = 4+i;
      vecFlagsInves[i] = cpp_string_in_terms(rowvec[truIdx],termsInves,false,true);
    }
    // reduce this to single boolean and negate
    bool flagInves = cpp_reduce_bools("|",vecFlagsInves);
    flagInves = !flagInves;
    // index 4+length(investigations) to end are treatment columns
    std::vector<bool> vecFlagsTreat(nTreatCols);
    for (size_t i=0;i<nTreatCols;i++){
      int truIdx = 4+nInvesCols+i;
      vecFlagsTreat[i] = cpp_string_in_terms(rowvec[truIdx],termsTreat,false,true);
    }
    // reduce to single bool and negate
    bool flagTreat = cpp_reduce_bools("|",vecFlagsTreat);
    flagTreat = !flagTreat;
    // combine and reduce to a single boolean (using and operator)
    bool flagResult = cpp_reduce_bools("&",
                                       std::vector<bool>{flagTypeED,flagAttDisch,flagAttCat,
                                                         flagArrMode,flagInves,flagTreat});
    // create resulting pair
    std::pair<uint64_t,bool> outrow{dfViewCpp[i].first,flagResult};
    resultOut[i] = outrow;
  }
  // if more than one thread...
  if (nthreads>1){
    // since openmp parallelised loop doesnt retain ordering, reorder the data
    // based on identifying integer in pair
    // to investigate parallelerising the sorting...
    // - seems c++17 supports it but via Intel TBB - new macs won't support on M1/M2
    // - libstdc++ with gcc has some in experimential mode requiring OpenMP
    // since alreadying using openMP to parallelise - and as nthreads > 1 by definition
    // means openMP should be available (as R does some checks); we use this
    __gnu_parallel::sort(resultOut.begin(),resultOut.end(),[&](const std::pair<uint64_t,bool> &a,const std::pair<uint64_t,bool> &b){
      // extract uint64_t identifier
      uint64_t numA = a.first, numB = b.first;
      // then order asc
      if (numA!=numB) return numA < numB;
    });
  }
  // resulting bool - extract second element from resultOut pair
  std::vector<bool> resultBool(dfNRows);
  extract_second(resultOut,resultBool);
  return resultBool;
}
