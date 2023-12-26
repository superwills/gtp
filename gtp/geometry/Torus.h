#ifndef TORUS_H
#define TORUS_H

#include "Shape.h"

enum MeshType ;
enum VertexType ;

struct Torus : public Shape
{
  real aDonutThickness, cRingRadius ;
  int slices,sides ;

  // the default donut goes along the
  // has the z-axis passing through it's hole.
  // if a >> c, it becomes a sphere
  // if c >> a, it becomes a thin, thin ring (like the ring around saturn)
  Torus( string iName, const Vector& iPos,
    real donutThicknessRadius, real ringRadius,
    const Material& iMaterial,
    int iSlices, int iSides ) ;
  
  void createMesh( MeshType iMeshType, VertexType iVertexType ) ;
  
  Vector eval( real iADonutThickness, real icRingRadius, real u, real v ) ;
} ;

#endif