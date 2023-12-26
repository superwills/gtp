#ifndef STDWILUTIL_H
#define STDWILUTIL_H

//cstdlib
#include <stdio.h>
#include <stdarg.h>
#include <ctime>
#include <cmath>
#include <string>
using namespace std;

// win o/s
#include <windows.h>

//stl
#include <vector>
#include <list>

#include "MersenneTwister.h" // for randInt(), randFloat()
#include "RichEditCtrl.h" // for the rich text window
class RichEdit ;
union Vector ;
namespace swu
{
  // FLOAT=SINGLE.  File reading will screw up
  // if you use wrong %f to read in a float
  // %f=(read in single) %lf=(read in double)

  // single precision is not good. It IS actually marginally slower
  // than double on x64, believe it or not, and 
  #define real double
  //#define SINGLE_PRECISION

  // Undef this if you don't want to use intel m256d optimizations
  //#define INTEL_OPT

  #ifdef SINGLE_PRECISION
  #define M256 __m256
  #define MMCMD(x) x##s
  #else
  #define M256 __m256d
  #define MMCMD(x) x##d
  #endif
  
  const static real sqrt2 = sqrt(2.0) ;
  const static real sqrt3 = sqrt(3.0) ;
  const static real INV_sqrt2 = 1.0 / sqrt(2.0) ;
  const static real INV_sqrt3 = 1.0 / sqrt(3.0) ;

  const static real sqrt3over2 = sqrt(3.0)/2.0;  

  #define CUBE(x) ( (x)*(x)*(x) )
  #define SQUARE(x) ( (x)*(x) )
  #define SQ(x) ( (x)*(x) )

  #define IsNaN(x) (x!=x)
  #define IsFinite(x) (x <= DBL_MAX && x >= -DBL_MAX)
  #define IsInfinite(x) (x > DBL_MAX || x < -DBL_MAX)
  #define IsNear(a,b,EPS_MIN) (fabs((a)-(b))<EPS_MIN)
  #define BetweenIn(var,min,max) (((min)<=var)&&(var<=(max)))
  #define BetweenEx(var,min,max) (((min)<var)&&(var<(max)))

  // "help" var be between min and max, with EPS_MIN,
  // or bend the conditions, so they are EPS_MIN more lax
  #define BetweenApprox(var,min,max) ( (min-EPS_MIN) <= (var) && (var) <= (max+EPS_MIN) )
  // if you want, you can also "pad" a condition with a negative EPS_MIN

  #define InRect( x,y, xMin,yMin, xMax,yMax ) (BetweenIn(x,xMin,xMax)&&BetweenIn(y,yMin,yMax))
  #define SameSign(x,y) ( (x >= 0 && y >= 0) || (x < 0 && y < 0) )

  // some global macros
  // DELETE THIS MACRO in favor of `for( auto x : list )`
  #define foreach( listPtrType, listPtr, listInstance ) for( listPtrType listPtr = listInstance.begin(); listPtr != listInstance.end(); ++listPtr )  
  #define PI 3.1415926535897932384626433832795
  #define π 3.1415926535897932384626433832795 // ALT+227=π
  
  #define DEGTORADCONST 0.017453292519943295769236907684886
  #define RADTODEGCONST 57.295779513082320876798154814105
  #define RADIANS(degreeMeasure) (degreeMeasure*DEGTORADCONST)
  #define DEGREES(radianMeasure) (radianMeasure*RADTODEGCONST)
  #define DESTROY(OBJ) if(OBJ){delete (OBJ); (OBJ)=0;}
  #define DESTROY_ARRAY(ARR) if(ARR){delete[] (ARR); (ARR)=0;}
  #define OVERWRITE(OBJ,WITHME) {if(OBJ) delete(OBJ); (OBJ)=WITHME;}

  #define IS_KEYDOWN(x) (GetAsyncKeyState(x)&0x8000)
  #define KEY_DOWN_MASK 0x80
  #define KEY_TOGGLE_MASK 0x1

  #define KEY_IS_DOWN(KeyStateArray,VK) (KeyStateArray[VK]&KEY_DOWN_MASK)
  #define KEY_IS_UP(KeyStateArray,VK) (!KEY_IS_DOWN(KeyStateArray,VK))

  

  // enumerated vals are really int but they
  // choke the compiler when you try to ++ them or do other bitwise ops on them
  //#define inc(enumVal) (*((int*)&enumVal)++)//boring
  #define containsFlag( val, flag ) ( val & flag )
  #define notContainsFlag( val, flag ) ( !(val & flag) )

  #define addFlag( val, flag ) ( val |= flag )
  #define removeFlag( val, flag ) ( val &= ~flag )
  #define toggleFlag( val, flag ) ( val ^= flag )

  // cycles a flag's values (to maxVal, then returns to 0)
  //#define cycleFlag( val, lowValAcceptable, maxValAcceptable ) val++; if(val>maxValAcceptable) val=lowValAcceptable ;
  //#define decycleFlag( val, lowValAcceptable, maxValAcceptable ) val--; if(val<lowValAcceptable) val=maxValAcceptable ;
  #define cycleFlag( val, MINVAL, MAXVAL ) ( ++val>MAXVAL?val=MINVAL:val )
  #define decycleFlag( val, MINVAL, MAXVAL ) ( --val<MINVAL?val=MAXVAL:val )

  #define every(XVariable,repTime) (!(XVariable%repTime))

  // A one-time only RUNTIME DEBUG OUTPUT warning that tells you not to use a certain function.
  // Used for inefficient functions - so usually the function this deprecates won't be impacted by the perm insertion of
  // if( !warned ) check.
  #define __DEPRECATED_FUNCTION_WARNING__ static bool warned=false; if( !warned ) { warning( "%s: This is an extremely inefficient function and you should stop using it. Use # %d. You have been warned", __FUNCTION__, __COUNTER__+1 ) ; warned = true ; }

  /// get an indexer for a 1d array
  /// indexed by 2d.  col + (row*width)
  /// because rrow*wwidth is cols to skip
  ///////////////////#define AT(WIDTH,ROW,COL) (COL+ROW*WIDTH)

  #define SAFE_RELEASE(ptr) if(ptr) { ptr->Release(); ptr = NULL; }
  #define CAST_AS_DWORD(x) *((DWORD*)&x)

  #define ALPHA_FULL 0xff000000

  /// b must exist.
  #define cstrfree(str) if(str){free(str);str=0;}

  /// Advance your str pointer to the next word
  /// in the string.  If there is no next word,
  /// you will get a NULL
  #define cstrNextWord(str) {str=strchr(str, ' '); if(str){while( (*str) && isspace(*str) ){++str;} if(!(*str)){str=0;}}}
  //                         1. find next ' '      2.  if there was a ' ', adv past all whitespace   3. if nothing but white space null out str

  // R S T 0
  // 0 1 2 3
  // len is 3
  // str[len] = 0 does nothing
  // str[len-1] sets last char to 0
  /// nulls the newline at the end of a string.
  /// we have to use strrchr to make sure its safe
  #define cstrnulllastnl(str) {char* nl=strrchr(str,'\n'); if(nl){*nl=0;}}
  #define cstrnullnextsp(str) {char* nl=strchr(str,' '); if(nl){*nl=0;}}
  //#define cfilereadbinary(dstPtr,filename) { FILE*____file=fopen(filename,"rb");if(____file){fseek(____file,0,SEEK_END);int ____fileSIZE=ftell(____file); dstPtr=malloc(____fileSize);rewind(file);fread(dstPtr,____fileSize,1,____file);fclose(____file);}else{printf("Can't open '%s'\n",filename);} }

  /// returns TRUE of a character x
  /// is a whitespace character
  ///    9 == horizontal tab
  ///   10 == line feed,
  ///   11 == "vertical tab" (not that anyone uses it..)
  ///   13 == carriage return
  ///   32 == space
  #define IS_WHITE(x) (x==9 || x==10 || x==11 || x==13 || x==32)

  #define WARN_ONCE(s) {static bool _wwarned = false ; if( !_wwarned ){ warning( s ) ; _wwarned = true ; }}

  int cfilesize(FILE* f);

  /// Gets you a random real between
  /// 'a' and 'b'
  real randFloat( real a, real b ) ;
  //
  /// Gets you a random real
  /// between 0.0 and 1.0
  real randFloat() ;

  /// Gets you a random int
  /// on [low,high)
  int randInt( int low, int high ) ;

  // randomly -1 or +1
  int randSign() ;

  void randSeed( UINT seed ) ;

  real interpolateAngle( real t1, real t2 ) ;

  extern real *facs ;
  void prepFactorials() ;
  //real factorial( int n ) ;

  int sprintNow( char* buf ) ;

  bool test( bool shouldBeTrue, char *messagePass, char *messageFail ) ;

  // so simple and so essential for clean coding
  // named 2 and 3 to avoid conflicts with lame versions
  // of "min" and "max" in winapi/stl
  template<typename T> inline T min2( const T& a, const T& b )
  {
    if( a < b )  return a ;
    else return b ;
  }

  template<typename T> inline T min3( const T& a, const T& b, const T& c )
  {
    if( a <= b && a <= c )  return a ;
    else if( b <= a && b <= c ) return b ;
    else return c ;
  }
  
  template<typename T> inline T max2( const T& a, const T& b )
  {
    if( a > b )  return a ;
    else return b ;
  }

  template<typename T> inline T max3( const T& a, const T& b, const T& c )
  {
    if( a >= b && a >= c )  return a ;
    else if( b >= a && b >= c ) return b ;
    else return c ;
  }

  template<typename T>
  bool contains( const std::vector<T> & container, const T & item )
  {
    for( int i = 0 ; i < container.size() ; i++ )
      if(container[i]==item)
        return true ;

    return false ;
  }

  template<typename T>
  int indexOf( const std::vector<T> & container, const T & item )
  {
    for( int i = 0 ; i < container.size() ; i++ )
      if(container[i]==item)
        return i ;

    return -1 ;
  }

  template<typename T>
  bool removeItem( std::vector<T> & container, const T & item )
  {
    for( int k = 0 ; k < container.size() ; k++ )
    {
      if( container[k] == item )
      {
        container.erase( container.begin() + k ) ;
        return true ;
      }
    }

    return false ;
  }

  template<typename T>
  T lerp( T a, T b, T t )
  {
    return a + ( b - a )*t ;
  }

  template<typename T>
  T& clamp( T &num, T low, T high )
  {
    if( num < low )
      num = low ;
    if( num > high )
      num = high ;
    return num ;
  }

  template<typename T>
  T clampedCopy( T num, T low, T high )
  {
    if( num < low )
      num = low ;
    if( num > high )
      num = high ;
    return num ;
  }

  template<typename T>
  T& wrap( T &num, T low, T high )
  {
    // you can do this using modulus too
    // int range = high-low+1; int spill=(num-low)%range ; return low+spill ;
    if( num > high )
      num = low ;
    return num ;
  }

  inline bool IsPow2( int x )
  {
    return x && ((x & (x-1))==0) ;
  }
  
  inline unsigned int log2( unsigned int x )
  {
    unsigned int ans = 0 ;
    while( x>>=1 ) ans++;
    return ans ;
  }
  
  inline unsigned int hibit( unsigned int x )
  {
    unsigned int log2Val = 0 ;
    while( x>>=1 ) log2Val++;  // eg x=63 (111111), log2Val=5
    return  (1 << log2Val) ; // finds 2^5=32
  }

  inline unsigned int NextPow2( unsigned int x )
  {
    x--;
    unsigned int log2Val = 0 ;
    while( x>>=1 ) log2Val++; 
    return (1 << (log2Val+1)) ;
  }

  inline unsigned int TwoToThe( unsigned int n )
  {
    return 1 << n ;
  }

  // don't provide this as it is provided in std::swap already
  //template<typename T>
  //inline void swap( T &a, T &b )
  //{
  //  T temp = a ;
  //  a = b ;
  //  b = temp ;
  //}

  inline char nextSpin( char current, bool ccw )
  {
    switch( current )
    {
    case '/':
      if( ccw ) return '|' ; else return '-' ;
    case '|':
      if( ccw ) return '\\' ; else return '/' ;
    case '\\':
      if( ccw ) return '-' ; else return '|' ;
    case '-':
    default:
      if( ccw ) return '/' ; else return '\\' ;
    }
  }

  wchar_t* getUnicode( const char* ascii ) ;

  int cStrWrite( FILE* file, const char* str, int fixedLen ) ;
  char* cStrRead( FILE* file, int fixedLen ) ;
  string& cStrRead( string& saveIntoStr, FILE* file, int fixedLen ) ;

  // Its considered a good practice to use
  // an object that locks on construct, releases on
  // destruct because your call to unlock can be missed
  // if someone does an early return or something
  struct MutexLock
  {
    HANDLE mutexLock ;

    MutexLock( HANDLE mutexToLock, DWORD waitTime ) ;
    
    // Release in dtor
    ~MutexLock() ;
  } ;
} // namespace swu


// Use a namespace as a compromise
// between functions in global namespace
// and a static class.
namespace Console
{
  
  const DWORD Black = ( BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY ) ;
  const DWORD White = ( FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY ) ;
  const DWORD Gray = ( FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE ) ;
  const DWORD DkGray = ( FOREGROUND_INTENSITY ) ;
  const DWORD Red = ( FOREGROUND_RED | FOREGROUND_INTENSITY ) ;
  const DWORD Green = ( FOREGROUND_GREEN | FOREGROUND_INTENSITY ) ;
  const DWORD Blue = ( FOREGROUND_BLUE | FOREGROUND_INTENSITY ) ;
  const DWORD Yellow = ( FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY ) ;
  const DWORD Magenta = ( FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY ) ;
  const DWORD Cyan = ( FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY ) ;
  const DWORD DkRed = ( FOREGROUND_RED ) ;
  const DWORD DkGreen = ( FOREGROUND_GREEN ) ;
  const DWORD DkBlue = ( FOREGROUND_BLUE ) ;
  const DWORD DkYellow = ( FOREGROUND_RED | FOREGROUND_GREEN ) ;
  const DWORD DkMagenta = ( FOREGROUND_RED | FOREGROUND_BLUE ) ;
  const DWORD DkCyan = ( FOREGROUND_GREEN | FOREGROUND_BLUE ) ;

  const DWORD RichColors[ 16 ] = {
    RGB( 0,0,0 ),
    RGB( 0,0,128 ),  // 1 B
    RGB( 0,128,0 ),  // 2=G
    RGB( 0,128,128 ),  // 3=Teal
    RGB( 128,0,0 ),
    RGB( 128,0,128 ),
    RGB( 128,128,0 ),
    RGB( 128,128,128 ),
    
    RGB( 180,180,180 ),
    RGB( 0,0,255 ),
    RGB( 0,255,255 ),
    RGB( 0,255,255 ),
    RGB( 255,0,0 ),
    RGB( 255,0,255 ),
    RGB( 255,255,0 ),
    RGB( 255,255,255 ),

  } ;

  // 2 options:
  //  1) Make everything flush by the main thread.
  //     this is bad because the main thread is doing something else (RUNNING THE PROGRAM!!)
  //     you'd have to insert.. a call on the mainthread to log() or something to have it jog
  //     the log, and if ONLY the extra threads output then its possible you won't ever flush
  //     until shutdown.

  //  2) create a thread specifically for the console.  this is the option I choose.
  //     !!!!! Aug 14 2011 This ended up NOT WORKING when if you want to
  //                       log to a GUI window.
  extern char *progname ;

  //extern DWORD  logThreadId ;    // stopped using since richtext
  //extern HANDLE logThreadHandle ;// stopped using since richtext

  // A structure for buffering log messages.
  struct LogMessage
  {
    char *msg ;
    int logLevel ;
    DWORD color ;
    DWORD richColor ;
    LogMessage( int iLogLevel, DWORD iColor, char *iMsg ) ;

    void flush() ;

    ~LogMessage() ;
  } ;

  extern std::vector<LogMessage*> logMessages ;
  
  static HWND hwnd ;
  static HANDLE handle ;

  /// Last color used by console so
  /// it can be reverted back one step
  static WORD colorUsedLastCall, colorUsed2CallsAgo ;

  /// This class should be a singleton.
  /// An application can have only 1 console.
  void initConsole() ;

  void setRowsAndCols( int rows, int cols ) ;

  // Sets the console's size in PIXELS.
  // This function has no effect on # rows
  // and cols the console has.  Use setRowsAndCols
  // for that.
  void setSizeInPixels( int pixelsWidth, int pixelsHeight ) ;

  // moves the console to x, y location,
  // x measured from LEFT EDGE of screen
  // y measured from TOP EDGE of screen
  void moveTo( int x, int y ) ;

  // Makes the console the topmost window
  void makeTopmostWindow() ;

  void setColor( WORD color );

  void revertToLastColor() ;

  /// Logging.
  enum ErrorLevel
  {
    Error   = 1 << 0,
    Warning = 1 << 1,
    Info    = 1 << 2
  } ;

  extern FILE *logFile ;
  extern RichEdit* logRichText ;
  extern char *logOutputFilename ;

  // Variables that make it so we can
  // only output ERROR and WARNING messages
  // to the file, but output ALL types of
  // ERROR, WARNING, INFO messages to the console.
  extern int logOutputsForConsole ;
  extern int logOutputsForFile ;
  extern int logOutputsForDebugStream ;
  extern int logOutputsForRichText ;
  //extern bool logging, loggingThreadExited ;

  // mutex for log writing, to make it thread safe
  static HANDLE mutexLog ;

  void logStartup( char *outputFilename, char *runningProgramName, int windX, int windY, int windWidth, int windHeight ) ;
  void logflush() ;
  void logShutdown() ;

  tm* getCurrentTime() ;
  char* getCurrentTimeString() ;

  void muteAll( bool muteIt ) ;

  void enqueueLog( int logLevel, DWORD color, char *msg ) ;

  /// the actual log output function
  void logDecorate( int logLevel, DWORD color, const char *fmt, va_list args ) ;

  /// Something went wrong
  /// in the program.
  void error( const char *fmt, ... ) ;

  /// Warnings are things that aren't really bad for the program,
  /// but they are things to watch out for, things you might not expect.
  void warning( const char *fmt, ... ) ;

  #ifdef _DEBUG
  #define vinfo(x) info(x)
  #else
  #define vinfo(x)
  #endif

  // info w/ color
  void info( DWORD iColor, const char *fmt, ... ) ;

  /// Just some information about things that are happening in the program.
  /// Normal, expected behavior should come in info() messages.
  void info( const char *fmt, ... ) ;

  void plain( DWORD iColor, const char *fmt, ... ) ;

  /// Logs a debug message but with no
  /// timestamp
  void plain( const char *fmt, ... ) ;

  // file dump only
  void plainFile( const char *fmt, ... ) ;

  /// Print an error message and quit the program.
  /// Calls logShutdown() before exiting.
  void bail( char *msg, bool openLog=false ) ;

  void openLogfile() ;

  void printWindowsLastError( char *msg ) ;

  inline char* onOrOff( bool v ) { return v?"on":"off"; }
}

namespace HALF
{
  #define  INT16_TYPE          short
  #define UINT16_TYPE unsigned short
  #define  INT32_TYPE          long
  #define UINT32_TYPE unsigned long

  // Prototypes -----------------------------------------------------------------
  int singles2halfp(void *target, void *source, int numel);
  int doubles2halfp(void *target, void *source, int numel);
  int halfp2singles(void *target, void *source, int numel);
  int halfp2doubles(void *target, void *source, int numel);
} ;

using namespace swu ;
using namespace Console ;
using namespace HALF ;

#endif
