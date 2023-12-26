#include "StdWilUtil.h"
#include "../window/WindowClass.h"
#include "../math/Vector.h"

namespace swu
{
  int cfilesize(FILE* f)
  {
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET); //rewind( f ) ;
    return size;
  }

  #pragma region random number generation.
  // <cstdlib> has __awful__ resolution, this only has a space of
  // 32768 values.  If you're trying to place 10,000 samples on the sphere,
  // you're starting to get a 1/3 chance of repeating theta/phi independently.
  // I chose to use MT instead of cstdlib.  There is also a constant time
  // type called a "linear congruence" approach, and it's in "Realistic Ray Tracing".
  real randFloat()
  {
    // <cstdlib>
    //return (real)rand() / RAND_MAX ;

    //mt
    return MersenneTwister::genrand_int32()*(1.0/4294967295.0);  // same as genrand_real1();
  }
  
  real randFloat( real low, real high )
  {
    // <cstdlib>
    //return lerp( low, high, randFloat() ) ;

    //mt
    return low + MersenneTwister::genrand_int32()*((high-low)/4294967295.0);
  }

  int randInt( int low, int high )
  {
    // <cstdlib>
    //  int diff = high - low ;
    //  if( diff <= 0 ) return low ;
    //  else return low + rand() % ( diff ) ;

    //mt
    return low + ((int)(MersenneTwister::genrand_int32()>>1)) % (high - low);
  }

  int randSign()
  {
    return -1 + 2*randInt(0,2) ; // -1+0 or -1+2=+1
  }

  void randSeed( UINT seed )
  {
    MersenneTwister::init_genrand( seed ) ;
  }

  real interpolateAngle( real t1, real t2 )
  {
    // phasor angle formula
    // if v1 and v2 are not unit vectors, then you need
    // to mult t1 terms by A1, t2 terms by A2.
    return atan2( sin(t1) + sin(t2), cos(t1) + cos(t2) ) ;
  }

  real *facs = 0;
  void prepFactorials()
  {
    static int factorialsPrecomputed = 0;
    if( !factorialsPrecomputed )
    {
      // PRECOMPUTATION
      int FACMAX = 170 ;
      facs = new real[ FACMAX ] ;
      facs[0]=facs[1]=1;
      for( int k = 2 ; k < FACMAX ; k++ )
      {
        int c = k;
        real res = c ;
        while( --c>1 ) res *= c ;
        facs[k] = res ;
      }

      factorialsPrecomputed = 1 ;
    }
    else
      warning( "You already precomputed factorials" ) ;
  }

  // Don't use this just access factorials directly.
  //real factorial( int n )
  //{
  //  // old direct.
  //  //////if( n <= 0 )  return 1 ; // safeguard 0 and -ve
  //  //////real res = n ;
  //  //////while( --n>1 ) res *= n ;
  //  //////return res ;
  //
  //  // no checks
  //  //////if( n < 0 || n > 170 ) {
  //  //////  error( "Overflow: cannot compute %d!", n ) ;
  //  //////  return 1 ;//you get 1
  //  //////}
  //  return facs[ n ] ;
  //}

  // Prints now into your buffer
  int sprintNow( char* buf )
  {
    time_t now ;
    tm * timeinfo ;

    time( &now ) ;
    timeinfo = localtime( &now ) ;

    // returns how far along into the buffer sprintNow did
    return strftime( buf, MAX_PATH, "%Y_%m_%d_%A_%H-%M-%S", timeinfo );
  }

  bool test( bool shouldBeTrue, char *messagePass, char *messageFail )
  {
    if( true == shouldBeTrue )
    {
      info( "Passed! %s", messagePass ) ;
    }
    else
    {
      error( "FAILED! %s", messageFail ) ;
    }

    return shouldBeTrue ; 
  }

  wchar_t* getUnicode( const char* ascii )
  {
    int len = strlen( ascii ) ;
    WCHAR * wstr = new WCHAR[ len+1 ] ;

    MultiByteToWideChar( CP_ACP, 0, ascii, len, wstr, len ) ;
    wstr[ len ] = 0 ; // add null terminator

    return wstr ;
  }

  int cStrWrite( FILE* file, const char* str, int fixedLen )
  {
    // 1. declare a buffer
    char *__tbuf0 = new char[ fixedLen ];
    // 2.clear it to 0's
    memset(__tbuf0,0,fixedLen);
    // 3. copy str to __tbuf0
    strcpy(__tbuf0,str); 
    // 4. write FIXEDLEN characters to file
    int written = fwrite( __tbuf0, 1, fixedLen, file ) ;

    delete[] __tbuf0 ;//clean up
    return written ;
  }

  char* cStrRead( FILE* file, int fixedLen )
  {
    // you have to know the len
    char* cstr = new char[fixedLen];
    fread( cstr, 1, fixedLen, file );
    return cstr ;
  }

  string& cStrRead( string& saveIntoStr, FILE* file, int fixedLen )
  {
    char* cstr = cStrRead( file, fixedLen ) ;
    saveIntoStr = string( cstr ) ;
    free( cstr ) ;
    return saveIntoStr ;
  }

  MutexLock::MutexLock( HANDLE mutexToLock, DWORD waitTime )
  {
    mutexLock = mutexToLock ;

    DWORD waitRes = WaitForSingleObject( mutexToLock, waitTime ) ;
    
    if( waitRes == WAIT_ABANDONED )
      warning( "A lock was abandoned" ) ;
  }
    
  // Release in dtor
  MutexLock::~MutexLock()
  {
    ReleaseMutex( mutexLock ) ;
  }
}

namespace Console
{
  char *progname= "unnamed prog" ;
  //DWORD logThreadId ;     // we stopped doing this because of richtext
  //HANDLE logThreadHandle ;// we stopped doing this because of richtext

  std::vector<LogMessage*> logMessages ;

  #pragma region log message member functions
  LogMessage::LogMessage( int iLogLevel, DWORD iColor, char *iMsg )
  {
    logLevel = iLogLevel ;
    // copy the buffer here,
    msg = (char*)malloc( strlen(iMsg) + 1 ) ;
    color = iColor ; // default color
    richColor = RichColors[ color ] ;
    strcpy( msg, iMsg ) ;
  }

  void LogMessage::flush()
  {
    // flush the log message to the globally active outputs

    // If the error's log level qualifies it
    // to be output to the console based on
    // current console flags..
    if( logOutputsForConsole & logLevel )
    {
      setColor( color ) ;
      printf( msg ) ; // puts will put an extra blank
    }
    if( logOutputsForFile & logLevel )
      fprintf( logFile, msg ) ;
    
    // Also put it in the Visual Studio debug window
    // this also appears in DEBUGVIEW by Mark Russinovich, which you can get
    // from http://technet.microsoft.com/en-us/sysinternals/bb896647.aspx
    if( logOutputsForDebugStream & logLevel )
      OutputDebugStringA( msg ) ;

    if( logOutputsForRichText & logLevel )
    {
      //logRichText->setTextColor( richColor ) ;
      logRichText->append( msg, richColor, RGB(200,200,200 ) ) ;
    }
  }

  LogMessage::~LogMessage()
  {
    free( msg ) ;
  }
  #pragma endregion

  void initConsole()
  {
    // Attach a console
    AllocConsole() ;
    AttachConsole( GetCurrentProcessId() ) ;
    freopen( "CON", "w", stdout ) ;

    hwnd = GetConsoleWindow() ;  // grab its "handle"..
    handle = GetStdHandle( STD_OUTPUT_HANDLE ) ;
  }

  void setRowsAndCols( int rows, int cols )
  {
    char cmd[ 60 ] ;
    sprintf( cmd, "mode %d, %d", cols, rows ) ;
    system( cmd ) ;
  }

  // Sets the console's size in PIXELS.
  // This function has no effect on # rows
  // and cols the console has.  Use setRowsAndCols
  // for that.
  void setSizeInPixels( int pixelsWidth, int pixelsHeight )
  {
    SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, pixelsWidth, pixelsHeight, SWP_NOMOVE | SWP_NOZORDER ) ;
  }

  // moves the console to x, y location,
  // x measured from LEFT EDGE of screen
  // y measured from TOP EDGE of screen
  void moveTo( int x, int y )
  {
    SetWindowPos( hwnd, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER ) ;
  }

  // Makes the console the topmost window
  void makeTopmostWindow()
  {
    SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE ) ;
  }

  void setColor( WORD color )
  {
    // Change the current color
    SetConsoleTextAttribute( handle, color ) ;
  
    // set up for NEXT call.

    // Now save off the color used 2 calls ago
    // as the color that was used last call.
    colorUsed2CallsAgo = colorUsedLastCall ;
  
    // Now update the color used on THIS call,
    // which will be the color used on the LAST call
    // at the next call.
    colorUsedLastCall = color ;
  
  }

  void revertToLastColor()
  {
    // Change the current color.  This DOESN'T affect
    // the colorUsedLastCall variable etc.
    SetConsoleTextAttribute( handle, colorUsed2CallsAgo ) ;
  }



  FILE *logFile ;
  RichEdit* logRichText = NULL ;
  char *logOutputFilename = NULL ;

  // Variables that make it so we can
  // only output ERROR and WARNING messages
  // to the file, but output ALL types of
  // ERROR, WARNING, INFO messages to the console.
  int logOutputsForConsole ;
  int logOutputsForFile ;
  int logOutputsForDebugStream ;
  int logOutputsForRichText ;
  //bool logging, loggingThreadExited ;

  /*
  DWORD logflush( LPVOID pparam )
  {
    while( logging )
    {
      // check the queue
      {
        MutexLock lock( mutexLog, INFINITE ) ;
        if( !logMessages.empty() )
        {
          // dump the queue
          for( int i = 0 ; i < logMessages.size() ; i++ )
          {
            logMessages[i]->flush() ;
            // after it flushes, you can delete it
            delete logMessages[i] ;
          }

          // Now clear the logMessages queue, you flushed and deleted them all
          logMessages.clear() ;
        }
      } // mutexLog lock release
      
      // actually a busy wait, ideally
      // this thread should go to sleep until
      // someone enqueues a message
      SuspendThread( logThreadHandle ) ; // Suspend myself
    }

    delete( logRichText ) ;

    // signal the logthread is finished,
    // after logging is DONE, you have
    // to wait until this gets set to true
    // so that all the pending log messages
    // are outputted.  You CANNOT achieve this
    // by asking for the lock on mutexLog, between
    // turning logging off and closing the output file handle because
    // that creates a race condition
    
    loggingThreadExited = true ;
    return 0 ;
  }
  */

  void logflush()
  {
    // check the queue
    {
      MutexLock lock( mutexLog, INFINITE ) ;
      if( !logMessages.empty() )
      {
        // dump the queue
        for( int i = 0 ; i < logMessages.size() ; i++ )
        {
          logMessages[i]->flush() ;
          // after it flushes, you can delete it
          DESTROY( logMessages[i] );
        }

        // Now clear the logMessages queue, you flushed and deleted them all
        logMessages.clear() ;
      }
    } // mutexLog lock release
  }

  /// LOGGING
  // start up a logwindow AROUND a certain window.
  void logStartup( char *outputFilename, char *runningProgramName, int windX, int windY, int windWidth, int windHeight )
  {
    // KEEP copy of filename
    logOutputFilename = strdup( outputFilename ) ;

    // Setup a console and logging

    // not using the console
    initConsole() ;
    moveTo( windX, windY+windHeight ) ; // align x, y bottom
    
    //setSizeInPixels( 400, 200 ) ;
    //setRowsAndCols( 10, 120 ) ;

    // create richtext window
    logRichText = new RichEdit( windX+windWidth+8, windY ) ; // MUST BE CREATED/MESSAGED ON MAIN THREAD.
    logRichText->show() ;
    // IT DOESN'T WORK OTHERWISE.

    progname = runningProgramName ;

    // Put this HERE because 
    logFile = fopen( outputFilename, "w" ) ;

    logOutputsForConsole = ErrorLevel::Error | ErrorLevel::Warning | ErrorLevel::Info ;
    logOutputsForFile = ErrorLevel::Error | ErrorLevel::Warning | ErrorLevel::Info ;
    logOutputsForDebugStream = ErrorLevel::Error ;//| ErrorLevel::Warning ;
    logOutputsForRichText = ErrorLevel::Error | ErrorLevel::Warning | ErrorLevel::Info ;

    mutexLog = CreateMutexA( 0, 0, "log mutex" ) ;

    info( "Startup" ) ;
    //logging = true ;

    // start the thread
    //logThreadHandle = CreateThread( 0, 0, (LPTHREAD_START_ROUTINE)&logflush, 0, 0, &logThreadId ) ; // UNFORTUNATELY
    // this is not allowed.  You must have a call to LOGFLUSH in the MAIN THREAD
    // otherwise what happens is you cannot flush to the RichText window.
    // In addition you get this bad delay between when you made log messages and when
    // you print them.  Putting the log on the same thread as d3d isn't very desirable,
    // (ie mutex locking to wait for all other threads to stop writing log messages
    // could potentially slow down the render pretty loop pretty visibly) but log messages should
    // be sparse and that shouldn't happen often.
  }

  void logShutdown()
  {
    info( "-- end" ) ;

    logflush() ;
    
    // now you can close the mutexLog handle,
    CloseHandle( mutexLog );

    // and finally the log file pointer
    fclose( logFile ) ;

    free( logOutputFilename ) ; // free the log filename duplicate pointer.
    // oh the completeness.
  }

  /*
  void logShutdown()
  {
    info( "-- end" ) ;
    
    logging = false ; // allow the thread to exit its loop
    // the logging thread should be woken up now,
    // I'm using WaitForSingleObject to wait for the thread to wake up.
    
    puts( "*** Waiting for logThread to wake up..." ) ;
    WaitForSingleObject( logThreadHandle, INFINITE ) ;
    puts( "*** logThread awake, waiting for logThread to exit.." ) ;
    
    while( !loggingThreadExited ) ; // active wait
    
    // now you can close the mutexLog handle,
    CloseHandle( mutexLog );

    // and finally the log file pointer
    fclose( logFile ) ;
  }
  */

  tm* getCurrentTime()
  {
    static time_t raw ;
  
    // grab the current time
    time( &raw ) ;

    // Now create that timeinfo struct
    static tm* timeinfo ;
    timeinfo = localtime( &raw ) ;
  
    return timeinfo ;
  }

  char* getCurrentTimeString()
  {
    // write time into timeBuff
    static char timeBuf[ 256 ] ;
    strftime( timeBuf, 255, "%c", getCurrentTime() ) ;
    return timeBuf ;
  }

  static char* getErrorLevelString( int logLevel )
  {
    switch( logLevel )
    {
    case ErrorLevel::Error :
      return "error" ;
      break;

    case ErrorLevel::Warning :
      return "warning" ;
      break;

    case ErrorLevel::Info :
      return "info" ;
      break;
      
    default:
      return "Unknown error level";
      break;
    }
  }

  void muteAll( bool muteIt )
  {
    logflush() ;
    static int llC, llD, llF, llR ;
    
    if( logOutputsForConsole && muteIt )
    {
      // its on and we want to turn it off
      llC = logOutputsForConsole ;
      llD = logOutputsForDebugStream ;
      llF = logOutputsForFile ;
      llR = logOutputsForRichText ;
      
      logOutputsForConsole =
      logOutputsForDebugStream =
      logOutputsForFile =
      logOutputsForRichText = 0 ;
    }
    else if( !logOutputsForConsole && !muteIt )
    {
      // its off and we want to turn it on
      logOutputsForConsole = llC ;
      logOutputsForDebugStream = llD ;
      logOutputsForFile = llF ;
      logOutputsForRichText = llR ;
    }
  }

  // final level for logging -- everything that goes here will get printed to the log
  void enqueueLog( int logLevel, DWORD color, char *msg )
  {
    LogMessage *LMSG = new LogMessage( logLevel, color, msg ) ;

    {
      MutexLock lock( mutexLog, INFINITE ) ;

      // ENQUEUE YOUR MESSAGE TO THE WAIT LOG
      logMessages.push_back( LMSG ) ;
    } // lock released

    // now wake up the output thread
    //ResumeThread( logThreadHandle ) ;
  }

  // decorates the log message with [appname][thread][error level][current time]:  message
  void logDecorate( int logLevel, DWORD color, const char *fmt, va_list args )
  {
    // to be threadsafe, removed static
    char msgBuffer[ 4096 ] ;  // oops. Had a 623 char error (from shader) and it err-d out.
    vsprintf( msgBuffer, fmt, args ) ;
    
    // write time into timeBuff. Should be about 8 chars hh:mm:ss
    char timeBuf[ 32 ] ;
    strftime( timeBuf, 255, "%X", getCurrentTime() ) ;
  
    // Put it all together
    char buf[ 4096 ] ;
    
    sprintf( buf, "[ %s ][ thread %d ][ %s ][ %s ]:  %s\n", progname, GetCurrentThreadId(), getErrorLevelString( logLevel ), timeBuf, msgBuffer ) ;
    // Add program name in front as well
    //sprintf( buf, "[ %s ][ %s ]:  %s\n", errLevel, timeBuf, msgBuffer ) ;
    // wait @ output to print to the streams,
    // so they don't get jambled together.

    // The reason we drop the log message to the console immediately here
    // is if you are running a long process on the main thread,
    // the log won't ever flush until logflush() is called, so
    // you won't see any error messages!
    printf( "%s", buf ) ;
    enqueueLog( logLevel, color, buf ) ;
  }

  void error( const char *fmt, ... )
  {
    va_list lp ;
    va_start( lp, fmt ) ;
  
    logDecorate( ErrorLevel::Error, Red, fmt, lp ) ;
  }

  void warning( const char *fmt, ... )
  {
    va_list lp ;
    va_start( lp, fmt ) ;
  
    logDecorate( ErrorLevel::Warning, Yellow, fmt, lp ) ;
  }

//#define → info
  void info( DWORD iColor, const char *fmt, ... )
  {
    va_list lp ;
    va_start( lp, fmt ) ;
  
    logDecorate( ErrorLevel::Info, iColor, fmt, lp ) ;
  }

  void info( const char *fmt, ... )
  {
    va_list lp ;
    va_start( lp, fmt ) ;
  
    logDecorate( ErrorLevel::Info, Gray, fmt, lp ) ;
  }

  void plain( DWORD iColor, const char *fmt, ... )
  {
    va_list args ;
    va_start( args, fmt ) ;

    static char msgBuffer[ 512 ] ;
    vsprintf( msgBuffer, fmt, args ) ;

    // skip log decorate and just print the raw message
    enqueueLog( ErrorLevel::Info, iColor, msgBuffer ) ;
  }

  void plain( const char *fmt, ... )
  {
    va_list args ;
    va_start( args, fmt ) ;
  
    static char msgBuffer[ 512 ] ;
    vsprintf( msgBuffer, fmt, args ) ;
  
    // skip log decorate and just print the raw message
    enqueueLog( ErrorLevel::Info, Console::White, msgBuffer ) ;
  }
  
  void plainFile( const char *fmt, ... )
  {
    va_list args ;
    va_start( args, fmt ) ;

    static char msgBuffer[ 512 ] ;
    vsprintf( msgBuffer, fmt, args ) ;

    // skip log decorate and just print the raw message
    //enqueueLog( ErrorLevel::Info, Console::White, msgBuffer ) ;
    fprintf( logFile, msgBuffer ) ;
  }

  void bail( char *msg, bool openLog )
  {
    info( "BAIL. %s", msg ) ;
    logShutdown() ; //properly shut down the log

    if( openLog )
      openLogfile() ;
  
    //exit( 1 );
    ExitProcess( 1 ) ;// exit process and all threads
  }

  void openLogfile()
  {
    logflush() ;
    system( "START notepad.exe C:/vctemp/builds/gtp/lastRunLog.txt" ) ;
    //char cmd[255];

    //sprintf( cmd, "START \"C:/Program Files (x86)/Notepad++/notepad++.exe\" %s", logOutputFilename ) ;
    //sprintf( cmd, "START notepad.exe %s", logOutputFilename ) ;
    
    //system( cmd ) ;
  }

  void printWindowsLastError( char *msg )
  {
    LPSTR errorString = NULL ;

    int result = FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_SYSTEM,
                   0,
                   GetLastError(),
                   0,
                   (LPSTR)&errorString,
                   0,
                   0 );

    error( "%s %s", msg, errorString ) ;
  
    LocalFree( errorString ) ;
  }
}

namespace HALF
{
  int singles2halfp(void *target, void *source, int numel)
  {
    UINT16_TYPE *hp = (UINT16_TYPE *) target; // Type pun output as an unsigned 16-bit int
    UINT32_TYPE *xp = (UINT32_TYPE *) source; // Type pun input as an unsigned 32-bit int
    UINT16_TYPE    hs, he, hm;
    UINT32_TYPE x, xs, xe, xm;
    int hes;
    static int next;  // Little Endian adjustment
    static int checkieee = 1;  // Flag to check for IEEE754, Endian, and word size
    double one = 1.0; // Used for checking IEEE754 floating point format
    UINT32_TYPE *ip; // Used for checking IEEE754 floating point format

    if( checkieee ) { // 1st call, so check for IEEE754, Endian, and word size
      ip = (UINT32_TYPE *) &one;
      if( *ip ) { // If Big Endian, then no adjustment
        next = 0;
      } else { // If Little Endian, then adjustment will be necessary
        next = 1;
        ip++;
      }
      if( *ip != 0x3FF00000u ) { // Check for exact IEEE 754 bit pattern of 1.0
        return 1;  // Floating point bit pattern is not IEEE 754
      }
      if( sizeof(INT16_TYPE) != 2 || sizeof(INT32_TYPE) != 4 ) {
        return 1;  // short is not 16-bits, or long is not 32-bits.
      }
      checkieee = 0; // Everything checks out OK
    }

    if( source == NULL || target == NULL ) { // Nothing to convert (e.g., imag part of pure real)
      return 0;
    }

    while( numel-- ) {
      x = *xp++;
      if( (x & 0x7FFFFFFFu) == 0 ) {  // Signed zero
        *hp++ = (UINT16_TYPE) (x >> 16);  // Return the signed zero
      } else { // Not zero
        xs = x & 0x80000000u;  // Pick off sign bit
        xe = x & 0x7F800000u;  // Pick off exponent bits
        xm = x & 0x007FFFFFu;  // Pick off mantissa bits
        if( xe == 0 ) {  // Denormal will underflow, return a signed zero
          *hp++ = (UINT16_TYPE) (xs >> 16);
        } else if( xe == 0x7F800000u ) {  // Inf or NaN (all the exponent bits are set)
          if( xm == 0 ) { // If mantissa is zero ...
            *hp++ = (UINT16_TYPE) ((xs >> 16) | 0x7C00u); // Signed Inf
          } else {
            *hp++ = (UINT16_TYPE) 0xFE00u; // NaN, only 1st mantissa bit set
          }
        } else { // Normalized number
          hs = (UINT16_TYPE) (xs >> 16); // Sign bit
          hes = ((int)(xe >> 23)) - 127 + 15; // Exponent unbias the single, then bias the halfp
          if( hes >= 0x1F ) {  // Overflow
            *hp++ = (UINT16_TYPE) ((xs >> 16) | 0x7C00u); // Signed Inf
          } else if( hes <= 0 ) {  // Underflow
            if( (14 - hes) > 24 ) {  // Mantissa shifted all the way off & no rounding possibility
              hm = (UINT16_TYPE) 0u;  // Set mantissa to zero
            } else {
              xm |= 0x00800000u;  // Add the hidden leading bit
              hm = (UINT16_TYPE) (xm >> (14 - hes)); // Mantissa
              if( (xm >> (13 - hes)) & 0x00000001u ) // Check for rounding
                hm += (UINT16_TYPE) 1u; // Round, might overflow into exp bit, but this is OK
            }
            *hp++ = (hs | hm); // Combine sign bit and mantissa bits, biased exponent is zero
          } else {
            he = (UINT16_TYPE) (hes << 10); // Exponent
            hm = (UINT16_TYPE) (xm >> 13); // Mantissa
            if( xm & 0x00001000u ) // Check for rounding
              *hp++ = (hs | he | hm) + (UINT16_TYPE) 1u; // Round, might overflow to inf, this is OK
            else
              *hp++ = (hs | he | hm);  // No rounding
          }
        }
      }
    }
    return 0;
  }

  int doubles2halfp(void *target, void *source, int numel)
  {
    UINT16_TYPE *hp = (UINT16_TYPE *) target; // Type pun output as an unsigned 16-bit int
    UINT32_TYPE *xp = (UINT32_TYPE *) source; // Type pun input as an unsigned 32-bit int
    UINT16_TYPE    hs, he, hm;
    UINT32_TYPE x, xs, xe, xm;
    int hes;
    static int next;  // Little Endian adjustment
    static int checkieee = 1;  // Flag to check for IEEE754, Endian, and word size
    double one = 1.0; // Used for checking IEEE754 floating point format
    UINT32_TYPE *ip; // Used for checking IEEE754 floating point format

    if( checkieee ) { // 1st call, so check for IEEE754, Endian, and word size
      ip = (UINT32_TYPE *) &one;
      if( *ip ) { // If Big Endian, then no adjustment
        next = 0;
      } else { // If Little Endian, then adjustment will be necessary
        next = 1;
        ip++;
      }
      if( *ip != 0x3FF00000u ) { // Check for exact IEEE 754 bit pattern of 1.0
        return 1;  // Floating point bit pattern is not IEEE 754
      }
      if( sizeof(INT16_TYPE) != 2 || sizeof(INT32_TYPE) != 4 ) {
        return 1;  // short is not 16-bits, or long is not 32-bits.
      }
      checkieee = 0; // Everything checks out OK
    }

    xp += next;  // Little Endian adjustment if necessary

    if( source == NULL || target == NULL ) { // Nothing to convert (e.g., imag part of pure real)
      return 0;
    }

    while( numel-- ) {
      x = *xp++; xp++; // The extra xp++ is to skip over the remaining 32 bits of the mantissa
      if( (x & 0x7FFFFFFFu) == 0 ) {  // Signed zero
        *hp++ = (UINT16_TYPE) (x >> 16);  // Return the signed zero
      } else { // Not zero
        xs = x & 0x80000000u;  // Pick off sign bit
        xe = x & 0x7FF00000u;  // Pick off exponent bits
        xm = x & 0x000FFFFFu;  // Pick off mantissa bits
        if( xe == 0 ) {  // Denormal will underflow, return a signed zero
          *hp++ = (UINT16_TYPE) (xs >> 16);
        } else if( xe == 0x7FF00000u ) {  // Inf or NaN (all the exponent bits are set)
          if( xm == 0 ) { // If mantissa is zero ...
            *hp++ = (UINT16_TYPE) ((xs >> 16) | 0x7C00u); // Signed Inf
          } else {
            *hp++ = (UINT16_TYPE) 0xFE00u; // NaN, only 1st mantissa bit set
          }
        } else { // Normalized number
          hs = (UINT16_TYPE) (xs >> 16); // Sign bit
          hes = ((int)(xe >> 20)) - 1023 + 15; // Exponent unbias the double, then bias the halfp
          if( hes >= 0x1F ) {  // Overflow
            *hp++ = (UINT16_TYPE) ((xs >> 16) | 0x7C00u); // Signed Inf
          } else if( hes <= 0 ) {  // Underflow
            if( (10 - hes) > 21 ) {  // Mantissa shifted all the way off & no rounding possibility
              hm = (UINT16_TYPE) 0u;  // Set mantissa to zero
            } else {
              xm |= 0x00100000u;  // Add the hidden leading bit
              hm = (UINT16_TYPE) (xm >> (11 - hes)); // Mantissa
              if( (xm >> (10 - hes)) & 0x00000001u ) // Check for rounding
                hm += (UINT16_TYPE) 1u; // Round, might overflow into exp bit, but this is OK
            }
            *hp++ = (hs | hm); // Combine sign bit and mantissa bits, biased exponent is zero
          } else {
            he = (UINT16_TYPE) (hes << 10); // Exponent
            hm = (UINT16_TYPE) (xm >> 10); // Mantissa
            if( xm & 0x00000200u ) // Check for rounding
              *hp++ = (hs | he | hm) + (UINT16_TYPE) 1u; // Round, might overflow to inf, this is OK
            else
              *hp++ = (hs | he | hm);  // No rounding
          }
        }
      }
    }
    return 0;
  }

  int halfp2singles(void *target, void *source, int numel)
  {
    UINT16_TYPE *hp = (UINT16_TYPE *) source; // Type pun input as an unsigned 16-bit int
    UINT32_TYPE *xp = (UINT32_TYPE *) target; // Type pun output as an unsigned 32-bit int
    UINT16_TYPE h, hs, he, hm;
    UINT32_TYPE xs, xe, xm;
    INT32_TYPE xes;
    int e;
    static int next;  // Little Endian adjustment
    static int checkieee = 1;  // Flag to check for IEEE754, Endian, and word size
    double one = 1.0; // Used for checking IEEE754 floating point format
    UINT32_TYPE *ip; // Used for checking IEEE754 floating point format

    if( checkieee ) { // 1st call, so check for IEEE754, Endian, and word size
      ip = (UINT32_TYPE *) &one;
      if( *ip ) { // If Big Endian, then no adjustment
        next = 0;
      } else { // If Little Endian, then adjustment will be necessary
        next = 1;
        ip++;
      }
      if( *ip != 0x3FF00000u ) { // Check for exact IEEE 754 bit pattern of 1.0
        return 1;  // Floating point bit pattern is not IEEE 754
      }
      if( sizeof(INT16_TYPE) != 2 || sizeof(INT32_TYPE) != 4 ) {
        return 1;  // short is not 16-bits, or long is not 32-bits.
      }
      checkieee = 0; // Everything checks out OK
    }

    if( source == NULL || target == NULL ) // Nothing to convert (e.g., imag part of pure real)
      return 0;

    while( numel-- ) {
      h = *hp++;
      if( (h & 0x7FFFu) == 0 ) {  // Signed zero
        *xp++ = ((UINT32_TYPE) h) << 16;  // Return the signed zero
      } else { // Not zero
        hs = h & 0x8000u;  // Pick off sign bit
        he = h & 0x7C00u;  // Pick off exponent bits
        hm = h & 0x03FFu;  // Pick off mantissa bits
        if( he == 0 ) {  // Denormal will convert to normalized
          e = -1; // The following loop figures out how much extra to adjust the exponent
          do {
            e++;
            hm <<= 1;
          } while( (hm & 0x0400u) == 0 ); // Shift until leading bit overflows into exponent bit
          xs = ((UINT32_TYPE) hs) << 16; // Sign bit
          xes = ((INT32_TYPE) (he >> 10)) - 15 + 127 - e; // Exponent unbias the halfp, then bias the single
          xe = (UINT32_TYPE) (xes << 23); // Exponent
          xm = ((UINT32_TYPE) (hm & 0x03FFu)) << 13; // Mantissa
          *xp++ = (xs | xe | xm); // Combine sign bit, exponent bits, and mantissa bits
        } else if( he == 0x7C00u ) {  // Inf or NaN (all the exponent bits are set)
          if( hm == 0 ) { // If mantissa is zero ...
            *xp++ = (((UINT32_TYPE) hs) << 16) | ((UINT32_TYPE) 0x7F800000u); // Signed Inf
          } else {
            *xp++ = (UINT32_TYPE) 0xFFC00000u; // NaN, only 1st mantissa bit set
          }
        } else { // Normalized number
          xs = ((UINT32_TYPE) hs) << 16; // Sign bit
          xes = ((INT32_TYPE) (he >> 10)) - 15 + 127; // Exponent unbias the halfp, then bias the single
          xe = (UINT32_TYPE) (xes << 23); // Exponent
          xm = ((UINT32_TYPE) hm) << 13; // Mantissa
          *xp++ = (xs | xe | xm); // Combine sign bit, exponent bits, and mantissa bits
        }
      }
    }
    return 0;
  }

  int halfp2doubles(void *target, void *source, int numel)
  {
    UINT16_TYPE *hp = (UINT16_TYPE *) source; // Type pun input as an unsigned 16-bit int
    UINT32_TYPE *xp = (UINT32_TYPE *) target; // Type pun output as an unsigned 32-bit int
    UINT16_TYPE h, hs, he, hm;
    UINT32_TYPE xs, xe, xm;
    INT32_TYPE xes;
    int e;
    static int next;  // Little Endian adjustment
    static int checkieee = 1;  // Flag to check for IEEE754, Endian, and word size
    double one = 1.0; // Used for checking IEEE754 floating point format
    UINT32_TYPE *ip; // Used for checking IEEE754 floating point format

    if( checkieee ) { // 1st call, so check for IEEE754, Endian, and word size
      ip = (UINT32_TYPE *) &one;
      if( *ip ) { // If Big Endian, then no adjustment
        next = 0;
      } else { // If Little Endian, then adjustment will be necessary
        next = 1;
        ip++;
      }
      if( *ip != 0x3FF00000u ) { // Check for exact IEEE 754 bit pattern of 1.0
        return 1;  // Floating point bit pattern is not IEEE 754
      }
      if( sizeof(INT16_TYPE) != 2 || sizeof(INT32_TYPE) != 4 ) {
        return 1;  // short is not 16-bits, or long is not 32-bits.
      }
      checkieee = 0; // Everything checks out OK
    }

    xp += next;  // Little Endian adjustment if necessary

    if( source == NULL || target == NULL ) // Nothing to convert (e.g., imag part of pure real)
      return 0;

    while( numel-- ) {
      h = *hp++;
      if( (h & 0x7FFFu) == 0 ) {  // Signed zero
        *xp++ = ((UINT32_TYPE) h) << 16;  // Return the signed zero
      } else { // Not zero
        hs = h & 0x8000u;  // Pick off sign bit
        he = h & 0x7C00u;  // Pick off exponent bits
        hm = h & 0x03FFu;  // Pick off mantissa bits
        if( he == 0 ) {  // Denormal will convert to normalized
          e = -1; // The following loop figures out how much extra to adjust the exponent
          do {
            e++;
            hm <<= 1;
          } while( (hm & 0x0400u) == 0 ); // Shift until leading bit overflows into exponent bit
          xs = ((UINT32_TYPE) hs) << 16; // Sign bit
          xes = ((INT32_TYPE) (he >> 10)) - 15 + 1023 - e; // Exponent unbias the halfp, then bias the double
          xe = (UINT32_TYPE) (xes << 20); // Exponent
          xm = ((UINT32_TYPE) (hm & 0x03FFu)) << 10; // Mantissa
          *xp++ = (xs | xe | xm); // Combine sign bit, exponent bits, and mantissa bits
        } else if( he == 0x7C00u ) {  // Inf or NaN (all the exponent bits are set)
          if( hm == 0 ) { // If mantissa is zero ...
            *xp++ = (((UINT32_TYPE) hs) << 16) | ((UINT32_TYPE) 0x7FF00000u); // Signed Inf
          } else {
            *xp++ = (UINT32_TYPE) 0xFFF80000u; // NaN, only the 1st mantissa bit set
          }
        } else {
          xs = ((UINT32_TYPE) hs) << 16; // Sign bit
          xes = ((INT32_TYPE) (he >> 10)) - 15 + 1023; // Exponent unbias the halfp, then bias the double
          xe = (UINT32_TYPE) (xes << 20); // Exponent
          xm = ((UINT32_TYPE) hm) << 10; // Mantissa
          *xp++ = (xs | xe | xm); // Combine sign bit, exponent bits, and mantissa bits
        }
      }
      xp++; // Skip over the remaining 32 bits of the mantissa
    }
    return 0;
  }
} ;

