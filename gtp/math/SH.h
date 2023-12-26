#ifndef SH_H
#define SH_H

#include <vector>
#include <functional>
using namespace std ;

#include "../util/StdWilUtil.h"
#include "Vector.h"
//#include "SHSample.h"
//#include "SHVector.h"

//#include <mpirxx.h>
//#pragma comment( lib, "C:/CPP/libraries/mpir-2.4.0/build.vc10/dll_mpir_k10/x64/Release/mpir.lib" )

struct Model ;

#define SH_INDEX( l,m ) (l*(l+1)+m)

// Kronecker delta function return {
//   1   when x==m,
//   0   otherwise
// δ=ALT+235
#define δ(m,x) (x==m?1:0)


/// An SHRotationMatrix ISN'T exactly a dense SHMatrix.
// IT is block diagonal sparse, because band (l+1) doesn't
// interact with band (l).  ie
// it looks like:
// // (1--------) (NOT INCLUDED)
// xxx-----
// xxx-----
// xxx-----
// ---xxxxx
// ---xxxxx
// ---xxxxx
// ---xxxxx
// ---xxxxx
// The dashes are 0 and the
// x's are float numbers.
// Note the constant (1) (for rotating a sphere)
// IS NOT INCLUDED.
struct SHRotationMatrix
{
  int bands ;
  // 
  vector<real> coeff ;

  SHRotationMatrix( int levels, const Vector& axis, real radians )
  {
    bands = levels ;
    coeff.resize( getNumElements( levels ), 0.0 ) ;
    Matrix r = Matrix::Rotation( axis, radians ) ;
    makeRotation( r ) ;
  }

  SHRotationMatrix( int levels, const Matrix& matrix )
  {
    bands = levels ;
    coeff.resize( getNumElements( levels ), 0.0 ) ;
    makeRotation( matrix ) ;
  }

  // only need 1 L band for look up,
  // because m and mm must be in the same L band
  // (otherwise coefficient is 0)
  static int index( int l, int m, int mm )
  {
    int row = m+l;
    int col =mm+l;
    return getOffsetToLevel( l ) + row*width(l) + col ;
  }

  void makeRotation( const Matrix& r ) ;

  // how many elements are TOTAL from level=1 to level=l?
  // l=1:   9
  // l=2:  34
  // l=3:  83
  // l=4: 164
  static int getNumElements( int l )
  {
    return ( 4*l*l*l + 12*l*l + 11*l ) / 3 ;
  }

  // actually #m bands for this l band
  static int width( int l )
  {
    // width/height of rotation matrix on level
    // l=1: 3
    // l=2: 5
    // l=3: 7
    return 2*l+1 ;
  }

  // offset (elements to skip) to reach level l.
  static int getOffsetToLevel( int l )
  {
    if( l <= 1 ) return 0 ;
    else return getNumElements( l - 1 ) ;
  }

  real getElement( int l, int m, int mm )
  {
    return coeff[ index(l,m,mm) ] ;
  }


} ;

// Provides the SH functions.
class SH
{
public:
  enum Kernel{ Legendre, Chebyshev } ;
  
  // The current orthogonal polynomial kernel being used
  static Kernel kernel ;

  // For outputting the name of the kernel being used
  static char* kernelName ; 

  #pragma region spherical harmonics
  // K for SH (only used by Green's formulation)
  static real KSH( int l, int m ) ;

  // from Green[2003], which in turn was from Numerical Recipes in C
  static real P( int l, int m, real x ) ;
  #pragma endregion

  #pragma region cpherical harmonics
  // The chebyshev polynomial
  static __forceinline real T( int n, real x )
  {
    // you can use the trigonometric or
    // the polynomial evaluation.
    // the trig eval is just as fast and accurate as the recursive polynomial eval,
    // and the trig eval is __much__ faster than the explicit polynomial equations eval (big switch)
    // (for large n).
    return cos( n*acos(x) ) ;
  }

  // ith derivative of nth chebyshev polynomial, helper
  static inline real Tb( int i, int n, int k )
  {
    int s = (k+n-i)/2;
    int c = n==0 ? 2 : 1 ;
    return ((1<<i) * k * facs[ s-n+i-1 ] * facs[ s+i-1 ]) /
           ( facs[i-1]*c * facs[s] * facs[ s-n ] ) ;
  }

  // Gets you the "ith" derivative of a chebyshev polynomial "k" at x,
  // due to Elbarbary et al
  static inline real TDiff( int i, int k, real x )
  {
    real sum = 0 ;
  
    for( int n = 0 ; n <= k-i ; n++ )
      if( ! ((n+k-i)&1) ) // isEven
        sum += Tb( i,n,k ) * T( n, x ) ;

    return sum ;
  }

  // Associated Chebyshev polynomial
  static inline real Tlm( int l, int m, real x )
  {
    return sqrt( pow( 1 - x*x, m ) ) * TDiff( m, l, x ) ;
  }

  // precomputed K values for each (l,m) band.
  static real KcCoeffs[] ;

  // Normalization constants Kc for Chebyshev
  static __forceinline real Kc( int l, int m )
  {
    // The formula only works exactly for the zonals.  use the table instead for everything.
    //real temp = (((2.0*l+1.0)*facs[l-m)) / (4.0*PI*facs[l+m)))  *  ( (2.0*(l+m) - 1.0) / (2.0*l*l+m*m - 1.0) )  ;
    //return sqrt(temp);
  
    // always called on +l, +m,
    int entry = (l)*(l+1)/2 + m ; // std pyramid of values.
    // (l,m) ->
    // (2,2) -> (2*3/2)+2 => 5
    // (8,3) -> (8*9/2)+3 => 39

    // You should probably check the band size.
    return KcCoeffs[ entry ] ;
  }
  #pragma endregion
  
  //real sh(int l, int m, real theta, real phi) ;
  // evaluate the (l,m) band SH function at angle theta, phi
  // you get a RADIUS value (it's a spherical function),
  // so it'll be a radial distance as your result.
  // each SHSample must call this function once (BANDS*BANDS)
  // times during it's construction, to figure out
  // it's "coefficients" for each SH band.
  //static real operator() (int l, int m, real theta, real phi) ;
  //static real Y(int l, int m, real theta, real phi) ;

  // We make Y a function pointer, and it points to
  // either YSH or YCheby, and it's set up at program start time.
  static function<real (int l, int m, real theta, real phi)> Y ;

  // Spherical harmonics Y function
  static real YSH(int l, int m, real theta, real phi) ;

  // chebyshev sh
  static real YCheby(int l, int m, real theta, real phi) ;

  // from Ruedenberg (1996)
  // actually l,m,mm are ints, but
  // to avoid integer division casting clutter,
  // i made them reals
  static real a( int l, int m, int mm ) ;
  static real b( int l, int m, int mm ) ;
  static real c( int l, int m, int mm ) ;
  static real d( int l, int m, int mm ) ;

  // In this family of R functions, 
  // Finally, at the lowest level, the R==R1 function,
  // looks up rotation parameters in the matrix,
  // using the axis mapping
  //   x==1, ie index 1 looks up 0
  //   y==0 index     0 looks up 1
  //   z==-1 index   -1 looks up 2
  // the remapping is then 
  //   ( (-INDEX) + 1 ) ==> matrix index to look up.
  // If you change the remapping, you can get
  // the z-axis to be the axis from which the
  // tElevation angle 
  // Retrieves row i, col j from 3x3 matrix r.
  inline static real R( const Matrix& r, int i, int j )
  {
    // i and j are {-1,0,1}
    ////int ii = -( i - 1 ) ;
    ////int jj = -( j - 1 ) ;
    // ii and jj are {0,1,2}
    ///return r.m[ ii ][ jj ] ;
    // simpler
    
    //!! Something is transposed.  It works, but I 
    // had to transpose the row/col indexing, and
    // this is probably because the paper used
    // column major matrices (but my matrix r is row-major).
    // This makes sense because he used FORTRAN in his original implementation
    // and FORTRAN is column major.
    //return r.m[ -i+1 ][ -j+1 ] ;
    return r.m[ -j+1 ][ -i+1 ] ;
  }
  
  static real ulmm( int l, int m, int mm ) ;
  static real vlmm( int l, int m, int mm ) ;
  static real wlmm( int l, int m, int mm ) ;

  static real Ulmm( const Matrix& r, int l, int m, int mm ) ;
  static real Vlmm( const Matrix& r, int l, int m, int mm ) ;
  static real Wlmm( const Matrix& r, int l, int m, int mm ) ;

  // P CALLS R.
  static real iPlumm( const Matrix& r, int i, int l, int u, int mm ) ;

  // This version exploits the recursive formulation
  // of building sh rotation matrices and
  // constructs the entire matrix for each level
  // based on ONE LEVEL UP (so level 8 uses level 7 directly,
  // and assumes it already has been computed correctly, instead of
  // running ITS OWN computation back to level 1).
  // This is how it should be done, but now how it is formulated.
  ////static real Rlmm( SHRotationMatrix& r, int l, int m, int mm ) ;

  // Rmm takes a matrix r, a band l, and m,mm.
  // l: band
  // m: SOURCE
  // mm: DESTINATION.
  // Calling Rlmm(r,2,2,-2) generates the
  // rotation matrix coefficient that you need
  // to multiply y2,2 by to get y2,-2,
  static real Rlmm( const Matrix& r, int l, int m, int mm ) ;

  //void project( Shape* shape, int meshNo, int startVertex, int endVertex ) ; 

  // Constructs the rotation matrix
  static void Rotation( const Vector& axis, real radians, vector<real>& rotation ) ;

  // These functions generate a visualization
  // of an (l,m) band SH function.
  static void generateBasisFuncVisualizationPoints( int numPoints, int l, int m, const Vector& center ) ;

  // Generates a visualization for a band,m
  static Model* generateBasisFuncVisualization( int l, int m, int slices, int stacks, const Vector& center ) ;

} ;


// Complex spherical harmonics
// (not used)
////class CSH
////{
////public:
////  
////} ;


#endif