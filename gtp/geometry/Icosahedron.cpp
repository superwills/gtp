#include "Icosahedron.h"
#include "Mesh.h"
#include "Sphere.h"
#include "../window/GTPWindow.h"

Icosahedron::Icosahedron( string iName, real ir, const Material& mat ) 
{
  r = ir ;
  name = iName ;
  material = mat ;

  createMesh( defaultMeshType, defaultVertexType ) ;
}

Icosahedron* Icosahedron::createMesh( MeshType iMeshType, VertexType iVertexType )
{
  Mesh *mesh = new Mesh( this, iMeshType, iVertexType ) ;

  //http://en.wikipedia.org/wiki/Icosahedron
  //(0, ±1, ±φ)
  //(±1, ±φ, 0)
  //(±φ, 0, ±1)
  //where φ = (1 + √5) / 2 

  const real t = ( 1 + sqrt( 5.0 ) ) / 2.0 ;
  // So each side has a length of sqrt( t*t + 1.0 )
  real rScale = r / sqrt( t*t + 1.0 ) ; // correct the radius
  Vector* v = &A ;
  
  for( int i = 0 ; i < 4; i++ )
    //v[ i ] = Vector( 0, -(i&2), -(i&1)*t ) ; 
    v[ i ] = rScale*Vector( 0, i&2?-1:1, i&1?-t:t ) ; 

  for( int i = 4 ; i < 8; i++ )
    //v[ i ] = Vector( -(i&2), -(i&1)*t, 0 ) ; 
    v[ i ] = rScale*Vector( i&2?-1:1, i&1?-t:t, 0 ) ; 

  for( int i = 8 ; i < 12; i++ )
    //v[ i ] = Vector( -(i&1)*t, 0, -(i&2) ) ; 
    v[ i ] = rScale*Vector( i&1?-t:t, 0, i&2?-1:1 ) ; 


  mesh->addTri( v[0], v[2], v[8] ) ;
  mesh->addTri( v[0], v[8], v[4] ) ;
  mesh->addTri( v[0], v[4], v[6] ) ;
  mesh->addTri( v[0], v[6], v[9] ) ;
  mesh->addTri( v[0], v[9], v[2] ) ;

  mesh->addTri( v[2], v[7], v[5] ) ;
  mesh->addTri( v[2], v[5], v[8] ) ;
  mesh->addTri( v[2], v[9], v[7] ) ;
    
  mesh->addTri( v[8], v[5], v[10] ) ;
  mesh->addTri( v[8], v[10], v[4] ) ;
  
  mesh->addTri( v[10], v[5], v[3] ) ;
  mesh->addTri( v[10], v[3], v[1] ) ;
  mesh->addTri( v[10], v[1], v[4] ) ;
  
  mesh->addTri( v[1], v[6], v[4] ) ;
  mesh->addTri( v[1], v[3], v[11] ) ;
  mesh->addTri( v[1], v[11], v[6] ) ;

  mesh->addTri( v[6], v[11], v[9] ) ;

  mesh->addTri( v[11], v[3], v[7] ) ;
  mesh->addTri( v[11], v[7], v[9] ) ;

  mesh->addTri( v[3], v[5], v[7] ) ;

  addMesh( mesh ) ;

  return this ;
}

Vector Icosahedron::getCentroid() const
{
  return (A+B+C+D + E+F+G+H + I+J+K+L) / 12 ;
}

Icosahedron* Icosahedron::transform( const Matrix& m, const Matrix& nT )
{
  A = A * m ;
  B = B * m ;
  C = C * m ;
  D = D * m ;
  
  E = E * m ;
  F = F * m ;
  G = G * m ;
  H = H * m ;
  
  I = I * m ;
  J = J * m ;
  K = K * m ;
  L = L * m ;

  Shape::transform( m, nT ) ;

  return this ;
}

