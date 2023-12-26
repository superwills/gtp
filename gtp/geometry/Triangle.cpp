#include "Triangle.h"
#include "Mesh.h"
#include "../window/D3DDrawingVertex.h"
#include "AABB.h"
#include "../window/GTPWindow.h" // DEBUG

Triangle::Triangle( const Vector& pa, const Vector& pb, const Vector& pc, Mesh*iMeshOwner ):
Plane(pa,pb,pc),a(pa),b(pb),c(pc),iA(-1),iB(-1),iC(-1), meshOwner(iMeshOwner)
{
  precomputeBary() ;
}

Triangle::Triangle( const Vector& pa, const Vector& pb, const Vector& pc, int ia, int ib, int ic, Mesh*iMeshOwner ) :
Plane(pa,pb,pc),a(pa),b(pb),c(pc),iA(ia),iB(ib),iC(ic),meshOwner( iMeshOwner )
{
  precomputeBary() ;
}

void Triangle::precomputeBary()
{
  v0 = b-a, v1=c-a ;
  d00 = v0 % v0;
  d01 = v0 % v1; 
  d11 = v1 % v1; 
  invDenomBary = 1.0/(d00 * d11 - d01 * d01);
}

void Triangle::load( FILE *file )
{
  fread( &a, 3*sizeof(Vector)+3*sizeof(int), 1, file ) ;
}

void Triangle::setColor( ColorIndex colorIndex, const Vector& color )
{
  vA()->color[ colorIndex ] =
  vB()->color[ colorIndex ] =
  vC()->color[ colorIndex ] = color ;
}

void Triangle::setId( int iid )
{
  id = iid ;
  if( iA != -1 && iB != -1 && iC != -1 )
  {
    // if its indexed, then setting these is errorful
    ///////////////if( meshOwner->getMeshType() == MeshType::Indexed )
    ///////////////  warning( "You're trying to set the id for an indexed mesh, results not going to be good" ) ;
    meshOwner->verts[ iA ].texcoord[ TexcoordIndex::RadiosityID ] =
    meshOwner->verts[ iB ].texcoord[ TexcoordIndex::RadiosityID ] =
    meshOwner->verts[ iC ].texcoord[ TexcoordIndex::RadiosityID ] = ByteColor(id).toRGBAVector() ; //!! YOU COULD ALSO CHANGE THIS TO (id|ALPHA_FULL) and
    // NOT have to demask the ALPHA_FULL bits every time you want to
    // use the "id" field as a straight integer.
  }
  else
  {
    error( "Cannot set id of triangle since it has no underlying mesh indices to attach" ) ;
  }
}

Vector Triangle::getAverageColor( ColorIndex what )
{
  Vector avg = vA()->color[ what ] + vB()->color[ what ] + vC()->color[ what ] ;
  return avg / 3.0 ;
}

Vector Triangle::getAverageTexcoord( TexcoordIndex what )
{
  Vector avg = vA()->texcoord[ what ] + vB()->texcoord[ what ] + vC()->texcoord[ what ] ;
  return avg / 3.0 ;
}

// a bit of an optimization avoid adding
// components you don't need
real Triangle::getAverageColor( ColorIndex what, int channelIndex )
{
  real avg = vA()->color[ what ].e[ channelIndex ] +
             vB()->color[ what ].e[ channelIndex ] +
             vC()->color[ what ].e[ channelIndex ] ;
  return avg / 3.0 ;
}

real Triangle::getAverageTexcoord( TexcoordIndex what, int channelIndex )
{
  real avg = vA()->texcoord[ what ].e[ channelIndex ] +
             vB()->texcoord[ what ].e[ channelIndex ] +
             vC()->texcoord[ what ].e[ channelIndex ] ;
  return avg / 3.0 ;
}

real Triangle::getAverageAO()
{
  return ( vA()->ambientOcclusion + vB()->ambientOcclusion + vC()->ambientOcclusion ) / 3 ; 
}

Vector Triangle::getBarycentricCoordinatesAt( const Vector & P ) const
{
  // 
  Vector bary ;
 
  // shirley
  #if 0 
  real areaABC = normal % ( (b - a) << (c - a) ) ;
  real areaPBC = normal % ( (b - P) << (c - P) ) ;
  real areaPCA = normal % ( (c - P) << (a - P) ) ;

  bary.x = areaPBC / areaABC ; // a
  bary.y = areaPCA / areaABC ; // b
  bary.z = 1.0 - bary.x - bary.y ; // c
  #else
  // rtcd ericsson
  // reduced runtime cost for static meshes
  Vector v2=P-a ;
  float d20 = v2 % v0;
  float d21 = v2 % v1;
  bary.y = (d11 * d20 - d01 * d21) * invDenomBary ;
  bary.z = (d00 * d21 - d01 * d20) * invDenomBary ;
  bary.x = 1.0 - bary.y - bary.z;
  #endif

  return bary ;

}

Vector Triangle::getRandomPointOn() const
{
  // barycentric
  real alpha = randFloat() ;
  real beta = randFloat() ;
  real gamma = 1 - alpha - beta ;

  return alpha*a + beta*b + gamma*c ; // point on triangle
}

bool Triangle::isDegenerate() const
{
  if( area() <= EPS_MIN ||
      a.isBad() || b.isBad() || c.isBad() )
    return true ;
  else
    return false ;
}

//static
bool Triangle::isDegenerate( const Vector& A, const Vector& B, const Vector& C )
{
  // you know it's degenerate if the 3 points are too close together OR any is bad
  if( A.Near( B ) && A.Near( C ) ||
      A.isBad() || B.isBad() || C.isBad() )
    return true ;

  return false ;
}

/// Computes and returns the
/// area of the triangle.
real Triangle::area() const
{
  Vector ab = b - a ;
  Vector ac = c - a ;

  real area = ( ab << ac ).len() / 2.0 ;
  return area ;
}

int Triangle::planeSide( const Plane& plane )
{
  int sa = plane.iSide( a ) ;
  int sb = plane.iSide( b ) ;
  int sc = plane.iSide( c ) ;

  // If they're ALL THE SAME, return that,
  if( sa==sb && sa==sc )
    return sa ;
  else
    return 0 ; // "straddling"
}

void Triangle::refresh()
{
  // update the pos and recalculate the normal for each tri
  a = vA()->pos ;
  b = vB()->pos ;
  c = vC()->pos ;

  recomputePlane( a, b, c ) ;
}

void Triangle::transform( const Matrix& m )
{
  a = a*m ;
  b = b*m ;
  c = c*m ;

  // this has gotten stale
  recomputePlane( a,b,c );
  precomputeBary() ;
}

bool Triangle::intersects( const Ray& ray, MeshIntersection* intn )
{
  // 1)  Intersect against plane in which triangle lies.
  //     Call this intersection point P.
  // 2)  Check that P lies strictly between
  //     the triangle vertices a, b, and c.  To do this
  //     we'll use barycentric coordinates.
  
  // 4x misses:
  //RayShootingAwayFromPlane
  // t < 0

  //RayFallsShortOfPlane
  // t > rayLength
  
  //RayParallelToPlane
  // Either inside plane or
  // skewed
  // t = INF
  
  //RayHitsPlaneMissesTriangle
  // Valid t, barycentric eliminates intersection
  // 0 < t < rayLength

  //RayHitsTriangle
  // 0 < t < rayLength, and
  // barycentric confirms intn
  real den = normal.Dot( ray.direction ) ;
  
  if( den == 0 )
  {
    // RayParallelToPlane:
    
    // ray is orthogonal to plane normal ie
    // parallel to PLANE
    return false ;
  }
  
  real num = normal.Dot(ray.startPos) + d ;
  real t = - (num / den) ;

  if( t < 0 || t > ray.length )
  {
    // t < 0 : RayShootingAwayFromPlane:  complete miss, ray is in front of plane
    // t > ray.len : RayFallsShortOfPlane:  "out of reach" of ray.
    // there isn't an intersection here either
    return false ;
  }

  // Now we are either:
  //   RayHitsPlaneMissesTriangle ||
  //   RayHitsTriangle
  Vector P = ray.at( t ) ;
  
  // In order to see if it's a hit, you need
  // the barycentric coordinates
  Vector bary = getBarycentricCoordinatesAt( P ) ;
  
  if( ! bary.isNonnegative() ) // RayHitsPlaneMissesTriangle
    return false ;
  else  // RayHitsTriangle
  {
    if( intn )
    {
      // interpolate the normal, only if it is called for
      Vector interpNormal = bary.x * vA()->norm + bary.y * vB()->norm + bary.z * vC()->norm ;
      interpNormal.normalize() ;
      *intn = MeshIntersection( P, interpNormal, bary, this ) ;
    }
    
    /// Doesn't know its parent mesh, could be saved in this->mesh
    return true ;
  }
}

//bool Triangle::intersects( const AABB& aabb )
//{
  // uses 3 ray-triangle intersection tests
  // to determine if the triangle penetrates the cube
  // (the tri straddles the cube)
  //Ray rayAB( a,b ), rayAC( a,c ), rayBC( b,c ) ;

  // then test the cube corners to determine if the cube
  // straddles the tri
  // (ie different signs in plane eq)

  // check that each is on the same side
  //if( sameSide( aabb.getCorners() ) )
//}

bool Triangle::belongsTo( const Shape* shape )
{
  return shape == meshOwner->shape ;
}

/// Returns to you the centroid of
/// the triangle. (average of all its points)
Vector Triangle::getCentroid() const
{
  return ( a + b + c ) / 3 ;
}

void Triangle::subdivide( int depth, int maxDepth, real maxArea, list<Triangle>& triList )
{
  // a base case: a final triangle is eithre maxdepth or area <= maxarea
  if( depth >= maxDepth || area() <= maxArea )
  {
    triList.push_back( *this ) ;
    return ;
  }

  // A----D----C
  //  \  / \  /
  //   E/___\F
  //    \   /
  //     \ /
  //      B
    
  Vector d = ( a + c ) / 2 ;
  Vector e = ( a + b ) / 2 ;
  Vector f = ( b + c ) / 2 ;

  // Call recursively on this mesh.
  Triangle( d, e, f,meshOwner ).subdivide( depth+1, maxDepth, maxArea, triList ) ;
  Triangle( a, e, d,meshOwner ).subdivide( depth+1, maxDepth, maxArea, triList ) ;
  Triangle( d, f, c,meshOwner ).subdivide( depth+1, maxDepth, maxArea, triList ) ;
  Triangle( e, b, f,meshOwner ).subdivide( depth+1, maxDepth, maxArea, triList ) ;
}

void Triangle::recursivelySubdivideAndAddToMeshWhenSmallEnough( int depth, int maxDepth, real maxArea, Mesh* mesh )
{
  if( depth >= maxDepth || area() <= maxArea )       // acceptable size
    mesh->addTri( a, b, c ) ;   // add it to the mesh already, which 
  else
  {
    // subdivide it and check THOSE tris

    // A----D----C
    //  \  / \  /
    //   E/___\F
    //    \   /
    //     \ /
    //      B
    
    Vector d = ( a + c ) / 2 ;
    Vector e = ( a + b ) / 2 ;
    Vector f = ( b + c ) / 2 ;

    //!! OOPS!!  I used MESHOWNER previously, which is a reference to the
    // ORIGINAL Mesh that is BEING tessellated. While that reference will
    // remain valid after Mesh::tessellate runs, meshOwner usually gets
    // destroyed (the original untessellated mesh is destroyed after the
    // new more finely tessellated mesh gets created.)
    Triangle( d, e, f,mesh ).recursivelySubdivideAndAddToMeshWhenSmallEnough( depth+1, maxDepth, maxArea, mesh ) ;
    Triangle( a, e, d,mesh ).recursivelySubdivideAndAddToMeshWhenSmallEnough( depth+1, maxDepth, maxArea, mesh ) ;
    Triangle( d, f, c,mesh ).recursivelySubdivideAndAddToMeshWhenSmallEnough( depth+1, maxDepth, maxArea, mesh ) ;
    Triangle( e, b, f,mesh ).recursivelySubdivideAndAddToMeshWhenSmallEnough( depth+1, maxDepth, maxArea, mesh ) ;
  } 
}

void Triangle::recursivelySubdivideAndPushToSphereThenAddToMeshWhenDepthReachedOrSmallEnough( int depth, int maxDepth, real maxArea, real r, Mesh* mesh )
{
  if( depth >= maxDepth || area() <= maxArea )       // acceptable size
    mesh->addTri( a, b, c ) ;   // add it to the mesh already, which 
  else
  {
    // subdivide it and check THOSE tris

    // A----D----C
    //  \  / \  /
    //   E/___\F
    //    \   /
    //     \ /
    //      B
    
    Vector d = ( a + c ) / 2 ;
    Vector e = ( a + b ) / 2 ;
    Vector f = ( b + c ) / 2 ;

    d.setMag( r ) ;
    e.setMag( r ) ;
    f.setMag( r ) ;

    Triangle( d, e, f,mesh ).recursivelySubdivideAndPushToSphereThenAddToMeshWhenDepthReachedOrSmallEnough( depth+1, maxDepth, maxArea, r, mesh ) ;
    Triangle( a, e, d,mesh ).recursivelySubdivideAndPushToSphereThenAddToMeshWhenDepthReachedOrSmallEnough( depth+1, maxDepth, maxArea, r, mesh ) ;
    Triangle( d, f, c,mesh ).recursivelySubdivideAndPushToSphereThenAddToMeshWhenDepthReachedOrSmallEnough( depth+1, maxDepth, maxArea, r, mesh ) ;
    Triangle( e, b, f,mesh ).recursivelySubdivideAndPushToSphereThenAddToMeshWhenDepthReachedOrSmallEnough( depth+1, maxDepth, maxArea, r, mesh ) ;
  } 
}

bool Triangle::isAdjacent( Triangle& other, Adjacency* adj )
{
  // 2/3 verts must be shared

  // either:   a|a  &&  b|b
  //                    c|c
  //                    b|c
  //                    c|b
  //
  //           b|b  &&  a|c // a is not shared
  //                    c|a
  //                    c|c
  //
  //           c|c  &&  a|b // a,b are not shared
  //                    b|a
  if( !adj )
  {
    // does not want adjacency information
    return (
             (a , other.a) &&  // a is shared
             ( (b , other.b) || (c , other.c) || (b , other.c) || (c , other.b) )
           ) ||
           (
             (b , other.b) &&  // b is shared, a NOT shared due to above (one less test below)
             ( (a , other.c) || (c , other.a) || (c , other.c) )
           ) ||
           (
             (c , other.c) &&  // c is shared, a,b NOT shared due to above (two less tests below)
             ( (a , other.b) || (b , other.a) )
           ) ;
  }
  else
  {
    adj->tri1 = this ;
    adj->tri2 = &other ;

    if( a , other.a )
    {
      // a is shared
      adj->sharedVertex1 = a ;
      adj->sharedIndex1onT1 = iA ;       // these will be the same if used index buffers
      adj->sharedIndex1onT2 = other.iA ; // these will be the same if used index buffers

      if( b , other.b )
      {
        adj->sharedVertex2 = b ;
        adj->sharedIndex2onT1 = iB ;
        adj->sharedIndex2onT2 = other.iB ;

        adj->nonSharedVertexOnT1 = c ;
        adj->nonSharedVertexOnT2 = other.c ;
      }
      else if( b , other.c )
      {
        adj->sharedVertex2 = b ;
        adj->sharedIndex2onT1 = iB ;
        adj->sharedIndex2onT2 = other.iC ;
        adj->nonSharedVertexOnT1 = c ;
        adj->nonSharedVertexOnT2 = other.b ;
      }
      else if( c , other.c )
      {
        adj->sharedVertex2 = c ;
        adj->sharedIndex2onT1 = iC ;
        adj->sharedIndex2onT2 = other.iC ;
        adj->nonSharedVertexOnT1 = b ;
        adj->nonSharedVertexOnT2 = other.b ;
      }
      else if( c , other.b )
      {
        adj->sharedVertex2 = c ;
        adj->sharedIndex2onT1 = iC ;
        adj->sharedIndex2onT2 = other.iB ;
        adj->nonSharedVertexOnT1 = b ;
        adj->nonSharedVertexOnT2 = other.c ;
      }
      else  // only one is shared
        return false ;
    }
    else if( b , other.b )
    {
      // b is shared, a NOT shared due to above (one less test below)
      adj->sharedVertex1 = b ;
      adj->sharedIndex1onT1 = iB ;       // these will be the same if used index buffers
      adj->sharedIndex1onT2 = other.iB ; // these will be the same if used index buffers

      if(a , other.c)
      {
        adj->sharedVertex2 = a ;
        adj->sharedIndex2onT1 = iA ;
        adj->sharedIndex2onT2 = other.iC ;
        adj->nonSharedVertexOnT1 = c ;
        adj->nonSharedVertexOnT2 = other.b ;
      }
      else if(c , other.a)
      {
        adj->sharedVertex2 = c ;
        adj->sharedIndex2onT1 = iC ;
        adj->sharedIndex2onT2 = other.iA ;
        adj->nonSharedVertexOnT1 = a ;
        adj->nonSharedVertexOnT2 = other.c ;
      }
      else if(c , other.c)
      {
        adj->sharedVertex2 = c ;
        adj->sharedIndex2onT1 = iC ;
        adj->sharedIndex2onT2 = other.iC ;
        adj->nonSharedVertexOnT1 = a ;
        adj->nonSharedVertexOnT2 = other.a ;
      }
      else
        return false ;
    }
    else if( c , other.c )
    {
      // c is shared, a,b NOT shared due to above (two less tests below)
      adj->sharedVertex1 = c ;
      adj->sharedIndex1onT1 = iC ;       // these will be the same if used index buffers
      adj->sharedIndex1onT2 = other.iC ; // these will be the same if used index buffers

      if(a , other.b)
      {
        adj->sharedVertex2 = a ;
        adj->sharedIndex2onT1 = iA ;
        adj->sharedIndex2onT2 = other.iB ;
        adj->nonSharedVertexOnT1 = b ;
        adj->nonSharedVertexOnT2 = other.a ;
      }
      else if(b , other.a)
      {
        adj->sharedVertex2 = b ;
        adj->sharedIndex2onT1 = iB ;
        adj->sharedIndex2onT2 = other.iA ;
        adj->nonSharedVertexOnT1 = a ;
        adj->nonSharedVertexOnT2 = other.b ;
      }
      else
        return false ;
    }
  }

  return true ; // if you didn't return false in
  // one of the traps above, then you get here
}

bool Triangle::usesVertex( const Vector& v )
{
  return a.Near( v ) || b.Near( v ) || c.Near( v ) ;
}

AllVertex* Triangle::vA()
{
  return &meshOwner->verts[ iA ] ;
}

AllVertex* Triangle::vB()
{
  return &meshOwner->verts[ iB ] ;
}

AllVertex* Triangle::vC()
{
  return &meshOwner->verts[ iC ] ;
}

PhantomTriangle::PhantomTriangle( Triangle& iTri ):Plane(iTri.a, iTri.b, iTri.c)
{
  a = iTri.a ;
  b = iTri.b ;
  c = iTri.c ;
  tri = &iTri ;
}

PhantomTriangle::PhantomTriangle( PhantomTriangle* iPhantom ):Plane( iPhantom->a, iPhantom->b, iPhantom->c )
{
  a = iPhantom->a ;
  b = iPhantom->b ;
  c = iPhantom->c ;
  tri = iPhantom->tri ;
}

PhantomTriangle::PhantomTriangle( const Vector& pa, const Vector& pb, const Vector& pc, Triangle* origTri ):Plane(pa,pb,pc)
{
  a = pa ;
  b = pb ;
  c = pc ;
  tri = origTri ;
}

// this check is meaningless, because the
// child triangle's a, b and c aren't
// along the same edges of the parent triangle's a, b and c.
//bool PhantomTriangle::hasLongerEdgesThanOwner() const
//{
//  return (b-a).len2() > (tri->b-tri->a).len2()
//  || (b-c).len2() > (tri->b-tri->c).len2()
//  || (c-a).len2() > (tri->c-tri->a).len2() ;
//}

bool PhantomTriangle::isDegenerate() const
{
  if( //IsNear( area(), 0.0, EPS_MIN ) ||
      area() <= 0 // 0/negative area degen also. Don't use any epsilon here because you will get really bad false positives for large epsilon.
      || a.isBad() || b.isBad() || c.isBad()
      
      // PHANTOM ONLY: any of the side lens greater than parent's side lens
      //||hasLongerEdgesThanOwner() 

      )
    return true ;
  else
    return false ;
}

real PhantomTriangle::area() const
{
  Vector ab = b - a ;
  Vector ac = c - a ;

  real area = ( ab << ac ).len() / 2.0 ;
  return area ;
}

int PhantomTriangle::planeSide( const Plane& plane )
{
  int sa = plane.iSide( a ) ;
  int sb = plane.iSide( b ) ;
  int sc = plane.iSide( c ) ;

  // If they're ALL THE SAME, return that,
  if( sa==sb && sa==sc )
    return sa ;
  else
    return 0 ; // "straddling"
}

