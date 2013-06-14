#include "AABB.h"
#include "Cube.h"
#include "../scene/Scene.h"
#include "../window/GTPWindow.h"
#include "../rendering/RaytracingCore.h"

AABB::AABB()
{
  resetInsideOut() ;
}

AABB::AABB( const Vector& iMin, const Vector& iMax )
{
  min=(iMin), max=(iMax);
}

AABB::AABB( const Scene * scene )
{
  resetInsideOut();
  bound( scene ) ;
}

AABB::AABB( const Shape * shape )
{
  resetInsideOut() ;
  bound( shape ) ;
}

void AABB::resetInsideOut()
{
  min.x=min.y=min.z= HUGE ;
  max.x=max.y=max.z=-HUGE ;
}

// Bound an entire Scene
void AABB::bound( const Scene * scene )
{
  for( int i = 0 ; i < scene->shapes.size() ; i++ )
  {
    Shape *shape = scene->shapes[i];
    
    // bound that shape
    bound( shape ) ;
  }

  // After you're done bounding all the shapes, you have got a bound
  // on the entire scene.
}

// Bound a shape. Somewhat misleading, its really based on the
// MESH, not the shape itself (so it doesn't bound a sphere with
// center/radius (which would be easy), rather it uses the
// polygonal mesh instead.
void AABB::bound( const Shape * shape )
{
  if( !shape->meshGroup || shape->meshGroup->isEmpty() ){
    error( "%s: AABB::bound cannot compute bound on general shape with no mesh, specify a mesh first", shape->name.c_str() ) ;
    return ;
  }
  for( int mn = 0 ; mn < shape->meshGroup->meshes.size() ; mn++ )
  {
    Mesh *mesh = shape->meshGroup->meshes[ mn ];
    for( int i = 0 ; i < mesh->verts.size() ; i++ )
    {
      const Vector& vertex = mesh->verts[i].pos ;
      // analyze each vertex

      //min.clamp( min, vertex ) ;
      //max.clamp( vertex, max ) ;

      // let the min bound be pushed out
      // by the vertices of the model
      if( min.x > vertex.x ) min.x = vertex.x ; // pushed out to reach vertex.x - ie min.x clamped BELOW by vertex.x
      if( min.y > vertex.y ) min.y = vertex.y ;
      if( min.z > vertex.z ) min.z = vertex.z ;

      if( max.x < vertex.x ) max.x = vertex.x ; // max.x pushed out to include vertex.x - ie max.x clamped ABOVE by vertex.x
      if( max.y < vertex.y ) max.y = vertex.y ;
      if( max.z < vertex.z ) max.z = vertex.z ;
    }
  }
}

void AABB::bound( const Triangle* tri )
{
  static Vector defMin( -HUGE, -HUGE, -HUGE ) ;
  static Vector defMax( HUGE, HUGE, HUGE ) ;
  min.clamp( defMin, tri->a ) ;  // if min.x is bigger than tri->a.x, make min.x=tri->a.x
  min.clamp( defMin, tri->b ) ;
  min.clamp( defMin, tri->c ) ;
  
  max.clamp( tri->a, defMax ) ;  // if max.x is smaller than tri->a, make max.x=tri->a.x
  max.clamp( tri->b, defMax ) ;
  max.clamp( tri->c, defMax ) ;
}

void AABB::bound( const PhantomTriangle* tri )
{
  static Vector defMin( -HUGE, -HUGE, -HUGE ) ;
  static Vector defMax( HUGE, HUGE, HUGE ) ;
  min.clamp( defMin, tri->a ) ;  // if min.x is bigger than tri->a.x, make min.x=tri->a.x
  min.clamp( defMin, tri->b ) ;
  min.clamp( defMin, tri->c ) ;
  
  max.clamp( tri->a, defMax ) ;  // if max.x is smaller than tri->a, make max.x=tri->a.x
  max.clamp( tri->b, defMax ) ;
  max.clamp( tri->c, defMax ) ;
}

bool AABB::intersects( const AABB& o ) const 
{
  // intersection requires overlap in all 3 axes
  if( max.x < o.min.x || // my max is left of his min, miss x overlap OR
      o.max.x < min.x )  // his max is left of my min
    return false ;
  if( max.y < o.min.y ||
      o.max.y < min.y )
    return false ;
  if( max.z < o.min.z ||
      o.max.z < min.z )
    return false ;

  // if nothing missed it's a hit
  return true;
}

bool AABB::intersects( const Ray& ray )
{
  // VERY IMPORTANT CHECK: If the ray starts inside
  // the box, then it's a hit.  This was important for octrees.
  if( containsIn( ray.startPos ) || containsIn( ray.getEndPoint() ) )
    return true ; 

  #define TECH 0
  #if TECH==0
  // the algorithm says, find 3 t's,
  Vector t ;

  // LARGEST t is the only only we need to test if it's on the face.
  for( int i = 0 ; i < 3 ; i++ )
  {
    if( ray.direction.e[i] > 0 ) // CULL BACK FACE
      t.e[i] = ( min.e[i] - ray.startPos.e[i] ) / ray.direction.e[i] ;
    else
      t.e[i] = ( max.e[i] - ray.startPos.e[i] ) / ray.direction.e[i] ;
  }

  int mi = t.maxIndex() ;
  if( BetweenIn( t.e[mi], 0, ray.length ) )
  {
    Vector pt = ray.at( t.e[mi] ) ;

    // check it's in the box in other 2 dimensions
    int o1 = ( mi + 1 ) % 3 ; // i=0: o1=1, o2=2, i=1: o1=2,o2=0 etc.
    int o2 = ( mi + 2 ) % 3 ;

    return BetweenIn( pt.e[o1], min.e[o1], max.e[o1] ) &&
           BetweenIn( pt.e[o2], min.e[o2], max.e[o2] ) ;
  }

  return false ;
  #elif TECH==1

  // This culls the back faces first, and gives you the answer with the largest valid t
  for( int i = 0 ; i < 3 ; i++ )
  {
    int o1 = ( i + 1 ) % 3 ; // i=0: o1=1, o2=2, i=1: o1=2,o2=0 etc.
    int o2 = ( i + 2 ) % 3 ;

    // min.x (NX) is possible, PX won't be hit. (unless you're inside the box.)
    if( ray.direction.e[i] > 0 ) // CULL BACK FACE
    {
      real t = ( min.e[i] - ray.startPos.e[i] ) / ray.direction.e[i] ;

      // 1. CHECK VALID T
      if( BetweenIn( t, 0, ray.length ) )
      {
        // 2. CHECK IN BOX
        Vector pt = ray.at( t ) ;
        if( BetweenIn( pt.e[o1], min.e[o1], max.e[o1] ) && BetweenIn( pt.e[o2], min.e[o2], max.e[o2] ) )
          return true ;  // it's valid in the box
      }
    }
    else if( ray.direction.e[i] < 0 ) // max.x (PX) is possible
    {
      real t = ( max.e[i] - ray.startPos.e[i] ) / ray.direction.e[i] ;
      if( BetweenIn( t, 0, ray.length ) ) // valid t
      {
        Vector pt = ray.at( t ) ;
        if( BetweenIn( pt.e[o1], min.e[o1], max.e[o1] ) && BetweenIn( pt.e[o2], min.e[o2], max.e[o2] ) )
          return true ;  // it's valid in the box
      }
    }
  }

  return false ; // no hit.
  #elif TECH==2
  // This solves ALL 6 solutions and finds the shortest t.
  // Rays starting and ending in an octree node must be considered
  // to hit the node.
  // if the start point OR the end point is in the AABB, then it's a hit.
  
  
  // These are red in the profiler.
  Vector tmins = (min - ray.startPos) / ray.direction ;
  Vector tmaxes = (max - ray.startPos) / ray.direction ;
  
  Vector* sols[2] = { &tmins, &tmaxes } ;
  
  for( int s = 0 ; s < 2 ; s++ )
  {
    Vector& candSols = *sols[s] ;
    
    // i walks xyz
    for( int i = 0 ; i < 3 ; i++ )
    {
      real& t = candSols.e[i] ; // candidate t
      
      // validity:  0 <= t <= length
      if( BetweenIn( t, 0, ray.length ) )
      {
        Vector raypoint = ray.at( t ) ;

        // AND ON FACE
        // o1 and o2 are the indices of the OTHER faces,
        // if i==0, o1=1, o2=2
        int o1 = ( i + 1 ) % 3 ; // if( i==0 ) o1=1, o2=2; 
        int o2 = ( i + 2 ) % 3 ; // else if( i == 1) o1=2, o2=0;

        if( InRect( raypoint.e[ o1 ],raypoint.e[ o2 ], 
          min.e[o1], min.e[o2],
          max.e[o1], max.e[o2] ) )
        {
          // the box got hit, any side
          return true ;
        }
      }
    }
  }

  // No hit
  return false ;

  #endif
}

bool AABB::intersects( const Ray& ray, Vector& pt )
{
  // Rays starting and ending in an octree node must be considered
  // to hit the node.
  // if the start point OR the end point is in the AABB, then it's a hit.
  if( containsIn( ray.startPos ) || containsIn( ray.getEndPoint() ) )
  {
    //generateDebugLines( Vector(0,1,0) ) ;//green is RAY STARTED INSIDE
    return true ; 
  }
  
  Vector tmins, tmaxes ;

  // These are red in the profiler.
  tmins = (min - ray.startPos) / ray.direction ;
  tmaxes = (max - ray.startPos) / ray.direction ;
  
  Vector* sols[2] = { &tmins, &tmaxes } ;
  int solS=0, solI=0 ;
  
  real smallestT = HUGE ; // smallest valid t ("so far"). put at inf so first time everything smaller than it.
  Vector closestRaypoint ; // smallest rayPoint (associated with "smallest t so far")

  for( int s = 0 ; s < 2 ; s++ )
  {
    Vector& candSols = *sols[s] ;
    
    // i walks xyz
    for( int i = 0 ; i < 3 ; i++ )
    {
      real& t = candSols.e[i] ; // candidate t
      
      // validity:  0 <= t <= length
      if( BetweenIn( t, 0, ray.length ) )
      {
        Vector raypoint = ray.at( t ) ;

        // AND ON FACE
        // o1 and o2 are the indices of the OTHER faces,
        // if i==0, o1=1, o2=2
        int o1 = ( i + 1 ) % 3 ; // if( i==0 ) o1=1, o2=2; 
        int o2 = ( i + 2 ) % 3 ; // else if( i == 1) o1=2, o2=0;

        if( InRect( raypoint.e[ o1 ],raypoint.e[ o2 ], 
          min.e[o1], min.e[o2],
          max.e[o1], max.e[o2] ) )
        {
          // this one wins, if it's smallest so far
          if( t < smallestT )
          {
            smallestT = t ;
            closestRaypoint = raypoint ;
            // record what face it was
            solS = s ;
            solI = i ;
          }
        }
      }
    }
  }

  // now here, it's possible smallestT was unassigned,
  // which means it'll still be HUGE
  if( smallestT == HUGE )
  {
    // no solution
    ///generateDebugLines( Vector(.2,.2,.2) ) ;//total miss
    return false ;
  }
  else
  {
    // there was an intersection.
    ///generateDebugLines( Vector(1,0,0) ) ;//red means it was a hit
    pt = closestRaypoint ;
    return true ;
  }
}

AABB AABB::getIntersectionVolume( const AABB& o ) const 
{
  AABB res ;
  for( int c = 0 ; c < 3 ; c++ )
  {
    // get the overlap in this component.

    // Total miss cases
    // ==tttttt======oooooo===========>
    // ==oooooo======tttttt===========>
    if( max.e[c] < o.min.e[c] ||
        min.e[c] > o.max.e[c] )
      return AABB(0,0) ;//MISS

    // ===============================>
    //         ttttttttttttttt
    //             ooooooo
    else if( BetweenIn( o.min.e[c], min.e[c], max.e[c] ) && 
        BetweenIn( o.max.e[c], min.e[c], max.e[c] ) )
    {
      res.min.e[c] = o.min.e[c] ;
      res.max.e[c] = o.max.e[c] ;
    }

    // ===============================>
    //         ooooooooooooooo
    //             ttttttt
    else if( BetweenIn( min.e[c], o.min.e[c], o.max.e[c] ) && 
        BetweenIn( max.e[c], o.min.e[c], o.max.e[c] ) )
    {
      res.min.e[c] = min.e[c] ;
      res.max.e[c] = max.e[c] ;
    }

    // ===============================>
    //  tttttttttttt
    //          oooooooo
    else if( BetweenIn( max.e[c], o.min.e[c], o.max.e[c] ) )
    {
      //o.min->t.max
      res.min.e[c] = o.min.e[c] ;
      res.max.e[c] = max.e[c] ;
    }

    // ===============================>
    //          tttttttt
    //    oooooooooo
    else 
    {
      //t.min->o.max
      res.min.e[c] = min.e[c] ;
      res.max.e[c] = o.max.e[c] ;
    }
  }

  return res ;
}

bool AABB::containsIn( const Shape* shape ) const
{
  // just use the shape's aabb
  return containsIn( *shape->aabb ) ;
}

vector<AABB> AABB::split2( int axisIndex, real val )
{
  vector<AABB> subAABB ;
  if( !BetweenIn(val, min.e[axisIndex], max.e[axisIndex] ) )
  {
    error( "split plane OOB AABB" ) ;
    subAABB.push_back( *this ) ;
    return subAABB ; // ASUS RMA (you get the same one back)
  }

  Vector newmin = min ;
  newmin.e[ axisIndex ] = val ;
  Vector newmax = max ;
  newmax.e[ axisIndex ] = val ;

  subAABB.push_back( AABB( min, newmax ) ) ;
  subAABB.push_back( AABB( newmin, max ) ) ;

  return subAABB ;
}

// there is a cleaner looking (for-loop using) 
// imp in the orange book (Realtime Collision Det.)
// but this one may be faster (no counters)
vector<AABB> AABB::split8()
{
  Vector c = (min + max) / 2 ;

  vector<AABB> subAABB ;

  //     ________
  //    /   /   /|
  //   /___/___/ |
  //  /   /   /| |
  // /___/___/ |/|
  // |   |   | | |
  // |___|___|/|/
  // |   |   | |
  // |___|___|/


  //   _______
  //  / 5  8 /
  // | 6  7   
  // |  1  4  
  // | 2  3  /

  subAABB.push_back( AABB( min, c ) ) ;
  subAABB.push_back( AABB( Vector(min.x,min.y,c.z), Vector(c.x,c.y,max.z) ) ) ;
  subAABB.push_back( AABB( Vector(c.x,min.y,c.z), Vector(max.x,c.y,max.z) ) ) ;
  subAABB.push_back( AABB( Vector(c.x,min.y,min.z), Vector(max.x,c.y,max.z) ) ) ;

  subAABB.push_back( AABB( Vector(min.x,c.y,min.z), Vector(c.x,max.y,c.z) ) ) ;
  subAABB.push_back( AABB( Vector(min.x,c.y,c.z), Vector(c.x,max.y,max.z) ) ) ;
  subAABB.push_back( AABB( c, max ) ) ;
  subAABB.push_back( AABB( Vector(c.x,c.y,min.z), Vector(max.x,max.y,c.z) ) ) ;

  return subAABB ;
}

vector<Vector> AABB::getCorners() const
{
  vector<Vector> corners( 8 ) ;
  corners.push_back( Vector( min.x,min.y,min.z ) ) ;
  corners.push_back( Vector( min.x,min.y,max.z ) ) ;
  corners.push_back( Vector( min.x,max.y,min.z ) ) ;
  corners.push_back( Vector( min.x,max.y,max.z ) ) ;
  
  corners.push_back( Vector( max.x,min.y,min.z ) ) ;
  corners.push_back( Vector( max.x,min.y,max.z ) ) ;
  corners.push_back( Vector( max.x,max.y,min.z ) ) ;
  corners.push_back( Vector( max.x,max.y,max.z ) ) ;
  return corners ;
}

void AABB::generateDebugLines( const Vector& color ) const
{
  // Construct the cuboid using the min/max points.
  Cube cube( min, max ) ;
  
  // So, generate a line mesh.
  cube.generateDebugLines( color ) ;
}

void AABB::generateDebugLines( const Vector& offset, const Vector& color ) const
{
  // Construct the cuboid using the min/max points.
  Cube cube( min, max ) ;
  
  // So, generate a line mesh.
  cube.generateDebugLines( offset, color ) ;
}

Vector AABB::random() const
{
  // gives you a random inside the aabb
  return Vector::randomWithin( min, max ) ;
}





