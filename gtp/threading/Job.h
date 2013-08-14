#ifndef JOB_H
#define JOB_H

#include <Windows.h>
#include "../util/Callback.h"
#include "../util/StdWilUtil.h"

#define TALKATIVE 0

// This should either be an inner class to 
// ThreadPool or have a private ctor.
// You don't need to see/use jobs.
struct Job
{
  friend class ThreadPool ;   // ThreadPool may touch my privates

  real when ;  // delay to execution

private:
  enum JOB_STATUS { NOTSTARTED, WORKING, PAUSED, ABORTED, DONE } ;

  Callback *callback ;
  JOB_STATUS status ;

  DWORD doneByThreadId ;
  //time_t completionTime ;

  // I want for only ThreadPool to be able to
  // create Job objects.
  // A job must have both a name, and a callback function
  // (the callback function IS THE DEFINITION OF THE JOB!!)
  Job( char* iname, Callback *iCallback ) {
    status = NOTSTARTED ;
    name = strdup( iname ) ;
    callback = iCallback ;
    when=0; // no delay.   0 is always in the "past" because time is always +.
  }
  Job( char* iname, real delay, Callback *iCallback ) {
    status = NOTSTARTED ;
    name = strdup( iname ) ;
    callback = iCallback ;
    when=delay;
  }

public:
  ~Job()
  {
    DESTROY( callback ) ; //!! delete this now.  there is no
    // other chance to, often the Callback is created using
    // NEW syntax like Job*job=new Job( "name", new Callback0().. )
    free( name ) ; // you strudup'd this in any possible ctors
  }
  char *name ;

  void start() {
    callback->exec() ; // this starts the job.
  }

  // If the THREAD on which the job
  // is running gets suspended or terminated,
  // we can save that information here
  // Pause the thread execution
  void pause() { status = PAUSED ; }
  void unpause() { status = WORKING ; }
  void abort() { status = ABORTED ; }
  void done() {
    status = DONE ;
    doneByThreadId = GetCurrentThreadId() ; // the thread that
    // was assigned this job will be the one to mark it done.
  }
  
  JOB_STATUS getStatus() { return status ; }
} ;

#endif
