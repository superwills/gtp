#include "ThreadPool.h"
#include "../window/GTPWindow.h"
#include "ParallelizableBatch.h"

ThreadPool::ThreadPool()
{
  _hold = false ;
  showInfo = false ;

  // get # cores
  SYSTEM_INFO sysInfo ;
  GetSystemInfo( &sysInfo ) ;

  cores = sysInfo.dwNumberOfProcessors ;
  info( "%d processors", cores ) ;

  // Create the mutex for accessing the job queues.
  mutexQueues = CreateMutexA( 0, 0, "queue lock" ) ;

  // You make the threads once when the ThreadPool
  // is first constructed.
  // create (cores - 1) threads
  for( int i = 0 ; i < cores - 1 ; i++ )
  {
    Thread *t = new Thread() ;
    threads.push_back( t );
  }

  activeBatch = 0 ;
}

ThreadPool::~ThreadPool()
{
  // kill all threads
  ////die(); // because the ThreadPool is not a pointer,
  // it gets destroyed TWICE. You need to call ::die()
  // BEFORE shutting down the window object.
  if( window ) die() ;
}

void ThreadPool::cancelAllScheduled()
{
  //!! I'm afraid there may be a deadlockable condition
  // between acquiring a lock to the queuedJobs list and
  // the code that terminates a ProgressBar (mutexPbs).

  // kills all existing jobs
  MutexLock lock( mutexQueues, INFINITE ) ;
  
  foreach( JobListIter, li, queuedJobs )
  {
    // The only way to make jobs ABORTABLE is if
    // you require every "function" to somehow respond to
    // a "quit" message (which would take the form of
    // a boolean.  But why slow down a loop like that?
    // just yeah.
    DESTROY( (*li) ) ;
  }

  queuedJobs.clear() ;

  activeBatch->abort() ;

  // cancel any PENDING batches. These batches
  // WILL NOT have been queued to joblist.
  foreach( BatchList::iterator, bi, queuedBatches )
    (*bi)->abort() ;

  
}

void ThreadPool::die()
{
  pause() ;
  for( int i = 0 ; i < threads.size() ; i++ )
    DESTROY( threads[i] ) ;
  threads.clear() ;
}

// You would ADD JOBS from the main thread.
//void ThreadPool::createJob( Job * job )
void ThreadPool::createJob( char *name, Callback *callbackJob, real delay, bool autoexec )
{
  //if( GetCurrentThreadId() != mainThreadId )
  //  warning( "creating job on NOT main thread." ) ; // not a big deal
  lockQueues() ;

  // Create the job object
  Job *job = new Job( name, delay+clock.getTime(), callbackJob ) ;

  // ENQUEUE THE JOB
  // Where does it go though?
  // we really don't know which thread will be done first.
  queuedJobs.push_back( job ) ;
  int numJobs = queuedJobs.size() ;
  unlockQueues() ;

  if( showInfo )  info( DkGreen, "Job '%s' submitted, there are now %d jobs", name, numJobs ) ;

  // now wake up threads
  if( autoexec )  callToWork() ;
}

void ThreadPool::createJobForMainThread( char *name, Callback *callbackJob, real delay )
{
  lockQueues() ;

  // Create the job object
  Job *job = new Job( name, delay+clock.getTime(), callbackJob ) ;
  ///info( "Will be done at %f sec", job->when ) ;

  MAINTHREADONLYJobs.push_back( job ) ;
  int numJobs = MAINTHREADONLYJobs.size() ;
  unlockQueues() ;

  if( showInfo )  info( DkGreen, "MAIN THREAD ONLY Job '%s' submitted, there are now %d MAIN THREAD ONLY jobs", name, numJobs ) ;

  // now wake up threads
  ////callToWork() ; // No, the MAIN THREAD will pick up jobs when
  // its good and ready.
}

Job* ThreadPool::getRemoveNextReadyJob( list<Job*>& jobList )
{
  // OLD WAY:
  //Job* job = MAINTHREADONLYJobs.front();
  //MAINTHREADONLYJobs.pop_front() ;
  //return job ;

  // NEW WAY:
  Job* job=NULL;
  double currentTime = clock.getTime() ;
  for( list<Job*>::iterator iter = jobList.begin() ; iter != jobList.end() ; ++iter )
  {
    if( (*iter)->when < currentTime ) // it's ready
    {
      ////info( "Job ready %f < %f", (*iter)->when, timer.total_time ) ;
      job = *iter ;
      jobList.erase( iter ) ;  // erase it from the list
      break ; // done
    }
  }
  
  return job ;
}

bool ThreadPool::execJobMainThread()
{
  if( GetCurrentThreadId() != mainThreadId )
  {
    error( "ThreadPool::execJobMainThread: YOU ARE NOT THE MAIN THREAD, WHY ARE YOU CALLING ME??" );
    return false ;
  }

  Job*job=NULL;
  if( MAINTHREADONLYJobs.size() )
  {
    lockQueues() ;
    
    job = getRemoveNextReadyJob( MAINTHREADONLYJobs ) ;
    unlockQueues() ;
    
    if(job)
    {
      job->start() ;
      delete job ; // delete job, but leave the memory pointer with a value so return non-null works
    }
    // if not.. then there were no ready jobs.
  }

  return MAINTHREADONLYJobs.size() && job ;
}

void ThreadPool::createBatch( char *name, vector<Callback*>& theJobsToDoInParallel, Callback* batchDoneJob )
{
  ParallelBatch* batch = new ParallelBatch( name, theJobsToDoInParallel, batchDoneJob ) ;
  queuedBatches.push_back( batch ) ;

  // When you create a ParallelBatch, there could be any number
  // of batches/jobs already submitted.
  batchReady() ; // like CallToWork, this checks to see
  // if no other BATCH is running.  BATCHES can't run
  // parallel to each other, since they represent
  // mutually exclusive operations, or operations that
  // must be performed in sequence.
  // If you don't use a ParallelBatch, you are implying that
  // ORDER DOESN'T MATTER
  // Batched jobs will take lower precedence than non
  // batched.
}

void ThreadPool::addBatch( ParallelBatch *batch )
{
  queuedBatches.push_back( batch ) ;
  batchReady() ;
}

void ThreadPool::lockQueues()
{
  DWORD waitResult = WaitForSingleObject( mutexQueues, INFINITE ) ;
  if( waitResult == WAIT_ABANDONED )
    error( "Thread %d mutexQueues wait abandoned..", GetCurrentThreadId() ) ;
}

void ThreadPool::unlockQueues()
{
  // drop the mutex
  if( !ReleaseMutex( mutexQueues ) )
    error( "Thread %d couldn't release the mutexQueues", GetCurrentThreadId() ) ;
}

Thread* ThreadPool::getThreadById( DWORD threadId )
{
  for( int i = 0 ; i < threads.size() ; i++ )
    if( threads[i]->threadId == threadId )
      return threads[i] ;

  warning( "Thread %d doesn't exist in the collection yet", threadId ) ;
  return NULL ;
}

// Move the job from the
// active queue to the done list
void ThreadPool::jobDone( Job *job )
{
  lockQueues() ;
  
  activeJobs.remove( job ) ;
  //doneJobs.push_back( job ) ;
  DESTROY( job ) ; //!! DESTROY JOB instead of keeping it

  int jobsRemaining = queuedJobs.size();
  
  unlockQueues() ;

  if( jobsRemaining == 0 )
  {
    // all jobs done, you can start the next BATCH.
    //batchReady() ; // you could call batchReady here, but
    // I won't.
    //info( Magenta, "ThreadPool::ALL JOBS DONE!!" ) ;
  }
}

Job* ThreadPool::getNextJob()
{
  Job *job=NULL;

  lockQueues() ;
  if( !queuedJobs.empty() ) // can also check getNumReadyJobs(), but that's another whole iteration thru!
  {
    // there's POSSIBLY a job to do
    job = getRemoveNextReadyJob( queuedJobs ) ;
    
    // add it to the active queue
    activeJobs.push_back( job ) ;
  }
  unlockQueues() ;

  if( showInfo )
    if( job ) // if there was a job
      info( Blue, "Job %s taken by thread %d, %d jobs remain", job->name, GetCurrentThreadId(), queuedJobs.size() ) ;
  
  return job ;
}

int ThreadPool::getNumThreadsRunning()
{
  //count # threads running
  int t = 0 ;
  for( int i = 0; i<threads.size();i++)
    if( threads[i]->getStatus()==Thread::Running )  t++;

  return t;
}

// call this only when its time to start the next batch
void ThreadPool::startNextBatch()
{
  if( queuedBatches.empty() )
  {
    error( "there are no batches to run" ) ;
    return ;
  }

  // dequeue the next available batch
  activeBatch = queuedBatches.front() ;
  queuedBatches.pop_front() ;

  // start it: enqueue jobs from this newly active batch
  activeBatch->enqueueAllJobs() ;
}

// Possible names: BatchDispatch
void ThreadPool::batchReady()
{
  // gets called when a job finishes.
  // enqueue enough jobs that callToWork maxes out
  // when it is called
  if( !activeBatch && !queuedBatches.size() )
  {
    // no active batch and no batches to queue.
    info( "no active batch and no batches to queue." ) ;
    return ;
  }

  else if( activeBatch ) // there is an active batch
  {
    // see if its done
    if( activeBatch->isFinished() ) 
    {
      // its done, so enqueue the next batch.
      if( showInfo )  info( Green, "BatchReady::DONE, enqueuing next batch.." ) ;
      DESTROY( activeBatch ) ;

      if( queuedBatches.size() )
        startNextBatch() ;
    }
    else
    {
      // still running the 'activeBatch'
      if( showInfo )  info( Green, "BatchReady::still running the activeBatch" ) ;
    }
  }

  else
  {
    // no active batch, but there is a queuedBatch
    if( showInfo )  info( Green, "BatchReady::starting a batch.." ) ;
    startNextBatch() ;
  }
}

int ThreadPool::getNumReadyJobs()
{
  double currentTime = clock.getTime() ;
  int ready = 0 ;
  for( list<Job*>::iterator iter = queuedJobs.begin() ; iter != queuedJobs.end() ; ++iter )
    if( (*iter)->when < currentTime )
      ready++;
  return ready ;
}

// Call Suspended as many threads to work as needed
void ThreadPool::callToWork()
{
  if( _hold ) return ; // PREVENT starting work when hold is on.

  int numThreads = threads.size() ;     // say 5
  int jobsRunning = activeJobs.size() ; // 3 
  int threadsRunning = getNumThreadsRunning();
  int idleThreads = numThreads - threadsRunning ;

  if( showInfo )  info( DkBlue, "Call to wake, %d/%d threads running", threadsRunning, numThreads ) ;
  if( numThreads == threadsRunning )
  {
    if( showInfo )  info( "All threads are already working, waiting.." ) ;
    return ;
  }
  
  //!!threadsafety!!
  int jobsAvailable = getNumReadyJobs() ; // 2
  int threadsAwoken = 0 ;

  // don't wake up more than #jobs rdy
  // don't bother trying to wake if no idle threads
  int threadsToWake = min( jobsAvailable, idleThreads ) ;
  if( showInfo )  info( DkBlue, "Waking up %d threads", threadsToWake ) ;
  // if there are jobs available and we didn't already return,
  // then we have some sleeping threads
  // Go through the Threads list and wake up jobsAvailable threads.
  for( int i = 0 ; i < threads.size() &&  // don't step oob
    threadsAwoken < threadsToWake ;
    i++ )
  {
    // This is the MAIN THREAD calling,
    // so you can't use GETCURRENT whatever

    if( threads[i]->getStatus() == Thread::Suspended )
    {
      threads[ i ]->wakeUp() ;
      threadsAwoken++ ; //count it
      idleThreads-- ;   //there's one less idle thread
    }
  }
}

void ThreadPool::pause()
{
  vector<HANDLE> mutexes ;

  mutexes.push_back( window->scene->mutexShapeList ) ;
  mutexes.push_back( window->scene->mutexVizList ) ;
  mutexes.push_back( window->scene->mutexDebugLines ) ;
  mutexes.push_back( window->mutexPbs ) ;
  
  // You have to get ALL the mutex locks
  // in order to put the threads to sleep.
  // this ensures that you don't try to move on
  // while a thread is in the middle of critical section
  // (will wait until it relinquishes any locks it has)
  vector<MutexLock*> locks ;

  for( int i = 0 ; i < mutexes.size() ; i++ )
  {
    locks.push_back( new MutexLock( mutexes[i], INFINITE ) ) ;
  }
  for( int i = 0 ; i < threads.size() ; i++ )
  {
    if( threads[i]->getStatus() == Thread::Status::Running )
      threads[i]->sleep() ;
  }

  for( int i = 0 ; i < mutexes.size() ; i++ )
    delete locks[i] ; // all mutexes die
} 

void ThreadPool::resume()
{
  for( int i = 0 ; i < threads.size() ; i++ )
    threads[i]->wakeUp() ;
}

