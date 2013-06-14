#include "Quad.h"
#include "Mesh.h"

Quad::Quad( string iname, const Vector& ia, const Vector& ib, const Vector& ic, const Vector& id, 
  int uPatches, int vPatches,
  const Material& iMaterial ) :
    a(ia),b(ib),c(ic),d(id)
{
  name = iname ;
  material = iMaterial ;
  this->hasMath = false ; // use mesh intn

  // Defaults indexed
  createMesh( MeshType::Indexed, defaultVertexType, uPatches, vPatches ) ;
}

Quad::Quad( MeshType meshType, VertexType vertexType, 
            string iname, const Vector& ia, const Vector& ib, const Vector& ic, const Vector& id,
            int uPatches, int vPatches, const Material& iMaterial ) :
    a(ia),b(ib),c(ic),d(id)
{
  name = iname ;
  material = iMaterial ;
  this->hasMath = false ; // use mesh intn

  createMesh( meshType, vertexType, uPatches, vPatches ) ;
}

Quad* Quad::createMesh( MeshType meshType, VertexType vertexType, int uPatches, int vPatches )
{
  Mesh *mesh = new Mesh( this, meshType, vertexType ) ;

  mesh->addTessQuad( a,b,c,d, uPatches, vPatches ) ;
  mesh->createVertexBuffer() ;

  addMesh( mesh ) ;

  /// There is no point in having an aabb.
  aabb = 0 ;

  return this ;
}

// mandatory to be able to give the centroid
Vector Quad::getCentroid() const
{
  return (a+b+c+d)/4 ;
}