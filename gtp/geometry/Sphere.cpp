#include "Sphere.h"
#include "Mesh.h"
#include "AABB.h"
#include "Icosahedron.h"
#include "../math/perlin.h"

Sphere::Sphere( MeshType meshType, VertexType vertexType,
  string iName, const Vector & iPos, real iRadius,
  const Material& iMaterial, int iSlices, int iStacks ) :
  pos( iPos ), radius( iRadius ), slices( iSlices ), stacks( iStacks )
{
  name = iName;
  material = iMaterial ;
  
  hasMath =true;

  createMesh( meshType, vertexType ) ;
}

Sphere::Sphere( string iName, const Vector & iPos, real iRadius,
  const Material& iMaterial, int iSlices, int iStacks ) :
  pos( iPos ), radius( iRadius ), slices( iSlices ), stacks( iStacks )
{
  name = iName;
  material = iMaterial ;
  
  hasMath =true;

  // Default settings
  createMesh( defaultMeshType, defaultVertexType ) ;
}

Sphere::Sphere( FILE *file ) : Shape( file )
{
  hasMath = true ;
  
  fread( &pos, (sizeof(Vector) + sizeof(real) + 2*sizeof(int)), 1, file ) ;
}

int Sphere::save( FILE *file ) //override
{
  int written = 0 ;
  HeaderShape head ;
  
  head.ShapeType = SphereShape ;
  written += sizeof(HeaderShape)*fwrite( &head, sizeof( HeaderShape ), 1, file ) ; // THIS WILL BE READ
  // BEFORE RECONSTRUCTING THE SHAPE, SO YOU CAN SAVE THE ACTUAL DATA IN ANY ORDER.
  // CHOOSE TO SAVE SHAPE (base class) MEMBERS __FIRST__ SO AT LOAD,
  // THE BASE CLASS CTOR WILL RUN FIRST.
  
  written += Shape::save( file ) ;

  // SPHERE-SPECIFIC INFO
  written += (sizeof(Vector) + sizeof(real) + 2*sizeof(int))*fwrite( &this->pos, sizeof(Vector) + sizeof(real) + 2*sizeof(int), 1, file ) ;
  
  return written ;
}

Sphere* Sphere::transform( const Matrix& m )
{
  return transform( m, m.getNormalMatrix() ) ;
}

Sphere* Sphere::transform( const Matrix& m, const Matrix& nT )
{
  pos = pos*m ; // update position vectors

  Shape::transform( m, nT ) ;// transform the meshes

  return this ;
}

/// Returns the normal to the sphere
/// at a point on the sphere nearest
/// "position"
Vector Sphere::getNormalAt( const Vector& position ) const
{
  // Compute vector from center sphere out to position.
  // That is the vector describing the direction of
  // the normal.  Simply normalize it, and you have
  // the normal at that point on the sphere
  Vector norm = (position - pos) ;
  return norm.normalize() ;
}

bool Sphere::intersectExact( const Ray& ray, Intersection* intn )
{
  // an important override

  //If E is the starting point of the ray,
  //.. and L is the end point of the ray,
  //.. and C is the center of sphere you're testing against
  //.. and r is the radius of that sphere

  // Compute:
  //    d = L - E ; // Direction vector of ray, from start to end
  //    f = E - C ; // Vector from center sphere to ray start
  const Vector &d = ray.direction ;
  Vector f = ray.startPos - this->pos ;

  real a = d % d ;  // a is nonnegative, this is actually always 1.0
  real b = 2*(f % d) ;
  real c = f % f - radius*radius ;

  real discriminant = b*b-4*a*c;
  if( discriminant < 0 )
  {
    return false ;  // no intersections.
    // the geometric meaning of this is
    // the ray totally misses the sphere.
  }
  else
  {
    // ray didn't totally miss sphere,
    // so there is a solution to
    // the equation.

    discriminant = sqrt( discriminant );

    // 3x HIT cases:
    //       -o->       --|-->  |            |  --|->
    // Impale(t1,t2), Poke(t1,t2>len), ExitWound(t1<0, t2), 
    
    // 3x MISS cases:
    //       ->  o                     o ->              | -> |
    // FallShort (t1>len,t2>len), Past (t1<0,t2<0), CompletelyInside(t1<0, t2>len)
    
    // t1 is always the smaller value, because BOTH discriminant and
    // a are nonnegative.
    real t1 = (-b - discriminant)/(2*a);
    real t2 = (-b + discriminant)/(2*a);

    // Just purely sphere info
    if( BetweenIn(t1,0,ray.length) )
    {
      // t1 is the intersection, and its closer
      // Impale, Poke
      Vector p = ray.at( t1 ) ;
      if( intn )  *intn = Intersection( p, getNormalAt( p ), this ) ;
      return true ;
    }

    // here t1 didn't intersect so we are either started
    // inside the sphere or too 
    if( BetweenIn( t2,0,ray.length ) )
    {
      // ExitWound
      Vector p = ray.at( t2 ) ;

      // We invert the normal because you are inside the sphere.
      if( intn )  *intn = Intersection( p, -getNormalAt( p ), this ) ;
      return true ;
    }

    return false ; // no intn: FallShort, Past, CompletelyInside
  }
}

// a mesh is a polygon soup
// its mandatory to compute the mesh
Sphere* Sphere::createMesh( MeshType meshType, VertexType vertexType )
{
  Mesh*mesh = new Mesh( this, meshType, vertexType ) ;

  // you get degenerate triangles if you don't spin the cap and bottom
  // separately.

  // BODY
  for( int t = 0 ; t < stacks ; t++ ) // stacks are ELEVATION so they count theta
  {
    real theta1 = ( (real)(t)/stacks )*PI ;
    real theta2 = ( (real)(t+1)/stacks )*PI ;
    
    for( int p = 0 ; p < slices ; p++ ) // slices are ORANGE SLICES so the count azimuth
    {
      real phi1 = ( (real)(p)/slices )*2*PI ; // azimuth goes around 0 .. 2*π
      real phi2 = ( (real)(p+1)/slices )*2*PI ;
      
      //phi2   phi1
      // |      |
      // 2------1 -- theta1
      // |\ _   |
      // |    \ |
      // 3------4 -- theta2
      //
      SVector sv1( radius, theta1, phi1 ),
              sv2( radius, theta1, phi2 ),
              sv3( radius, theta2, phi2 ),
              sv4( radius, theta2, phi1 ) ;    
      
      // get cartesian coordinates (+r valued)
      Vector c1 = sv1.toCartesian() ;
      Vector c2 = sv2.toCartesian() ;
      Vector c3 = sv3.toCartesian() ;
      Vector c4 = sv4.toCartesian() ;

      // compute the normals
      // the "normals" you calculate here 
      // are ALWAYS FACING OUT because
      // they are vectors from sphere center
      // TO point on sphere, simply normalized.
      Vector n1 = c1.normalizedCopy() ;
      Vector n2 = c2.normalizedCopy() ;
      Vector n3 = c3.normalizedCopy() ;
      Vector n4 = c4.normalizedCopy() ;

      AllVertex vertex1( pos + c1, n1, material ) ;
      AllVertex vertex2( pos + c2, n2, material ) ;
      AllVertex vertex3( pos + c3, n3, material ) ;
      AllVertex vertex4( pos + c4, n4, material ) ;

      // facing out
      if( t == 0 ) // top cap
        mesh->addTri( vertex1, vertex3, vertex4 ) ; //t1p1, t2p2, t2p1
      else if( t + 1 == stacks ) //end cap
        mesh->addTri( vertex3, vertex1, vertex2 ) ; //t2p2, t1p1, t1p2
      else
      {
        mesh->addTri( vertex1, vertex2, vertex4 ) ;
        mesh->addTri( vertex2, vertex3, vertex4 ) ;
      }
      // facing in
      // You have to invert it totally
      //vertex1.norm = Vec3( -vertex1.norm.getVector() ) ;
      //vertex2.norm = Vec3( -vertex2.norm.getVector() ) ;
      //vertex3.norm = Vec3( -vertex3.norm.getVector() ) ;
      //vertex4.norm = Vec3( -vertex4.norm.getVector() ) ;
      //mesh->addTri( vertex1, vertex4, vertex2 ) ; // CCW
      //mesh->addTri( vertex2, vertex4, vertex3 ) ; // CCW
    }
  }

  mesh->createVertexBuffer() ;
  
  addMesh( mesh ) ;

  aabb = new AABB( this ) ;

  return this ;
}

// mandatory to be able to give the centroid
Vector Sphere::getCentroid() const //override
{
  return pos ;
}

Vector Sphere::getRandomPointFacing( const Vector& dir ) //override
{
  // find a random direction on the hemisphere with -dir as
  // normal
  return pos + radius*SVector::randomHemi( -dir ) ;
  // go from the center and turn the direction around
  //return pos - radius*dir ;
}

Sphere* Sphere::fromShape( string iName, Shape* shape, real r, int subdivLevels )
{
  Sphere* s = new Sphere() ;
  s->name = iName ;
  s->material = shape->material;
  //s->material.ks = 1 ;
  s->radius = r ;

  // first force all triangles to surface of sphere..

  // at each subdivision level, PUSH OUT ALL POINTS
  // to the surface of the sphere.

  // a level 0 sphere is just an octahedron.
  //Mesh* mesh = new Mesh( s, defaultMeshType, defaultVertexType ) ;
  // Since it's round, it does very well with indexed vertices thank you.
  Mesh* mesh = new Mesh( s, MeshType::Indexed, defaultVertexType ) ;
 
  for( int i = 0 ; i < shape->meshGroup->meshes.size() ; i++ )
  {
    // copy the list, because we're going to deform it.
    vector<Triangle> tris = shape->meshGroup->meshes[i]->tris ;

    for( int j = 0 ; j < tris.size() ; j++ )
    {
      // put every point on a sphere
      tris[j].a.setMag( r ) ;
      tris[j].b.setMag( r ) ;
      tris[j].c.setMag( r ) ;

      tris[j].recursivelySubdivideAndPushToSphereThenAddToMeshWhenDepthReachedOrSmallEnough( 
        0, subdivLevels, EPS_MIN, r, mesh 
      ) ;
    }
  }

  // fix the normals
  for( int i = 0 ; i < mesh->verts.size() ; i++ )
  {
    mesh->verts[i].norm = mesh->verts[i].pos.normalizedCopy() ;
    //mesh->verts[i].color[ ColorIndex::SpecularMaterial ] = 1 ;
  }

  mesh->createVertexBuffer() ;
  s->addMesh( mesh ) ;
  return s;
}

Sphere* Sphere::fromIcosahedron( string iName, real r, int subdivLevels, const Material& iMaterial ) 
{
  char tName[255];
  sprintf( tName, "temp ico %s", iName.c_str() ) ;
  Icosahedron* ic = new Icosahedron( tName, 1, iMaterial ) ;
  Sphere *sp = fromShape( iName, ic, r, subdivLevels ) ;
  delete ic ;
  return sp ;
}

