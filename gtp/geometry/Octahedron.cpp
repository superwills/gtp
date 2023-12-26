#include "Octahedron.h"
#include "Mesh.h"
#include "Sphere.h"

Octahedron::Octahedron( string iName, real ir, const Material& mat )
{
  r = ir ;
  name = iName ;
  material=mat;
  // an octahedron is created by
  // setting points on each of the 6 axes
  A = Vector( 0, r, 0 ) ;
  B = Vector( r, 0, 0 ) ;
  C = Vector( 0, 0, r ) ;
  D = Vector(-r, 0, 0 ) ;
  E = Vector( 0, 0,-r ) ;
  F = Vector( 0,-r, 0 ) ;

  createMesh( defaultMeshType, defaultVertexType ) ;
}

Octahedron* Octahedron::createMesh( MeshType iMeshType, VertexType iVertexType )
{
  Mesh*mesh = new Mesh( this, iMeshType, iVertexType ) ;

  //Vector n1( 1/√3, 1/√3, 1/√3 ) ;
  //Vector n2(-1/√3, 1/√3, 1/√3 ) ;
  //Vector n3(-1/√3, 1/√3,-1/√3 ) ;
  //Vector n4( 1/√3, 1/√3,-1/√3 ) ;

  //mesh->addTri(
  //  AllVertex( A, n1, material ),
  //  AllVertex( C, n1, material ),
  //  AllVertex( B, n1, material ) ) ;
  mesh->addTri( A, C, B ) ;
  mesh->addTri( A, D, C ) ;
  mesh->addTri( A, E, D ) ;
  mesh->addTri( A, B, E ) ;

  mesh->addTri( F, B, C ) ;
  mesh->addTri( F, C, D ) ;
  mesh->addTri( F, D, E ) ;
  mesh->addTri( F, E, B ) ;

  addMesh( mesh ) ;
  return this ;
}

Vector Octahedron::getCentroid() const
{
  return (A+B+C+D+E+F)/6;
}

Octahedron* Octahedron::transform( const Matrix& m, const Matrix& nT )
{
  A = A * m ;
  B = B * m ;
  C = C * m ;
  D = D * m ;
  
  E = E * m ;
  F = F * m ;

  Shape::transform( m, nT ) ;

  return this ;
}





