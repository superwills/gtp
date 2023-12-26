#include "Cube.h"
#include "../window/GTPWindow.h"

Cube::Cube( 
  MeshType meshType, VertexType vertexType, 
  string iname,
  const Vector& center, real s, //HALF sidelen
  int uPatches, int vPatches,
  real yaw, real pitch, real roll,
  Material iMaterial
) :
A( -s, -s, -s ),
B( -s, -s,  s ),
C( -s,  s, -s ),
D( -s,  s,  s ),

E(  s, -s, -s ),
F(  s, -s,  s ),
G(  s,  s, -s ),
H(  s,  s,  s )
{
  this->name = iname ;
  this->material = iMaterial ;
  
  Matrix rot = Matrix::RotationYawPitchRoll( yaw, pitch, roll ) ;
  Matrix trans = Matrix::Translate( center ) ;

  // rotate first, then translate.
  rot = trans * rot ;
    
  // rotation is first to "spin" the object in place.
  // translation follows about the coordinate axes to
  // move it where you want it
  // if you TRANSLATE first, then you will get
  // this "swinging" of the object, not a spinning,
  // because the object is now NOT centered at the
  // origin, so its vertices will swing about the
  // origin, from far away from it, which is not
  // what you want.
  
  //if( normalsOut )
  // this doesn't work unless you flip
  // the vertex positions AFTER computation
  // of the normals.
  //{
  //  /*

  //    C----G        F----B
  //   /|   /|  =>   /|   /|
  //  D-A--H E  =>  E-H--A D
  //  |/   |/   =>  |/   |/
  //  B----F        G----C

  //  */
  //  // inside out the box to reverse the winding.
  //  rot = rot * Matrix::Scale( -1, -1, -1 ) ; // post multiply
  //  // to apply this operation first
  //}


  // Rotate the cube if you like here
  transform( rot, rot.getNormalMatrix() ) ;

  /*

    C----G
   /|   /|
  D-A--H E
  |/   |/
  B----F

  */

  createMesh( meshType, vertexType, uPatches, vPatches ) ;
}

Cube::Cube( string iname,
  const Vector& center, real s, //HALF sidelen
  int uPatches, int vPatches,
  real yaw, real pitch, real roll,
  Material iMaterial
) :
A( -s, -s, -s ),
B( -s, -s,  s ),
C( -s,  s, -s ),
D( -s,  s,  s ),

E(  s, -s, -s ),
F(  s, -s,  s ),
G(  s,  s, -s ),
H(  s,  s,  s )
{
  name = iname ;
  material = iMaterial ;
  
  Matrix rot = Matrix::RotationYawPitchRoll( yaw, pitch, roll ) ;
  Matrix trans = Matrix::Translate( center ) ;

  rot = trans * rot ;
  transform( rot, rot.getNormalMatrix() ) ;
  createMesh( MeshType::Nonindexed, defaultVertexType, uPatches, vPatches ) ;
}

Cube::Cube( 
  const Vector& min,
  const Vector& max
) :
A( min ),
B( min.x, min.y, max.z ),
C( min.x, max.y, min.z ),
D( min.x, max.y, max.z ),

E( max.x, min.y, min.z ),
F( max.x, min.y, max.z ),
G( max.x, max.y, min.z ),
H( max )
{
  //       y
  //     ^
  //     |
  //    C----G
  //   /|   /|
  //  D-A--H E  -> x
  //  |/   |/ 
  //  B----F  
  //  /
  // z

  /////createMesh() ;//!! this does NOT call compute mesh, because
  // it results in an infinite recursion when an AABB constructs this
  // cube to compute its mesh..

  // uh, make a random material in case its drawn
  material = Material::randomSolid() ;
}

Cube::Cube( 
    string iname,
    const Vector& min,
    const Vector& max,
    int uPatches, int vPatches,
    Material iMaterial
  ):
A( min ),
B( min.x, min.y, max.z ),
C( min.x, max.y, min.z ),
D( min.x, max.y, max.z ),

E( max.x, min.y, min.z ),
F( max.x, min.y, max.z ),
G( max.x, max.y, min.z ),
H( max )
{
  name = iname ;
  material = iMaterial ;
  createMesh( MeshType::Nonindexed, defaultVertexType, uPatches, vPatches ) ;
}

Cube::Cube( FILE *file ) : Shape( file )
{
  fread( &this->A, sizeof(Vector), 8, file ) ;
}

int Cube::save( FILE *file ) //override
{
  HeaderShape head ;
  head.ShapeType = CubeShape ;
  int written = 0 ;
  written += sizeof(HeaderShape)*fwrite( &head, sizeof( HeaderShape ), 1, file ) ;
  written += Shape::save( file ) ;
  written += fwrite( &A, sizeof(Vector), 8, file ) ;
  return written ;
}

Cube* Cube::createMesh( MeshType meshType, VertexType vertexType, int uPatches, int vPatches )
{
  Mesh *mesh = new Mesh( this, meshType, vertexType ) ;
  // wind so ccw facing __OUTSIDE CUBE__
  // winding is important because this is
  // how we determine placement of the hemicube
  // on the polygon faces.
  // bottom face
    
  /*

    C----G
    /|   /|
  D-A--H E
  |/   |/
  B----F

  */
  // bottom
  //mesh->addTri( B, A, E );// "cube bottom BAE"
  //mesh->addTri( B, E, F );// "cube bottom BEF"
  mesh->addTessQuad( B, A, E, F, uPatches, vPatches ) ;

  // left
  //mesh->addTri( B, D, C );// "cube left BDC"
  //mesh->addTri( B, C, A );// "cube left BCA"
  mesh->addTessQuad( B, D, C, A, uPatches, vPatches ) ;

  // right face
  //mesh->addTri( F, G, H );// "cube right FGH"
  //mesh->addTri( F, E, G );// "cube right FEG"
  mesh->addTessQuad( F, E, G, H, uPatches, vPatches ) ;

  // back face
  //mesh->addTri( A, C, G );// "cube back ACG"
  //mesh->addTri( A, G, E );// "cube back AGE"
  mesh->addTessQuad( A, C, G, E, uPatches, vPatches ) ;

  // front face
  //mesh->addTri( B, H, D );// "cube front BHD" 
  //mesh->addTri( B, F, H );// "cube front BFH" 
  mesh->addTessQuad( B, F, H, D, uPatches, vPatches ) ;

  // top face
  //mesh->addTri( D, G, C );// "cube top DGC" 
  //mesh->addTri( D, H, G );// "cube top DHG" 
  mesh->addTessQuad( D, H, G, C, uPatches, vPatches ) ;

  mesh->createVertexBuffer() ;

  addMesh( mesh ) ;

  aabb = new AABB( this ) ;

  return this ;
}

void Cube::generateDebugLines( const Vector& offset, const Vector& color )
{
  //       y
  //     ^
  //     |
  //    C----G
  //   /|   /|
  //  D-A--H E  -> x
  //  |/   |/ 
  //  B----F  
  //  /
  // z

  // ls
  window->addDebugLine( offset+A, color, offset+B, color ) ;
  window->addDebugLine( offset+B, color, offset+D, color ) ;
  window->addDebugLine( offset+D, color, offset+C, color ) ;
  window->addDebugLine( offset+C, color, offset+A, color ) ;

  // rs
  window->addDebugLine( offset+E, color, offset+F, color ) ;
  window->addDebugLine( offset+F, color, offset+H, color ) ;
  window->addDebugLine( offset+H, color, offset+G, color ) ;
  window->addDebugLine( offset+G, color, offset+E, color ) ;

  // LS/RS connectors
  window->addDebugLine( offset+D, color, offset+H, color ) ;
  window->addDebugLine( offset+B, color, offset+F, color ) ;
  window->addDebugLine( offset+A, color, offset+E, color ) ;
  window->addDebugLine( offset+C, color, offset+G, color ) ;
}

void Cube::generateDebugLines( const Vector& color )
{
  //       y
  //     ^
  //     |
  //    C----G
  //   /|   /|
  //  D-A--H E  -> x
  //  |/   |/ 
  //  B----F  
  //  /
  // z

  // ls
  window->addDebugLine( A, color, B, color ) ;
  window->addDebugLine( B, color, D, color ) ;
  window->addDebugLine( D, color, C, color ) ;
  window->addDebugLine( C, color, A, color ) ;

  // rs
  window->addDebugLine( E, color, F, color ) ;
  window->addDebugLine( F, color, H, color ) ;
  window->addDebugLine( H, color, G, color ) ;
  window->addDebugLine( G, color, E, color ) ;

  // LS/RS connectors
  window->addDebugLine( D, color, H, color ) ;
  window->addDebugLine( B, color, F, color ) ;
  window->addDebugLine( A, color, E, color ) ;
  window->addDebugLine( C, color, G, color ) ;
}

Vector Cube::getCentroid() const
{
  return (A+B+C+D+E+F+G+H)/8;
}

Cube* Cube::transform( const Matrix& m, const Matrix& nT )
{
  A = A * m ;
  B = B * m ;
  C = C * m ;
  D = D * m ;
  
  E = E * m ;
  F = F * m ;
  G = G * m ;
  H = H * m ;

  Shape::transform( m, nT ) ;

  return this ;
}

