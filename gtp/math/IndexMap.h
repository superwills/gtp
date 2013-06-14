#ifndef INDEXMAP_H
#define INDEXMAP_H

#include <vector>
using namespace std ;
#include "../window/GTPWindow.h"
struct CubeMap ;

// generates the index visitation map
struct IndexMap
{
  struct LatticeVisit
  {
    int index;
    double multiplier ;
    int toVisit[4] ;
    // result is multiplier*( toVisit[0] + toVisit[1] + toVisit[2] + toVisit[3] )

    LatticeVisit( int iindex, double imult, int i0, int i1, int i2, int i3 )
    {
      index = iindex ;
      multiplier = imult ;
      toVisit[0] = i0 ;
      toVisit[1] = i1 ;
      toVisit[2] = i2 ;
      toVisit[3] = i3 ;
      /////errchk(256);
    }
    void errchk( int size )
    {
      int max = 6 * SQUARE( size - 1 ) + (CUBE( size + 1 ) -  6 * SQUARE( size - 1 ) - CUBE( size-1 ));
      
      //printf( "max=%d\n", max ) ;
      if( !BetweenIn( index, 0, max-1 ) )
        error( "OOB" ) ;

      for( int i = 0 ; i < 4 ; i++ )
        if( !BetweenIn( toVisit[i], 0, max-1 ) )
          error( "OOB" ) ;
    }  
  } ;

  // the normalization actually depends on _signal content_.
  // so this object contains routines to analyze power AT
  // SPECIFIC ENTRIES, 
  struct LatticeNormalization
  {
    vector< int > indices ;

    // power before normalization, factor to correct current power to "what it was before"
    real powerBefore, factor ;

    void computePowerBefore( Vector* s )
    {
      powerBefore = computeSquarePower( s ) ;
      ///info( "The square power before is %f", powerBefore ) ;
    }

    real computeSquarePower( Vector* s )
    {
      real power = 0.0 ;
      
      // gets the power from a signal, using
      // indices provided in indices.
      for( int i = 0 ; i < indices.size() ; i++ )
        power += s[ indices[i] ].len2() ;

      return power ;
    }

    void apply( Vector* s )
    {
      // get the power after, then 
      real powerAfter = computeSquarePower( s ) ;
      ///info( "The square power AFTER is %f", powerAfter ) ;

      factor = sqrt( powerBefore/powerAfter ) ; // if it was 2, now it is 1, then factor=2.
      ///info( "Correction factor %f", factor ) ;

      // apply the multiplication factor to the signal,
      for( int i = 0 ; i < indices.size() ; i++ )
        s[ indices[i] ] *= factor ;

      // dbl check the power change was right
      real powerNow = computeSquarePower( s ) ;
      ///info( "The square power is NOW %f", powerNow ) ;
    }
  } ;
  
  // ordered list of vertices to visit.
  // the operation performed is
  // visit[ 0 ] contains the indices that index 0
  // should visit.
  vector< vector< LatticeVisit >* > visits, unvisits ;

  // for every lifting step (visits, unvisits) there
  // is a corresponding normalization.
  vector< LatticeNormalization* > normalizations, unnormalizations ;

  // generate the visitation order for this cubemap
  // it only does quincunx lattice wrapped transform.
  ///void generateLattice( CubeMap* cubemap ) ;

  void generateRailedLattice( CubeMap* cubemap ) ;

  // after each "lift"
  void railedLift( CubeMap* cubemap ) ;
  void railedUnlift( CubeMap* cubemap ) ;


};

#endif