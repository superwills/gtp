#ifndef VizFunc_H
#define VizFunc_H

#include "../math/Vector.h"
#include "../math/EigenUtil.h"
#include "../geometry/Triangle.h"
#include "RayCollection.h"

struct RayCollection ;
class Scene ;
class SHVector ;
struct CubeMap ;




// A visibility function
// PROBABLY RENAME AO.
// Ambient occlusion is found by POSITION, in all directions
// around that position.
// It doesn't care what shape it's for, or WHAT shapes are causing
// the light blockage.  It only reports to you what directions you
// receive ambient light from, and how much.
// The normal passed in is used to weigh the visibility function.
// It's kind of odd to call it a "visibility" function when you
// are weighing in the dot product of the direction with the normal,
// but it makes it much easier to work with.
struct VizFunc
{
  static RayCollection* rc ;

  // a raw visibility function consists of
  // indices into the RAYCOLLECTION as well as
  // real weights for the strength of the visibility
  // in that direction.
  vector<int>  *rayIndices ;
  vector<real> *rayWeights ;

  VizFunc() ;

  void cleanup() ;
  ~VizFunc() ;

  // the "quality" of the visibility function is
  // how many rays you've used.

  // projects this visibility function to spherical harmonics.
  SHVector* toSH( int bands ) ;

  // So the cubemap you get will be approximate.
  // each "ray" in the cubemap looks for the CLOSEST
  // ray (dot product nearest to 1) that it can find
  // in the raycollection, and uses the weight for
  // that ray.  these cubemaps are always gray with full alpha.
  CubeMap* toCubeMap( int pxPerSide ) ;

  ///SphericalWavelet* toSphericalWavelet() ;
} ;




#endif