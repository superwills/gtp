#ifndef CUBE_H
#define CUBE_H

#include "Shape.h"
#include "Mesh.h"
#include "AABB.h"

// represent a cube by using
// either 6 planes or 8 points.
struct Cube : public Shape
{
  Vector A ;
  Vector B ;
  Vector C ;
  Vector D ;

  Vector E ;
  Vector F ;
  Vector G ;
  Vector H ;

  // A cube has 8 points.
  Cube( 
    MeshType meshType, VertexType vertexType, 
    string iname,
    const Vector& center, real s, //HALF sidelen
    int uPatches, int vPatches,
    real yaw, real pitch, real roll,
    Material iMaterial
  ) ;

  Cube( 
    string iname,
    const Vector& center, real s, //HALF sidelen
    int uPatches, int vPatches,
    real yaw, real pitch, real roll,
    Material iMaterial
  ) ;

  // geometric cube only (no AABB calculated)
  Cube( 
    const Vector& min,
    const Vector& max
  ) ;

  Cube( 
    string iname,
    const Vector& min,
    const Vector& max,
    int uPatches, int vPatches,
    Material iMaterial
  ) ;

  Cube( FILE *file ) ;
  int save( FILE* file ) override ;

private:
  void addTri( const Vector& p1, const Vector& p2, const Vector& p3 ) ;

public:
  Cube* createMesh( MeshType meshType, VertexType vertexType, int uPatches, int vPatches ) ;

  //allows you to generate the debug lines, offset by some vector
  void generateDebugLines( const Vector& offset, const Vector& color ) ;
  void generateDebugLines( const Vector& color ) ;

  Vector getCentroid() const override ;

  Cube* transform( const Matrix& m, const Matrix& nT ) override ;

} ;

#endif