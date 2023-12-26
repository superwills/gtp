#include "Intersection.h"
#include "Mesh.h"
#include "Triangle.h"
#include "../math/perlin.h"

// static variables
Intersection Intersection::HugeIntn = Intersection( Vector(HUGE,HUGE,HUGE), Vector(0,0,1), NULL ) ;
MeshIntersection MeshIntersection::HugeMeshIntn = MeshIntersection( Vector(HUGE,HUGE,HUGE), Vector(0,0,1), Vector(0,0,1), NULL ) ;
MeshIntersection MeshIntersection::SmallMeshIntn = MeshIntersection( Vector(0), Vector(0,0,1), Vector(0,0,1), NULL ) ;


Intersection::Intersection()
{
  shape = 0 ;
  point = 0 ;
}

Intersection::Intersection( const Vector& pt, const Vector& norm, Shape*iShape ) : point(pt), normal(norm), shape(iShape)
{
}

bool Intersection::isCloserThan( Intersection* intn, const Vector& toPoint )
{
  return distanceBetween2( this->point, toPoint )   <   distanceBetween2( intn->point, toPoint ) ;
}

real Intersection::getDistanceTo( const Vector& toPoint )
{
  return distanceBetween( point, toPoint ) ;
}

Vector Intersection::getColor( ColorIndex colorIndex )
{
  // an implicit shape uses the MATERIAL,
  // since there are no vertex values to store.

  // MATHEMATICAL shapes will have a completely
  // different color everywhere though, determined
  // by their phi,theta.
  
  // procedural texture must be applied
  // at point of intersection
  Vector baseColor = shape->material.getColor( colorIndex ) ;
  
  if( shape->material.hasPerlinTexture( colorIndex ) )
    return baseColor*shape->material.getPerlinTextureColor( point, colorIndex ) ;
  else
    return baseColor ;
}

bool Intersection::didHit()
{
  return (bool)shape ; // if the shape is null, then nothing was hit.
}

MeshIntersection::MeshIntersection()
{
  tri = 0 ; // if this is null, then it means the MeshIntn has not been init.
}

MeshIntersection::MeshIntersection( const Vector& pt, const Vector& norm, const Vector& iBary, Triangle* iTri ) :
    Intersection( pt, norm, NULL ), bary(iBary), tri( iTri )
{
  if( iTri )  this->shape = iTri->meshOwner->shape ; //cannot pass to Intersection ctor b/c may be NULL
}

Mesh *MeshIntersection::getMesh()
{
  if( tri )  return tri->meshOwner ; // the mesh that was intersected (that owned the tri)
  else return NULL ;
}

Vector MeshIntersection::getColor( ColorIndex colorIndex )
{
  if( !tri ) {  error( "Trying to getColor() on Intersection object that has no hit" ) ; return 0 ;  }
  
  // IF THERE'S A PERLIN TEXTURE, use it,
  Vector weightedColor = (bary.x * tri->vA()->color[ colorIndex ] +
                          bary.y * tri->vB()->color[ colorIndex ] +
                          bary.z * tri->vC()->color[ colorIndex ]) ;
  
  if( shape->material.hasPerlinTexture( colorIndex ) )
    return weightedColor * shape->material.getPerlinTextureColor( point, colorIndex ) ; // procedural texture must be applied at point of intersection.
  
  // DON'T query the material here, that should have already been used in vertex color creation
  // Diffuse, Emissive, Specular, Transmissive materials
  // all may NOT be uniform throughout the mesh.
  // SH Computation changes the color at each vertex.
  // for the other cases, 
  else
    return weightedColor ;
}

Vector MeshIntersection::getTexcoord( TexcoordIndex texIndex )
{
  return bary.x * tri->vA()->texcoord[ texIndex ] +
         bary.y * tri->vB()->texcoord[ texIndex ] +
         bary.z * tri->vC()->texcoord[ texIndex ] ;
}

real MeshIntersection::getAO()  //interpolated
{
  return bary.x * tri->vA()->ambientOcclusion +
         bary.y * tri->vB()->ambientOcclusion +
         bary.z * tri->vC()->ambientOcclusion ;
}

///Vector MeshIntersection::getInterpolatedNormal()
///{
///  return bary.x * tri->vA()->norm +
///         bary.y * tri->vB()->norm +
///         bary.z * tri->vC()->norm ;
///}