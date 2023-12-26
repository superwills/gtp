#include "Tetrahedron.h"
#include "Mesh.h"

Tet::Tet( MeshType meshType, VertexType vertexType, string iname, const Vector& center, real a, const Material& iMaterial )
{
  name = iname ;
  material = iMaterial ;
  // from the top:
  /*
  Points of a tet with edge length=2*ROOT(2)
  ( 1, 1, 1)
  (-1,-1, 1)
  (-1, 1,-1)
  ( 1,-1,-1) 
  */
  
  calcPoints( center, a ) ;
  createMesh( meshType, vertexType ) ;
}

Tet::Tet( string iname, const Vector& center, real a, const Material& iMaterial )
{
  name = iname ;
  material = iMaterial ;
  
  calcPoints( center, a ) ;
  createMesh( defaultMeshType, defaultVertexType ) ;
}

void Tet::calcPoints( const Vector& center, real a )
{
  //         desired side len      /     ( value to make sidelen 1 )
  real den = 2.0 * sqrt2 ;
  real mult = a / den ;

  // the radius of the circumsphere is sqrt( 3 / 8 ) * a, 
  // where a is the side length.
  real radiusCircumsphere = sqrt( 3.0 / 8.0 ) * a ;

  // A tetrahedron has 4 points.
  topFront=Vector( mult, mult, mult );
  topBack=Vector( -mult, mult, -mult );
  bottomFront=Vector( -mult, -mult, mult );
  bottomBack=Vector( mult, -mult, -mult );

  // rotate them a bit
  Matrix rot = Matrix::RotationYawPitchRoll(
    randFloat(0,2*π),
    randFloat(0,2*π),
    randFloat(0,2*π)
  ) ;

  // apply xform: rotate first
  topFront    = topFront    * rot ;
  topBack     = topBack     * rot ;
  bottomFront = bottomFront * rot ;
  bottomBack  = bottomBack  * rot ;

  // and translate
  topFront    += center ;
  topBack     += center ;
  bottomFront += center ;
  bottomBack  += center ;
}

Tet::Tet( FILE * file ) : Shape( file )
{
  fread( &topFront, sizeof(Vector), 4, file ) ;
}

int Tet::save( FILE *file )
{
  // the class specific members are here:
  HeaderShape head ;
  head.ShapeType = TetrahedronShape ;
  int written = 0 ;
  written += sizeof(HeaderShape)*fwrite( &head, sizeof( HeaderShape ), 1, file ) ;
  
  // shape-substructure
  written += Shape::save( file ) ;

  // Tet-specific
  written += sizeof(Vector)*fwrite( &topFront, sizeof(Vector), 4, file ) ;
  
  return written ;
}

Tet* Tet::createMesh( MeshType meshType, VertexType vertexType )
{
  Mesh *mesh = new Mesh( this, meshType, vertexType ) ;
  // tet comprised of 4 tris
  // 1. tf, tb, bf
  // 2. tf, bf, bb
  // 3. tb, bb, bf
  // 4. tf, bb, tb
  mesh->addTri( topFront, topBack, bottomFront ) ;
  mesh->addTri( topFront, bottomFront, bottomBack ) ;
  mesh->addTri( topBack, bottomBack, bottomFront ) ;
  mesh->addTri( topFront, bottomBack, topBack ) ;

  addMesh( mesh ) ;

  return this ;
}

Vector Tet::getCentroid() const
{
  return (topFront + topBack + bottomFront + bottomBack) / 4 ;
}

