#ifndef SHSAMPLE_H
#define SHSAMPLE_H

#include "SH.h"
#include "../math/Vector.h"
#include "../rendering/RayCollection.h"

class SH ;

// An SHSample is a "piece" of EVERY spherical harmonic
// function.

// it's basically a DIRECTION, and HOW BIG THE SH BASIS FUNCTION
// IS IN EACH BAND IN THAT DIRECTION.

// ALWAYS HAS SAME NUMBER OF BANDS AS PARENT SH FUNCTION

// So take an SHSample with direction (1,0,0).
//   - In band (0,0), the "coefficient" is 1.
//   - That means the SH band (0,0) function's shape
//     measures 1.0 units in length in direction (1,0,0)
//     (it's a perfect sphere).
//   - Do the same for the other bands.
//     

// I don't like the name Sample, but to approximate
// SH functions numerically we need to "sample" the functions.
// This is what each of theses "samples" represent:
//   - Each sample is a "direction" (or a (thetaElevation,phiAzimuth) pair on the sphere)
//   - the vector of "weights" is the coefficient FOR THAT DIRECTION, FOR EACH BAND (from 0..L,
//     with m additional functions per band)
struct SHSample //!! SEAL THIS CLASS
{
  int dirIndex ; // INDEXES RAYCOLLECTION  // vector that describes the DIRECTION of this sample
  static RayCollection* rc ; // static member that points to the rayCollection to use.
  ///real tElevation, pAzimuth ; // cache the phi/theta so you don't have to recompute them later
  // (need these as well as Cartesian dir). (Radius of SVector is always 1).

  // For each direction, we need to store
  // the length of the vector.
  vector<real> bandWeights ;  // the vector of COEFFS are the WEIGHTS IN EACH BAND
  // describe how strong THIS sample is in EACH band from 0..L (with +m functions).
  // The band weights are stored with the sample because its easier.
  // One BANDWEIGHT for each (l,m) band.

  // Now what you COULD do instead is scale the sample vectors,
  // so there are just a `vector<Vector> samples' in each SHSample
  // and you'd have an array `vector<SHSample> bands' in class SH.
  // (l,m) indexed by the SH_INDEX() macro.

  // so that would be:
  ////vector<Vector> values ; // this has sh->bands*sh->bands entries,
  // each vector is the l,m band value,
  // so for the cost of 3*bands*bands MORE floats, you save
  // 3 multiplies each time you call for the values of a shband
  
  // (but that would be more
  // memory intensive, but not by that much + retrieval
  // of a scaled sample vector would be cheaper, BUT
  // retrieving the coefficient for a dir would be more
  // expensive (.len() operation))

  // Construct SHSample by evaluating the
  // SH functions at each available band (relies on SH superglobal,
  // which can be changed to a static class variable here if needed).
  // I need you to pass me the rayCollection because in GTPWindow ctor
  // GTPWindow object isn't init.
  SHSample( int iBands, int idx ) ;
  
  // Construct an SHSample based on a vector.
  SHSample( int iBands, SVector sv ) ;

  // Retrieve the directions corresponding to this sample.
  inline Vector dir() { return rc->randomDirs[ dirIndex ].c ; }
  inline SVector sdir() { return rc->randomDirs[ dirIndex ].s ; }

  inline real tEl() { return rc->randomDirs[ dirIndex ].s.tElevation ; }
  inline real pAz() { return rc->randomDirs[ dirIndex ].s.pAzimuth ; }

  // debugging function (make sure l and m are valid indices)
  // corrects l & m if they are invalid
  void checkIndex( int &l, int &m ) ;

  /// Set a coefficient weight at a particular (l,m) band.
  void setWeight( int l, int m, real coeff ) ;

  /// Get coefficient weight at (l,m) band.
  /// The integer band index is given by
  /// (l*(l+1)+m) (SH_INDEX macro)
  real getWeight( int l, int m ) ;
} ;

// The special thing about SHSampleCollection is
// it PRESAMPLES the SH functions for every ray,
// which reduces compute time when running SH computations
struct SHSampleCollection
{
  // this wraps the collection of shsamples,
  // keeps important members like
  int n ; // sample collection size
  int nU ; // number samples to USE
  int bands ; // number of SH bands to use for all projections
  int bandsToRender ; // number of SH bands to use IN RENDERING only (allows us to see
  // what using fewer SH bands would look like at a glance)
  vector< SHSample > samps ; // the sh samples.  CAN go in rayCollection
  real solidAngleRay, dotScaleFactor ; 
  
  SHSampleCollection( int iBands, int N, int NU )
  {
    bandsToRender = bands = iBands ;
    
    if( !SHSample::rc )
    {
      error( "Create SHSample::rc first" ) ;
      return ;
    }

    n = N ;
    nU = NU ; // typically use 1/10th of available samples.  make 100k, use 10k.
    if( n > SHSample::rc->n )
    {
      warning( "I can only make as many SHSamples as I have RayCollection rays for (%d > %d), clamping..", n, SHSample::rc->n ) ;
      n = SHSample::rc->n ;
      nU = n/10 ;// NU was probably wrong, just use 1/10th.
    }

    // each sample REPRESENTS a certain solid angle.
    // This SOLID ANGLE is: 4*PI (SA unit sphere) divided by #samples you have.
    // The fewer samples you have, the cruder the approximation.
    // If you have 1 sample, then that sample is supposedly
    // approximating the whole sphere (4PI),
    // if you have 20 samples, then each sample
    // represents (4PI/20) steradians.
    
    solidAngleRay = (4.0*PI)/nU ; 
    
    // Divide by PI to normalize the fact that
    // the cosine lobe integrates to PI
    //dotScaleFactor = ( 4.0*PI/nU ) / PI ;
    dotScaleFactor = 4.0/nU ;//simplified
  
    

    //logRichText->trackProgress( 1, "starting sh" ) ;
    info( Magenta, "Generating %d SH samples with %d bands..", n, bands ) ;
    for( int i = 0 ; i < n ; i++ )
    {
      printf( "Sample %d / %d\r", i+1, n ) ;
      samps.push_back( SHSample( bands, i ) ) ;
      //logRichText->updateProgress( 1, 0.5 ) ;
    }

    //logRichText->killProgress( 1 ) ;
    printf( "Done generating samples\n" ) ;
    info( Magenta, "SH-done your samples" ) ;
  }
} ;







#endif