#ifndef ICOSAHEDRON_H
#define ICOSAHEDRON_H

#include "Shape.h"

enum MeshType;
enum VertexType;
struct Sphere ;

struct Icosahedron : public Shape
{
  Vector A,B,C,D, E,F,G,H, I,J,K,L ;

  real r ;

  Icosahedron( string iName, real ir, const Material& mat ) ;

  Icosahedron* createMesh( MeshType iMeshType, VertexType iVertexType ) ;

  Vector getCentroid() const override ;

  Icosahedron* transform( const Matrix& m, const Matrix& nT ) override ;

} ;

#endif