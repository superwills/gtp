#ifndef OCTAHEDRON_H
#define OCTAHEDRON_H

#include "Shape.h"

enum MeshType;
enum VertexType;

struct Sphere ;

struct Octahedron : public Shape
{
  Vector A,B,C,D,E,F ;

  // original size parameter of this Octahedron
  real r ;

  Octahedron( string iName, real ir, const Material& mat ) ;

  Octahedron* createMesh( MeshType iMeshType, VertexType iVertexType ) ;

  Vector getCentroid() const override ;

  Octahedron* transform( const Matrix& m, const Matrix& nT ) override ;

  
} ;

#endif