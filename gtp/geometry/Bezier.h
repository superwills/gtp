#ifndef BEZIER_H
#define BEZIER_H

#include "../math/Vector.h"
#include "Mesh.h"

#ifdef SINGLE_PRECISION
#define THREE_COMMA_FLOATS_NEWLINE "%f,%f,%f\n"
#else
#define THREE_COMMA_FLOATS_NEWLINE "%lf,%lf,%lf\n"
#endif

struct BezierSpline
{
  // 4 control points
  Vector P[4] ;
  BezierSpline(){}
  // TEMP i variable
  BezierSpline( int i )
  {
    for( int b = 0 ; b < 4 ; b++ )
      P[b] = Vector( b + randFloat(-.2,.2), i, randFloat( -.2,.2 ) ) ;
  }

  /// t is between 0 and 1
  Vector getPoint( real t )
  {
    if( t < 0 || t > 1 )
    {
      error( "Must have 0 < t < 1, t=%f", t ) ;
      return 0 ;
    }

    // cubic bezier spline
    Vector pt = CUBE(1 - t) * P[0] +
      3*SQUARE(1-t)*t*P[1] +
      3*(1-t)*t*t*P[2] +
      CUBE(t)*P[3] ;

    return pt ;
  }
} ;


struct BezierPatch : public Shape
{
  // 4+4 bezier curves (4 horiz, 4 vert)
  BezierSpline beziers[4] ; // 4 bezier splines

  int uPatches, vPatches ;

  BezierPatch( const Material& mat )
  {
    material = mat ;

    for( int i = 0 ; i < 4 ; i++ )
      beziers[i] = BezierSpline( i ) ;

    uPatches=vPatches=10;
  }

  BezierPatch( MeshType meshType, VertexType vertexType, char* filename, int iuPatches, int ivPatches, const Material& mat )
  {
    uPatches = iuPatches ;
    vPatches = ivPatches ;

    material = mat ;
    loadNewell( meshType, vertexType, filename ) ;
  }

  BezierPatch( const Matrix& matrix, const Material& iMaterial )
  {
    uPatches=vPatches=10;
    material = iMaterial ;

    // assign the values
    for( int bp = 0 ; bp < 4 ; bp++ )
      for( int c = 0 ; c < 4 ; c++ )
        beziers[bp].P[c] = matrix.m[bp][c];

    // construct the mesh
    createMesh( defaultMeshType, defaultVertexType ) ;
  }

  // loads a bezier spline
  void loadNewell( MeshType meshType, VertexType vertexType, char* filename )
  {
    // base it on a file
    int ii;
    real x,y,z;
    //int a,b,c,d, e,f,g,h, i,j,k,l, m,n,o,p;
  
    int PATCH ;
    int numPatches, numVertices ;

    FILE* file = fopen( filename, "r" ) ;
    if( !file )
    {
      error( "Could not open bezier patch file %s", filename ) ;
      return ;
    }

    fscanf( file, "%i\n", &numPatches ) ; // get __#__ patches,

    vector<int> patchIndices ;
    patchIndices.resize( 16 * numPatches ) ; // there are 16 indices PER PATCH specified

    // Unfortuneately this is UPSIDE DOWN,
    // the indices come first before the points,
    for (ii = 0; ii < numPatches; ii++)
    {
      // each patch is 16 ints, exactly in this format.
      //fscanf( file,"%i, %i, %i, %i,",&a,&b,&c,&d );
      //fscanf( file,"%i, %i, %i, %i,",&e,&f,&g,&h );
      //fscanf( file,"%i, %i, %i, %i,",&i,&j,&k,&l );
      //fscanf( file,"%i, %i, %i, %i\n",&m,&n,&o,&p );
      for( int patchNo = 0 ; patchNo < 15 ; patchNo++ )
      {
        fscanf( file, "%i,", &PATCH ) ;
        PATCH-- ; // SWITCH TO 0-BASED INDEXING
        int patchIndex = ii*16 + patchNo ;
        patchIndices[ patchIndex ] = PATCH ;
        //printf( "patch %d = %d; ", patchIndex, PATCH ) ;
      }

      // the last one is %i\n, as indicated
      fscanf( file, "%i\n", &PATCH ) ;
      PATCH-- ; // SWITCH TO 0-BASED INDEXING
      int patchIndex = ii*16 + 15;
      patchIndices[ patchIndex ] = PATCH ;
      //printf( "patch %d = %d -- END OF PATCH\n", patchIndex, PATCH ) ;
      ///B_patch(ii, a,b,c,d, e,f,g,h, i,j,k,l, m,n,o,p); // INDICES to points, that were loaded in B_point.
    
    }

    // now have all the patch indices.
    // now need the actual POINTS to make patches.
  
    // unfortunately the indexing was originally 1-BASED.
    // Changed to 0-based.

    fscanf( file,"%i\n", &numVertices ); // get __#__ vertices
    vector<Vector> vertices ;
    vertices.resize( numVertices ) ;

    for( ii = 0; ii < numVertices; ii++ )
    {
      fscanf( file,THREE_COMMA_FLOATS_NEWLINE,&x,&y,&z);
      //B_point( ii, x, y, z );
      vertices[ ii ] = Vector( x,y,z ) ;

      ///printf( "Vertex %d = (%f,%f,%f)\n", ii, x,y,z ) ;
    }

    // at this point done with the file
    fclose( file ) ;

    // NOW, construct numPatches bezier patches
    for( int patchNo = 0 ; patchNo < numPatches ; patchNo++ )
    {
      int basePatchIndex = patchNo*16 ;
    
      // fill the 16 entries in bezierPatch
      for( int bez = 0 ; bez < 4 ; bez++ )
      {
        for( int i = 0 ; i < 4 ; i++ )
        {
          int patchIndex = basePatchIndex + bez*4 + i ;
          beziers[ bez ].P[ i ] = vertices[ patchIndices[ patchIndex ] ] ;
        }
      }

      // construct a Mesh out of the bezierPatch, and add it to the MeshGroup
      createMesh( meshType, vertexType ) ;
    }
  }

  // CREATES the mesh 
  void createMesh( MeshType meshType, VertexType vertexType )
  {
    real expOffset = 0; // fro the "explodede" view

    Mesh* mesh = new Mesh( this, meshType, vertexType ) ;
    BezierSpline vSplineL, vSplineR ;

    for( int i = 0 ; i < uPatches ; i++ )
    {
      real u1 = (real)i / uPatches ;
      real u2 = (real)(i+1) / uPatches ;

      for( int b = 0 ; b < 4 ; b++ )
      {
        vSplineL.P[b] = beziers[ b ].getPoint( u1 ) ;
        vSplineR.P[b] = beziers[ b ].getPoint( u2 ) ;
      }

      for( int j = 0 ; j < vPatches ; j++ )
      {
        // generate 2 v pts
        real v1 = (real)j/vPatches ;
        real v2 = (real)(j+1)/vPatches ;

        // Generate 4 points on the 
        // 2 new L,R bezier splines
        Vector p1 = vSplineR.getPoint( v2 ) ;
        Vector p2 = vSplineL.getPoint( v2 ) ;

        Vector p3 = vSplineL.getPoint( v1 ) ;
        Vector p4 = vSplineR.getPoint( v1 ) ;

        //CCW
        // now form 2 polygons
        // REUSED FROM visualizeCreateMesh().
        //splineL splineR
        // |      |
        // 2------1 -- v2
        // |\ _   |
        // |    \ |
        // 3------4 -- v1
        //
        // at poles, it's likely p1==p2, or p3==p4
        // IT CAN HAPPEN, that 2 of the vertices 
        // are the same.  IF THAT HAPPENS, ELIMINATE
        // ONE OF THE TRIANGLES.
        // TOTALLY DEGENERATE CASE (no triangles)
        if( (p1.Near( p2 ) || p1.Near( p4 )) &&
            (p2.Near( p3 ) || p3.Near( p4 )) )
        {
          // so, if you try and sample a bezier
          // curve TOO finely, you will get a bunch of SLIVER
          // faces, which are degenerate basically.
          info( Magenta, "Totally degenerate quad has no faces" ) ;
        }
        else if( p1.Near( p2 ) || p1.Near( p4 ) )
        {
          // there is only one triangle 2,3,4
          Vector n2 = ( (p3 - p2) << (p4 - p2) ).normalize() ;
          Vector n3 = ( (p4 - p3) << (p2 - p3) ).normalize() ;
          Vector n4 = ( (p2 - p4) << (p3 - p4) ).normalize() ;

          AllVertex vertex2( p2 + expOffset*n2, n2, material ) ;
          AllVertex vertex3( p3 + expOffset*n3, n3, material ) ;
          AllVertex vertex4( p4 + expOffset*n4, n4, material ) ;

          mesh->addTri( vertex2, vertex3, vertex4 ) ; 
        }
        else if( p2.Near( p3 ) || p3.Near( p4 ) )
        {
          // there is only one triangle 1,2,4
          Vector n1 = ( (p2 - p1) << (p4 - p1) ).normalize() ;
          Vector n2 = ( (p4 - p2) << (p1 - p2) ).normalize() ;
          Vector n4 = ( (p1 - p4) << (p2 - p4) ).normalize() ;

          AllVertex vertex1( p1 + expOffset*n1, n1, material ) ;
          AllVertex vertex2( p2 + expOffset*n2, n2, material ) ;
          AllVertex vertex4( p4 + expOffset*n4, n4, material ) ;

          mesh->addTri( vertex1, vertex2, vertex4 ) ;
        }
        else
        {
          // both exist.
          // compute the normals
          Vector n1 = ( (p2 - p1) << (p4 - p1) ).normalize() ;
          Vector n2 = ( (p3 - p2) << (p1 - p2) ).normalize() ;
          Vector n3 = ( (p4 - p3) << (p2 - p3) ).normalize() ;
          Vector n4 = ( (p1 - p4) << (p3 - p4) ).normalize() ;
        
          AllVertex vertex1( p1 + expOffset*n1, n1, material ) ;
          AllVertex vertex2( p2 + expOffset*n2, n2, material ) ;
          AllVertex vertex3( p3 + expOffset*n3, n3, material ) ;
          AllVertex vertex4( p4 + expOffset*n4, n4, material ) ;

          ///mesh->addTri( vertex1, vertex2, vertex4 ) ;
          ///mesh->addTri( vertex2, vertex3, vertex4 ) ; 
          mesh->addQuad( vertex1, vertex2, vertex3, vertex4 ) ;
        }
      } // vPatches
    } // uPatches

    mesh->createVertexBuffer() ;
    addMesh( mesh ) ;
  }

  
} ;


#endif