#include "Torus.h"
#include "Mesh.h"
#include "../window/GTPWindow.h"

Torus::Torus( string iName, const Vector& iPos,
    real donutThicknessRadius, real ringRadius,
    const Material& iMaterial,
    int iSlices, int iSides ):
    aDonutThickness(donutThicknessRadius),cRingRadius(ringRadius),slices(iSlices),sides(iSides)
{
  name = iName ;
  material = iMaterial ;
  hasMath = false ; // this SHOULD be true, but isn't now because I didn't code the formula.
  // 
  createMesh( defaultMeshType, defaultVertexType ) ;
}
  
void Torus::createMesh( MeshType iMeshType, VertexType iVertexType )
{
  Mesh*mesh = new Mesh( this, iMeshType, iVertexType ) ;

  // Generate the points of a torus
  for( real slice = 0 ; slice < slices ; slice++ )
  {
    real u1 = 2*PI* slice / slices ;
    real u2 = 2*PI* (slice+1) / slices ;

    // now go in a circle ABOUT the center axis,
    for( real side = 0 ; side < sides ; side++ )
    {
      // v travels UP AND DOWN the sides of the donut
      real v1 = 2*PI* side / sides ;
      real v2 = 2*PI* (side+1) / sides ;

      Vector p1 = eval( aDonutThickness,cRingRadius,u1,v1 ),
        p2 = eval( aDonutThickness,cRingRadius,u2,v1 ),
        p3 = eval( aDonutThickness,cRingRadius,u2,v2 ),
        p4 = eval( aDonutThickness,cRingRadius,u1,v2 ) ;

      // the normal is in the direction from
      // the center ring to the surface point
      Vector centerRing1 = eval( 0, cRingRadius, u1, v1 ) ;
      Vector centerRing2 = eval( 0, cRingRadius, u2, v1 ) ;
      Vector centerRing3 = eval( 0, cRingRadius, u2, v2 ) ;
      Vector centerRing4 = eval( 0, cRingRadius, u1, v2 ) ;
      
      Vector n1 = p1 - centerRing1 ;
      Vector n2 = p2 - centerRing2 ;
      Vector n3 = p3 - centerRing3 ;
      Vector n4 = p4 - centerRing4 ;

      mesh->addTri(
        AllVertex(p1,n1,material),
        AllVertex(p2,n2,material),
        AllVertex(p3,n3,material)
      ) ;
      mesh->addTri(
        AllVertex(p3,n3,material),
        AllVertex(p4,n4,material),
        AllVertex(p1,n1,material)
      ) ;
    }
  }

  addMesh( mesh ) ;
}

Vector Torus::eval( real iADonutThickness, real icRingRadius, real u, real v )
{
  return Vector(
          ( icRingRadius + iADonutThickness*cos(v) ) * cos(u),
          ( icRingRadius + iADonutThickness*cos(v) ) * sin(u),
          iADonutThickness*sin(v)
        ) ;
}