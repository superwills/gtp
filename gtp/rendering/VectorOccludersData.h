#ifndef VO_DATA_H
#define VO_DATA_H

#include <map>
using namespace std ;

#include "../math/Vector.h"

struct Mesh ;

// the data needed at each vertex for Vector Occluders,
// PER MESH.
struct VOVectors
{
  // average position of the mesh FROM THE PERSPECTIVE of vertex
  Vector pos ;
  
  //Vector normShadow ; // the straight averaged normal. used for shadow. -this tells you WHAT DIRECTION IS __BEHIND__ this surface.
  // normShadow PHASED OUT in favor of shadowDir.

  // Average direction and strength of blockage due to this mesh.
  // This is the average of direction vectors WE SHOT
  // to see this blocker.
  Vector shadowDir ;
  
  // direction from which a caustic is received
  // for this vertex DUE TO this mesh.
  Vector causticDir ;
  
  Vector normDiffuse ;  // ff weighted average normal of diffuse contribution from this mesh.
  // AO of sender patch is used to reduce contribution of
  // occluded senders.

  Vector normSpecular ; // ff weighted average normal with blocked reflectors removed from the average

  Vector colorDiffuse ; // the avg diffuse color of this mesh.
  Vector colorSpecular ;// the avg specular reflectance of this mesh.
  Vector colorTrans ;   // the average transmissive color of this mesh.

  void Add( const VOVectors & other )
  {
    pos           += other.pos ;
    shadowDir     += other.shadowDir ;
    causticDir    += other.causticDir ;
    normDiffuse   += other.normDiffuse ;
    normSpecular  += other.normSpecular ;
    colorDiffuse  += other.colorDiffuse ;
    colorSpecular += other.colorSpecular ;
    colorTrans    += other.colorTrans ;
  }

  void Divide( real val )
  {
    pos           /= val ;
    shadowDir     /= val ;
    causticDir    /= val ;
    normDiffuse   /= val ;
    normSpecular  /= val ;
    colorDiffuse  /= val ;
    colorSpecular /= val ;
    colorTrans    /= val ;
  }
} ;

struct VectorOccludersData
{
  // for each shape in the scene,
  // we maintain ONE average "position"
  // and average normal of the reflector.
  // this is because specular interreflection is
  // FROM THE SCENE GEOMETRY, so using the objects
  // (instead of octants of space or whatever)
  // to index makes the most sense.
  map< Mesh*, VOVectors > reflectors ;
} ;

#endif