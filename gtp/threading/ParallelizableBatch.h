#ifndef PARALLELIZABLEBATCH_H
#define PARALLELIZABLEBATCH_H

#include "../util/StdWilUtil.h"
#include "../util/Callback.h"
#include "ProgressBar.h"
#include "../window/GTPWindow.h"
#include <list>
#include <vector>
using namespace std ;

// A set of Jobs that can run in parallel,
// with a special JOBDONE callback to be run when
// all are complete
class ParallelBatch
{
  HANDLE mutexBatch ;  // when a job is done, 
  // it must increment the COMPLETED counter
  
  bool aborting ; // a bool used to abort.

  int numDone ;

  // Times runtime for the batch.
  Timer timer ;

public:
  //Callback* preJob ;  // job to run BEFORE this PB is actually dispatched.
  Callback* doneJob ; // the job to call when ALL jobs are done
  // If you don't set this in the ctor you can still set it
  // before you submit the batch

  // flag to indicate the doneJob has completed execution and hence
  // the parallelbatch is COMPLETELY done (fixes bug where ThreadPool::Batchready()
  // thought the batch was ALL DONE (and ready for deletion) but the donejob wasn't done.)
  // this bug only appeared when you tried to enqueue a NEW parallelbatch in the donejob
  // of a previous parallelbatch.
  bool doneDoneJob ;

private:
  char *name ;
  ProgressBar *pb ;
  vector<Callback*> jobs ;

public:
  ///ParallelBatch( char* iname ) ;
  ParallelBatch( char* iname, Callback* iDoneJob=NULL ) ;
  ParallelBatch( char* iname, vector<Callback*> & theJobsToDoInParallel, Callback* iDoneJob ) ;
  void baseInit( char *iname ) ;
  ~ParallelBatch() ;
  void abort() ;

  void addJob( Callback* theJob ) ;

  ////void execOne()
  ////{
  ////  // exec one job.
  ////  // this gets called by
  ////  char buf[60];
  ////  sprintf( buf, "batch job %d", numDone ) ;
  ////  threadPool.createJob( buf, jobs[numDone] ) ;
  ////}

  void enqueueAllJobs() ;

  bool isFinished() ;

  // Cuts a for loop and adds it to this batch.
  template <typename T>
  void cutForLoop( vector<T> dataElts,
    function<void (T elt)> funcToRunOnEach_forLoopBody,
    int maxEltsToProcPerJob,
    Callback *onDone  // sometimes there's a callback to exec
    // when all for "loop" execution is done.
  )
  {
    int* itemsRun = new int(0) ; // can't use a static here b/c this will be called multiple times.

    int numJobsThatWillBeCreated = dataElts.size() / maxEltsToProcPerJob ;
    for( int i = 0 ; i < dataElts.size() ; i+=maxEltsToProcPerJob )
    {
      for( int j = 0 ;
        j < maxEltsToProcPerJob &&
        (i+j) < dataElts.size();  // make sure we don't step out of bounds of dataElts on last job
        j++ )
      {
        int itemNo = i+j;  // wrap each func to run with counter increments, so we know
        // WHEN the cut loop completes.

        // the logic here is like for a ParallelBatch, but you can do any number of these
        // forloopCuts and have them all run in parallel.

        // in addition those are actually entry points to the begin/end
        // of the funcToRun, BUT kinda pointless to exploit since you
        // may as well put them in funcToRun itself.
        function<void (T)> wrapper = [itemsRun,itemNo]( T currentItem ){
          funcToRunOnEach_forLoopBody( currentItem ) ; // after this runs,
          // you can check if you're done the (split up) "loop"
          if( itemNo == dataElts.size() )
          {
            if( onDone )
            {
              onDone->exec() ;
              DESTROY( onDone ) ;
              delete itemsRun ;
            }
          }
        } ;
        this->addJob( new Callback1<T>( wrapper, dataElts[i+j] ) ) ;
      }
    }
  }
} ;

#endif
