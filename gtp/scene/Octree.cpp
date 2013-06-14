#include "Octree.h"
#include "../window/GTPWindow.h" // DEBUG
#include "../geometry/Shape.h"
#include "../geometry/Mesh.h"


bool Octree<Shape*>::useKDDivisions = false ;
bool Octree<PhantomTriangle*>::useKDDivisions = false ;

// The trade off is, the more ONodes you create, the longer the
// ONODE intersection takes.  If you don't have enough ONodes,
// then severe slow down on high poly models happens.
int  ONode<Shape*>::maxItems = 15 ;
int  ONode<Shape*>::maxDepth = 3 ;
bool ONode<Shape*>::splitting = false ; // you don't split non triangular primitives

int  ONode<PhantomTriangle*>::maxItems = 15 ;
int  ONode<PhantomTriangle*>::maxDepth = 7 ;
bool ONode<PhantomTriangle*>::splitting = true ;



#pragma region global functions
bool tryPutAway( PhantomTriangle* pTri,
  vector<AABB>& aabbCandChildren,
  map< AABB*, list<PhantomTriangle*> >& aabbsAndTheirContainedItems )
{
  for( int i = 0 ; i < aabbCandChildren.size() ; i++ )
    if( aabbCandChildren[i].containsAlmost( pTri ) )
    //if( aabbCandChildren[i].containsIn( pTri ) )
    {
      // add pTri as being in the AABB:
      aabbsAndTheirContainedItems[ &aabbCandChildren[i] ].push_back( pTri ) ;
      //drawTri( pTri, Vector( .5,.25, .04 ) ) ;
      return true ; // it put away
    }

  // tri didn't fit in any of available child nodes
  return false ;
}

bool tryPutAway( list<PhantomTriangle*>& pTris,
  vector<AABB>& aabbCandChildren,
  map< AABB*, list<PhantomTriangle*> >& aabbsAndTheirContainedItems )
{
  // try and put away the new tris into the
  // child aabbs.
  bool allAway = true ;
  for( list<PhantomTriangle*>::iterator iter = pTris.begin() ; iter != pTris.end() ; )
  {
    if( tryPutAway( *iter, aabbCandChildren, aabbsAndTheirContainedItems ) )
    {
      // it put away.
      // these 2 lines aren't strictly necessary but I need
      // to know the tris that didn't place for debug purposes.
      // they actually get put back in root (if there are problems for whatever reason)
      // if they don't get put away
      iter = pTris.erase( iter ) ;// remove it
      //info( "Tri put away, %d remain", pTris.size() ) ;
    }
    else
    {
      allAway = false ;  // at least one didn't get put away into a child box
      ++iter;
    }
  }

  //info( "Total tris remain: %d", pTris.size() ) ;
  return allAway ;
}

// REMOVES TRIS COINCIDENT WITH PLANE FROM toSPLIT
// AND PLACES THEM IN coincidentTRIS
void removeCoincidentTris( Plane plane, list<PhantomTriangle*> & toSplit, list<PhantomTriangle*> & coincidentTris )
{
  //static Vector call = 0 ;
  //call.y+=.01;
  for( list<PhantomTriangle*>::iterator iter = toSplit.begin() ; iter != toSplit.end() ; )
  {
    PhantomTriangle* pTri = *iter ;
    
    int aV = plane.iSide( pTri->a ) ;
    int bV = plane.iSide( pTri->b ) ;
    int cV = plane.iSide( pTri->c ) ;
    
    // a really bad case is |aV|, |bV|, and |cV| all
    // near 0. this means the points of the triangle are
    // all shavingly close to the plane (ie almost parallel).
    //if( IsNear(aV,0.0,1e-11) && IsNear(bV,0.0,1e-11) && IsNear(cV,0.0,1e-11) )
    if( aV==0&&bV==0&&cV==0 )
    {
      // Do not split this triangle on this plane. take it out.
      coincidentTris.push_back( pTri ) ; 
      //drawTri( call, pTri, Vector(1,0,1) ) ;
      iter = toSplit.erase( iter ) ; // ADVANCES ITER
    }
    else
    {
      //drawTri( call, pTri, Vector(0,0,1) ) ;
      ++iter ;
    }
  }
}

// SPLIT toSPLIT LIST OF PHANTOMTRIANGLE'S ON PLANE.
// Uses a variant of the Sutherland-Hodgman algorithm.
void splitTris( Plane plane, list<PhantomTriangle*> & toSplit, list<PhantomTriangle*> & newTris )
{
  // We create 9 pointers, but only 3 will be used.
  // Each of the 3 points a,b,c of each tri needs
  // to be classified as negative-side, +-side, or
  // ON the splitting plane.
  Vector *nS[3] ;
  Vector *pS[3] ; // ptr to the vert on the _other_ side,
  Vector *on[3] ;

  int origTris = toSplit.size() ;

  for( list<PhantomTriangle*>::iterator iter = toSplit.begin() ; iter != toSplit.end() ; )
  {
    // these are counters for how many
    // vertices so far were on what side of the plane.
    int pSide=0, nSide=0, onPlane=0 ;

    PhantomTriangle* pTri = *iter ;
    Vector* triVectors = &pTri->a ; // start here
    
    // test 3 vertices
    for( int i = 0 ; i < 3 ; i++ )
    {
      int v = plane.iSide( triVectors[i] ) ;
      //triVectors[i].print() ;
      //printf( "  v=%f\n", v ) ;
      if( v == 0 ) on[onPlane++]= &triVectors[i];
      else if( v < 0 ) nS[nSide++]= &triVectors[i];
      else pS[pSide++]= &triVectors[i] ;
    }

    ///info( "pSide=%d, nSide=%d, onPlane=%d", pSide, nSide, onPlane ) ;

    if( nSide>=1 && pSide>=1 ) // split.
    {
      // NOT keeping the winding order.  it doesn't matter anyway because
      // these are intersectable, not renderable.

      // the first ray finds the intersection between the negative side and the ps,
      Intersection i1 ;

      //---|+++
      // o |   
      // |\|--->N
      // | |\  
      // o-X-o 

      // Make a ray FROM a point on the negative side,
      // to a point on the +side of the plane
      Ray r1( *(nS[0]), *(pS[0]) ) ;
      
      // use the plane to get the intersection point
      plane.intersectsPlane( r1, &i1 ) ;
      ///pTri->tri->intersects( r1, &mi1 ) ; // use the original tri to get the 3d space intersection point

      // A vertex is on or very near the plane, so 
      // we'll split AT the vertex (cut only one edge)
      if( onPlane==1 )
      {
        // split using the VERTEX on the plane as the splitter.
        // gen 2 tris.
        // 1) nS, D, ONPLANE,
        // 2) pS, ONPLANE, D.
        PhantomTriangle *pt1 = new PhantomTriangle( *(nS[0]), i1.point, *(on[0]), pTri->tri ) ;
        PhantomTriangle *pt2 = new PhantomTriangle( *(pS[0]), *(on[0]), i1.point, pTri->tri ) ;
        if( pt1->isDegenerate() || pt2->isDegenerate() )
        {
          // This is a very important error to catch, because it happens
          // when something is terribly screwed up.
          window->addSolidDebugTri( pTri, Vector(1,0,0) ) ;
          error( "split fail." ) ;
          delete pt1 ; delete pt2 ;
          ++iter ;
          continue ;
        }
        newTris.push_back( pt1 ) ;
        newTris.push_back( pt2 ) ;
      }
      else 
      {
        // 2 points on nSide
        // We cut 2 edges.

        // if there are 2 nsides, use nS[1] and pS[0]. If 2 psides, nS[0],ps[1].

        // get the side with 2 verts on it
        Vector** sideWith2 = (pSide>nSide)?pS:nS;
        Vector** sideWith1 = (pSide<nSide)?pS:nS;
        
        // Get the second intersection point, from sideWith2[1] to sidewith1[0]
        Ray r2( *(sideWith2[1]), *(sideWith1[0]) ) ;
        Intersection i2 ;
        plane.intersectsPlane( r2, &i2 ) ;

        // 3 tris.
        //      *   
        //     / \
        //    /___\  
        //   / __/ \ 
        //  /_/     \ 
        // *---------*
        //
        // LET D be mi1.point, E mi2.point
        // 1)  sw2[0],D,E
        // 2)  sw2[0],E,sw2[1]
        // 3)  E,D,sw1[0]
        PhantomTriangle *pt1 = new PhantomTriangle( *(sideWith2[0]), i1.point, i2.point, pTri->tri ) ;
        PhantomTriangle *pt2 = new PhantomTriangle( *(sideWith2[0]), i2.point, *(sideWith2[1]), pTri->tri ) ;
        PhantomTriangle *pt3 = new PhantomTriangle( i2.point, i1.point, *(sideWith1[0]), pTri->tri ) ;
        if( pt1->isDegenerate() || pt2->isDegenerate() || pt3->isDegenerate() )
        {
          // This is a very important error to catch, because it happens
          // when something is terribly screwed up.
          window->addSolidDebugTri( pTri, Vector(1,0,0) ) ;
          error( "split fail." ) ; // split fail.
          delete pt1 ; delete pt2 ; delete pt3 ;
          ++iter ;
          continue ;
        }
        newTris.push_back( pt1 ) ;
        newTris.push_back( pt2 ) ;
        newTris.push_back( pt3 ) ;
      }

      // destroy the old PhantomTriangle that got split,
      // we don't need it anymore.
      DESTROY( *iter ) ;
      iter = toSplit.erase( iter ) ; // ADVANCES ITER
    }
    else
    {
      // Here you cannot split the polygon, either because 
      // all 3 are on one side, or one is on the plane and the
      // other 2 are on the same side.
      // I mean this is not a big deal and it is expected to happen a lot,
      // because a lot of tris you try to split will be on one side of the plane,
      // more rarely all 3 verts will be IN the plane.

      // if it does happen then like ASUS RMA you get the same triangle back.
      ///info( "Could not split the polygon, pSide=%d, nSide=%d, onPlane=%d", pSide, nSide, onPlane ) ;
      ++iter ; // advance iterator
    }
  }

  // Splits fail a lot, because this gets called 3x per division.
  ////info( "%d tris became %d tris, %d remain", origTris, newTris.size(), toSplit.size() ) ;
}

void getMeanStdDev( const list<PhantomTriangle*>& items, Vector& mean, Vector& stddev )
{
  for( auto pt : items )
    mean += pt->a + pt->b + pt->c ;
  mean /= 3*items.size() ;

  for( auto pt : items )
  {
    Vector diffa = (pt->a - mean) ;
    Vector diffb = (pt->b - mean) ;
    Vector diffc = (pt->c - mean) ;

    stddev += diffa.componentSquared() + 
      diffb.componentSquared() + 
      diffc.componentSquared() ;
  }
}
#pragma endregion






void drawRay( const Ray& ray, const Vector& color )
{
  window->addDebugRayLock( ray, color, color*2 ) ;
}

void drawTri( const Vector& offset, PhantomTriangle * pTri, const Vector& color )
{
  window->addSolidDebugTri( offset, pTri, color ) ;
}

void drawTri( PhantomTriangle * pTri, const Vector& color )
{
  drawTri( 0, pTri, color ) ;
}

void drawTris( list<PhantomTriangle *>& pTri, const Vector& stain )
{
  for( list<PhantomTriangle*>::iterator iter = pTri.begin() ; iter != pTri.end(); ++ iter )
    drawTri( *iter, stain ) ;
}

void drawTris( list<PhantomTriangle *>& pTri )
{
  drawTris( pTri, Vector(1,1,1) ) ;
}

