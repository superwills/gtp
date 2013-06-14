#ifndef SPHERE_H
#define SPHERE_H

#include "../math/Vector.h"
#include "../scene/Material.h"
#include "Shape.h"

enum MeshType ;
enum VertexType ;

struct Sphere : public Shape
{
  enum SphereConstruction
  {
    // construct by walking phi/theta.  this
    // generates a sphere with really obvious poles
    // at the top and bottom, higher tesselation
    // at these poles.
    SphericalCoordWalk,
    GeodesicOctahedron
  } ;
  Vector pos ;
  real radius ;
  
  int slices ;
  int stacks ;

protected:
  // for CubeMap, and Octahedron
  Sphere() {hasMath=true; material=Material::Red ;}

  // Octahedron is a friend class because sphere
  // can be made using an octahedron as a base.
  friend struct Octahedron ;
  friend struct Icosahedron ;

public:
  Sphere( MeshType meshType, VertexType vertexType,
          string iName, const Vector & iPos, real iRadius,
          const Material& iMaterial, int iSlices, int iStacks ) ;

  Sphere( string iName, const Vector & iPos, real iRadius,
          const Material& iMaterial, int iSlices, int iStacks ) ;

  Sphere( FILE *file ) ;
  int save( FILE *file ) override ;

  Sphere* transform( const Matrix& m ) override ;
  Sphere* transform( const Matrix& m, const Matrix& nT ) override ;

  /// Returns the normal to the sphere
  /// at a point on the sphere nearest
  /// "position"
  Vector getNormalAt( const Vector& position ) const ;
  bool intersectExact( const Ray& ray, Intersection* intn ) override ;

  // overidden by CubeMap
  virtual Sphere* createMesh( MeshType meshType, VertexType vertexType ) ;

  Vector getCentroid() const override ;

  Vector getRandomPointFacing( const Vector& dir ) override ;
  
  // you must raymesh for this, because you have no idea
  // about the color @ vertex
  ///Vector getColor( real tElevation, real pAzimuth ) override ;

  // trivial for sphere
  real getRadialDistance( real tEl, real pAz, bool fromCentroid, bool outerShell ) override
  {
    if( fromCentroid )
      return radius; // from the centroid of a sphere the radial distance is always the radius
    else // it's not going to be relative to the sphere's distance from the origin. use raycast in base class
      return Shape::getRadialDistance( tEl, pAz, fromCentroid, outerShell ) ;
  }
  
  // Really meant to be used with Icosahedron and Octahedron,
  // it creates a sphere using triangular subdivisions and
  // forcing points at each subdivision stage to
  // sit on the surface of the sphere.
  static Sphere* fromShape( string iName, Shape* shape, real r, int subdivLevels ) ;

  static Sphere* fromIcosahedron( string iName, real r, int subdivLevels, const Material& iMaterial ) ;

} ;

#endif