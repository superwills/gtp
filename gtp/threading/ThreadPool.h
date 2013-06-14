#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <Windows.h>
#include <process.h>
#include <vector>
#include <list>
#include <map>
#include <ctime>
#include "../util/Callback.h"
#include "../util/StopWatch.h"

#include "Thread.h"

using namespace std ;

class ParallelBatch ;

typedef list<Job*> /*as simply*/ JobList ;
typedef list<Job*>::iterator /*as simply*/ JobListIter ;
typedef list<ParallelBatch*> BatchList ;

// XX Maps a THREAD-ID to a LIST of jobs to do.
// It WAS going to be a list, but we have NO IDEA how long
// each Job will take.

typedef map<DWORD, Job*> /*as simply*/ ThreadJobMap ;
typedef map<DWORD, JobList>::iterator /* as simply */ ThreadJobMapIter ;


// Singleton.
#define threadPool ThreadPool::getInstance()

struct Thread ;

class ThreadPool
{
private:
  bool showInfo ;
  int cores ;
  vector<Thread*> threads ;
  Timer clock ; // Need my own clock.

  // A Callback is a "job" to be run,
  // or a function to be executed, complete
  // with method to call and arguments.
  // I wanted to mark these volatile, but advice is not to use it.
  JobList queuedJobs ;
  JobList activeJobs ;
  //JobList doneJobs ; // this ended up never being used
  JobList MAINTHREADONLYJobs ;  // DUE TO the nature of the Windows API,
  // windowing functions can only be called on the main thread.  also d3d only allows a single thread to
  // access the d3ddevice.

  BatchList queuedBatches ;
  ParallelBatch* activeBatch ;  // ..ONLY ONE AT A TIME!!

  // the mutex lock for protecting access to
  // the job queues.
  HANDLE mutexQueues ;
  bool _hold ;

public:
  DWORD mainThreadId ;
  HANDLE mainThread ;
  
private:
  ThreadPool() ;
  ~ThreadPool() ;

public:
  // aborts ALL pending threadPool jobs.
  // You CAN'T go in and inject/stop a running task.
  // but you can cancel existing tasks.
  void cancelAllScheduled() ;
  void die();

  void hold( bool on )
  {
    _hold = on ; 
    if( !_hold )
      callToWork() ;
  }

  bool onMainThread() { return GetCurrentThreadId() == mainThreadId ; }

  static ThreadPool& getInstance()
  {
    static ThreadPool instance ;
    return instance ;
  }

  // You would ADD JOBS from the main thread.
  // You don't explicitly create Job objects however.
  void createJob( char *name, Callback *callbackJob, real delay=0.0, bool autoexec=true ) ;
  void createJobForMainThread( char *name, Callback *callbackJob, real delay=0.0 ) ;

  // Gets and removes the next READY job in the list
  Job* getRemoveNextReadyJob( list<Job*>& jobList ) ;

  // return true WHEN the main thread 
  bool execJobMainThread() ;
  void createBatch( char *name, vector<Callback*>& theJobsToDoInParallel, Callback* batchDoneJob=NULL ) ;
  void addBatch( ParallelBatch *batch ) ;

private:
  void lockQueues() ;
  void unlockQueues() ;

public:
  Thread* getThreadById( DWORD threadId ) ;

  // Move the job from the
  // active queue to the done list
  void jobDone( Job *job ) ;

  Job* getNextJob() ;

  //#threads running not necessarily length of the active queue
  int getNumThreadsRunning() ;

  void startNextBatch() ;

  // batchDispatch
  void batchReady() ;

  // gets you the # jobs ready to execute, not just the queue size
  int getNumReadyJobs() ;

  // Call Suspended as many threads to work as needed
  void callToWork() ;

  // In order 
  void pause() ;

  void resume() ;
} ;


#endif