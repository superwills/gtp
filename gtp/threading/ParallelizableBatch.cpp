#include "ParallelizableBatch.h"
#include "ProgressBar.h"
#include "../Globals.h" // for Fonts enumeration

ParallelBatch::ParallelBatch( char* iname, Callback* iDoneJob )
{
  baseInit( iname ) ;
  doneJob = iDoneJob ;
  doneDoneJob = false ;
}
ParallelBatch::ParallelBatch( char* iname, vector<Callback*> & theJobsToDoInParallel, Callback* iDoneJob )
{
  baseInit( iname ) ;
  doneJob = iDoneJob ;
  jobs = theJobsToDoInParallel ; //copy it.
  doneDoneJob = false ;
}
void ParallelBatch::baseInit( char *iname )
{
  aborting = false ;
  name = strdup( iname ) ;
  mutexBatch = CreateMutexA( 0, 0, "Mutexbatch" ) ;
  numDone = 0 ;

  pb = new ProgressBar( name, Fonts::Arial12 ) ;
}
ParallelBatch::~ParallelBatch()
{
  cstrfree( name ) ;
  for(int i=0;i<jobs.size();i++)
    DESTROY( jobs[i] ) ;
  DESTROY( doneJob );
  DESTROY( pb ) ;
}
void ParallelBatch::abort()
{
  info( "Aborting batch %s", name ) ;
  aborting = true ;
}
void ParallelBatch::addJob( Callback* theJob )
{
  jobs.push_back( theJob ) ;
}

void ParallelBatch::enqueueAllJobs()
{
  info( "ParallelBatch %s, enqueuing all jobs", name ) ;
  timer.reset() ; // start the timer
  sprintf( pb->txt, "%s 0 / %d", name, jobs.size() ) ;

  // Spin off ALL threads, and wrap each in a basic method
  // to increment NUMDONE every time one finishes.
  // when the last one finishes, call DONEJOB.

  // ((all this is VERY LOW computation overhead compared to
  //   the size of the job, usually, or you need to reduce overhead
  //   if you're doing tons of short quick jobs))
  if( !jobs.size() )
  {
    warning( "ParallelBatch %s was empty!", name ) ;
    if( doneJob )
    {
      doneJob->exec() ; // execute the special done job, if it was sent
      doneDoneJob = true ; // when return, set this true.now ThreadPool::batchReady() will see
      DESTROY( doneJob ) ;
    }
    threadPool.batchReady() ;
    return ;
  }

  for( int i = 0 ; i < jobs.size() ; i++ )
  {
    // Create a basic wrapper around
    // the ith job.
    function<void ()> wrapper = [this,i](){
      
      if( aborting ) { info( " - job %d aborted", i );  return ; }
      jobs[i]->exec() ;  // exec the ith job (this will happen on some thread)

      {
        MutexLock LOCK( mutexBatch, INFINITE ) ;
        ++numDone ; // this int is protected by a lock.
      }

      // Update the pb
      {
        MutexLock LOCK( window->mutexPbs, INFINITE ) ;
        sprintf( pb->txt, "%s %d / %d", name, numDone, jobs.size() ) ;
        pb->percDone = ((real)numDone/jobs.size()) ;
      }
        
      // When that's done, CHECK IF THIS WAS THE LAST JOB.
      // If it was, then run the DONEJOB on this thread,
      // when THAT'S done, THEN the batch will be done
      // (and another one can start)
      if( numDone == jobs.size() ) // (jobs aren't removed from the list when they are executed)
      {
        if( doneJob )
        {
          doneJob->exec() ; // execute the special done job, if it was sent
          doneDoneJob = true ; // when return, set this true.now ThreadPool::batchReady() will see
          // this parallelbatch as done.
          // since it WASN'T enqueued in the threadPool,
          // you have to delete it here
          DESTROY( doneJob ) ;
        }
        
        

        // HERE the batch is completely finished, including the doneJob.
        info( "Batch %s done in %f seconds", name, timer.getTime() ) ;
        // the batch processor in ThreadPool will automatically advance
        // to the next batch.

        threadPool.batchReady() ; // trigger another call to BatchReady,
        // because the ParallelBatch is completely done after all.
      }
    } ; // end wrapper fcn

    // DISPATCH THE JOB TO __ANY__ AVAILABLE THREAD VIA THREADPOOL
    threadPool.createJob( "batch job", new Callback0( wrapper ), 0.0, false ) ;  // don't launch thread until we're done submitting
  }

  #if THREAD_VERBOSE
  info( "%d jobs queued", jobs.size() ) ;
  #endif

  // Now dispatch, after enqueuing all jobs.
  threadPool.callToWork() ;
  
}

bool ParallelBatch::isFinished()
{
  if( numDone > jobs.size() )  error( "%d jobs done, but jobs.size() was %d", numDone, jobs.size() ) ;
  return (numDone == jobs.size()) && // have to have completed all jobs
         ((!doneJob)||(doneJob && doneDoneJob)) ;  // IF there's a donejob, it has to be done too.
}

