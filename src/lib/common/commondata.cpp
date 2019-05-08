#include <common/constants.hpp>
#include <common/commondata.hpp>

#include <cstdlib>

unsigned long getNumberOfThreads(){
  char* threads=getenv("THREADS");
  if(threads==0) return NUMBER_OF_THREADS;
  else return atoi(threads);
}

ThreadPool CommonData::threadPool(getNumberOfThreads());
