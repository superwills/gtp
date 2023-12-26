#include "SHSample.h"
#include "../geometry/Model.h"

RayCollection* SHSample::rc ;

SHSample::SHSample( int iBands, int idx )
{
  dirIndex = idx ;
  // the number of bands will be the same as the SH object
  // this sample is "from"
  // I will resize my vector to allocate enough space
  // for n^2 samples.
  // Note that n^2 is THE MAXIMUM VALUE OF L+1,
  // since the first band is l=0.
  bandWeights.resize( iBands*iBands, 0 ) ;

  // based on the spherical angle you have to
  // go thru all bands and set coefficient values
  // to correct magnitudes SO THIS IS A SAMPLING
  // OF THE (l,m) band SH FUNCTION IN DIRECTION
  // (tElevation, pAzimuth)

  // gets spherical harmonic coefficients for
  // every band (every band that we are USING..!)
  // FOR THIS tElevation, pAzimuth
  // direction on the sphere.

  for( int l = 0 ; l < iBands; l++ )
    for( int m = -l ; m <= l ; m++ )
      bandWeights[ SH_INDEX( l,m ) ] = SH::Y( l,m, rc->tEl(dirIndex), rc->pAz(dirIndex) ) ;
}

SHSample::SHSample( int iBands, SVector sv )
{
  dirIndex = -1 ;  // Error should occur if you try and use this
  // to index the rayCollections arrays.

  bandWeights.resize( iBands*iBands, 0 ) ;
  for( int l = 0 ; l < iBands; l++ )
    for( int m = -l ; m <= l ; m++ )
      bandWeights[ SH_INDEX( l,m ) ] = SH::Y( l,m, sv.tElevation,sv.pAzimuth ) ;
}
  
void SHSample::checkIndex( int &l, int &m )
{
  if( l < 0 )
  {
    error( "l < 0, this is wrong. Setting l=0" ) ;
    l=0 ;
  }
  if( abs(m) > l )
  {
    error( "|m| > l (|%d| > %d), this is wrong. Setting m=l", m, l ) ;
    m=l;
  }
}

void SHSample::setWeight( int l, int m, real coeff )
{
  // Find the index as l*(l+1)+m, which gives a valid
  // coeff for any l, m with l>=0 and |m| <= l
#ifdef _DEBUG
  checkIndex( l, m ) ;
#endif
  bandWeights[ SH_INDEX( l,m ) ] = coeff ;
}

real SHSample::getWeight( int l, int m )
{
#ifdef _DEBUG
  checkIndex( l, m ) ;
#endif
  return bandWeights[ SH_INDEX( l,m ) ] ;
}


