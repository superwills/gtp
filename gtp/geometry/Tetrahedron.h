#ifndef TETRAHEDRON_H
#define TETRAHEDRON_H

#include "Shape.h"

enum MeshType ;
enum VertexType ;

struct Tet : public Shape
{
  Vector topFront ;
  Vector topBack ;
  Vector bottomFront ;
  Vector bottomBack ;

  /// Creates a tetrahedron
  /// center: Tet's center in space
  /// a: Tet's edge length
  /// and tet's material
  /// are accepted as parameters
  Tet( MeshType meshType, VertexType vertexType, string iname, const Vector& center, real a, const Material& iMaterial ) ;
  Tet( string iname, const Vector& center, real a, const Material& iMaterial ) ;
private:
  void calcPoints( const Vector& center, real a ) ; // ctor helper function
public:
  Tet( FILE * file ) ;
  int save( FILE *file ) override ;

  // No need to override basic AABB/mesh
  //Intersection* Tet::intersects( const Ray& ray ) const override ;

  Tet* createMesh( MeshType meshType, VertexType vertexType ) ;

  Vector getCentroid() const override ;

} ;

#endif