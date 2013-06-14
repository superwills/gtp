#ifndef SPARSE_MATRIX_H
#define SPARSE_MATRIX_H

#include "../util/StdWilUtil.h"
#include "../math/Vector.h"

#if 1

#include <vector>
using namespace std ;


struct SparseMatrix
{
  struct SparseMatrixElement
  {
    int index ;
    Vector val ;

    SparseMatrixElement( int iIndex, Vector iVal )
    {
      index = iIndex ;
      val   = iVal ;
    }
  } ;

  vector<SparseMatrixElement> vals ;

  void insert( int index, Vector& value )
  {
    // find the insertion point.
    /////for( int i = 0 ; i < vals.size() ; i++ )
    /////{ 
    /////  if( index < vals[i].index )
    /////  {  // you go before me:
    /////    printf( "Inserting index %d @ position %d, after is %d\n", index, i, vals[i].index ) ;
    /////    vals.insert( vals.begin() + i, SparseMatrixElement( index, value ) ) ;
    /////    return ;
    /////  }
    /////  else
    /////    printf( "%d NOT less than %d\n", index, vals[i].index ) ;
    /////}
    // they always go in order.
    vals.push_back( SparseMatrixElement( index, value ) ) ;
  }

  void clear()
  {
    vals.clear() ;
  }

  Vector dot( SparseMatrix& o )
  {
    SparseMatrix *smaller, *larger ;

    // iterate over "both at the same time"
    if( vals.size() < o.vals.size() )
    {
      // walk vals
      smaller = this ;
      larger = &o ;
    }
    else
    {
      smaller = &o ;
      larger = this ;
    }
    
    // walk LS
    Vector sum = 0 ;
    
    int j=0;
    for( int i = 0; i < smaller->vals.size() && j < larger->vals.size(); i++ )
    {
      while( smaller->vals[i].index < larger->vals[j].index )  i++ ;
      if( i >= smaller->vals.size() )  break ; // check oob
      
      while( larger->vals[j].index < smaller->vals[i].index )  j++ ;
      if( j >= larger->vals.size() )  break ; // check oob

      if( smaller->vals[i].index == larger->vals[j].index )
        sum += smaller->vals[i].val * larger->vals[j].val ;
    }

    return sum ;
  }
} ;

// First implementation (on par with Eigen)
#else

#include <map>
using namespace std ;

struct SparseMatrix
{
  map<int, Vector> vals ;

  void insert( int index, Vector& value )
  {
    vals.insert( make_pair( index, value ) ) ;
  }

  void clear()
  {
    vals.clear() ;
  }

  Vector dot( SparseMatrix& o )
  {
    SparseMatrix *LS, *RS ;

    // iterate over the smaller of the 2 
    if( vals.size() < o.vals.size() )
    {
      // walk vals
      LS = this ;
      RS = &o ;
    }
    else
    {
      LS = &o ;
      RS = this ;
    }
    
    // walk LS
    Vector sum = 0 ;
    
    for( map<int,Vector>::iterator LSIter = LS->vals.begin() ; LSIter != LS->vals.end() ; ++LSIter )
    {
      const int& key = LSIter->first ;

      // use the key, see if RS has a similar entry.
      map<int,Vector>::iterator RSIter = RS->vals.find( key );
      if( RSIter != RS->vals.end() )
        sum += RSIter->second * LSIter->second ;
    }

    return sum ;
  }
} ;
#endif

#endif