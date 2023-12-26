#ifndef RAYCOLLECTION_H
#define RAYCOLLECTION_H

#include "../math/Vector.h"

// keeps both the cartesian and spherical vectors cached
struct VectorPair
{
  Vector c;
  SVector s;

  VectorPair(){}

  VectorPair( const Vector& iv ){
    c = iv;
    s = iv.toSpherical() ;
  }
  VectorPair( const SVector& sv ){
    s = sv ;
    c = sv.toCartesian() ;
  }
} ;

struct RayCollection
{
  int n ;

  // the cases are different to more visually distinguish them.
  vector<VectorPair> randomDirs ;

  // these are unit vectors on the unit hemisphere
  // proportional to cosine-weighted solid angle
  vector<VectorPair> cosineDirs ;

  // THIS is PRECOMPUTED cubemap pixel directions,
  // in a standard order (top left corner of PX face.. NX, PY, NY..)
  // (same order as in CubeFace enum).  Does not store the rail directions.
  map< int, vector<VectorPair> > cubemapPixelDirs ; // int keys are cubemap sizes, like 64, 256

  // sphere dirs
  vector<VectorPair> octSphereDirs ; // octahedral sphere directions, precomputed.

  // n is how many rays you want.
  RayCollection( int N )
  {
    n = N ;
    randomDirs.resize( n ) ;
    for( int i = 0 ; i < n ; i++ )
      randomDirs[i] = VectorPair( SVector::random( 1 ) ) ; // 

    // cubemappixeldirs is done in CubeMap::pregenDirections()
    cosineDirs.resize( n ) ;
    for( int i = 0 ; i < n ; i++ )
      cosineDirs[i] = VectorPair( SVector::randomHemiCosine( 1 ) ) ; // NOT equally distributed on the hemisphere. more at pole.

  }

  // Retrieval functions wrap idx so that
  // if you are OOB the actual #samples array,
  // you wrap.  This way client code can
  // 
  // Defaults to randomDirs, but 
  inline real tEl( int idx ) { return randomDirs[ idx%n ].s.tElevation ; } 
  inline real pAz( int idx ) { return randomDirs[ idx%n ].s.pAzimuth ; } 

  // COPY
  inline Vector v( int idx ) { return randomDirs[ idx%n ].c ; }

  // AVOID COPY
  inline Vector& vAddr( int idx ) { return randomDirs[ idx%n ].c ; }

  // give you a vector, rejecting those more than 90 degrees from normal
  inline Vector vAbout( const Vector& normal ) {
    int idx = randInt( 0, n ) ; // start at a random position
    while( randomDirs[ idx%n ].c % normal < 0 )  idx++;
    return randomDirs[ idx%n ].c ;
  }

  inline Vector& vAboutAddr( const Vector& normal ) {
    int idx = randInt( 0, n ) ; // start at a random position
    while( randomDirs[ idx%n ].c % normal < 0 )  idx++;
    return randomDirs[ idx%n ].c ;
  }

  inline Vector vAbout( int & idx, const Vector& normal ) {
    // advance idx until you reach one that is 90 degrees about the normal.
    // idx is advanced (pass by ref) so that if you are iterating here,
    // then it means you are probably are using the same normal multiple times
    while( randomDirs[ idx%n ].c % normal < 0 )  idx++;
    return randomDirs[ idx%n ].c ;
  }

  inline Vector& vAboutAddr( int & idx, const Vector& normal ) {
    while( randomDirs[ idx%n ].c % normal < 0 )  idx++;
    return randomDirs[ idx%n ].c ;
  }
  
  inline Vector vCos( int idx ){
    return cosineDirs[ idx%n ].c ;
  }
} ;

#endif