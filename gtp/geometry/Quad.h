#ifndef QUAD_H
#define QUAD_H

#include "../util/StdWilUtil.h"
#include "Shape.h"

enum MeshType;
enum VertexType;

struct Quad : public Shape
{
  Vector a,b,c,d ;

  // Exactly as Mesh::addTessQuad() expects it.
  // B--A
  // |  |
  // C--D
  Quad( string iname, const Vector& ia, const Vector& ib, const Vector& ic, const Vector& id, int uPatches, int vPatches, const Material& iMaterial ) ;

  Quad( MeshType meshType, VertexType vertexType, 
        string iname, const Vector& ia, const Vector& ib, const Vector& ic, const Vector& id,
        int uPatches, int vPatches, const Material& iMaterial ) ;

  Quad* createMesh( MeshType meshType, VertexType vertexType, int uPatches, int vPatches ) ;

  // mandatory to be able to give the centroid
  Vector getCentroid() const ;
} ;

#endif