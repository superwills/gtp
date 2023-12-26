#ifndef WAVELET_TRANSFORM_H
#define WAVELET_TRANSFORM_H

#include "../util/StdWilUtil.h"
#include "Vector.h"
#include "IndexMap.h"

// an easy to use class that
// puts all wavelet transforms here

// T MUST HAVE DEFINE A CONSTRUCTOR THAT
// CREATES BASED ON INTEGER 0

// T MUST DEFINE operator+, operator/ etc
// T MUST DEFINE abs( const T& val )

// these are static functions so the pointers
// are c pointers
//template <typename T>
//typedef void (Wavelet::*WaveletFunction)( T*, int, int ) ;

//template typedefs are illegal.
//template <typename T>
//typedef (*WaveletFunction)( T*, int, int ) ;

// simulate using #define
//#define WaveletFunction(T) void (*NAME)( T*, int, int )

//template <typename T>
struct Wavelet
{
  static const real d4normEvens ;
  static const real d4normOdds ;
  
  // note ( √3 + 1 ) / √2 == 1/(( √3 - 1 ) / √2)
  static const real d4unNormEvens ;
  static const real d4unNormOdds ;
  
  static const real d4c1 ;
  static const real d4c2 ;

  // the actual functions are all lowercase (C)
  // and the names start with uppercase (proper names)
  enum WaveletType
  {
    // Sumdiff is like Haar only without magnitude correction factors
    // Linear "predicts" the signal increases linearly
    SumDiff, Linear, Haar, D4, D6,
    CDF22, CDF24, CDF26,
    CDF31, CDF33, CDF35
  } ;

  // <return type of returned fp> (*functionName( parameters of function ))( params of returned fp )
  // because of the tie up, it's actually more awkward to use a typedef, better off using #define.
  template <typename T>
  static void (*getWaveletFunction( WaveletType type ))( T*, int, int )
  {
    switch( type )
    {
      case WaveletType::SumDiff:
        return &Wavelet::sumdiff<T> ;
      case WaveletType::Linear:
        return &Wavelet::linear<T> ;
      default: // default is haar, you get a warning
        warning( "No such transform" ) ;
      case WaveletType::Haar:
        return &Wavelet::haar<T> ;
      case WaveletType::D4:
        return &Wavelet::d4<T> ;
      case WaveletType::D6:
        return &Wavelet::d6<T> ;
      case WaveletType::CDF22:
        return &Wavelet::cdf_22<T> ;
      case WaveletType::CDF24:
        return &Wavelet::cdf_24<T> ;
      case WaveletType::CDF26:
        return &Wavelet::cdf_26<T> ;
      case WaveletType::CDF31:
        return &Wavelet::cdf_31<T> ;
      case WaveletType::CDF33:
        return &Wavelet::cdf_33<T> ;
      case WaveletType::CDF35:
        return &Wavelet::cdf_35<T> ;
    }
  }

  template <typename T>
  static void (*getUnwaveletFunction( WaveletType type ))( T*, int, int )
  {
    switch( type )
    {
      case WaveletType::SumDiff:
        return &Wavelet::unsumdiff<T> ;
      case WaveletType::Linear:
        return &Wavelet::unlinear<T> ;
      default: // default is haar, you get a warning
        warning( "No such transform" ) ;
      case WaveletType::Haar:
        return &Wavelet::unhaar<T> ;
      case WaveletType::D4:
        return &Wavelet::und4<T> ;
      case WaveletType::D6:
        return &Wavelet::und6<T> ;
      case WaveletType::CDF22:
        return &Wavelet::uncdf_22<T> ;
      case WaveletType::CDF24:
        return &Wavelet::uncdf_24<T> ;
      case WaveletType::CDF26:
        return &Wavelet::uncdf_26<T> ;
      case WaveletType::CDF31:
        return &Wavelet::uncdf_31<T> ;
      case WaveletType::CDF33:
        return &Wavelet::uncdf_33<T> ;
      case WaveletType::CDF35:
        return &Wavelet::uncdf_35<T> ;
    }
  }

  template <typename T>
  static void (*getOneLevelWaveletFunction( WaveletType type ))( T*, int, int, int )
  {
    switch( type )
    {
      case WaveletType::CDF31:
        return &Wavelet::cdf_31OneLevel<T> ;
      case WaveletType::D4:
        return &Wavelet::d4OneLevel<T> ;
      case WaveletType::Haar:
      default:
        return &Wavelet::haarOneLevel<T> ;
    }
  }

  template <typename T>
  static void (*getOneLevelUnwaveletFunction( WaveletType type ))( T*, int, int, int )
  {
    switch( type )
    {
      case WaveletType::CDF31:
        return &Wavelet::uncdf_31OneLevel<T> ;
      case WaveletType::D4:
        return &Wavelet::und4OneLevel<T> ;
      case WaveletType::Haar:
      default:
        return &Wavelet::unhaarOneLevel<T> ;
    }
  }

  WaveletType waveletType ;

  // Whether or not to
  // call the "normalizing" version of
  // each function.
  // you can choose to turn OFF the
  // normalizing multiplies and divides
  // if the signal is being distorted by them
  // (which it does happen for high compression)
  bool normalizationEachStep ;

  // pads 's' to contain 
  template<typename T> static void pad( vector<T>& signal )
  {
    // pads 's' to the nearest power of 2.
    int np = nextPow2( signal.size() ) ;
    signal.resize( np ) ; // resizing shouldn't invalidate previous contents
  }

  template<typename T> static T* pad( const T* signal, int signalLen )
  {
    // pads 's' to the nearest power of 2.

    // because this is a c-style array, we
    // have to reallocate it.  the original
    // isn't touched (hence CONST)
    int np = nextPow2( signalLen ) ;
    T* newSignal = new T[ np ] ;
    memset( newSignal, 0, np*sizeof(T) ); // 0 it.
  }

  enum FilterType
  {
    NearZero,
    BalancedSparsityNorm
  } ;

  // if within "threshold" units of 0, then zero it out.
  template <typename T> static int filterNearZero( T* s, int n, real threshold )
  {
    int cuts = 0 ;
    for( int i = 1 ; i < n ; i++ )
      if( IsNear( s[ i ], 0, threshold ) )
      {
        s[ i ] = 0 ;
        cuts++ ;
      }

    return cuts ;
  }

  // Specialize the template for Vector b/c of the components in the vector
  static Vector filterNearZero( Vector* s, int n, real threshold )
  {
    Vector cuts = 0 ;
    for( int i = 1 ; i < n ; i++ )
      for( int comp = 0 ; comp < 3 ; comp++ )
        if( IsNear( s[ i ].e[ comp ], 0, threshold ) ) { s[ i ].e[ comp ] = 0 ; ++cuts.e[comp] ; }

    return cuts ;
  }

  // WEIGHTS is a collection of filter weights.
  // multiresolution analysis by amping / attenuating
  // certain components of the xform.
  // Use the standard lifting wavelet transform walk functions,
  // but at each stage, instead of sum/differencing, use
  // the multiplication factor instead.
  template<typename T>
  static void filter( T* s, int n, int sp, vector<real> weights )
  {
    // check there are not more weights than levels (you can do log2( n ) levels)
    int availableLevels = log2( n ) ;
    
    if( weights.size() != availableLevels )
    {
      error( "You have %d weights, but I need %d of them, nothin' doin'.", weights.size(), availableLevels ) ;
      return ;
    }

    // log_2(i) is the "level".
    for( int i = 1 ; i <= n/2 ; i *= 2 )
    {
      int level = log2( i ) ;

      // j is a pass through at level i
      for( int j = 0 ; j <= n/2 - i ; j += i )
      {
        // IODD is the difference coefficient for that level
        s[IODD] *= weights[ level ] ; // __i__ is the LEVEL.
      }
    }
  }

  template<typename T>
  static void filter2d( T* s, int rows, int cols, vector<real> weights )
  {
    // filter rows, then the cols.
    for( int row = 0 ; row < rows ; row++ )
      filter<T>( &s[ row*cols ], cols, 1, weights ) ;
    for( int col = 0 ; col < cols ; col++ )
      filter<T>( &s[ 0 + col ], rows, cols, weights ) ;
  }

  #pragma region filter percentage
  template <typename T> static int filterPercentage( T* s, int n, int sp, double ifBelowThisPercentOfMaxCutItOff )
  {
    int cutCount = 0 ;
      
    // find the max val
    T maxVal = 0 ;
    for( int i = 1 ; i < n ; i++ ) // VALUE @ 0 is the AVG, don't count it in max
      if( abs(s[sp*i]) > maxVal ) maxVal=abs(s[sp*i]);

    T minValToSurvive = maxVal*minPerc ; // so the maximum value allowed is (minPerc) of max.
    info( "Max component is %f. Min val req'd to survive is %f\n", max, minValToSurvive ) ;

    // cut off
    for( int i = 1 ; i < n; i++ )
      if( abs(s[sp*i]) < minValToSurvive ) { s[sp*i]=0; cutCount++ ; } // cut off

    return cutCount ; // # items you cut
  }

  static Vector filterPercentage( Vector* s, int n, int sp, double ifBelowThisPercentOfMaxCutItOff )
  {
    Vector cutCounts ; //rgb cut counts
      
    // find the max val
    Vector maxVals ; // max in 3 channels
    for( int i = 1 ; i < n ; i++ ) // VALUE @ 0 is the AVG, don't count it in max
      for( int comp = 0 ; comp < 3 ; comp++ )  // do this in 3 channels
        if( abs( s[sp*i].e[comp] ) > maxVals.e[comp] )
          maxVals.e[comp]=abs(s[sp*i].e[comp]);

    Vector minValToSurvive = maxVals*ifBelowThisPercentOfMaxCutItOff ; // so the maximum value allowed is (minPerc) of max.
    ///info( "Max components are %f %f %f. Min vals req'd to survive are %f percent of that, or %f %f %f\n",
    ///  maxVals.x, maxVals.y, maxVals.z,
    ///  ifBelowThisPercentOfMaxCutItOff,
    ///  minValToSurvive.x, minValToSurvive.y, minValToSurvive.z ) ;

    // cut off
    for( int i = 1 ; i < n; i++ )
      for( int comp = 0 ; comp < 3 ; comp++ )
        if( abs( s[sp*i].e[comp] < minValToSurvive.e[comp] ) ) {
          s[ sp*i ].e[comp] = 0 ;
          ++cutCounts.e[comp] ;
        }
    
    ///info( "Cut red=%.0f/%d green=%.0f/%d blue=%.0f/%d", cutCounts.x,n,cutCounts.y,n,cutCounts.z,n ) ;
    return cutCounts ; // # items you cut
  }

  static Vector filterPercentage2d( Vector* s, int rows, int cols, double ifBelowThisPercentOfMaxCutItOff )
  {
    // filter rows, then the cols.
    for( int row = 0 ; row < rows ; row++ )
      filterPercentage( &s[ row*cols ], cols, 1, ifBelowThisPercentOfMaxCutItOff ) ;
    for( int col = 0 ; col < cols ; col++ )
      filterPercentage( &s[ 0 + col ], rows, cols, ifBelowThisPercentOfMaxCutItOff ) ;

    // the only way to find the cut percentage is to count zeros,
    // because you'd either double count by using returns
    // from the above, OR
    // you'd have to insert if statements in the loop above (if it wasn't already 0.. increase cut count.)
    Vector cutCount ;
    for( int i = 0 ; i < rows*cols ; i++ )
      for( int comp = 0 ; comp < 3 ; comp++ )
        if( s[i].e[comp] == 0 )
          ++cutCount[comp] ;

    return cutCount ;
  }
  #pragma endregion

  #pragma region macros
  #define IEVEN (sp*(2*j))
  #define IPREVEVEN (sp*(2*j - 2*i))
  #define IPREVPREVEVEN (sp*(2*j - 4*i))
  #define IPREVPREVPREVEVEN (sp*(2*j - 6*i))
  #define INEXTEVEN (sp*(2*j + 2*i))
  #define INEXTNEXTEVEN (sp*(2*j + 4*i))
  #define INEXTNEXTNEXTEVEN (sp*(2*j + 6*i))
    
  #define IODD (sp*(2*j + i))
  #define IPREVODD (sp*(2*j - i))
  #define IPREVPREVODD (sp*(2*j - 3*i))
  #define IPREVPREVPREVODD (sp*(2*j - 5*i))
  #define INEXTODD (sp*(2*j + 3*i))
  #define INEXTNEXTODD (sp*(2*j + 5*i))
  #define INEXTNEXTNEXTODD (sp*(2*j + 7*i))

  // i is the "level".
  // max 8 levels for n=512:
  // 1, 2, 4, 8, 16,  32, 64, 128, 256
  // log_2(i) is the "level".
  #define waveletLoopI for( int i = 1 ; i <= n/2 ; i *= 2 )

  // j is a pass through at level i
  #define waveletLoopJ for( int j = 0 ; j <= n/2 - i ; j += i )

  // if shaveBegin is 1, you skip 1 element at the beginning.
  // if shaveEnd is 1, you skip 1 element at the end.
  // This is used for loops that use the PREV element,
  // so they will out of bounds access if you start
  // at the very first element.
  // Remember, there are i elements between adjacent elements.
  // If you are going to do PREVEVEN, you need to shave __2__ from begin,
  // PREVEVEN: begin 2 (2*j-2*i)
  // PREVODD:  begin 1 (2*j-i)
  // NEXTEVEN: end 1 (2*j+2*i)  // -i is already done so the odd doesn't go oob.
  // NEXTODD:  end 2 (2*j+3*i)
  #define waveletLoopJLTD(shaveBegin,shaveEnd) for( int j = shaveBegin*i ; j <= n/2 - i - shaveEnd*i; j += i )

  #define unwaveletLoopI for( int i = n/2 ; i >= 1 ; i /= 2 )
  #define unwaveletLoopJ for( int j = n/2 - i; j >= 0 ; j -= i )
  
  #define unwaveletLoopJLTD(shaveBegin,shaveEnd) for( int j = n/2 - i - i*shaveEnd ; j >= i*shaveBegin ; j -= i )
  
  #define SAFE_PREV(s,INDEX) ((INDEX>=0)?s[INDEX]:0)
  #define SAFE_NEXT(s,INDEX) ((INDEX<n)?s[INDEX]:0)
  #pragma endregion

  #pragma region transformations

  #pragma region sumdiff transforms
  template<typename T> static void sumdiff( T*s, int n, int sp )
  {
    waveletLoopI
    {
      waveletLoopJ
      {
        s[ sp*( j + i ) ] -= s[ sp*( j ) ] ;
        s[ sp*( j ) ] += s[ sp*( j + i ) ]/2 ;
      }
    }
  }
  template<typename T> static void unsumdiff( T*s, int n, int sp )
  {
    unwaveletLoopI
    {
      unwaveletLoopJ
      {
        s[ sp*( j ) ] -= s[ sp*( j + i ) ]/2 ;
        s[ sp*( j + i ) ] += s[ sp*( j ) ] ;
      }
    }
  }

  // THIS IS THE WAVELET TRANSFORM
  // windowSize IS DATALEN right now
  // does not handle non-pow2 datalen
  template <typename T> static void dupsumdiff( int n, const T* s, T* wavelet )
  {
    // make a copy of s in t2
    T* t2 = new T[ n ] ;
    memcpy( t2, s, n*sizeof(T) ) ;

    // i tells how much of the original array
    // to traverse.  this halfs on each iteration. (n=8, i=8,4,2)
    for( int i = n ; i > 1 ; i /= 2 )
    {
      // j walks until i, doing sum and differencing operations
      // on each PAIR.
      for( int j = 0 ; j < i; j += 2 )
      {
        // j counts from 0 .. 4, then 0 .. 2, then 0 .. 1
        T avg = (t2[j] + t2[j+1])/2.0 ;
        T diff = t2[j] - avg ;
        wavelet[ j/2 ] = avg ; // FRONT half saves averages
        wavelet[ i/2 + j/2 ] = diff ; // BACK HALF saves differences
      }

      // after finish forming wav, update t2 as new starting buffer
      memcpy( t2, wavelet, i*sizeof(T) ) ; // only need to copy i elts, NOT entire array each time (2nd half not modified)
    }

    DESTROY_ARRAY( t2 ) ;
  }

  // undo the sum/diff operation
  // to get back the original signal
  template <typename T> static void dupunsumdiff( int n, const T* wav, T* unwav )
  {
    T* t2 = new T[ n ] ;
    memcpy( t2, wav, n*sizeof(T) ) ;
    for( int i = 1 ; i < n ; i *= 2 )
    {
      // j walks by 1's because it is
      // walking on the summed elements
      for( int j = 0 ; j < i ; j++ )
      {
        // undo the w_sumdiff transform of earlier
        // step on every transformed element
        T avg = t2[j];
        T diff = t2[j+i];
        unwav[ 2*j ] = avg + diff ;
        unwav[ 2*j + 1 ] = avg - diff ;
      }

      memcpy( t2, unwav, i*2*sizeof(T) ); //copy i*2 elts (# produced)
    }

    DESTROY_ARRAY( t2 ) ;
  }

  // 2d std means do all the rows,
  // then do all the columns.
  // nonstd (which is actually STD USE in the literature)
  // is row, col, row, col.
  template <typename T> static void dupsumdiff2dnonstd( int rows, int cols, const T* s, T* wavelet )
  {
    // assumes "wavelet" is same size as s.
    // make a copy of s in t2
    T* t2 = new T[ rows*cols ] ;
    memcpy( t2, s, rows*cols*sizeof(T) ) ;
    
    // i tells how much of the original array
    // to traverse.  this halfs on each iteration. (n=8, i=8,4,2)
    // i is LEVEL COUNTER.  When there is ONE element left, you stop.
    //for( int i = rows ; i > rows/2 ; i /= 2 )
    for( int i = rows ; i > 1 ; i /= 2 )
    {
      ///*
      // cols
      for( int k = 0 ; k < i ; k++ )
      {
        // now do the cols.
        // the offset to the current col is 'col'.
        // you have to SKIP 'rows' elements to
        // get to the next element in the COLUMN.
        // j walks until i, doing sum and differencing operations on each PAIR.
        int off = k ;
        for( int j = 0 ; j < i; j += 2 )
        {
          // j counts from 0 .. 4, then 0 .. 2, then 0 .. 1
          wavelet[ off + rows*j/2 ] = (t2[off + rows*j] + t2[off + rows*(j+1)])/2.0 ; // get avg.  // FRONT half saves averages
          wavelet[ off + rows*i/2 + rows*j/2 ] = t2[off + rows*j] - wavelet[ off + rows*j/2 ] ; // first comp - avg.  // BACK HALF saves differences

          // normalization
          //wavelet[ off + rows*j/2 ] *= sqrt2 ;
          //wavelet[ off + rows*i/2 + rows*j/2 ] *= INV_sqrt2 ;
        }
      
        // can't use memcpy here because they're not in order
        // (must skip "rows" elements between column entries)
        for( int elt = 0 ; elt < i ; elt++ )
          t2[ off + rows*elt ] = wavelet[ off + rows*elt ] ;
      }
      //*/
      
      ///*
      // ROWS
      // wavelet transform "i" elements of each row.
      for( int k = 0 ; k < i ; k++ )
      {
        // the offset to the current row is row*cols.
        int off = k*cols ; // cols elts per row, so need to skip k*cols elts to get to the beginning of the kth row

        // j walks until i, doing sum and differencing operations on each PAIR.
        for( int j = 0 ; j < i; j += 2 )
        {
          // wavelet[off] takes you to the current row.
          // wavelet[off + j/2] takes you to the first half of the current row.
          // j counts from 0 .. 4, then 0 .. 2, then 0 .. 1
          wavelet[ off + j/2 ] = (t2[off + j] + t2[off + j+1])/2.0 ; // get avg.  // FRONT half saves averages
          wavelet[ off + i/2 + j/2 ] = t2[off + j] - wavelet[ off + j/2 ] ; // first comp - avg.  // BACK HALF saves differences
        }

        // copy the new avgs into the t2 buffer, from 'offset' to 'offset + i'
        memcpy( t2 + off, wavelet + off, i*sizeof(T) ) ; // only need to copy i elts, NOT entire array each time (2nd half not modified)
      }
      //*/
      
    }

    DESTROY_ARRAY( t2 ) ;
  }

  // undo the sum/diff operation
  // to get back the original signal
  template <typename T> static void dupunsumdiff2dnonstd( int rows, int cols, const T* wav, T* unwav )
  {
    T* t2 = new T[ rows*cols ] ;
    memcpy( t2, wav, rows*cols*sizeof(T) ) ;
    
    // i is the level / num elts to traverse.
    // increase the starting value of i to
    // untransform fewer "times".
    //for( int i = rows/2 ; i < rows ; i *= 2 )
    for( int i = 1 ; i < rows ; i *= 2 )
    {
      // do col, row, col, row
      
      // ROWS
      for( int k = 0 ; k < 2*i ; k++ )
      {
        // the offset to the current row is row*cols.
        int off = k*cols ;

        for( int j = 0 ; j < i ; j++ )
        {
          // undo the w_sumdiff transform of earlier
          // step on every transformed element
          unwav[ off+ 2*j ] = t2[off+j] + t2[off+j+i] ;  // avg+diff
          unwav[ off+ 2*j + 1 ] = t2[off+j] - t2[off+j+i] ; //avg-diff
        }

        memcpy( t2+off, unwav+off, 2*i*sizeof(T) ); //copy i*2 elts (# produced)
      }

      // COLS
      //for( int k = i ; k >= 0 ; k-- )
      ///*
      for( int k = 0 ; k < 2*i ; k++ )
      {
        int off = k ;
        for( int j = 0 ; j < i ; j++ )
        {
          unwav[ off+ rows*(2*j) ] = t2[off+rows*j] + t2[off+rows*(j+i)] ;  // avg+diff
          unwav[ off+ rows*(2*j + 1) ] = t2[off+rows*j] - t2[off+rows*(j+i)] ; //avg-diff
        }
        for( int elt = 0 ; elt < 2*i ; elt++ )
          t2[ off+ rows*elt ] = unwav[ off+ rows*elt ] ;
      }
      //*/
      
    }
  }

  template <typename T> static void dupsumdiffOneLevel( int n, int level, const T* s, T* wavelet )
  {
    // make a copy of s in t2
    T* t2 = new T[ n ] ;
    memcpy( t2, s, n*sizeof(T) ) ;

    // i tells how much of the original array
    // to traverse.  this halfs on each iteration. (n=8, i=8,4,2)
    // &&i is a safety that i hasn't become 0
    for( int i = n ; i >= n/level &&i; i /= 2 )
    {
      // j walks until i, doing sum and differencing operations
      // on each PAIR.
      for( int j = 0 ; j < i; j += 2 )
      {
        // j counts from 0 .. 4, then 0 .. 2, then 0 .. 1
        T avg = (t2[j] + t2[j+1])/2.0 ;
        T diff = t2[j] - avg ;
        wavelet[ j/2 ] = avg ; // FRONT half saves averages
        wavelet[ i/2 + j/2 ] = diff ; // BACK HALF saves differences
      }

      // after finish forming wav, update t2 as new starting buffer
      memcpy( t2, wavelet, i*sizeof(T) ) ; // only need to copy i elts, NOT entire array each time (2nd half not modified)
    }

    DESTROY_ARRAY( t2 ) ;
  }

  template <typename T> static void dupunsumdiffOneLevel( int n, int level, const T* wav, T* unwav )
  {
    T* t2 = new T[ n ] ;
    memcpy( t2, wav, n*sizeof(T) ) ;
    for( int i = n/level ; i < n ; i *= 2 )
    {
      for( int j = 0 ; j < i ; j++ )
      {
        // undo the w_sumdiff transform of earlier
        // step on every transformed element
        T avg = t2[j];
        T diff = t2[j+i];
        unwav[ 2*j ] = avg + diff ;
        unwav[ 2*j + 1 ] = avg - diff ;
      }

      memcpy( t2, unwav, i*2*sizeof(T) ); //copy i*2 elts (# produced)
    }

    DESTROY_ARRAY( t2 ) ;
  }
  #pragma endregion

  #pragma region linear transforms
  template<typename T> static void linear( T*s, int n, int sp )
  {
    waveletLoopI
    {
      waveletLoopJ
      {
        int nextEvenIndex = INEXTEVEN ;
        s[ IODD ] -= 0.5*( s[ IEVEN ] + SAFE_NEXT(s,nextEvenIndex) ) ;

        int prevOddIndex = IPREVODD ;
        s[ sp*( 2*j ) ] += 0.25* ( SAFE_PREV(s,prevOddIndex) + s[IODD] ) ;
      }
    }
  }

  template<typename T> static void unlinear( T*s, int n, int sp )
  {
    unwaveletLoopI
    {
      unwaveletLoopJ
      {
        int prevOddIndex = IPREVODD ;
        s[ IEVEN ] -= 0.25*( SAFE_PREV(s,prevOddIndex) + s[IODD] ) ;
          
        int nextEvenIndex = INEXTEVEN ;
        s[ IODD ] += 0.5*( s[ IEVEN ] + SAFE_NEXT( s,nextEvenIndex ) ) ;
      }
    }
  }
  #pragma endregion

  #pragma region haar transform
  // This is the function body of the HAAR transform.
  // We "paste" this into function bodies,
  // because we don't trust inlining.
  #define HAAR_TRANSFORM s[IODD]-=s[IEVEN];s[IEVEN]+=0.5*s[IODD];s[IEVEN]*=sqrt2;s[IODD]*=INV_sqrt2;
  #define UNHAAR_TRANSFORM s[IODD]*=sqrt2;s[IEVEN]*=INV_sqrt2;s[IEVEN]-=0.5*s[IODD];s[IODD]+=s[IEVEN];

  template<typename T> static void haar( T*s, int n, int sp )
  {
    waveletLoopI
    {
      waveletLoopJ
      {
        s[ IODD ] -= s[ IEVEN ] ; // PREDICT: //s[ IODD ] -= P( s[ IEVEN ] ) ; // here P(x)=1*x.
        s[ IEVEN ] += 0.5*s[ IODD ] ; // UPDATE: //s[ IEVEN ] += U( s[ IODD ] ) ; // here U(x)=0.5*x.
        
        s[ IEVEN ] *= sqrt2 ;  // Normalization
        s[ IODD ] *= INV_sqrt2 ;
        //s[ IEVEN ] *= sqrt2 ;
        //s[ IODD ] *= sqrt2 ;
      }
    }
  }

  template <typename T> static void unhaar( T* s, int n, int sp )
  {
    unwaveletLoopI
    {
      unwaveletLoopJ
      {
        s[ IODD ] *= sqrt2 ; // "UN-NORMALIZE"
        s[ IEVEN ] *= INV_sqrt2 ;
        //s[ IODD ] *= INV_sqrt2 ; // "UN-NORMALIZE"
        //s[ IEVEN ] *= INV_sqrt2 ;
        
        s[ IEVEN ] -= 0.5*s[ IODD ] ; // UN-UPDATE
        s[ IODD ] += s[ IEVEN ] ; // UN-PREDICT
      }
    }
  }

  // 2d std means do all the rows,
  // then do all the columns.
  // nonstd (which is actually STD USE in the literature)
  // is row, col, row, col.
  template <typename T> static void duphaar2dnonstd( int rows, int cols, const T* s, T* wavelet )
  {
    // assumes "wavelet" is same size as s.
    // make a copy of s in t2
    T* t2 = new T[ rows*cols ] ;
    memcpy( t2, s, rows*cols*sizeof(T) ) ;
    
    // i tells how much of the original array
    // to traverse.  this halfs on each iteration. (n=8, i=8,4,2)
    // i is LEVEL COUNTER.  When there is ONE element left, you stop.
    //for( int i = rows ; i > rows/2 ; i /= 2 )
    for( int i = rows ; i > 1 ; i /= 2 )
    {
      ///*
      // cols
      for( int k = 0 ; k < i ; k++ )
      {
        // now do the cols.
        // the offset to the current col is 'col'.
        // you have to SKIP 'rows' elements to
        // get to the next element in the COLUMN.
        // j walks until i, doing sum and differencing operations on each PAIR.
        int off = k ;
        for( int j = 0 ; j < i; j += 2 )
        {
          // j counts from 0 .. 4, then 0 .. 2, then 0 .. 1
          wavelet[ off + rows*j/2 ] = sqrt2 * .5*(t2[off + rows*j] + t2[off + rows*(j+1)]) ; // a+b.  // FRONT half saves averages
          wavelet[ off + rows*i/2 + rows*j/2 ] = sqrt2 * .5*(t2[off + rows*j] - t2[off + rows*(j+1)]) ; // a-b.  // BACK HALF saves differences
        }
      
        // can't use memcpy here because they're not in order
        // (must skip "rows" elements between column entries)
        for( int elt = 0 ; elt < i ; elt++ )
          t2[ off + rows*elt ] = wavelet[ off + rows*elt ] ;
      }
      //*/
      
      ///*
      // ROWS
      // wavelet transform "i" elements of each row.
      for( int k = 0 ; k < i ; k++ )
      {
        // the offset to the current row is row*cols.
        int off = k*cols ; // cols elts per row, so need to skip k*cols elts to get to the beginning of the kth row

        // j walks until i, doing sum and differencing operations on each PAIR.
        for( int j = 0 ; j < i; j += 2 )
        {
          // wavelet[off] takes you to the current row.
          // wavelet[off + j/2] takes you to the first half of the current row.
          // j counts from 0 .. 4, then 0 .. 2, then 0 .. 1
          wavelet[ off + j/2 ] = sqrt2 * .5*(t2[off + j] + t2[off + j+1]) ; // get avg.  // FRONT half saves averages
          wavelet[ off + i/2 + j/2 ] = sqrt2 * .5*(t2[off + j] - t2[off + j+1]) ; // diff.  // BACK HALF saves differences
        }

        // copy the new avgs into the t2 buffer, from 'offset' to 'offset + i'
        memcpy( t2+off, wavelet+off, i*sizeof(T) ) ;
        //for( int elt = 0 ; elt < i ; elt++ )
        //  t2[ off + elt ] = wavelet[ off + elt ] ;
      }
      //*/
      
    }

    DESTROY_ARRAY( t2 ) ;
  }

  // undo the sum/diff operation
  // to get back the original signal
  template <typename T> static void dupunhaar2dnonstd( int rows, int cols, const T* wav, T* unwav )
  {
    T* t2 = new T[ rows*cols ] ;
    memcpy( t2, wav, rows*cols*sizeof(T) ) ;
    
    // i is the level / num elts to traverse.
    // increase the starting value of i to
    // untransform fewer "times".
    //for( int i = rows/2 ; i < rows ; i *= 2 )
    for( int i = 1 ; i < rows ; i *= 2 )
    {
      // do col, row, col, row
      
      // ROWS
      ///*
      for( int k = 0 ; k < 2*i ; k++ )
      {
        // the offset to the current row is row*cols.
        int off = k*cols ;

        for( int j = 0 ; j < i ; j++ )
        {
          // undo the w_sumdiff transform of earlier
          // step on every transformed element
          unwav[ off+ 2*j ] = INV_sqrt2 * ( t2[off+j] + t2[off+j+i] ) ;  // avg+diff
          unwav[ off+ 2*j + 1 ] = INV_sqrt2 * ( t2[off+j] - t2[off+j+i] ) ; //avg-diff
        }

        memcpy( t2+off, unwav+off, 2*i*sizeof(T) ); //copy i*2 elts (# produced)
        //for( int elt = 0 ; elt < 2*i ; elt++ )
        //  t2[ off + elt ] *= INV_sqrt2 ;
      }
      //*/
      // COLS
      //for( int k = i ; k >= 0 ; k-- )
      ///*
      for( int k = 0 ; k < 2*i ; k++ )
      {
        int off = k ;
        for( int j = 0 ; j < i ; j++ )
        {
          unwav[ off+ rows*(2*j) ] = INV_sqrt2 * ( t2[off+rows*j] + t2[off+rows*(j+i)] ) ;   //avg+diff
          unwav[ off+ rows*(2*j + 1) ] = INV_sqrt2 * (t2[off+rows*j] - t2[off+rows*(j+i)]) ; //avg-diff
        }
        
        for( int elt = 0 ; elt < 2*i ; elt++ )
          t2[ off+ rows*elt ] = unwav[ off+ rows*elt ] ;
        // after each stage apply normalization
        //for( int elt = 0 ; elt < 2*i ; elt++ )
        //  t2[ off+ rows*elt ] *= INV_sqrt2 ;
      }
      //*/
      
      
    }
  }

  template<typename T> static void haarOneLevel( T*s, int n, int level, int sp )
  {
    // i is the level
    int i = TwoToThe( level - 1 ) ;
    waveletLoopJ
    {
      s[ IODD ] -= s[ IEVEN ] ; // PREDICT: //s[ IODD ] -= P( s[ IEVEN ] ) ; // here P(x)=1*x.
      s[ IEVEN ] += 0.5*s[ IODD ] ; // UPDATE: //s[ IEVEN ] += U( s[ IODD ] ) ; // here U(x)=0.5*x.
      
      //s[ IEVEN ] *= sqrt2 ;  // Normalization
      //s[ IODD ] *= sqrt2;
      s[ IEVEN ] *= sqrt2 ;  // Normalization
      s[ IODD ] *= INV_sqrt2 ;
    }
  }

  template <typename T> static void unhaarOneLevel( T* s, int n, int level, int sp )
  {
    int i = TwoToThe( level - 1 ) ;
    unwaveletLoopJ
    {
      s[ IODD ] *= sqrt2 ; // "UN-NORMALIZE"
      s[ IEVEN ] *= INV_sqrt2 ;
      
      //s[ IODD ] *= INV_sqrt2;   // "UN-NORMALIZE"
      //s[ IEVEN ] *= INV_sqrt2 ;
      s[ IEVEN ] -= 0.5*s[ IODD ] ; // UN-UPDATE
      s[ IODD ] += s[ IEVEN ] ; // UN-PREDICT
    }
  }
  
  
  #pragma endregion

  #pragma region d4 transforms
  template<typename T> static void d4( T*s, int n, int sp )
  {
    for( int i = 1 ; i <= n/2 ; i *= 2 )
    {
      for( int j = 0 ; j <= n/2 - i ; j += i )
        s[ IEVEN ] += sqrt3 * s[ IODD ] ;
      
      for( int j = 0 ; j <= n/2 - i ; j += i )
      {
        int prevEvenIndex = IPREVEVEN ;
        s[ IODD ] -= d4c1*s[ IEVEN ] + d4c2*SAFE_PREV(s,prevEvenIndex) ;
      }
      
      for( int j = 0 ; j <= n/2 - i ; j += i )
      {
        int nextOddIndex = INEXTODD ;
        s[ IEVEN ] -= SAFE_NEXT(s,nextOddIndex) ;
      }

      for( int j = 0 ; j <= n/2 - i ; j += i )
      {
        s[ IEVEN ] *= d4normEvens ;
        s[ IODD ] *= d4normOdds ;
      }
    }
  }

  template<typename T> static void und4( T*s, int n, int sp )
  {
    unwaveletLoopI
    {
      unwaveletLoopJ
      {
        s[ IODD ] *= d4unNormOdds ;
        s[ IEVEN ] *= d4unNormEvens ;
      }

      unwaveletLoopJLTD( 0, 2 )
        s[ IEVEN ] += s[ INEXTODD ] ;
      
      unwaveletLoopJLTD( 2, 0 )
        s[ IODD ] += d4c1*s[ IEVEN ] + d4c2*s[ IPREVEVEN ] ;

      unwaveletLoopJ
        s[ IEVEN ] -= sqrt3 * s[ IODD ] ;
    }
  }

  template<typename T> static void d4OneLevel( T*s, int n, int level, int sp )
  {
    int i = TwoToThe( level - 1 ) ;
    waveletLoopJ
      s[ IEVEN ] += sqrt3 * s[ IODD ] ;

    waveletLoopJLTD( 2, 0 )
      s[ IODD ] -= d4c1*s[ IEVEN ] + d4c2*s[ IPREVEVEN ] ;
          
    waveletLoopJLTD( 0, 2 )
      s[ IEVEN ] -= s[ INEXTODD ] ;

    waveletLoopJ
    {
      s[ IEVEN ] *= d4normEvens ;
      s[ IODD ] *= d4normOdds ;
    }
  }

  template <typename T> static void und4OneLevel( T* s, int n, int level, int sp )
  {
    int i = TwoToThe( level - 1 ) ;
    unwaveletLoopJ
    {
      s[ IODD ] *= d4unNormOdds ;
      s[ IEVEN ] *= d4unNormEvens ;
    }

    unwaveletLoopJLTD( 0, 2 )
      s[ IEVEN ] += s[ INEXTODD ] ;

    unwaveletLoopJLTD( 2, 0 )
    {
      s[ IODD ] += d4c1*s[ IEVEN ] + d4c2*s[ IPREVEVEN ] ;
      s[ IEVEN ] -= sqrt3 * s[ IODD ] ;
    }
  }
  #pragma endregion
  template<typename T> static void d6( T*s, int n, int sp )
  {
    waveletLoopI
    {
      waveletLoopJ
      {
      }
    }
  }
  template<typename T> static void und6( T*s, int n, int sp )
  {
    unwaveletLoopI
    {
      unwaveletLoopJ
      {
      }
    }
  }

  template<typename T> static void cdf_22( T*s, int n, int sp )
  {
    waveletLoopI
    {
      // PREVEVEN: begin 2 (2*j-2*i)
      // PREVODD:  begin 1 (2*j-i)
      // NEXTEVEN: end 1 (2*j+2*i)  // -i is already done so the odd doesn't go oob.
      // NEXTODD:  end 2 (2*j+3*i)
  
      //cdf (2,2), (2,4), (2,6)
      waveletLoopJLTD( 0, 1 )
        s[ IODD ] -= 0.5*( s[ IEVEN ] + s[ INEXTEVEN ] ) ;

      //cdf (2,2)
      waveletLoopJLTD( 1, 0 )
        s[ IEVEN ] += 0.25*( s[ IPREVODD ] + s[ IODD ] ) ;

      //cdf (2,2), (2,4), (2,6)
      waveletLoopJ
      {
        s[ IODD ] *= INV_sqrt2 ;
        s[ IEVEN ] *= sqrt2 ;
      }
    }
  }
  
  template<typename T> static void uncdf_22( T*s, int n, int sp )
  {
    unwaveletLoopI
    {
      //cdf (2,2), (2,4), (2,6)
      unwaveletLoopJ
      {
        
        s[ IEVEN ] *= INV_sqrt2 ;
        s[ IODD ] *= sqrt2 ;
      }
      
      //cdf (2,2)
      unwaveletLoopJLTD( 1, 0 )
        s[ IEVEN ] -= 0.25*( s[ IPREVODD ] + s[ IODD ] ) ;

      //cdf (2,2), (2,4), (2,6)
      unwaveletLoopJLTD( 0, 1 )
        s[ IODD ] += 0.5*( s[ IEVEN ] + s[INEXTEVEN] ) ;
    }
  }

  template<typename T> static void cdf_24( T*s, int n, int sp )
  {
    waveletLoopI
    {
      waveletLoopJ
      {
        //cdf (2,2), (2,4), (2,6)
        int nextEvenIndex = INEXTEVEN ;
        s[ IODD ] -= 0.5*( s[ IEVEN ] + SAFE_NEXT(s,nextEvenIndex) ) ;
      }
      
      //cdf (2,4)
      waveletLoopJ
      {
        int prevprevOddIndex = IPREVPREVODD ;
        int prevOddIndex = IPREVODD ;
        int nextOddIndex = INEXTODD ;
        s[ IEVEN ] -= (1.0/64.0) *
          ( 3*s[ IPREVPREVODD ]
          - 19*s[ IPREVODD ]
          - 19*s[ IODD ]
          + 3*s[ INEXTODD ] ) ;
      }

      //cdf (2,2), (2,4), (2,6)
      waveletLoopJ
      {
        s[ IODD ] *= INV_sqrt2 ;
        s[ IEVEN ] *= sqrt2 ;
      }
    }
  }
  
  template<typename T> static void uncdf_24( T*s, int n, int sp )
  {
    unwaveletLoopI
    {
      //cdf (2,2), (2,4), (2,6)
      unwaveletLoopJ
      {
        s[ IEVEN ] *= INV_sqrt2 ;
        s[ IODD ] *= sqrt2 ;
      }

      //cdf (2,4)
      unwaveletLoopJLTD( 3, 2 )
      {
        s[ IEVEN ] += (1.0/64.0) *
          ( 3*s[ IPREVPREVODD ]
          - 19*s[ IPREVODD ]
          - 19*s[ IODD ]
          + 3*s[ INEXTODD ] ) ;
      }

      //cdf (2,2), (2,4), (2,6)
      unwaveletLoopJ
        s[ IODD ] += 0.5*( s[ IEVEN ] + s[ INEXTEVEN ] ) ;
      
    }
  }

  template<typename T> static void cdf_26( T*s, int n, int sp )
  {
    waveletLoopI
    {
      //cdf (2,2), (2,4), (2,6)
      waveletLoopJLTD( 0, 1 )
        s[ IODD ] -= 0.5*( s[ IEVEN ] + s[ INEXTEVEN ] ) ; // +2

      //cdf (2,6)
      waveletLoopJLTD( 5, 4 )
      {
        s[ IEVEN ] -= (1.0/512.0)*(
          - 5*s[ IPREVPREVPREVODD ]
          + 39*s[ IPREVPREVODD ]
          - 162*s[ IPREVODD ]
          - 162*s[ IODD ]
          + 39*s[ INEXTODD ]
          - 5*s[ INEXTNEXTODD ] );
      }

      //cdf (2,2), (2,4), (2,6)
      waveletLoopJ
      {
        s[ IODD ] *= INV_sqrt2 ;
        s[ IEVEN ] *= sqrt2 ;
      }
    }
  }
  
  template<typename T> static void uncdf_26( T*s, int n, int sp )
  {
    unwaveletLoopI
    {
      unwaveletLoopJ
      {
        //cdf (2,2), (2,4), (2,6)
        s[ IEVEN ] *= INV_sqrt2 ;
        s[ IODD ] *= sqrt2 ;
      }

      //cdf (2,6)
      // there are a couple in the back and front that we'll use the protected loop for.
      {
        int j = n/2 - i ; // 3*i (nextodd) is oob
        s[ IEVEN ] += (1.0/512.0)*( - 5*s[ IPREVPREVPREVODD ] + 39*s[ IPREVPREVODD ] - 162*s[IPREVODD] - 162*s[IODD] + 0 - 0 );
        j-=i; // j=n/2 - 2*i
        s[ IEVEN ] += (1.0/512.0)*( - 5*s[ IPREVPREVPREVODD ] + 39*s[ IPREVPREVODD ] - 162*s[IPREVODD] - 162*s[IODD] + 0 - 0 );
        j-=i; // j=n/2 - 3*i
        s[ IEVEN ] += (1.0/512.0)*( - 5*s[ IPREVPREVPREVODD ] + 39*s[ IPREVPREVODD ] - 162*s[IPREVODD] - 162*s[IODD] + 39*s[INEXTODD] - 0 );
        j-=i; // j=n/2 - 4*i
        s[ IEVEN ] += (1.0/512.0)*( - 5*s[ IPREVPREVPREVODD ] + 39*s[ IPREVPREVODD ] - 162*s[IPREVODD] - 162*s[IODD] + 39*s[INEXTODD] - 5*s[ INEXTNEXTODD ] );
      }

      unwaveletLoopJLTD( 5, 4 )
      {
        s[ IEVEN ] += (1.0/512.0)*(
          - 5*s[ IPREVPREVPREVODD ] // -5
          + 39*s[IPREVPREVODD] // -3
          - 162*s[IPREVODD] // -1
          - 162*s[IODD] // +1
          + 39*s[INEXTODD] // +3
          - 5*s[ INEXTNEXTODD ] ); // +5
      }

      // 5 was shaved from the front. start at shaveBegin.  (prev loop terminates @ j >= i*shaveBegin)
      {
        int j = i*4 ; // (i*5 was already done)
        s[ IEVEN ] += (1.0/512.0)*( 0 + 39*s[ IPREVPREVODD ] - 162*s[IPREVODD] - 162*s[IODD] + 39*s[INEXTODD] - 5*s[ INEXTNEXTODD ] );
        j-=i ; //j=3*i
        s[ IEVEN ] += (1.0/512.0)*( 0 + 39*s[ IPREVPREVODD ] - 162*s[IPREVODD] - 162*s[IODD] + 39*s[INEXTODD] - 5*s[ INEXTNEXTODD ] );
        j-=i ; //j=2*i, IPREVODD still in scope
        s[ IEVEN ] += (1.0/512.0)*( 0 + 0 - 162*s[IPREVODD] - 162*s[IODD] + 39*s[INEXTODD] - 5*s[ INEXTNEXTODD ] );
        j-=i ; //j=i, IPREVODD still in scope
        s[ IEVEN ] += (1.0/512.0)*( 0 + 0 - 162*s[IPREVODD] - 162*s[IODD] + 39*s[INEXTODD] - 5*s[ INEXTNEXTODD ] );
        j-=i ; //j=0, not in scope
        s[ IEVEN ] += (1.0/512.0)*( 0 + 0 - 0 - 162*s[IODD] + 39*s[INEXTODD] - 5*s[ INEXTNEXTODD ] );
      }

      //cdf (2,2), (2,4), (2,6)
      unwaveletLoopJLTD( 0, 1 )
        s[ IODD ] += 0.5*( s[ IEVEN ] + s[ INEXTEVEN ] ) ; //+2
    }
  }

  template<typename T> static void cdf_31( T*s, int n, int sp )
  {
    waveletLoopI
    {
      waveletLoopJ
      {
        s[ IEVEN ] -= (1.0/3.0) * s[ IPREVODD ] ;
        s[ IODD ] -= (1.0/8.0) * ( 9*s[IEVEN] + 3*s[ INEXTEVEN ] ) ;
      }

      // cdf(3,1)
      waveletLoopJ
        s[IEVEN] += (4.0/9.0)*s[IODD] ;

      waveletLoopJ
      {
        s[IODD] *= sqrt2/3.0 ;
        s[IEVEN] *= 3.0/sqrt2 ;
      }
    }
  }

  template<typename T> static void uncdf_31( T*s, int n, int sp )
  {
    unwaveletLoopI
    {
      unwaveletLoopJ
      {
        s[IODD] *= 3.0/sqrt2 ;
        s[IEVEN] *= sqrt2/3.0 ;

        // cdf(3,1)
        s[IEVEN] -= (4.0/9.0)*s[IODD] ;

        int nextEvenIndex = INEXTEVEN ;
        s[ IODD ] += (1.0/8.0) * ( 9*s[IEVEN] + 3*SAFE_NEXT(s,nextEvenIndex) ) ;
        int prevOddIndex = IPREVODD ;
        s[ IEVEN ] += (1.0/3.0) * SAFE_PREV( s, prevOddIndex) ;
      }
    }
  }

  template<typename T> static void cdf_31OneLevel( T*s, int n, int level, int sp )
  {
    int i = TwoToThe( level - 1 ) ;
    waveletLoopJ
    {
      int prevOddIndex = IPREVODD ;
      s[ IEVEN ] -= (1.0/3.0) * SAFE_PREV( s, prevOddIndex) ;
      int nextEvenIndex = INEXTEVEN ;
      s[ IODD ] -= (1.0/8.0) * ( 9*s[IEVEN] + 3*SAFE_NEXT(s,nextEvenIndex) ) ;

      // cdf(3,1)
      s[IEVEN] += (4.0/9.0)*s[IODD] ;

      s[IODD] *= sqrt2/3.0 ;
      s[IEVEN] *= 3.0/sqrt2 ;
    }
  }

  template<typename T> static void uncdf_31OneLevel( T*s, int n, int level, int sp )
  {
    int i = TwoToThe( level - 1 ) ;
    unwaveletLoopJ
    {
      s[IODD] *= 3.0/sqrt2 ;
      s[IEVEN] *= sqrt2/3.0 ;

      // cdf(3,1)
      s[IEVEN] -= (4.0/9.0)*s[IODD] ;

      int nextEvenIndex = INEXTEVEN ;
      s[ IODD ] += (1.0/8.0) * ( 9*s[IEVEN] + 3*SAFE_NEXT(s,nextEvenIndex) ) ;
      int prevOddIndex = IPREVODD ;
      s[ IEVEN ] += (1.0/3.0) * SAFE_PREV( s, prevOddIndex) ;
    }
  }

  template<typename T> static void cdf_33( T*s, int n, int sp )
  {
    waveletLoopI
      waveletLoopJ
      {
      }
  }
  template<typename T> static void uncdf_33( T*s, int n, int sp )
  {
    unwaveletLoopI
      unwaveletLoopJ
      {
      }
  }

  template<typename T> static void cdf_35( T*s, int n, int sp )
  {
    waveletLoopI
      waveletLoopJ
      {
      }
  }
  template<typename T> static void uncdf_35( T*s, int n, int sp )
  {
    unwaveletLoopI
      unwaveletLoopJ
      {
      }
  }
  #pragma endregion

  // s: signal
  // n: number of elements to include in the transform
  // sp: inter-elemental spacing
  // dim: dimensionality of the transform (1d or 2d)
  // OR you could have a separate function (1d or 2d)
  template<typename T>
  static void transform( WaveletType type, T* s, int n, int sp )
  {
    // performs the currently selected transform on your function.
    // check that n is a power of 2
    if( !IsPow2( n ) )
    {
      warning( "n should be a power of 2.. call pad()" ) ;
      return ;
    }

    // Use a function pointer to resolve to
    // the correct transform.

    // n is the number of elements to
    // operate on (this varies for __2d__ transforms)
    // sp is the interelemental spacing

    // void (Wavelet::*)( int n, int sp )
    // I'm using a function pointer to avoid switching
    // inside a loop
    void (*wfn)( T*, int, int ) = getWaveletFunction<T>( type ) ;

    wfn( s, n, sp ) ;
  }

  template<typename T>
  static void untransform( WaveletType type, T* s, int n, int sp )
  {
    void (*unwfn)( T*, int, int ) = getUnwaveletFunction<T>( type ) ;

    unwfn( s, n, sp ) ;
  }

  template<typename T>
  static void transform2d( WaveletType type, T* s, int rows, int cols )
  {
    // now, if the person wanted 2d..
    // then the root of n ALSO must be
    // a power of 2 (indicating the 
    // signal is both square and a power of 2)
    if( !IsPow2( rows ) || !IsPow2( cols ) )
    {
      warning( "both rows and cols should be a power of 2 for 2d transforms.. call pad()" ) ;
      return ;
    }

    void (*wfn)( T*, int, int ) = getWaveletFunction<T>( type ) ;

    // transform the rows
    for( int row = 0 ; row < rows ; row++ )
      wfn( &s[ row*cols ], cols, 1 ) ;

    // transform the cols
    for( int col = 0 ; col < cols ; col++ )
      wfn( &s[ 0 + col ], rows, cols ) ;
    
  }

  template<typename T>
  static void untransform2d( WaveletType type, T* s, int rows, int cols )
  {
    void (*unwfn)( T*, int, int ) = getUnwaveletFunction<T>( type ) ;
   
    // untransform the cols
    for( int col = 0 ; col < cols ; col++ )
      unwfn( &s[ 0 + col ], rows, cols ) ;
      
    // untransform the rows
    for( int row = 0 ; row < rows ; row++ )
      unwfn( &s[ row*cols ], cols, 1 ) ;
  }

  template<typename T>
  static void transform2dnonstd( WaveletType type, T* s, int rows, int cols )
  {
    // row, then col, then row, then col, until done
    void (*wfn)( T*, int, int, int ) = getOneLevelWaveletFunction<T>( type ) ;
    int maxLevel = log2( rows ) ;
    // Try doing the whole level first, then switch to cols.
    for( int level = 1 ; level <= maxLevel ; level++ )
    {
      // assume rows=cols, k is the current row/col (do both)
      for( int k = 0 ; k < rows ; k++ )
        wfn( &s[ k*cols ], cols, level, 1 ) ; //kth row, xform cols elts here
      for( int k = 0 ; k < rows ; k++ )
        wfn( &s[ k ], rows, level, cols ) ; //kth col
    }
  }

  template<typename T>
  static void untransform2dnonstd( WaveletType type, T* s, int rows, int cols )
  {
    void (*unwfn)( T*, int, int, int ) = getOneLevelUnwaveletFunction<T>( type ) ;
    int maxLevel = log2( rows ) ;
    for( int level = maxLevel ; level >= 1 ; level-- )
    {
      for( int k = 0 ; k < rows ; k++ )
        unwfn( &s[ k ], rows, level, cols ) ; //kth col
      for( int k = 0 ; k < rows ; k++ )
        unwfn( &s[ k*cols ], cols, level, 1 ) ; //kth row
    }
  }

  // performs a lattice-style transformation
  #pragma region lattice-style lifting
  //    0  1  2  3  4  5  6  7
  // 0  o  •  o  •  o  •  o  •
  // 1  •  o  •  o  •  o  •  o
  // 2  o  •  o  •  o  •  o  •
  // 3  •  o  •  o  •  o  •  o
  // 4  o  •  o  •  o  •  o  •
  // 5  •  o  •  o  •  o  •  o
  // 6  o  •  o  •  o  •  o  •
  // 7  •  o  •  o  •  o  •  o


  // The black dots are gone,
  // you have a 45 degree lattice
  //    0  1  2  3  4  5  6  7
  // 0  o     o     o     o   
  // 1     o     o     o     o
  // 2  o     o     o     o   
  // 3     o     o     o     o
  // 4  o     o     o     o   
  // 5     o     o     o     o
  // 6  o     o     o     o   
  // 7     o     o     o     o

  // now turn your head 45 degrees and
  // see even, odd, even, odd again
  //    0  1  2  3  4  5  6  7
  // 0  o     o     o     o   
  // 1     •     •     •     •
  // 2  o     o     o     o   
  // 3     •     •     •     •
  // 4  o     o     o     o   
  // 5     •     •     •     •
  // 6  o     o     o     o   
  // 7     •     •     •     •

  // The black dots are gone and,
  // you have a rectangular lattice again
  //    0  1  2  3  4  5  6  7
  // 0  o     o     o     o   
  // 1 
  // 2  o     o     o     o   
  // 3 
  // 4  o     o     o     o   
  // 5 
  // 6  o     o     o     o   
  // 7 

  // even, odd, even, odd
  //    0  1  2  3  4  5  6  7
  // 0  o     •     o     •
  // 1 
  // 2  •     o     •     o
  // 3 
  // 4  o     •     o     •
  // 5 
  // 6  •     o     •     o
  // 7 

  // again
  //    0  1  2  3  4  5  6  7
  // 0  o           o
  // 1 
  // 2        o           o
  // 3 
  // 4  o           o
  // 5 
  // 6        o           o
  // 7 

  // odds
  //    0  1  2  3  4  5  6  7
  // 0  o           o
  // 1 
  // 2        •           •
  // 3 
  // 4  o           o
  // 5 
  // 6        •           •
  // 7 

  // eliminate, odds
  //    0  1  2  3  4  5  6  7
  // 0  o           •
  // 1 
  // 2 
  // 3 
  // 4  •           o
  // 5 
  // 6 
  // 7 

  // final
  //    0  1  2  3  4  5  6  7
  // 0  o
  // 1 
  // 2 
  // 3 
  // 4              •
  // 5 
  // 6 
  // 7 
    
  // end.
  #pragma endregion

  // WRAPS. You add ROWS to row and COLS to col in case you got a negative value.
  #define AT( row,col,ROWS,COLS )    ( ((row+ROWS)%ROWS)*COLS + ((col+COLS)%COLS) )
  
  #define LEFT(row,col,ROWS,COLS,sp)     AT(row,col-sp,ROWS,COLS)
  #define RIGHT(row,col,ROWS,COLS,sp)    AT(row,col+sp,ROWS,COLS)
  #define UP(row,col,ROWS,COLS,sp)       AT(row-sp,col,ROWS,COLS)
  #define DOWN(row,col,ROWS,COLS,sp)     AT(row+sp,col,ROWS,COLS)

  #define LEFT_45(row,col,ROWS,COLS,sp)  AT(row+sp,col-sp,ROWS,COLS)
  #define RIGHT_45(row,col,ROWS,COLS,sp) AT(row-sp,col+sp,ROWS,COLS)
  #define UP_45(row,col,ROWS,COLS,sp)    AT(row-sp,col-sp,ROWS,COLS)
  #define DOWN_45(row,col,ROWS,COLS,sp)  AT(row+sp,col+sp,ROWS,COLS)

  template<typename T>
  static real getPower( T*s, int rows, int cols )
  {
    real power = 0 ;
    for( int row = 0 ; row < rows ; row++ )
      for( int col = 0 ; col < cols ; col++ )
        power += s[ AT(row,col,rows,cols) ] * s[ AT(row,col,rows,cols) ] ;

    return sqrt( power ) ;
  }

  template<>
  static real getPower( Vector*s, int rows, int cols )
  {
    real power = 0 ;
    for( int row = 0 ; row < rows ; row++ )
      for( int col = 0 ; col < cols ; col++ )
        power += s[ AT(row,col,rows,cols) ].len2() ;
    
    return sqrt( power ) ;
  }

  // square lattice.
  template <typename T>
  static void latticeLift( T* s, const IndexMap& indexMap )
  {
    // use indexmap on signal s
    ////for( int i = 0 ; i < indexMap.visits.size() ; i++ ){
    ////  const IndexMap::LatticeVisit *visit = &indexMap.visits[i] ;
    ////  s[ visit->index ] += visit->multiplier * (
    ////    s[ visit->toVisit[0] ] + s[ visit->toVisit[1] ] +
    ////    s[ visit->toVisit[2] ] + s[ visit->toVisit[3] ]
    ////  ) ;
    ////  s[ visit->index ] = Vector(0,0,1);
    ////  s[ visit->toVisit[0] ] = Vector(1,0,1);
    ////  s[ visit->toVisit[1] ] = Vector(0,1,0);
    ////  s[ visit->toVisit[2] ] = Vector(0,1,1);
    ////  s[ visit->toVisit[3] ] = Vector(0,0,1);
    ////    
    ////   TEMP CHANGE
    ////  s[ visit->index ] = .5 ;
    ////  for( int i = 0 ; i < 4 ; i++ )
    ////    s[ visit->toVisit[i] ] = Vector(1,1,0); /// visited items yellow
    ////  
    ////  s[ visit->index ] = visit->multiplier * (
    ////    s[ visit->toVisit[0] ] + s[ visit->toVisit[1] ] +
    ////    s[ visit->toVisit[2] ] + s[ visit->toVisit[3] ]
    ////  ) ;
    ////  s[ visit->index ] += visit->multiplier * s[ visit->toVisit[0] ] ;
    ////}
    if( indexMap.visits.size() != indexMap.normalizations.size() )
    {
      error( "visits.size(%d) != normalizations.size(%d)", indexMap.visits.size(), indexMap.normalizations.size() ) ;
      return ;
    }
    for( int i = 0 ; i < indexMap.visits.size() ; i++ )
    {
      vector<IndexMap::LatticeVisit> *visitSet = indexMap.visits[ i ] ;
      IndexMap::LatticeNormalization* norm = indexMap.normalizations[ i ] ;
      
      // store signal power
      norm->computePowerBefore( s ) ;

      for( int j = 0 ; j < visitSet->size() ; j++ )
      {
        IndexMap::LatticeVisit& visit = (*visitSet)[j] ;
        s[ visit.index ] += visit.multiplier * (
          s[ visit.toVisit[0] ] + s[ visit.toVisit[1] ] +
          s[ visit.toVisit[2] ] + s[ visit.toVisit[3] ]
        ) ;
        
        ////printf( "power now %f\r", norm->computeSquarePower( s ) ) ;
      }
      
      // APPLY THE NORMALIZATION FOR THIS STAGE
      //norm->apply( s ) ;
    }
  }

  template <typename T>
  static void unlatticeLift( T* s, const IndexMap& indexMap )
  {
    ////for( int i = 0 ; i < indexMap.unvisits.size() ; i++ )
    ////{
    ////  const IndexMap::LatticeVisit *unvisit = &indexMap.unvisits[i] ;
    ////  s[ unvisit->index ] += unvisit->multiplier * (
    ////    s[ unvisit->toVisit[0] ] + s[ unvisit->toVisit[1] ] +
    ////    s[ unvisit->toVisit[2] ] + s[ unvisit->toVisit[3] ]
    ////  ) ;
    ////  // TEMP CHANGE
    ////  s[ unvisit->index ] += unvisit->multiplier * s[ unvisit->toVisit[0] ] ;
    ////  s[ unvisit->index ] = Vector(1,1,0);
    ////  s[ unvisit->toVisit[0] ] = Vector(1,0,1);
    ////}
    if( indexMap.unvisits.size() != indexMap.unnormalizations.size() )
    {
      error( "unvisits.size(%d) != unnormalizations.size(%d)", indexMap.unvisits.size(), indexMap.unnormalizations.size() ) ;
      return ;
    }
    for( int i = 0 ; i < indexMap.unvisits.size() ; i++ )
    {
      vector<IndexMap::LatticeVisit> *visitSet = indexMap.unvisits[ i ] ;
      IndexMap::LatticeNormalization* norm = indexMap.unnormalizations[ i ] ;
      
      // store signal power
      norm->computePowerBefore( s ) ;

      for( int j = 0 ; j < visitSet->size() ; j++ )
      {
        IndexMap::LatticeVisit& visit = (*visitSet)[j] ;
        s[ visit.index ] += visit.multiplier * (
          s[ visit.toVisit[0] ] + s[ visit.toVisit[1] ] +
          s[ visit.toVisit[2] ] + s[ visit.toVisit[3] ]
        ) ;
      }
      
      // APPLY THE NORMALIZATION FOR THIS STAGE
      //norm->apply( s ) ;
    }
  }

  #define POWER_NORMALIZE 1
  // square lattice.
  template <typename T>
  static void latticeLift( T* s, int rows, int cols )
  {
    int maxLevel = log2( rows ) ;

    // for lattice, skip is 2^level
    //double pWayBefore = getPower<T>( s, rows, cols ) ;
      
    for( int level = 1 ; level <= maxLevel ; level++ )
    {
      int skip = 1 << level ;
      int sp = skip / 2 ;
      #if POWER_NORMALIZE
      double pBefore, pAfter, pMult ;
      #endif
      ////print( s, rows, cols ) ;
      ////printf( "========================\nSTART LEVEL %d POWER %f\n", level, getPower<T>( s, rows, cols ) ) ;

      #pragma region NORMALIZATION ANALYSIS BEFORE ODD SQUARE
      // VISIT ALL ELEMENTS YOU HIT AND RAISE BY FACTOR OF √2.
      // Measure the power it was before, and lift it back
      // to being that same value.
      #if POWER_NORMALIZE
      pBefore = 0 ;
      for( int row = 0 ; row < rows ; row += sp )
        for( int col = 0 ; col < cols ; col += sp )
          pBefore += s[ AT( row,col,rows,cols ) ].len2() ; // vector specialization
          //pBefore += s[ AT( row,col,rows,cols ) ] * s[ AT( row,col,rows,cols ) ] ;
      if( !pBefore )  continue ; // don't process 0 signals
      #endif
      #pragma endregion

      #pragma region odd square lattice
      // ODD SQUARE:
      //TR box
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] -= .25*( s[ LEFT(row,col,rows,cols,sp) ] + s[ RIGHT(row,col,rows,cols,sp) ] +
                                              s[ UP(row,col,rows,cols,sp) ] + s[ DOWN(row,col,rows,cols,sp) ] ) ;
      //BL box
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] -= .25*( s[ LEFT(row,col,rows,cols,sp) ] + s[ RIGHT(row,col,rows,cols,sp) ] +
                                              s[ UP(row,col,rows,cols,sp) ] + s[ DOWN(row,col,rows,cols,sp) ] ) ;
      #pragma endregion
    
      #pragma region NORMALIZE AFTER ODD SQUARE
      #if POWER_NORMALIZE
      // get the power now
      pAfter = 0 ;
      for( int row = 0 ; row < rows ; row += sp )
        for( int col = 0 ; col < cols ; col += sp )
          pAfter += s[ AT( row,col,rows,cols ) ].len2() ;
          //pAfter += s[ AT( row,col,rows,cols ) ] * s[ AT( row,col,rows,cols ) ]  ;
    
      pMult = sqrt( pBefore / pAfter) ;
    
      ///printf( "power was %f before, after its %f, loss %f\n", pBefore, pAfter, pMult ) ;

      for( int row = 0 ; row < rows ; row += sp )
        for( int col = 0 ; col < cols ; col += sp )
          s[ AT( row,col,rows,cols ) ] *= pMult ;

      //printf( "power now %f\n", getPower<double>( s, rows, cols ) ) ;
      #endif
      #pragma endregion

      //print( s, rows, cols ) ;
      //printf( "LEVEL %d, after odd square lift POWER %f\n", level, getPower<double>( s, rows, cols ) ) ;

      #pragma region power analysis before even square
      #if POWER_NORMALIZE
      pBefore = 0 ;
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          pBefore += s[ AT(row,col,rows,cols) ].len2();
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          pBefore += s[ AT(row,col,rows,cols) ].len2();
      #endif
      #pragma endregion

      #pragma region even square lattice
      // EVEN square
      // TL box
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] += .125 *( s[ LEFT(row,col,rows,cols,sp) ] + s[ RIGHT(row,col,rows,cols,sp) ] + 
                                                s[ UP(row,col,rows,cols,sp) ] + s[ DOWN(row,col,rows,cols,sp) ] ) ;
      // BR box
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] += .125 *( s[ LEFT(row,col,rows,cols,sp) ] + s[ RIGHT(row,col,rows,cols,sp) ] +
                                                s[ UP(row,col,rows,cols,sp) ] + s[ DOWN(row,col,rows,cols,sp) ] ) ;
      #pragma endregion
    
      #pragma region NORMALIZE AFTER EVEN SQUARE
      #if POWER_NORMALIZE
      pAfter = 0 ;
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          pAfter += s[ AT(row,col,rows,cols) ].len2();
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          pAfter += s[ AT(row,col,rows,cols) ].len2();
    
      pMult = sqrt( pBefore / pAfter) ;
    
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] *= pMult ;
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] *= pMult ;
      #endif
      #pragma endregion
    
      //print( s, rows, cols ) ;
      //printf( "LEVEL %d, after EVEN square lift POWER %f\n", level, getPower<double>( s, rows, cols ) ) ;

      #pragma region NORMALIZATION ANALYSIS BEFORE 45 ODD LIFT
      #if POWER_NORMALIZE
      pBefore = 0 ;
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          pBefore += s[ AT(row,col,rows,cols) ].len2();
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          pBefore += s[ AT(row,col,rows,cols) ].len2();
      #endif
      #pragma endregion

      #pragma region 45 odd 
      // the blacks and whites are actually grid aligned now.
      // 45 ODD
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] -= .25*( s[ LEFT_45(row,col,rows,cols,sp) ] + s[ RIGHT_45(row,col,rows,cols,sp) ] +
                                              s[ UP_45(row,col,rows,cols,sp) ] + s[ DOWN_45(row,col,rows,cols,sp) ] ) ;
      #pragma endregion
    
      #pragma region NORMALIZE AFTER 45 ODD LIFT
      #if POWER_NORMALIZE
      // 45 odd normalization
      pAfter = 0 ;
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          pAfter += s[ AT(row,col,rows,cols) ].len2();
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          pAfter += s[ AT(row,col,rows,cols) ].len2();

      pMult = sqrt( pBefore / pAfter ) ;

      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] *= pMult ;//NORM
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] *= pMult ;//NORM
      #endif
      #pragma endregion

      //print( s, rows, cols ) ;
      //printf( "LEVEL %d, after 45 ODD POWER %f\n", level, getPower<double>( s, rows, cols ) ) ;

      #pragma region power analysis before 45 even lift
      #if POWER_NORMALIZE
      pBefore = 0 ;
      for( int row = 0 ; row < rows ; row += skip )
        for( int col = 0 ; col < cols ; col += skip )
          pBefore += s[ AT( row,col,rows,cols ) ].len2();
      #endif
      #pragma endregion

      #pragma region 45 even
      // 45 EVEN
      // TL
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] += .125 *( s[ LEFT_45(row,col,rows,cols,sp) ] + s[ RIGHT_45(row,col,rows,cols,sp) ] +
                                                s[ UP_45(row,col,rows,cols,sp) ] + s[ DOWN_45(row,col,rows,cols,sp) ] ) ;
      #pragma endregion

      #pragma region NORMALIZE AFTER 45 EVEN
      #if POWER_NORMALIZE
      pAfter = 0 ;
      for( int row = 0 ; row < rows ; row += skip )
        for( int col = 0 ; col < cols ; col += skip )
          pAfter += s[ AT( row,col,rows,cols ) ].len2();

      pMult = sqrt( pBefore / pAfter ) ;

      for( int row = 0 ; row < rows ; row += skip )
        for( int col = 0 ; col < cols ; col += skip )
          s[ AT( row,col,rows,cols ) ] *= pMult ;//NORM
      #endif
      #pragma endregion

      //print( s, rows, cols ) ;
      //printf( "LEVEL %d, after 45 even POWER %f\n", level, getPower<double>( s, rows, cols ) ) ;
      //printf( "LEVEL %d, POWER %f\n", level, getPower<T>( s, rows, cols ) ) ;
      
    }

    //double pWayAfter = getPower<T>( s, rows, cols ) ;
    //info( "POWER %f -> %f", pWayBefore, pWayAfter ) ;
  }

  template <typename T>
  static void unlatticeLift( T* s, int rows, int cols )
  {
    int maxLevel = log2( rows ) ;

    // for lattice, 
    int level = 1 ; // skip is 2^level
  
    for( int level = maxLevel ; level >= 1; level-- ) 
    {
      int skip = 1 << level ;
      int sp = skip / 2 ;
      #if POWER_NORMALIZE
      double pBefore, pAfter, pMult ;
      #endif

      double pWayBefore = getPower<T>( s, rows, cols ) ;
      #pragma region GET POWER UNNORMALIZE BEFORE 45 EVEN
      #if POWER_NORMALIZE
      pBefore = 0 ;
      for( int row = 0 ; row < rows ; row += skip )
        for( int col = 0 ; col < cols ; col += skip )
          pBefore += s[ AT( row,col,rows,cols ) ].len2();
      #endif
      #pragma endregion
      
      #pragma region 45 even
      // 45 EVEN
      // TL
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] -= .125 *( s[ LEFT_45(row,col,rows,cols,sp) ] + s[ RIGHT_45(row,col,rows,cols,sp) ] +
                                                s[ UP_45(row,col,rows,cols,sp) ] + s[ DOWN_45(row,col,rows,cols,sp) ] ) ;
      #pragma endregion

      #pragma region UNNORMALIZE __AFTER__ 45 EVEN
      #if POWER_NORMALIZE
      pAfter = 0 ;
      for( int row = 0 ; row < rows ; row += skip )
        for( int col = 0 ; col < cols ; col += skip )
          pAfter += s[ AT( row,col,rows,cols ) ].len2();
    
      pMult = sqrt( pBefore / pAfter ) ;

      for( int row = 0 ; row < rows ; row += skip )
        for( int col = 0 ; col < cols ; col += skip )
          s[ AT( row,col,rows,cols ) ] *= pMult ;
      #endif
      #pragma endregion

      #pragma region GET POWER UNNORMALIZE BEFORE 45 ODD
      #if POWER_NORMALIZE
      pBefore = 0 ;
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          pBefore += s[ AT(row,col,rows,cols) ].len2();
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          pBefore += s[ AT(row,col,rows,cols) ].len2();
      #endif
      #pragma endregion

      #pragma region 45 odd
      /////// BEGIN 45 BR
      // the blacks and whites are actually grid aligned now.
      // 45 ODD
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] += .25*( s[ LEFT_45(row,col,rows,cols,sp) ] + s[ RIGHT_45(row,col,rows,cols,sp) ] +
                                              s[ UP_45(row,col,rows,cols,sp) ] + s[ DOWN_45(row,col,rows,cols,sp) ] ) ;
      #pragma endregion

      #pragma region GET POWER UNNORMALIZE BEFORE 45 ODD
      #if POWER_NORMALIZE
      pAfter = 0 ;
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          pAfter += s[ AT(row,col,rows,cols) ].len2();
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          pAfter += s[ AT(row,col,rows,cols) ].len2();

      pMult = sqrt( pBefore / pAfter ) ;

      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] *= pMult ;
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] *= pMult ;
      #endif
      #pragma endregion

      #pragma region UNNORMALIZE BEFORE EVEN SQUARE
      #if POWER_NORMALIZE
      pBefore = 0 ;
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          pBefore += s[ AT(row,col,rows,cols) ].len2();
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          pBefore += s[ AT(row,col,rows,cols) ].len2();
      #endif
      #pragma endregion

      #pragma region even square
      // EVEN SQUARE
      // BR box
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] -= .125 *( s[ LEFT(row,col,rows,cols,sp) ] + s[ RIGHT(row,col,rows,cols,sp) ] + s[ UP(row,col,rows,cols,sp) ] + s[ DOWN(row,col,rows,cols,sp) ] ) ;

      // TL box
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] -= .125 *( s[ LEFT(row,col,rows,cols,sp) ] + s[ RIGHT(row,col,rows,cols,sp) ] + s[ UP(row,col,rows,cols,sp) ] + s[ DOWN(row,col,rows,cols,sp) ] ) ;
      #pragma endregion

      #pragma region UNNORMALIZE __AFTER__ EVEN SQUARE
      #if POWER_NORMALIZE
      pAfter = 0 ;
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          pAfter += s[ AT(row,col,rows,cols) ].len2();
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          pAfter += s[ AT(row,col,rows,cols) ].len2();

      pMult = sqrt( pBefore / pAfter ) ;

      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] *= pMult ;
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] *= pMult ;
      #endif
      #pragma endregion

      #pragma region UNNORMALIZE BEFORE ODD SQUARE
      #if POWER_NORMALIZE
      // ODD:
      // VISIT ALL ELEMENTS YOU HIT AND UN-RAISE BY FACTOR OF √2.
      pBefore = 0 ;
      for( int row = 0 ; row < rows ; row += sp )
        for( int col = 0 ; col < cols ; col += sp )
          pBefore += s[ AT( row,col,rows,cols ) ].len2();
      #endif
      #pragma endregion
    
      #pragma region odd square
      // I'm doing 2 staggered evenly spaced
      // ODD square

      //TR box
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] += .25*( s[ LEFT(row,col,rows,cols,sp) ] + s[ RIGHT(row,col,rows,cols,sp) ] + s[ UP(row,col,rows,cols,sp) ] + s[ DOWN(row,col,rows,cols,sp) ] ) ;

      //BL box
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          s[ AT(row,col,rows,cols) ] += .25*( s[ LEFT(row,col,rows,cols,sp) ] + s[ RIGHT(row,col,rows,cols,sp) ] + s[ UP(row,col,rows,cols,sp) ] + s[ DOWN(row,col,rows,cols,sp) ] ) ;
      #pragma endregion

      #pragma region UNNORMALIZE BEFORE ODD SQUARE
      #if 0
      pAfter = 0 ;
      for( int row = 0 ; row < rows ; row += sp )
        for( int col = 0 ; col < cols ; col += sp )
          pAfter += s[ AT( row,col,rows,cols ) ].len2();

      pMult = sqrt( pBefore / pAfter ) ;

      for( int row = 0 ; row < rows ; row += sp )
        for( int col = 0 ; col < cols ; col += sp )
          s[ AT( row,col,rows,cols ) ] *= pMult ;
      #endif
      #pragma endregion

      double pWayAfter = getPower<T>( s, rows, cols ) ;
      printf( "\nLEVEL %d, POWER %f -> %f\n", level, pWayBefore, pWayAfter ) ;
    }
  }




} ;


#endif