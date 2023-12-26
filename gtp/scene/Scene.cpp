#include "Scene.h"
#include "../window/GTPWindow.h"
#include "../geometry/AABB.h"

#include "../geometry/Cube.h"
#include "../geometry/Tetrahedron.h"
#include "../geometry/Mesh.h"
#include "../geometry/MathematicalShape.h"
#include "../geometry/Model.h"
#include "../math/SHVector.h"
#include "../math/perlin.h"

#include "Octree.h"
#include "../Globals.h"

Scene::Scene()
{
  mediaEta = 1 ;
  bgColor = 0; //Vector(1,1,1,1) ; // If you use a WHITE bg color and monte trace with 1 bounce,
  // you get an AO render
  
  useFarLights = false ;

  mutexShapeList = CreateMutexA( 0, 0, "Shape list lock" ) ;
  mutexVizList = CreateMutexA( 0, 0, "Viz list lock" ) ;
  mutexScenery = CreateMutexA( 0, 0, "scenery lock" ) ;
  mutexDebugLines = CreateMutexA( 0, 0, "mutex-mutexDebugLines" ) ;
  mutexFCR = CreateMutexA( 0, 0, "mutex-mutexFCR" ) ;

  spExact=0,spMesh=0,spAll=0;
  lastVertexCount = 0 ;
  spacePartitioningOn = 1 ; // assume it's on.
  perlinSkyOn = 1 ;
  spRotated = 0 ;
  shCombinedViz = 0 ;
  activeCubemap = 0 ;
  //activeLightIndex = 0 ;
}

Scene::~Scene()
{
  clearEntireScene() ;
  DESTROY( spRotated ) ;
  DESTROY( shCombinedViz ) ;
}

void Scene::addShape( Shape * s )
{
  if( !s->makeReadyForScene() )
  {
    error( "%s doesn't have a mesh, nothin' addin'", s->name.c_str() ) ;
    return ;
  }
  
  // MUST BE FAR LIGHT FOR VO TO WORK.
  // IF IT'S A SHAPE THEN IT WILL OCCLUDE!
  if( useFarLights )
  {
    // If you're using far lights, and you added an EMISSIVE shape,
    // then I'll assume it shouldn't be part of the scene (classic OpenGL light).
    // If you want emissive surfaces to be part of the scene,
    // then DO NOT useFarLights.
    // FAR LIGHTS ARE FOR VO and SH and WAVELET,
    // because you don't want the light to interact with the scene
    // in those cases.
    if( s->isEmissive() )
    {
      lights.push_back( s ) ;
      farLights.push_back( s ) ;
      return ;
    }
  }

  // Otherwise, this is the default shape-adding code.
  shapes.push_back( s ) ;

  // if it emits, then save it redundantly
  // in the lights array.  Now it is going to be sh projected and
  // used as an SH light source as well.
  if( s->isEmissive() )
    lights.push_back( s ) ;

  if( s->isTransmitting() )
    translucentShapes.push_back( s ) ;
  else
    opaqueShapes.push_back( s ) ;
}

void Scene::addCubemap( CubeMap* cubemap )
{
  window->scene->cubeMaps.push_back( cubemap ) ;
  window->scene->activeCubemap = window->scene->cubeMaps.size()-1 ;
  // let the last one be the active one
}

void Scene::addVisualization( Shape *s )
{
  // on adding a shape to the scene make sure its bounding box is calc
  if( !s->makeReadyForScene() )
  {
    error( "Viz no meshes, not added" ) ;
    return ;
  }
  viz.push_back( s ) ;
}

int Scene::countFaces()
{
  lastFaceCount=0;
  for( int i= 0 ; i < shapes.size() ; i++ )
    for( int j = 0 ; j < shapes[i]->meshGroup->meshes.size() ; j++ )
      lastFaceCount += shapes[i]->meshGroup->meshes[j]->tris.size() ;

  return lastFaceCount ;
}

int Scene::countVertices()
{
  lastVertexCount=0;
  for( int i= 0 ; i < shapes.size() ; i++ )
    for( int j = 0 ; j < shapes[i]->meshGroup->meshes.size() ; j++ )
      lastVertexCount += shapes[i]->meshGroup->meshes[j]->verts.size() ;

  return lastVertexCount ;
}

int Scene::countMeshes()
{
  lastMeshCount=0;
  for( int i= 0 ; i < shapes.size() ; i++ )
    lastMeshCount += shapes[i]->meshGroup->meshes.size() ;
  return lastMeshCount ;
}

void Scene::computeSpacePartition()
{
  if( !spacePartitioningOn )
  {
    error( "You turned space partitioning off but still called this, not computing space part" ) ;
    return ;
  }

  window->timer.reset() ;
  info( "Computing space partition.." ) ;
  
  DESTROY( spExact ) ;
  DESTROY( spMesh ) ;
  DESTROY( spAll ) ;

  if( window->spacePartitionType == PartitionOctree )
  {
    info( Magenta, "Octree" ) ;
    spExact = new Octree<Shape*>() ;
    spMesh = new Octree<PhantomTriangle*>() ;
    spAll = new Octree<PhantomTriangle*>() ;
  }
  else
  {
    info( Magenta, "KD Tree" ) ;
    spExact = new KDTree<Shape*>() ;
    spMesh = new KDTree<PhantomTriangle*>() ;
    spAll = new KDTree<PhantomTriangle*>() ;
  }
  

  for( int i = 0 ; i < shapes.size() ; i++ )
    if( shapes[i]->hasMath )
      spExact->add( shapes[i] ) ;
  
  // add NON-EXACT (mesh-only) tris to spMesh
  for( int i = 0 ; i < shapes.size() ; i++ )
    if( !shapes[i]->hasMath )
      for( int j = 0 ; j < shapes[i]->meshGroup->meshes.size() ; j++ )
        for( int k = 0 ; k < shapes[i]->meshGroup->meshes[ j ]->tris.size() ; k++ )
        {
          PhantomTriangle* pt = new PhantomTriangle( shapes[i]->meshGroup->meshes[j]->tris[ k ] ) ;
          spMesh->add( pt ) ;
          // let's see the initial set
          //window->addDebugTriLock( pt, Vector( 0,0,1 ) ) ;
        }
  
  // ADD ALL TRIS TO THE ROOT.
  for( int i = 0 ; i < shapes.size() ; i++ )
    for( int j = 0 ; j < shapes[i]->meshGroup->meshes.size() ; j++ )
      for( int k = 0 ; k < shapes[i]->meshGroup->meshes[ j ]->tris.size() ; k++ )
        spAll->add( new PhantomTriangle( shapes[i]->meshGroup->meshes[j]->tris[ k ] ) ) ;
      
  // Recursively splits the octree now
  // to acceptable divisions.
  spExact->split() ;
  spMesh->split() ;
  spAll->split() ;

  info( Magenta, "SpacePartition %d exact shapes, %d nodes", spExact->numItems(), spExact->numNodes() ) ;
  info( Magenta, "SpacePartition %d mesh only shapes, %d nodes", spMesh->numItems(), spMesh->numNodes() ) ;
  info( Magenta, "SpacePartition %d tris, %d nodes", spAll->numItems(), spAll->numNodes() ) ;

  info( "SpacePartition computation complete, %f ms", 1000*window->timer.getTime() ) ;
  
  // GEN DEBUG LINES
  //////spExact->root->generateDebugLines( Vector(0,1,0) ) ;
  //////spMesh->root->generateDebugLines( Vector(1,1,0) ) ;
  //////spAll->root->generateDebugLines( Vector(1,0,1) ) ;
  //////for( int i = 0 ; i < shapes.size() ; i++ )
  //////  shapes[i]->aabb->generateDebugLines( Vector(0,0,1) ) ;
  
  // INTEGRITY CHECK
  ////list<Triangle*> allTris ;
  ////spAll->root->all( allTris ) ;
  ////for( list<Triangle*>::iterator i= allTris.begin() ; i != allTris.end() ; ++i )
  ////{
  ////  Mesh* meshOwnerForThisTri = (*i)->meshOwner ;
  ////  Mesh* mesh = (*i)->meshOwner->shape->meshGroup->meshes[0] ;
  ////
  ////  if( &meshOwnerForThisTri->verts != &mesh->verts )
  ////    error( "mesh owner verts not the same address as meshOwner->shape->meshGroup->meshes[0]->verts but it should be" ) ;
  ////}
}

void Scene::clearEntireScene()
{
  for( UINT i = 0 ; i < shapes.size() ; i++ )
    DESTROY( shapes[i] );
  shapes.clear() ;

  DESTROY( spExact ) ;
  DESTROY( spMesh ) ;
  DESTROY( spAll ) ;
}

void Scene::createVertexBuffers()
{
  {
    MutexLock LOCK( mutexShapeList, INFINITE ) ;
    for( int i = 0 ; i < shapes.size() ; i++ )
      for( int j = 0 ; j < shapes[i]->meshGroup->meshes.size() ; j++ )
        shapes[i]->meshGroup->meshes[j]->createVertexBuffer() ;
  }

  {
    MutexLock LOCK( mutexVizList, INFINITE ) ;
    for( int i = 0 ; i < viz.size() ; i++ )
      for( int j = 0 ; j < viz[i]->meshGroup->meshes.size() ; j++ )
        viz[i]->meshGroup->meshes[j]->createVertexBuffer() ;

    for( int i = 0 ; i < scenery.size() ; i++ )
      for( int j = 0 ; j < scenery[i]->meshGroup->meshes.size() ; j++ )
        scenery[i]->meshGroup->meshes[j]->createVertexBuffer() ;
  }
}

void Scene::shAddInterreflections()
{
  // ADD INTERREFLECTIONS TOGETHER.
  for( int i = 0 ; i < shapes.size() ; i++ )
  {
    // Add the interreflection stage SH functions to the 1st stage functions
    for( int j=0; j < shapes[i]->meshGroup->meshes.size() ; j++ )
    {
      Mesh *mesh = shapes[i]->meshGroup->meshes[j] ;
      
      for( int v = 0 ; v < mesh->verts.size() ; v++ )
      {
        AllVertex& vertex = mesh->verts[ v ] ;

        //if( !vertex.shDiffuseAmbient ) { error ("No ambient shproj" ) ; continue ; }
        //if( !vertex.shDiffuseInterreflected ) { error( "No interreflected shproj" ) ; continue ; }
        
        // SHOW ONLY THE SHADOWED, DIRECT LIGHT
        //vertex.shDiffuseSummed->add( vertex.shDiffuseAmbient ) ;
        // SHOW ONLY THE INTERREFLECTIONS
        //vertex.shDiffuseSummed->add( vertex.shDiffuseInterreflected ) ;

        // add the ambient occlusion and interreflection sh projs
        vertex.shDiffuseSummed->add( vertex.shDiffuseAmbient )->add( vertex.shDiffuseInterreflected ) ;
        
        // CLEAN UP, don't need these anymore
        DESTROY( vertex.shDiffuseAmbient ) ;
        DESTROY( vertex.shDiffuseInterreflected ) ;

        // Just overwrite the specular with the interreflected part.
        vertex.shSpecularMatrix->add( vertex.shSpecularMatrixInterreflected ) ;

        // CLEAN UP, don't need these anymore
        DESTROY( vertex.shSpecularMatrixInterreflected ) ;
        
      }
    }
  }
}

void Scene::waveletAddInterreflections()
{
  // ADD INTERREFLECTIONS TOGETHER.
  for( int i = 0 ; i < shapes.size() ; i++ )
  {
    for( int j=0; j < shapes[i]->meshGroup->meshes.size() ; j++ )
    {
      Mesh *mesh = shapes[i]->meshGroup->meshes[j] ;
      
      for( int v = 0 ; v < mesh->verts.size() ; v++ )
      {
        AllVertex& vertex = mesh->verts[ v ] ;

        // ADD the interreflected sum proj (shDiffuseInterreflected) to current vertex proj
        vertex.cubeMap->sparseTransformedColor += vertex.cubeMap->newSparseTransformedColor ;

        // at this point you can destroy/clear newSparse,
        // to avoid double-add bugs, and to save some mems
        vertex.cubeMap->newSparseTransformedColor.resize(0);
      }
    }
  }
}

void Scene::smoothNormals()
{
  for( int i = 0 ; i < shapes.size() ; i++ )
  {
    info( "Smoothing %s", shapes[i]->name.c_str() ) ;

    for( int j = 0 ; j < shapes[i]->meshGroup->meshes.size() ; j++ )
    {
      printf( "mesh %d / %d\n", j+1, shapes[i]->meshGroup->meshes.size() ) ;

      // actually need to smooth ACROSS MESHES, but NOT across shapes.
      // if a vertex does NOT belong to the same meshGroup, (ie same shape)
      // then it really shouldn't be averaged in with another vertex,
      // no matter how close in space they are.
      for( int k = 0 ; k < shapes[i]->meshGroup->meshes[j]->verts.size() ; k++ )
      {
        AllVertex* vertex = &shapes[i]->meshGroup->meshes[j]->verts[k];

        if( every( k, 1000 ) )
          printf( "  vertex %d / %d     \r", k+1, shapes[i]->meshGroup->meshes[j]->verts.size() ) ;

        // retrieve all vertices close to
        //spMesh->intersectsNodes( 
      }
    }
  }
}

SHVector* Scene::sumAllLights()
{
  // Sum all the lights.
  SHVector* summedLight = new SHVector() ;

  for( int i = 0 ; i < lights.size() ; i++ )
  {
    if( !lights[i]->isActive )  continue ; //skip
    if( !lights[i]->shProjection )
    {
      warning( "No SH projection for %s, projecting now", lights[i]->name.c_str() ) ;
      lights[i]->shProject( lights[i]->material.ke ) ;
    }
    summedLight->add( lights[i]->shProjection ) ;
  }
  for( int i = 0 ; i < cubeMaps.size() ; i++ )
  {
    if( !cubeMaps[i]->isActive )  continue ; //skip
    if( !cubeMaps[i]->shProjection )
    {
      warning( "No SH projection for %s, projecting now", cubeMaps[i]->name.c_str() ) ;
      cubeMaps[i]->shProject( 1 ) ;
    }
    summedLight->add( cubeMaps[i]->shProjection ) ;
  }
  
  // Make the viz on the first pass
  if( !shCombinedViz ){
    info( "Creating sh light viz.." ) ;
    shCombinedViz = summedLight->generateVisualization( 64, 64, 0 ) ;
  }
  
  return summedLight ;
}

void Scene::shRotateAllLights( const Matrix& matrixRotation )
{
  DESTROY( spRotated ) ; // destroy the old one before making the new one.
  
  SHVector* summedLight = sumAllLights() ;
  
  // rotate the summed light
  spRotated = summedLight->rotate( matrixRotation ) ;
  DESTROY( summedLight ) ; // after i rotate it i only need the rotated result
}

// active light
//void Scene::shRotateLight( const Matrix& matrixRotation )
//{
//  if( !lights[activeLightIndex]->shProjection )
//  {
//    warning( "No SH projection for %s, projecting now", lights[activeLightIndex]->name.c_str() ) ;
//    lights[activeLightIndex]->shProject() ;
//  }
//
//  // rotate the light
//  DESTROY( spRotated ) ; // destroy the old one before making the new one.
//  spRotated = lights[activeLightIndex]->shProjection->rotate( matrixRotation ) ;
//}

void Scene::shLightCPU( const Matrix& matrixRotation )
{
  // CPU LIGHTING:::THIS CAN BE PARALLELIZED
  for( int i = 0 ; i < shapes.size() ; i++ )
    shapes[i]->shLight( spRotated ) ;
}

void Scene::shLightGPU( const Matrix& matrixRotation )
{
  // GPU LIGHTING:
  // only update the shcoefficients for lighting

  // do a quick check to see if the size of the cbuffer is too small
  if( sizeof( GPUCDATA4_SHCoeffs ) / (4*sizeof(float)) <  //#vectors i can store
      spRotated->coeffsScale.size() ) // #vectors i need to store
  {
    error( "cbuffer too small, allocate more room for SH lighting coeffs" ) ;
    return ;
  }
  for( int i = 0 ; i < spRotated->coeffsScale.size() ; i++ )
    for( int comp = 0 ; comp < 3 ; comp++ )
      window->gpuCData4.coeffs[ i ][ comp ] = (float)spRotated->coeffsScale[i].e[comp] ;

  // save in the shOption, it gets squared because there are n^2 coeffs when using n sh bands.
  window->gpuCData4.shOptions[0] = window->shSamps->bandsToRender * window->shSamps->bandsToRender ;
  window->updateGPUCData( 4 ) ;  
}

void Scene::waveletLight()
{
  // Try and cast the active light as a cubemap.  If that fails,
  // then you can't light the scene using that current cubemap.
  CubeMap* cubeMap = getActiveCubemap() ;
  if( !cubeMap )
  {
    error( "I can't wavelet light without a cubemap" ) ; 
    return ;
  }
  
  for( int i = 0 ; i < window->scene->shapes.size() ; i++ )
  {
    Shape *shape = shapes[ i ] ;
    for( int j = 0 ; j < shape->meshGroup->meshes.size() ; j++ )
    {
      Mesh *mesh = shape->meshGroup->meshes[j];

      for( int k = 0 ; k < mesh->verts.size() ; k++ )
      {
        Vector totalLight ;
        AllVertex& vertex = mesh->verts[ k ] ;

        /// The next line doesn't work due to an Eigen bug
        ///Vector totalLight = vertex.cubeMap->sparseTransformedColor.cwiseProduct( scene->cubeMap->sparseTransformedColor ).sum() ;  // EIGENBUG: This doesn't work.  You must save it in a temporary, as done below

        ////if( !vertex.cubeMap->sparseTransformedColor.nonZeros() )
        ////  error( "vertex cubemap empty" ) ;
        ////if( !cubeMap->sparseTransformedColor.nonZeros() )
        ////  error( "cubemap empty" ) ;

        // This is a bottleneck.
        Eigen::SparseVector<Vector> v = vertex.cubeMap->sparseTransformedColor.cwiseProduct( cubeMap->sparseTransformedColor );
        totalLight = v.sum() ;
        //totalLight = vertex.cubeMap->sparseM.dot( scene->cubeMap->sparseM ) ;
        
        // my ability to reflect limited by my diffuse material, and assign the color to the wavelet color
        vertex.color[ ColorIndex::WaveletComputed ] = totalLight * vertex.color[ ColorIndex::DiffuseMaterial ] ;
      }//each vertex
    }//each mesh
  }//each shape
}

/// Generates 'num' tetrahedra
/// with sizes between sizeMin and sizeMax,
/// spatially within the AABB formed
/// by min, max
void Scene::generateRandomTetrahedra( int num, real sizeMin, real sizeMax, Vector min, Vector max )
{
  for( int i = 0 ; i < num ; i++ )
    shapes.push_back(
      new Tet(
        "Tet " + i, 
        Vector::randomWithin( min, max ), // random centre
        randFloat( sizeMin, sizeMax ), 
        Material() )
    ) ;
}

void Scene::generateRandomCubes( int num, real sizeMin, real sizeMax, Vector min, Vector max )
{
  for( int i = 0 ; i < num ; i++ )
    /// random center
    shapes.push_back( new Cube( 
      "cube " + i,
      Vector::randomWithin( min, max ),
      randFloat( sizeMin, sizeMax ),
      10,10,
      randFloat( 0, 2*PI ), randFloat( 0, 2*PI ), randFloat( 0, 2*PI ),
      Material() )
    ) ;
}

bool Scene::getClosestIntn( const Ray& ray, vector<Shape*>& collection, Intersection **closestIntersection )
{
  Intersection ni, ci=Intersection::HugeIntn;
  MeshIntersection mni, mci=MeshIntersection::HugeMeshIntn ;

  for( int i = 0 ; i < collection.size() ; i++ ) // every shape
  {
    if( collection[i]->hasMath )
    {
      if( collection[i]->intersectExact( ray, &ni ) )
        if( ni.isCloserThan( &ci, ray.startPos ) )  // ni always is closer than ci on first hit
          ci = ni ;
    }
    else // no exact intersection, use the mesh intn
    {
      if( collection[i]->intersectMesh( ray, &mni ) )
        if( mni.isCloserThan( &mci, ray.startPos ) )
          mci = mni ;
    }
  }

  if( !mci.didHit() && !ci.didHit() )  return false ;  // total miss

  // here, definitely a hit.  use the closer one.
  if( !closestIntersection )  return true ; // you don't want any more information

  if( mci.isCloserThan( &ci, ray.startPos ) ) {
    *closestIntersection = new MeshIntersection( mci ) ;
  }
  else {
    *closestIntersection = new Intersection( ci ) ;
  }
  
  return true ;
}

bool Scene::getClosestIntn( const Ray& ray, Intersection **closestIntersection ) const
{
  Intersection ni, ci=Intersection::HugeIntn;
  MeshIntersection mni, mci=MeshIntersection::HugeMeshIntn ;

  if( !spacePartitioningOn ) // don't use the octree
  {
    for( int i = 0 ; i < shapes.size() ; i++ ) // every shape
    {
      if( shapes[i]->hasMath )
      {
        if( shapes[i]->intersectExact( ray, &ni ) )
          if( ni.isCloserThan( &ci, ray.startPos ) )  // ni always is closer than ci on first hit
            ci = ni ;
      }
      else // no exact intersection, use the mesh intn
      {
        if( shapes[i]->intersectMesh( ray, &mni ) )
          if( mni.isCloserThan( &mci, ray.startPos ) )
            mci = mni ;
      }
    }
  }
  else
  {
    ////
    // There IS an octree
    // First look for exact intersections
    vector< ONode<Shape*>* > exactNodes ;
    spExact->intersectsNodes( ray, exactNodes ) ;
    for( auto node : exactNodes )
      for( auto shape : node->items )
        if( shape->intersectExact( ray, &ni ) )
          if( ni.isCloserThan( &ci, ray.startPos ) )  // ni always is closer than ci on first hit
            ci = ni ;
  
    // THEN try the meshonlyintersectable tree
    vector< ONode<PhantomTriangle*> * > triNodes ;
    spMesh->intersectsNodes( ray, triNodes ) ;
    for( auto node : triNodes )
      for( auto pt : node->items )
        if( pt->tri->intersects( ray, &mni ) )
          if( mni.isCloserThan( &mci, ray.startPos ) )
            mci = mni ;
  }

  if( !mci.didHit() && !ci.didHit() )  return false ;  // total miss

  // here, definitely a hit.  use the closer one.
  if( !closestIntersection )  return true ; // you don't want any more information

  if( mci.isCloserThan( &ci, ray.startPos ) ) {
    *closestIntersection = new MeshIntersection( mci ) ;
  }
  else {
    *closestIntersection = new Intersection( ci ) ;
  }
  
  return true ;
}

bool Scene::getClosestIntnExact( const Ray& ray, Intersection *closestIntersection ) const
{
  Intersection ni, ci=Intersection::HugeIntn;
  MeshIntersection mni, mci=MeshIntersection::HugeMeshIntn ;

  if( !spacePartitioningOn ) // don't use the octree
  {
    for( int i = 0 ; i < shapes.size() ; i++ ) // every shape
    {
      if( shapes[i]->hasMath )
      {
        if( shapes[i]->intersectExact( ray, &ni ) )
          if( ni.isCloserThan( &ci, ray.startPos ) )  // ni always is closer than ci on first hit
            ci = ni ;
      }
      else // no exact intersection, use the mesh intn
      {
        if( shapes[i]->intersectMesh( ray, &mni ) )
          if( mni.isCloserThan( &mci, ray.startPos ) )
            mci = mni ;
      }
    }
  }
  else
  {
    ////
    // There IS an octree
    // First look for exact intersections
    vector< ONode<Shape*>* > exactNodes ;
    spExact->intersectsNodes( ray, exactNodes ) ;
    for( auto node : exactNodes )
      for( auto shape : node->items )
        if( shape->intersectExact( ray, &ni ) )
          if( ni.isCloserThan( &ci, ray.startPos ) )  // ni always is closer than ci on first hit
            ci = ni ;
  
    // THEN try the meshonlyintersectable tree
    vector< ONode<PhantomTriangle*> * > triNodes ;
    spMesh->intersectsNodes( ray, triNodes ) ;
    for( auto node : triNodes )
      for( auto pt : node->items )
        if( pt->tri->intersects( ray, &mni ) )
          if( mni.isCloserThan( &mci, ray.startPos ) )
            mci = mni ;
  }

  // here, compare mesh/exact intns.
  // if there was no mesh/exact shapes,
  // mci and ci will be HUGE
  // Now compare the two (different types of intersections)
  if( mci.isCloserThan( &ci, ray.startPos ) )  ci = mci ;  // we'll use mci

  if( closestIntersection )  *closestIntersection = ci ;
  return ci.didHit() ;
}

bool Scene::getClosestIntnMesh( const Ray& ray, MeshIntersection *closestIntersection ) const
{
  // note if he doesn't want the closestintn information,
  // routine is the same because we need ci info
  // to find the closest intersection!

  MeshIntersection ni ;
  
  // put the "closest intersection" extremely far away,
  // so the first test against it will always return TRUE.
  // (first ni will always be closer than extremely far ci).
  MeshIntersection ci = MeshIntersection::HugeMeshIntn ;

  if( !spacePartitioningOn ) // don't use the octree
  {
    for( int i = 0 ; i < shapes.size() ; i++ ) // YOU COULD USE AABB'S HERE.
      if( shapes[i]->intersectMesh( ray, &ni ) )  // never use intersectsExact
        if( ni.isCloserThan( &ci, ray.startPos ) )
          ci = ni ;
  }
  else
  {
    // don't try the exact tree, just use the all tree
    vector< ONode<PhantomTriangle*> * > triNodes ;
    
    spAll->intersectsNodes( ray, triNodes ) ;
    for( auto node : triNodes )
      for( auto pt : node->items )
        if( pt->tri->intersects( ray, &ni ) )
          if( ni.isCloserThan( &ci, ray.startPos ) )
            ci = ni ;
  }
  // copy it
  if( closestIntersection )
    *closestIntersection = ci ;
  
  return ci.didHit() ;
}

void Scene::getAllTrisUsingVertex( const Vector& v, set<Triangle*> tris )
{
  // first uses the octree to retrieve you references to all tris that
  // use a specific vertex
  if( spAll )
  {
    vector< ONode<PhantomTriangle*>* > nodes ;
    spAll->intersectsNodes( v, nodes ) ;

    // you may get back several phantom triangles that are pieces of
    // the same real triangle.  Resolve the ambiguity here by adding them
    // to a set
    set<Triangle*> allTris ;
  
    // get the parent tris.  this is to avoid redundantly
    // checking the same parent tri multiple times in the loop
    // below
    for( auto node : nodes )
      // check the 3 vertices of each tri matched.
      for( auto pt : node->items )
        allTris.insert( pt->tri ) ;

    // now iterate the set and look for the AllVertex* that 
    // have the same position as v
    for( auto tri : tris )
      // only 1 of these can be true
      if( v.Near( tri->a ) || v.Near( tri->b ) || v.Near( tri->c ) )
        tris.insert( tri ) ; //insert into final collection
  }
  else
  {
    error( "No octree, can't getAllTrisUsingVertex" ) ;
  }
}

int Scene::getNumTris() const
{
  int count = 0 ;
  for( int i = 0 ; i < shapes.size() ; i++ )
    for( int j = 0 ; j < shapes[i]->meshGroup->meshes.size() ; j++ )
      for( int k = 0 ; k < shapes[i]->meshGroup->meshes[j]->tris.size() ; k++ )
        count++;
  return count ;
}

void Scene::eachShape( const function<void (Shape*shape)>& func )
{
  for( int i = 0 ; i < shapes.size() ; i++ )
    func( shapes[i] ) ;
}

void Scene::eachTri( function<void (Triangle*tri)> func )
{
  for( int i = 0 ; i < shapes.size() ; i++ )
    for( int j = 0 ; j < shapes[i]->meshGroup->meshes.size() ; j++ )
      for( int k = 0 ; k < shapes[i]->meshGroup->meshes[j]->tris.size() ; k++ )
        func( &shapes[i]->meshGroup->meshes[j]->tris[k] ) ;
}

void Scene::eachVert( function<void (AllVertex*vert)> func )
{
  for( int i = 0 ; i < shapes.size() ; i++ )
    for( int j = 0 ; j < shapes[i]->meshGroup->meshes.size() ; j++ )
      for( int k = 0 ; k < shapes[i]->meshGroup->meshes[j]->verts.size() ; k++ )
        func( &shapes[i]->meshGroup->meshes[j]->verts[k] ) ;
}


// TOO MODAL
//Shape* Scene::getActiveLight()
//{
//  if( !lights.size() )
//  {
//    WARN_ONCE( "No scene lights" ) ;
//    return 0 ;
//  }
//
//  return lights[ activeLightIndex ] ;
//}

CubeMap* Scene::getActiveCubemap()
{
  if( !cubeMaps.size() )
  {
    //warning( "No cubemaps loaded" ) ;
    return 0 ;
  }
  if( activeCubemap < 0 )
  {
    // The cubemap is loaded, but just disabled
    return 0 ;
  }
  return cubeMaps[ activeCubemap ] ;
}