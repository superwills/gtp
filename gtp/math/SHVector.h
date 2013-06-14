#ifndef SHPROJECTION_H
#define SHPROJECTION_H

#include <functional>
#include <vector>
using namespace std ;

#include "Vector.h"
#include "SH.h"
#include "SHSample.h"

class SH ;
struct SHSample ;
struct Model ;
struct Shape ;

// An "SH projection" is the collection of
// a function's similarity with the "samples" (in class SH) 
// Automatically 
class SHVector
{
public:
  int bands ; // how many bands does this projection use?
  // it CANNOT use more than its SH generator class does

public:
  vector<Vector> coeffsScale ; // this wastes 1 float/entry, but it's not the memory use, it's the operations that limit
  
  SHVector() ;

  #if 0
  SHVector( const SHVector& other ) ;

  //SHVector( function<Vector (real tElevation, real pAzimuth)> funcToProject ) ;
  //SHVector( Shape* shapeToProject ) ;
  #endif

  // Convenience
  inline Vector at( int l, int m ) {
    return coeffsScale[ SH_INDEX( l,m ) ] ;
  }

  inline void set( int l, int m, const Vector& val ) {
    coeffsScale[ SH_INDEX(l,m) ] = val ;
  }

  void allocate() ;

  int save( FILE* file ) ;
  SHVector( FILE*file, int numCoeffs ) ;
  // I need a FUNCTION that will produce a value
  // at each theta, phi.  Then I can see how much
  // the SH functions are like each band.

  // Gets you the value at a (theta,phi)
  //real getFunctionValue( real tElevation, real pAzimuth ) ;
  Vector operator()( real tElevation, real pAzimuth ) ;
  ///!!!! ((Vector))

  SHVector* scaledCopy( real value ) const ;
  SHVector* scaledCopy( const Vector& value ) const ;

  bool isZero() ;

  void scale( real value ) ;

  // scale r,g,b independently by vector value
  void scale( Vector value ) ;
  
  SHVector* add( SHVector *other ) ;
  SHVector* addedCopy( SHVector *other ) ;

  // Rename this MINUS?
  SHVector* sub( SHVector *other ) ;
  SHVector* subFloor( SHVector *other ) ;
  SHVector* subbedCopy( SHVector *other ) ;
  SHVector* subbedFloorCopy( SHVector *other ) ;

  SHVector* mult( SHVector *proj ) ;
  SHVector* multedCopy( SHVector *proj ) ;

  // "cuts" bread with "knife"
  SHVector* cut( SHVector* cutter ) const ;

  // convolve THIS with another sh function
  // THIS must be a circularly symmetric kernel
  // ie have ONLY the zonal harmonics defined.
  // ONLY uses m=0 from this.
  SHVector* conv( SHVector* other ) const ;

  // zeros out everything except the zonal harmonics (m=0)
  void makeZonal() ;
  Vector dot( SHVector *other ) ;

  // rotates this SH function by rotation matrix r.
  SHVector* rotate( const Matrix& r ) const ;

  void integrateSample( SHSample& sample, real proportion, const Vector& color ) ;

  // This SUMS the base SH functions using
  // the weightings inside the coeffsScale
  Model* generateVisualization( int slices, int stacks, const Vector& center ) ;

  // AUTO ADDS TO scene->viz.
  void generateVisualizationAutoAdd( int slices, int stacks, const Vector& center ) ;

  void generateVisualizationR( int slices, int stacks, const Vector& center ) ;

  // prints the sh coefficients
  void print() ;

  // print the zonal harmonics only
  void printZonal() ;

} ;


struct SHMatrix
{
  int bands,rows,cols ;
  vector<Vector> coeff ;//rgb

  SHMatrix() ;
  Vector& operator()( int l1, int m1, int l2, int m2 ) ;
  Vector& operator()( int row, int col ) ;

  SHMatrix* add( SHMatrix* other ) ;
  SHMatrix* sub( SHMatrix* other ) ;
  SHMatrix* scale( real val ) ;
  SHMatrix* scale( const Vector& val ) ;
  SHMatrix* scaledCopy( real val ) const ;

  void integrateSample( SHSample& sample, const Vector& scale ) ;

  SHVector* mult( SHVector* vec ) ;
} ;


// post multiply a shmatrix by a shvector to
// the shMatrix WILL NOT change, but operator()
// isn't const,
// i avoid operator overloading for sh classes
//SHVector* operator*( SHMatrix& shMatrix, SHVector& shVector ) ;


#endif