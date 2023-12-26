#ifndef OCTREE_H
#define OCTREE_H

#include <list>
#include <functional>
#include <set>
using namespace std ;
#include "../geometry/AABB.h"
#include "../geometry/Intersection.h"
#include "../math/Vector.h"

// I need these for the template specializations,
// but whenever i try to move the specs to a different file,
// I get this linker error.
#include "../geometry/Triangle.h"
#include "../geometry/Shape.h"
#include "../geometry/Mesh.h"

class Scene ;
struct Shape ;
struct Triangle ;



// dummy handoff function because I cannot access GTPWindow.h here
// (it's a template and the cross-include problem comes up)
void drawRay( const Ray& ray, const Vector& color ) ;
void drawTri( const Vector& offset, PhantomTriangle * pTri, const Vector& color ) ;
void drawTri( PhantomTriangle * pTri, const Vector& color ) ;
void drawTris( list<PhantomTriangle *>& pTri, const Vector& stain ) ;
void drawTris( list<PhantomTriangle *>& pTri ) ;
void getMeanStdDev( const list<PhantomTriangle*>& items, Vector& mean, Vector& stddev ) ;

// You try and put away PT into any of aabbCandChildren,
// the result is TRUE or FALSE and an update to the mapping
// of which AABB's contain which PT's.
bool tryPutAway( PhantomTriangle* pTri,
  vector<AABB>& aabbCandChildren,
  map< AABB*, list<PhantomTriangle*> >& aabbsAndTheirContainedItems ) ;

bool tryPutAway( list<PhantomTriangle*>& pTris,
  vector<AABB>& aabbCandChildren,
  map< AABB*, list<PhantomTriangle*> >& aabbsAndTheirContainedItems ) ;

void removeCoincidentTris( Plane plane, list<PhantomTriangle*> & toSplit, list<PhantomTriangle*> & coincidentTris ) ;

// General slicing plane
void splitTris( Plane plane, list<PhantomTriangle*> & toSplit, list<PhantomTriangle*> & newTris ) ;

#pragma region abstract node classes
template <typename T> struct ONode
{
  static int maxItems, maxDepth ;
  static bool splitting ;
  
  // List because I'm erasing from the middle.
  list<T> items; // items in THIS node.
  
  // supports querying and splitting
  //virtual void split() ;
  //virtual void select( const Ray& ray, vector<T>& addList ) ;
  virtual void generateDebugLines( Vector color ) const = 0 ;
} ;

// Now that the CubicSpacePartition is templated, it doesn't know
// what you want to fill it with.  So you must do that
// yourself.
// Bounding volume hierarchy class
template <typename T> class CubicSpacePartition
{
public:
  virtual void add( T item ) = 0 ;
  virtual int numNodes() const = 0 ;
  // number of "items" hanging in the tree
  virtual int numItems() const = 0 ;
  
  // This really could be called SELECT
  virtual void intersectsNodes( const Ray& ray, vector< ONode<T> * >& addList ) const = 0 ;
  virtual void intersectsNodes( const Vector& pt, vector< ONode<T> * >& addList ) const = 0 ;
  virtual void allNodes( vector< ONode<T> * >& addList ) = 0 ;
  virtual void allItems( list<T>& addList ) const = 0 ;
  virtual void split() = 0 ;

  // This actually DELETES the items in the root. Used for octrees that construct "fictional"
  // overlay geometry such as Octree<PhantomTriangle*>
  // Clears the tree without destroying the items.
  virtual void deleteItems() = 0 ;
  virtual void generateDebugLines( Vector color ) const = 0 ;
} ;
#pragma endregion

#pragma region concrete nodes
// An octree node
// T MUST SUPPORT .CONTAINS
// An ONode is AABB-based.
template <typename T> struct OctreeNode : public ONode<T>
{
  AABB bounds ;
  
  //ONode* parent ; // facilitate movement thru the tree
  vector< OctreeNode<T> * > children; // cells, octants, cubelets
  
  OctreeNode() { }

  // Destruction
  ~OctreeNode() {
    // DO NOT delete all the items.
    // Usually the ONode only indexes objects
    // that are in the scene some other how (in another list somehwere)
    // if you delete them here, other references become invalid. enter argument for "smart pointers".
    deleteChildren() ;// make him kill all his subchildren
  }

  // Deletes the actual child nodes of the tree,
  // deallocates the lists of items but does NOT
  // invoke a destructor on each item.
  void deleteChildren() {
    // destruct each child, which in turn results in calls for them to destruct their children
    for( int i = 0 ; i < children.size() ; i++ )
      delete children[i] ;
    children.clear() ; // in case you called delete children NOT from the dtor
  }

  // Destructs the ITEMS in the nodes, but leaves the tree intact.
  void deleteItems() {
    for( list<T>::iterator iter = items.begin() ; iter != items.end() ; ++iter )
      delete (*iter) ;
    // delete all items in children
    for( int i = 0 ; i < children.size() ; i++ )
      children[i]->deleteItems() ;
  }

  // Adding
  void add( T item ){
    // try and push it down to the furthest node
    // that will already accept it..

    // if it can't be accepted 
    // wait, no. if the root is empty,
    // this is wrong.  you can't expand an octree
    // after SPLIT easily.

    // Expand the bounds to allow for this item.
    bounds.bound( item ) ; // the item must either be
    // supported by AABB::bound() (ie have an overload written for it)
    
    items.push_back( item ) ;
  }

  bool fits( T item ) const {
    return bounds.containsIn( item ) ;
  }

  bool hasChildren() const {
    return !children.empty();
  }
  
  // QUERYING AND SELECTION
  // how many children do I have (r)?
  int numNodes() const {
    int count = 0 ; // NOT counting me (my parent does that)
    for( int i = 0 ; i < children.size() ; i++ )
      count += children[i]->numNodes() ; // +size of my children (recursive call)
    return children.size() + count ;
  }
  // how many items do my CHILDREN and (r: my children's children) hold (r)? (not me)
  int numItems() const {
    int count = items.size() ;
    for( int i = 0 ; i < children.size() ; i++ )
      count += children[i]->numItems() ;
    return count ;
  }
  void allItems( list<T>& addList ) const {
    addList.insert( addList.end(), items.begin(), items.end() ) ;
    for( int i = 0 ; i < children.size() ; i++ )
      children[i]->allItems( addList ) ;
  }
  // Check if me ONode or my children ONodes 
  // are intersected by the ray, if they are add to addList.
  void allNodes( vector< ONode<T> * >& addList ) {
    addList.push_back( this ) ;
    for( int i = 0 ; i < children.size() ; i++ )
      children[i]->allNodes( addList ) ;
  }

  #pragma region octree split
  // Tries to put all the items into
  // these AABBCandidateChildren.
  // Some of them may be empty, so those nodes will be unused.
  // remember, these are CANDIDATE children. If they contain no tris,
  // they will not be used
  void tryPutItemsInto( const vector<AABB>& aabbCandChildren )
  {
    // See if the children completely contain
    // which of my items.
    //   - go thru (ALL #BOXES) boxes ONCE, see what tris fit in each box
    //     - go thru list of tris (BIG) max (#BOXES) times (shrinks each iteration)
    for( int i = 0 ; i < aabbCandChildren.size() ; i++ )
    {
      list<T> aabbContainedItems ;
      
      for( list<T>::iterator iter = items.begin() ; iter != items.end() ; )
      {
        if( aabbCandChildren[i].containsIn( *iter ) )
        {
          // move the tri to the other list.
          aabbContainedItems.push_back( *iter ) ;
          iter = items.erase( iter ) ; // remove from this list.. advances iter.
        }
        else
          ++iter ; // advance iter.
      }

      // if the candChild AABB contains at least ONE
      // tri, then instantiate it as an ONode
      if( aabbContainedItems.size() > 0 )
      {
        OctreeNode* chNode = new OctreeNode() ;
        chNode->bounds = aabbCandChildren[i] ;
        chNode->items = aabbContainedItems ;
        children.push_back( chNode ) ;
      }
      // else AABB DROPPED/not used.
    }
  }
  
  // Split me into 8 or less octants, until
  // each octant. Recurse until my children/grandchildren
  // each meet maxTris class-level rest.
  void splitAsOctree( int depth )
  {
    // Divide that into 8 children.
    // so long as each child contains more than 
    // X tris
    if( items.size() < maxItems || depth > maxDepth )
      return ;
   
    // needs to be split.
    vector<AABB> aabbCandChildren = bounds.split8() ;
    tryPutItemsInto( aabbCandChildren ) ;

    // Now split my children
    for( int i = 0 ; i < children.size() ; i++ )
      children[i]->splitAsOctree( depth+1 ) ;
  }

  // Passes the last split axis, because this version
  // cycles thru the split axes
  void splitAsKdtree( int depth, int splitAxis )
  {
    if( items.size() < maxItems || depth > maxDepth )
      return ;
   
    // needs to be split.
    // find an appropriate split point.
    vector<AABB> aabbCandChildren = bounds.split2( splitAxis, bounds.mid().e[ splitAxis ] ) ;
    tryPutItemsInto( aabbCandChildren ) ;

    // Now split my children
    for( int i = 0 ; i < children.size() ; i++ )
      children[i]->splitAsKdtree( depth+1, (splitAxis+1)%3 ) ; // axis=3 wraps back to 0
  }
  #pragma endregion

  void generateDebugLines( Vector color ) const override {
    // use the AABB
    bounds.generateDebugLines( color ) ;  // put your debug lines on the map..
  }

  // Also generates children's debug lines.
  void generateDebugLines( Vector offset, Vector color ) {
    // USING THIS NODE'S AABB:
    offset.y += 2*bounds.extents().y ;
    // use the midpoint to determine x/z offset
    //offset.x += .1*bounds.mid().x ;
    //offset.z += .1*bounds.mid().z ;

    bounds.generateDebugLines( offset, color ) ;  // put your debug lines on the map..

    for( int i = 0 ; i < children.size() ; i++ )  // and if you have children..
      children[i]->generateDebugLines( offset, color*1.1 ) ; // ..tell them to do the same, but darker
  }

  // Check if me ONode or my children ONodes 
  // are intersected by the ray, if they are add to addList.
  void intersects( const Ray& ray, vector< ONode<T> * >& addList ) {
    if( bounds.intersects( ray ) )
    {
      // it hit me,
      addList.push_back( this ) ;

      // check my children in this case,
      // recursively calling back this function.
      // termination is when a node has no children
      for( int i = 0 ; i < children.size() ; i++ )
        children[i]->intersects( ray, addList ) ;
    }
  }

  // Check if me ONode or my children ONodes 
  // are intersected by the ray, if they are add to addList.
  void intersects( const Vector& pt, vector< ONode<T> * >& addList ) {
    if( bounds.containsAlmost( pt ) )
    {
      // it hit me,
      addList.push_back( this ) ;

      // check my children in this case,
      // recursively calling back this function.
      // termination is when a node has no children
      for( int i = 0 ; i < children.size() ; i++ )
        children[i]->intersects( pt, addList ) ;
    }
  }

  // tells you if this ONode contains at least part of
  // (at least 1 tri of) the Shape in question.
  // this is used to 
  // Possible because each has: tri->mesh->shape.
  //bool contains( const T* item )
  //{
  //  for( list<Triangle*>::iterator iter = tris.begin(); iter != tris.end() ; ++iter )
  //    if( (*iter)->belongsTo( shape ) )
  //      return true ;
  //  return false ;
  //}

  //void eachChild( function<void (ONode<T>*)> func )
  //{
  //  for_each( children.begin(), children.end(), func ) ;
  //}
} ;

// INTERFACE BASE CLASS FOR A SPACE PARTITIONER.


// A KDTree node has it's 
// T MUST BE ABLE TO TELL ME WHAT SIDE
// OF A PLANE IT IS ON!
// T SHOULD BE A POINTER,
// T SHOULD SUPPORT T->planeSide( Plane )
// T SHOULD SUPPORT T->getCentroid()
template <typename T> struct KDNode : public ONode<T>
{
  int splitAxis ; // 0==x,1=y,2=z
  int o1, o2 ;  // the 'other' axes
  real splitVal ; // distance from the origin of the splitting plane
  //Plane splittingPlane ; // makes it general.

  // need to know BOUNDS of splitting plane.
  //real wo1, wo2 ; // width in o1 and o2 axes.
  AABB bounds ; // I need bounds for my space.
  // This isn't used in traditional intersection, but
  // 

  // nodes BEHIND the plane, and infront of the plane,
  KDNode *behind, *infront ;
  
  KDNode() {
    defParams() ;
  }
  KDNode( const list<T>& iitems )
  {
    defParams() ;
    items = iitems ; 
    for( auto it : items )
      bounds.bound( it ) ;
  }
  void defParams()
  {
    behind=infront=0;
    splitAxis=0,o1=1,o2=2,splitVal=0;
  }
  ~KDNode(){
    deleteChildren() ;
  }
  void deleteChildren() {
    // destruct each child, which in turn results in calls for them to destruct their children
    // clears ptrs to 0, in case you called delete children NOT from the dtor
    DESTROY( infront ) ;
    DESTROY( behind ) ;
  }
  // Destructs the items in the tree while leaving the tree intact.
  void deleteItems() {
    for( list<T>::iterator iter = items.begin() ; iter != items.end() ; ++iter )
      delete (*iter) ;
    // delete all items in children
    if( behind ) behind->deleteItems() ;
    if( infront ) infront->deleteItems() ;
  }
  
  void add( T item )
  {
    bounds.bound( item ) ;
    items.push_back( item ) ;
  }

  bool hasChildren(){
    return behind || infront;
  }
  int numNodes() {
    int n = 1 ;
    if( behind ) n += behind->numNodes() ;
    if( infront ) n += infront->numNodes() ;
    // Using ternary didn't work has an error.
    //return 1 +
    //  behind?behind->numNodes():0 +
    //  infront?infront->numNodes():0;
    return n ;
  }
  int numItems()
  {
    int n = items.size() ;
    if( behind ) n += behind->numItems() ;
    if( infront ) n += infront->numItems() ;
    return n ;
    //return items.size() +
    //  behind?behind->numItems():0 +
    //  infront?infront->numItems():0;
  }
  
  // SELECTION
  void allItems( list<T>& addList ) {
    addList.insert( addList.end(), items.begin(), items.end() ) ;
    if( behind ) behind->allItems( addList ) ;
    if( infront ) infront->allItems( addList ) ;
  }
  void allNodes( vector< ONode<T> * >& addList ) {
    addList.push_back( this ) ;
    if( behind ) behind->allNodes( addList ) ;
    if( infront ) infront->allNodes( addList ) ;
  }
  
  void split( int depth )
  {
    error( "Must implement KDNode::split for each templated class T, your tree won't split." )
  }

  void generateDebugLines( Vector color ) const override
  {
    // The AABB defining this volume is,
    // put your debug lines on the map..
    bounds.generateDebugLines( color ) ;
  }

  // Also generates children's debug lines.
  void generateDebugLines( Vector offset, Vector color )
  {
    // USING THIS NODE'S AABB:
    offset.y += 2*bounds.extents().y ;
    // use the midpoint to determine x/z offset
    //offset.x += .1*bounds.mid().x ;
    //offset.z += .1*bounds.mid().z ;

    bounds.generateDebugLines( offset, color ) ;  // put your debug lines on the map..

    // and if you have children..
    if( behind )  behind->generateDebugLines( offset, color*1.1 ) ; // ..tell them to do the same, but lighter
    if( infront )  infront->generateDebugLines( offset, color*1.1 ) ;
  }

  // Check if me ONode or my children ONodes 
  // are intersected by the ray, if they are add to addList.
  void intersects( const Ray& ray, vector< ONode<T> * >& addList )
  {
    // as soon as you ask if you hit me, you did.
    // it's which of my children we're not sure if you hit.

    addList.push_back( this ) ;  // performance of this line is bad 

    // naive intersect code.
    // check if the start point of the ray is inside my space.
    ////if( bounds.intersects( ray ) )
    ////{
    ////  addList.push_back( this ) ;
    ////  // check the children.
    ////  if( behind )  behind->intersects( ray, addList ) ;
    ////  if( infront )  infront->intersects( ray, addList ) ;
    ////}
    ////return ;

    // ray-plane intn only reasonable if ray direction component not 0
    if( ray.direction.e[splitAxis] != 0 )
    {
      // see if you hit the splitting plane.
      real t = ( splitVal - ray.startPos.e[splitAxis] ) / ray.direction.e[splitAxis] ;
      if( BetweenIn( t, 0, ray.length ) )
      {
        // both my children are candidates
        if( behind )  behind->intersects( ray, addList ) ;
        if( infront )  infront->intersects( ray, addList ) ;
      }
    }
    else
    {
      // only one child is a candidate.
      // what side of my splitting plane
      // is the ray starting on?
      
      // The ray is parallel to the splitAxis.  You
      // hit the side where ray originates ONLY.
      if( ray.startPos.e[splitAxis] < -splitVal ) // you are BEHIND plane
        if( behind )  behind->intersects( ray, addList ) ; // traverse the back node
      else
        if( infront )  infront->intersects( ray, addList ) ;
    }


    #if 0
    if( bounds.containsIn( ray.startPos ) || bounds.containsIn( ray.getEndPoint() ) )
    {
      // you hit me.
      addList.push_back( this ) ;
      // check the children.
      if( behind )  behind->intersects( ray, addList ) ;
      if( infront )  infront->intersects( ray, addList ) ;
    }
    else
    {
      // check if the ray is going towards my boundary or away.
      //if( ray.direction.e[splitAxis] > 0 ) // CULL BACK FACE

      // You can only check my boundary.. IF I HAVE CHILDREN
      // If I don't have children, then I have no splitting plane set.
      ////if( hasChildren() )
      {
        // It's cheaper to just find t, because telling if
        // your ray is facing the right way to hit the plane
        // is more expensive WHEN you have to actually find t.
        real t = ( splitVal - ray.startPos.e[splitAxis] ) / ray.direction.e[splitAxis] ;
        if( BetweenIn( t, 0, ray.length ) )
        {
          Vector pt = ray.at( t ) ;

          // Check that the point is within bounds of the plane-aligned box,
          if( BetweenIn( pt.e[o1], bounds.min.e[o1], bounds.max.e[o1] ) && 
              BetweenIn( pt.e[o2], bounds.min.e[o2], bounds.max.e[o2] ) )
          {
            addList.push_back( this ) ;
            // check the children.
            if( behind )  behind->intersects( ray, addList ) ;
            if( infront )  infront->intersects( ray, addList ) ;
          }
        }
      }
    }
    #endif
  }

  // Check if me ONode or my children ONodes 
  // are intersected by the ray, if they are add to addList.
  void intersects( const Vector& pt, vector< ONode<T> * >& addList )
  {
    // an AABB is still the fastest way to check for inclusion.
    if( bounds.containsIn( pt ) )
    {
      // it hit me,
      addList.push_back( this ) ;

      // check my children in this case,
      // recursively calling back this function.
      // termination is when a node has no children
      if( behind ) behind->intersects( pt, addList ) ;
      if( infront ) infront->intersects( pt, addList ) ;
    }
  }
} ;

#pragma endregion

#pragma region concrete trees
template <typename T> class Octree : public CubicSpacePartition<T>
{
  OctreeNode<T> * root ;

public:
  static bool useKDDivisions ;

  Octree()
  {
    root = new OctreeNode<T>() ;
  }

  // this DOESN'T clear the memory occupied by octree itesm.
  // to do so, you must call deleteItems() explicitly
  ~Octree()
  {
    //int nItems = numItems() ;
    //if( nItems )  warning( "Deleting an octree, but it still has %d items in it that will remain in memory", nItems ) ;
    // The only way to kill the root is to destroy the tree,
    // then the member functions don't have to check if ROOT
    // exists, because root is created on octree creation.
    DESTROY( root ) ;
  }
  //void clear() override { DESTROY( root ) ; }

  // You throw the item into the root.
  void add( T item ) override {
    root->add( item ) ;
  }
  int numNodes() const override {
    return 1+root->numNodes() ; // me + children
  }
  int numItems() const override {
    return root->numItems() ;
  }

  // This really could be called SELECT
  void intersectsNodes( const Ray& ray, vector< ONode<T> * >& addList ) const override {
    root->intersects( ray, addList ) ;
  }
  void intersectsNodes( const Vector& pt, vector< ONode<T> * >& addList ) const override {
    root->intersects( pt, addList ) ;
  }
  void allNodes( vector< ONode<T> * >& addList ) override {
    root->allNodes( addList ) ;
  }
  void allItems( list<T>& addList ) const override {
    root->allItems( addList ) ;
  }
  void split() override {
    if( useKDDivisions ) // split as a kd-tree
      root->splitAsKdtree( 0, 0 ) ;
    else
      root->splitAsOctree( 0 ) ; // split as an octree
  }
  
  // This actually DELETES the items in the root.
  void deleteItems() override {
    root->deleteItems() ;
  }
  void generateDebugLines( Vector color ) const override {
    // start the root out
    root->generateDebugLines( 0, color ) ;
  }
} ;

// The "dimensionality" of the kdtree refers to
// DEPTH.
// Each ONode in the KDTree contains __2__ children,
// not 8, and that's the only difference between
// a KDTree and an octree.
template <typename T> class KDTree : public CubicSpacePartition<T>
{
  KDNode<T> *root ;

public:
  KDTree() {
    root = new KDNode<T>() ;
  }
  ~KDTree() {
    DESTROY( root ) ;
  }
  void add( T item ) override {
    root->add( item ) ;
  }
  int numNodes() const override {
    return root->numNodes() ;
  }
  int numItems() const override {
    return root->numItems() ;
  }

  // This really could be called SELECT
  void intersectsNodes( const Ray& ray, vector< ONode<T> * >& addList ) const override {
    root->intersects( ray, addList ) ;
  }
  void intersectsNodes( const Vector& pt, vector< ONode<T> * >& addList ) const override {
    root->intersects( pt, addList ) ;
  }
  void allNodes( vector< ONode<T> * >& addList ) override {
    root->allNodes( addList ) ;
  }
  void allItems( list<T>& addList ) const override {
    root->allItems( addList ) ;
  }
  void split() override {
    root->split( 0 ) ;
  }

  // This actually DELETES the items in the root. Used for octrees that construct "fictional"
  // overlay geometry such as Octree<PhantomTriangle*>
  // Clears the tree without destroying the items.
  void deleteItems() override
  {
    root->deleteItems() ;
  }
  void generateDebugLines( Vector color ) const override
  {
    root->generateDebugLines( color ) ;
  }

} ;

template <typename T> class BSPTree
{
  // split as a BSPTree
  ///void splitBSPTree() { }
  // bsp is totally different, because the volumes are not cubic
} ;

#pragma endregion


#pragma region octree template spec
// OctreeNode
// "override"/specialize the split methods
template <> void OctreeNode<PhantomTriangle*>::splitAsOctree( int depth )
{
  if( items.size() < maxItems || depth > maxDepth )
    return ; // no splitting to be done
  
  //info( "%d phantomtris > %d (maxitems), splitting", items.size(), maxItems ) ;
  //info( "Splitting a box with size %f %f %f", bounds.extents().x, bounds.extents().y, bounds.extents().z ) ;
  // needs to be split.
  vector<AABB> aabbCandChildren = bounds.split8() ;
  
  //   - go thru tris, see if they go in any of 8 boxes
  //     - if it does, move it
  //     - tris not moved
  //     - go thru list of tris (BIG) 1 time, 8 boxes many times.
  map< AABB*, list<PhantomTriangle*> > aabbsAndTheirContainedItems ;
      
  if( splitting )
  {
    // 1. try and put polys in children they completely fit in
    for( list<PhantomTriangle*>::iterator iter = items.begin() ; iter != items.end() ; )
    {
      // let's see if we're feeding degenerates
      if( (*iter)->isDegenerate() ) { 
        error( "degenerate triangle" ) ; }  // this CAN happen, usually if there's a major bug. Leave this check IN.

      // 2. if tri didn't fit in any of the 8 boxes, it must be split (leave it in the root node)
      if( tryPutAway( *iter, aabbCandChildren, aabbsAndTheirContainedItems ) )
      {
        //drawTri( *iter, Vector(0,0,.2) ) ; //dark blue, PLACED!
        iter = items.erase( iter ) ; // FIT, so ERASE from this list.. THIS ADVANCES ITER.
      }
      else
        ++iter ;
    }
    
    // `items` now contains only the objects that DIDN'T place
    // directly in the children.  We need to split every tri left in `items` now.
    
    // The 3 splitting planes align with the midpoint of this node's bounding box
    Vector midBox = bounds.mid() ;
    ///drawRay( Ray( 0, midBox ), .2 ) ;

    list<PhantomTriangle*> toAddBackToRoot ; // tris that must be added back to root. ideally: empty all the time,
    // but practically, sometimes there are one or two triangles in a split
    // that simply don't sit properly in the new children nodes.
    // (ie a triangle that sits DIRECTLY ALONG/PARALLEL TO one of the new __splitting__ planes.
    // this tri will sit in the root because of the error in putting it in
    // any of the new boxes.  Say the tri is along y=2, and the splitting plane
    // is at y=2.  Does it go into the top half or the bottom half?)

    ////info( "There are %d tris to split", items.size() ) ;
      
    // go thru remaining tris (that didn't fully fit in child boxes)
    for( list<PhantomTriangle*>::iterator iter = items.begin() ; iter != items.end() ; ++iter )
    {
      list<PhantomTriangle*> toSplit ;// collection of tris that need to be split this stage
      list<PhantomTriangle*> newTris ;// collection of new tris, after a split stage
      list<PhantomTriangle*> coincidentTris ;// bad tris coincident with split plane that must remain in root
      
      // we want to split just the current triangle
      toSplit.push_back( *iter ) ;

      // Now, the process of triangle splitting is:
      // 1 - Check if triangles are COINCIDENT with to any of
      //     the splitting planes.  If they are, those
      //     tris will remain in root.
      for( int axis = 0 ; axis < 3 ; axis++ )
      {
        Plane splittingPlane( axis, midBox.e[ axis ] ) ;
        removeCoincidentTris( splittingPlane, toSplit, coincidentTris ) ;
      }

      // This number should be low.
      //info( "%d tris were coincident with splitting planes, so _must_ remain in root", coincidentTris.size() ) ;
      // add the coincident tris back to the root.  these are the tris we cannot split with the slicing planes
      if( coincidentTris.size() )
        toAddBackToRoot.insert( toAddBackToRoot.end(), coincidentTris.begin(), coincidentTris.end() ) ;

      // if the tri was coincident with one of the splitting planes,
      // then there will be no splitting work to do.
      if( !toSplit.size() ) continue ; // skip to next tri.

      // 2 - Now the rest can be split entirely.
      for( int axis = 0 ; axis < 3 ; axis++ )
      {
        Plane splittingPlane( axis, midBox.e[ axis ] ) ;

        // split 'em
        splitTris( splittingPlane, toSplit, newTris ) ;
        ///drawTris( newTris, Vector(0,.5,0) ) ;

        // now the newTris need to join toSplit on the next axis iteration
        toSplit.insert( toSplit.end(), newTris.begin(), newTris.end() ) ;

        newTris.clear() ; // clear these out
      }
      //info( "There are now %d split", newTris.size() ) ;
      
      // 3 - After doing all the splitting,
      //     put the tris away
      if( tryPutAway( toSplit, aabbCandChildren, aabbsAndTheirContainedItems ) )
      {
        // good
        ///info( "All tris put away" ) ;
        ///newTris.clear() ;
      }
      else //if( newTris.size() )  // Ok, here there are still some tris left
      {
        // Some of them who I split FAIL to put away.
        // This is expected.  They go back in root.
        //warning( "Not all the tris got put away after splitting, %d tris remain in root", newTris.size() ) ;
        //drawTris( newTris, Vector(1,0,1) ) ;
        toAddBackToRoot.insert( toAddBackToRoot.end(), toSplit.begin(), toSplit.end() ) ;
      }
    }

    // You must ERASE all the item references in the root node,
    // the original larger triangles that failed to place
    // in an octree child node were SPLIT first, then destroyed in the loop above
    items.clear() ;

    // add back tris that were coincident.
    if( toAddBackToRoot.size() )
    {
      //warning( "%d error tris that didn't place in child octree nodes after splitting", toAddBackToRoot.size() ) ;
      // the triangles that COULD NOT be put away into child nodes
      // even after the split, are here:
      items.insert( items.end(), toAddBackToRoot.begin(), toAddBackToRoot.end() ) ;
      toAddBackToRoot.clear() ;
    }
  } // splitting
  else
  {
    #pragma region not splitting
    bool foundBox ;
    // go thru every tri
    for( list<PhantomTriangle*>::iterator iter = items.begin() ; iter != items.end() ; )
    {
      if( (*iter)->isDegenerate() ) { error( "degen," ) ; }

      foundBox = false ;
      // try and put each tri in a new sub box that contains it
      for( int i = 0 ; i < aabbCandChildren.size() ; i++ )
      {
        // Use containsIn because cut triangles will be VERY close to the borders.
        // Or you can use containsAlmost with VERY small epsilon.
        if( aabbCandChildren[i].containsIn( *iter ) )
        {
          // found a box for it
          foundBox = true ;
            
          // put it in the new box
          aabbsAndTheirContainedItems[ &aabbCandChildren[i] ].push_back( *iter ) ;
            
          // remove it from the root (ie it doesn't need to be split or remain in the root node)
          iter = items.erase( iter ) ; // remove from this list.. THIS ADVANCES ITER.
            
          break ; // end this loop
        }
      }
      if( !foundBox )
        ++iter ; // advance to next tri

      // when a box is not found the item is left in root (and result is correct)
    }
    #pragma endregion
  }
      
  // if the candChild AABB contains at least ONE
  // tri, then instantiate it as an ONode
  for( map< AABB*, list<PhantomTriangle*> >::iterator iter = aabbsAndTheirContainedItems.begin();
        iter != aabbsAndTheirContainedItems.end() ; ++iter )
  {
    if( iter->second.size() > 0 )
    {
      OctreeNode* chNode = new OctreeNode() ;
      chNode->bounds = *(iter->first) ;
      chNode->items = iter->second ;
      children.push_back( chNode ) ;
    }
  }

  // Now split my children
  //info( "There are %d children", children.size() ) ;
  for( int i = 0 ; i < children.size() ; i++ )
  {
    //info( "splitting children[%d]", i ) ;
    children[i]->splitAsOctree( depth+1 ) ;
  }
}

template <> void OctreeNode<PhantomTriangle*>::splitAsKdtree( int depth, int splitAxis )
{
  if( items.size() < maxItems || depth > maxDepth )
    return ; // no splitting to be done
  
  //info( "%d phantomtris > %d (maxitems), splitting", items.size(), maxItems ) ;
  //info( "Splitting a box with size %f %f %f", bounds.extents().x, bounds.extents().y, bounds.extents().z ) ;
  // needs to be split.

  // FOR NOW choose the split point to be the middle of the extents on the axis we're splitting.
  // TECH 1:
  //real splitVal = bounds.mid().e[ splitAxis ] ;

  // Find the average value of items in the axis i'm splitting
  //splitAxis = bounds.extents().maxIndex() ;
  //real splitVal = 0 ;
  //for( auto pt : items )
  //  splitVal += pt->a.e[splitAxis] + pt->b.e[splitAxis] + pt->c.e[splitAxis] ;
  //splitVal /= 3*items.size() ; // that's how many verts we have


  // Measure the deviation from the mean in the 3 axes
  Vector mean ;
  Vector stddev ;
  getMeanStdDev( items, mean, stddev ) ;
  
  splitAxis = stddev.maxIndex() ; // split on the axis with the largest std deviation

  // Use the mean on that axis
  real splitVal = mean.e[splitAxis] ;

  vector<AABB> aabbCandChildren = bounds.split2( splitAxis, splitVal ) ;
  
  //   - go thru tris, see if they go in any of 2 boxes
  //     - if it does, move it
  //     - tris not moved
  //     - go thru list of tris (BIG) 1 time, 2 boxes many times.
  map< AABB*, list<PhantomTriangle*> > aabbsAndTheirContainedItems ;
      
  if( splitting )
  {
    // 1. try and put polys in children they completely fit in
    for( list<PhantomTriangle*>::iterator iter = items.begin() ; iter != items.end() ; )
    {
      // let's see if we're feeding degenerates
      if( (*iter)->isDegenerate() ) { 
        error( "degenerate triangle" ) ; }  // this CAN happen, usually if there's a major bug. Leave this check IN.

      // 2. if tri didn't fit in any of the 2 boxes, it must be split (leave it in the root node)
      if( tryPutAway( *iter, aabbCandChildren, aabbsAndTheirContainedItems ) )
      {
        //drawTri( *iter, Vector(0,0,.2) ) ; //dark blue, PLACED!
        iter = items.erase( iter ) ; // FIT, so ERASE from this list.. THIS ADVANCES ITER.
      }
      else
        ++iter ;
    }
    
    // `items` now contains only the objects that DIDN'T place
    // directly in the children.  We need to split every tri left in `items` now.
    
    list<PhantomTriangle*> toAddBackToRoot ; // tris that must be added back to root. ideally: empty all the time,
    // but practically, sometimes there are one or two triangles in a split
    // that simply don't sit properly in the new children nodes.
    // (ie a triangle that sits DIRECTLY ALONG/PARALLEL TO one of the new __splitting__ planes.
    // this tri will sit in the root because of the error in putting it in
    // any of the new boxes.  Say the tri is along y=2, and the splitting plane
    // is at y=2.  Does it go into the top half or the bottom half?)

    ////info( "There are %d tris to split", items.size() ) ;
      
    // go thru remaining tris (that didn't fully fit in child boxes)
    for( list<PhantomTriangle*>::iterator iter = items.begin() ; iter != items.end() ; ++iter )
    {
      list<PhantomTriangle*> toSplit ;// collection of tris that need to be split this stage
      list<PhantomTriangle*> newTris ;// collection of new tris, after a split stage
      list<PhantomTriangle*> coincidentTris ; // bad tris coincident with split plane that must remain in root
      
      // we want to split just the current triangle
      toSplit.push_back( *iter ) ;

      // Now, the process of triangle splitting is:
      // 1 - Check if triangles are COINCIDENT with to
      //     the splitting plane.  If they are, those
      //     tris will remain in root.
      Plane splittingPlane( splitAxis, splitVal ) ;
      removeCoincidentTris( splittingPlane, toSplit, coincidentTris ) ;

      // This number should be low.
      //info( "%d tris were coincident with splitting planes, so _must_ remain in root", coincidentTris.size() ) ;
      // add the coincident tris back to the root.  these are the tris we cannot split with the slicing planes
      if( coincidentTris.size() )
        toAddBackToRoot.insert( toAddBackToRoot.end(), coincidentTris.begin(), coincidentTris.end() ) ;

      // if the tri was coincident with one of the splitting planes,
      // then there will be no splitting work to do.
      if( !toSplit.size() ) continue ; // skip to next tri.

      // 2 - Now the rest can be split entirely.
      splitTris( splittingPlane, toSplit, newTris ) ;
      ///drawTris( newTris, Vector(0,.5,0) ) ;

      // now the newTris need to join toSplit on the next iteration
      toSplit.insert( toSplit.end(), newTris.begin(), newTris.end() ) ;

      newTris.clear() ; // clear these out
      //info( "There are now %d split", newTris.size() ) ;
      
      // 3 - After doing all the splitting,
      //     put the tris away
      if( tryPutAway( toSplit, aabbCandChildren, aabbsAndTheirContainedItems ) )
      {
        // good
        ///info( "All tris put away" ) ;
        ///newTris.clear() ;
      }
      else //if( newTris.size() )  // Ok, here there are still some tris left
      {
        // this is expected.
        //warning( "Not all the tris got put away after splitting, %d tris remain in root", newTris.size() ) ;
        //drawTris( newTris, Vector(1,0,1) ) ;
        toAddBackToRoot.insert( toAddBackToRoot.end(), toSplit.begin(), toSplit.end() ) ;
      }
    }

    // You must ERASE all the item references in the root node,
    // the original larger triangles that failed to place
    // in an octree child node were SPLIT first, then destroyed in the loop above
    items.clear() ;

    // add back tris 
    if( toAddBackToRoot.size() )
    {
      //warning( "%d error tris that didn't place in child octree nodes after splitting", toAddBackToRoot.size() ) ;
      // the triangles that COULD NOT be put away into child nodes
      // even after the split, are here:
      items.insert( items.end(), toAddBackToRoot.begin(), toAddBackToRoot.end() ) ;
      toAddBackToRoot.clear() ;
    }
  } // splitting
  else
  {
    #pragma region not splitting
    bool foundBox ;
    // go thru every tri
    for( list<PhantomTriangle*>::iterator iter = items.begin() ; iter != items.end() ; )
    {
      if( (*iter)->isDegenerate() ) { error( "degen," ) ; }

      foundBox = false ;
      // try and put each tri in a new sub box that contains it
      for( int i = 0 ; i < aabbCandChildren.size() ; i++ )
      {
        // Use containsIn because cut triangles will be VERY close to the borders.
        // Or you can use containsAlmost with VERY small epsilon.
        if( aabbCandChildren[i].containsIn( *iter ) )
        {
          // found a box for it
          foundBox = true ;
            
          // put it in the new box
          aabbsAndTheirContainedItems[ &aabbCandChildren[i] ].push_back( *iter ) ;
          
          // remove it from the root (ie it doesn't need to be split or remain in the root node)
          iter = items.erase( iter ) ; // remove from this list.. THIS ADVANCES ITER.
            
          break ; // end this loop
        }
      }
      if( !foundBox )
        ++iter ; // advance to next tri

      // when a box is not found the item is left in root (and result is correct)
    }
    #pragma endregion
  }
      
  // if the candChild AABB contains at least ONE
  // tri, then instantiate it as an ONode
  for( map< AABB*, list<PhantomTriangle*> >::iterator iter = aabbsAndTheirContainedItems.begin();
        iter != aabbsAndTheirContainedItems.end() ; ++iter )
  {
    if( iter->second.size() > 0 )
    {
      OctreeNode* chNode = new OctreeNode() ;
      chNode->bounds = *(iter->first) ;
      chNode->items = iter->second ;
      children.push_back( chNode ) ;
    }
  }

  // Now split my children
  //info( "There are %d children", children.size() ) ;
  for( int i = 0 ; i < children.size() ; i++ )
  {
    //info( "splitting children[%d]", i ) ;
    children[i]->splitAsKdtree( depth+1, (splitAxis+1)%3 ) ;
  }
}

template <> void OctreeNode<PhantomTriangle*>::generateDebugLines( Vector offset, Vector color )
{
  // USING THIS NODE'S AABB:
  offset.y += 2*bounds.extents().y ;
  // use the midpoint to determine x/z offset
  //offset.x += .1*bounds.mid().x ;
  //offset.z += .1*bounds.mid().z ;
  for( list<PhantomTriangle*>::iterator iter = items.begin() ; 
        iter != items.end() ;
        ++iter )
    drawTri( offset, *iter, Vector(1,1,1) ) ; //window->addSolidDebugTri( offset, *iter, Vector(1,1,1) ) ;

  bounds.generateDebugLines( offset, color ) ;  // put your debug lines on the map..

  for( int i = 0 ; i < children.size() ; i++ )  // and if you have children..
    children[i]->generateDebugLines( offset, color*.95 ) ; // ..tell them to do the same, but darker
}
 
// "override"/specialize the CubicSpacePartition of PhantomTriangle destructor,
// to delete it's contents on destruction
template <> Octree<PhantomTriangle*>::~Octree()
{
  info( "Destroying an octree of phantom tris" ) ;
  deleteItems() ; // insert a call to delete items,
  DESTROY( root ); // after deleting all items in the octree, delete the onodes by calling the regular clear method
}
#pragma endregion

#pragma region kdnode template specialization
template <> void KDNode<Shape*>::split( int depth )
{
  if( items.size() < maxItems || depth > maxDepth )
  {
    // REFUSE TO SPLIT
    info( "KDNode.split<Shape*> recursion terminated with %d items in leaf", items.size() ) ;
    return ;
  }
  // FIND SPLIT AXIS AND SPLIT VALUE.
    
  // you're either finding it here or before recursion, so
  // best off finding it here (so you don't also have to find
  // the first value before calling this function)
    
  // while we're here, we'll also find the spread of the o1 and o2 axes
  // just using an AABB (which gets discarded after).

  // Find best split axis, based on axis with largest stddev.
  int nVerts = 0;
  Vector mean ;
    
  // SHAPE* class specialization
  for( list<Shape*>::iterator iter = items.begin() ; iter != items.end(); ++iter )
  {
    Shape * shape = *iter ;
    for( int i = 0 ; i < shape->meshGroup->meshes.size() ; i++ )
    {
      Mesh* mesh = shape->meshGroup->meshes[i] ;
      nVerts += mesh->verts.size() ;

      for( int j = 0 ; j < mesh->verts.size() ; j++ )
        mean += mesh->verts[j].pos ;
    }
  }

  // Get avgs in 3 axes
  mean /= nVerts ;

  // Find split val, based on the spread of ITEMS.
  Vector devs ;
  for( list<Shape*>::iterator iter = items.begin() ; iter != items.end(); ++iter )
  {
    Shape * shape = *iter ;
    for( int i = 0 ; i < shape->meshGroup->meshes.size() ; i++ )
    {
      Mesh* mesh = shape->meshGroup->meshes[i] ;

      for( int j = 0 ; j < mesh->verts.size() ; j++ )
      {
        Vector diff = (mesh->verts[j].pos - mean) ;
        devs += diff.componentSquared() ;
      }
    }
  }

  splitAxis = devs.maxIndex() ; // split on the axis with the largest std deviation
  o1 = ( splitAxis + 1 ) % 3 ;
  o2 = ( splitAxis + 2 ) % 3 ;

  // Use the mean on that axis
  splitVal = mean.e[splitAxis] ;

  Plane splitPlane( splitAxis, splitVal ) ;
    
  // specialized template will actually split items for phantomtris,
  // but here we don't split

  // Collections of items that belong infront of, or behind this plane.
  list<Shape*> itemsInfront, itemsBehind ;

  // the ::planeSide( Plane ) function must be supported
  // by the geometry
  for( list<Shape*>::iterator iter=items.begin() ; iter != items.end() ; )
  {
    int side = (*iter)->planeSide( splitPlane ) ;
    if( side == PlaneSide::Straddling )
    {
      // HERE THIS WOULD BE SPLIT, if the item were splittable.
      ++iter ;  // don't move the item, just move on to the next.
    }
    else
    {
      if( side == PlaneSide::InFront )
        itemsInfront.push_back( *iter ) ;
      else //PlaneSide::Behind:
        itemsBehind.push_back( *iter ) ;

      // TAke it out of root.
      iter = items.erase( iter ) ; // ADVANCES ITER
    }
  }

  // if the backside had items, make the node
  if( itemsBehind.size() )
    behind = new KDNode<Shape*>( itemsBehind ) ;
  if( itemsInfront.size() )
    infront = new KDNode<Shape*>( itemsInfront ) ;
    
  // any items that don't place end up in THIS node.
  // info( "%d items remained in root", items.size() ) ;

  // split the children
  if( behind )  behind->split( depth+1 ) ;
  if( infront )  infront->split( depth+1 ) ;
  
}

template <> void KDNode<PhantomTriangle*>::split( int depth )
{
  if( items.size() < maxItems || depth > maxDepth )
  {
    // REFUSE TO SPLIT
    //info( "KDNode.split<PhantomTriangle*> recursion terminated with %d items in leaf", items.size() ) ;
    return ;
  }
  // FIND SPLIT AXIS AND SPLIT VALUE.
    
  // you're either finding it here or before recursion, so
  // best off finding it here (so you don't also have to find
  // the first value before calling this function)
    
  // while we're here, we'll also find the spread of the o1 and o2 axes
  // just using an AABB (which gets discarded after).

  // Find best split axis, based on axis with largest stddev.
  Vector mean, stddev ;
  getMeanStdDev( items, mean, stddev ) ;

  splitAxis = stddev.maxIndex() ; // split on the axis with the largest std deviation
  o1 = ( splitAxis + 1 ) % 3 ;
  o2 = ( splitAxis + 2 ) % 3 ;

  // Use the mean on that axis
  splitVal = mean.e[splitAxis] ;

  Plane splitPlane( splitAxis, splitVal ) ;
    
  // specialized template will actually split items for phantomtris,
  // but here we don't split

  // Collections of items that belong infront of, or behind this plane.
  list<PhantomTriangle*> itemsInfront, itemsBehind ;

  // the ::planeSide( Plane ) function must be supported
  // by the geometry
  for( list<PhantomTriangle*>::iterator iter=items.begin() ; iter != items.end() ; )
  {
    int side = (*iter)->planeSide( splitPlane ) ;
    if( side == PlaneSide::Straddling )
    {
      // HERE THIS WOULD BE SPLIT, if the item were splittable.
      ++iter ;  // don't move the item, just move on to the next.
    }
    else
    {
      if( side == PlaneSide::InFront )
        itemsInfront.push_back( *iter ) ;
      else //PlaneSide::Behind:
        itemsBehind.push_back( *iter ) ;

      // TAke it out of root.
      iter = items.erase( iter ) ; // ADVANCES ITER
    }
  }

  // any items that don't place end up in THIS node.
  // info( "%d items remained in root", items.size() ) ;
  if( splitting )
  {
    // SPLIT ITEMS LEFT IN items,
    list<PhantomTriangle*> newTris ;
    splitTris( splitPlane, items, newTris ) ;

    // some will have remained in items,
    // others will now be solely in one of the child nodes.

    // try and place each newTri in one of the child nodes,
    // the ones that fail add back to root
    for( list<PhantomTriangle*>::iterator iter = newTris.begin() ; iter != newTris.end() ; )
    {
      int side = (*iter)->planeSide( splitPlane ) ;
      if( side == PlaneSide::InFront )
        itemsInfront.push_back( *iter ) ;
      else if( side == PlaneSide::Behind )
        itemsBehind.push_back( *iter ) ;
      else // put it back in root (splitTris will have taken it out / deleted the original)
      {
        // This represents a failure of splitting the triangle.. you split it,
        // but it still didn't fit in any of the child nodes
        //warning( "Split triangle not placed in child KDNode, it went back in root." ) ;
        items.push_back( *iter ) ; 
      }
    }
  }

  // if the backside had items, make the node
  if( itemsBehind.size() )
    behind = new KDNode<PhantomTriangle*>( itemsBehind ) ;
  if( itemsInfront.size() )
    infront = new KDNode<PhantomTriangle*>( itemsInfront ) ;

  // call split on the children,
  if( behind )  behind->split( depth+1 ) ;
  if( infront )  infront->split( depth+1 ) ;
}
#pragma endregion


#endif