#include "Mesh.h"
#include "MeshGroup.h"
#include "../rendering/VectorOccludersData.h"
#include "../window/GTPWindow.h"
#include "../math/SHVector.h"

MeshGroup::MeshGroup( Shape* ownerShape, MeshType meshType, VertexType vertexType, int iMaxTrisPerMesh )
{
  shape = ownerShape ;
  maxTrisPerMesh = iMaxTrisPerMesh ; // default maximum tris / mesh.
  // only enforced when you add to the mesh THRU
  // the MeshGroup's AddTri interface.

  defaultMeshType = meshType ;
  defaultVertexType = vertexType ;
}

MeshGroup::MeshGroup( FILE * file, Shape* iShape ) // instantiate from a file
{
  shape = iShape ;
  HeaderMeshGroup head ;
  fread( &head, sizeof(HeaderMeshGroup), 1, file ) ;
  
  meshes.resize( head.numMeshes ) ;
  for( int i = 0 ; i < meshes.size() ; i++ )
    meshes[i] = new Mesh( file, shape ) ;
}

MeshGroup* MeshGroup::clone( Shape* ownerToBe ) const
{
  puts( "MeshGroup::clone start" ) ;
  MeshType mt = MeshType::Nonindexed;
  VertexType vt = defaultVertexType ;
  if( meshes.size() ) {
    mt = meshes[0]->meshType ;
    vt = meshes[0]->vertexType ;
  }
  
  // Explicit request to clone a mesh.
  MeshGroup* mg = new MeshGroup( ownerToBe, mt, vt, maxTrisPerMesh ) ;
  for( int i = 0 ; i < meshes.size() ; i++ )
  {
    printf( "Cloning mesh %d/%zd", i+1, meshes.size() ) ;
    mg->meshes.push_back( new Mesh( meshes[i], ownerToBe ) ) ;
  }

  puts( "MeshGroup::clone done" ) ;
  return mg ;
}

int MeshGroup::save( FILE * file )
{
  int written = 0 ;
  
  // write the MeshGroup header
  HeaderMeshGroup head ;
  head.defMeshType = defaultMeshType ;
  head.defVertexType = defaultVertexType ;
  head.numMeshes = meshes.size() ;

  written += sizeof(HeaderMeshGroup)*fwrite( &head, sizeof(HeaderMeshGroup), 1, file ) ;
  for( int i = 0 ; i < meshes.size() ; i++ )
    written += meshes[i]->save( file ) ;

  return written ;
}

MeshGroup::~MeshGroup()
{
  // delete all the meshes, not the Shape*
  for( int i = 0 ; i < meshes.size() ; i++ )
    DESTROY( meshes[i] ) ;
}

void MeshGroup::each( function<void (Mesh*)> func )
{
  for( int i = 0 ; i < meshes.size() ; i++ )
    func( meshes[i] ) ;
}

void MeshGroup::transform( const Matrix& m, const Matrix& nT )
{
  for( int i = 0 ; i < meshes.size() ; i++ )
  {
    meshes[i]->transform( m, nT ) ;

    // if there was a vb, update it.
    // otherwise, person may want to defer instantiation
    if( meshes[i]->vb ) 
      meshes[i]->updateVertexBuffer() ;
  }
}

void MeshGroup::transformSetWorld( const Matrix& m )
{
  for( int i = 0 ; i < meshes.size() ; i++ )
    meshes[i]->world = m ; // seen in gpu only
}

void MeshGroup::updateMesh()
{
  for( int i = 0 ; i < meshes.size() ; i++ )
    meshes[i]->updateVertexBuffer() ;
}

bool MeshGroup::intersects( const Ray& ray )
{
  // don't care about location of intn
  for( int i = 0 ; i < meshes.size() ; i++ )
    if( meshes[i]->intersects( ray, NULL ) )
      return true ;
  return false ;
}

bool MeshGroup::intersects( const Ray& ray, MeshIntersection* mintn )
{
  // intersection information not desired ("shadow/feeler ray")
  if( !mintn )
    return intersects( ray ) ;

  MeshIntersection ni, ci=MeshIntersection::HugeMeshIntn ;
  
  for( int i = 0 ; i < meshes.size() ; i++ )
    if( meshes[i]->intersects( ray, &ni ) ) 
      if( ni.isCloserThan( &ci, ray.startPos ) )
          ci = ni ;  // new one closer than "closest"

  if( mintn ) *mintn = ci ;
  return ci.didHit() ;
}

bool MeshGroup::intersectsFurthest( const Ray& ray, MeshIntersection* mintn )
{
  MeshIntersection ni, fi=MeshIntersection::SmallMeshIntn ;
  
  for( int i = 0 ; i < meshes.size() ; i++ )
    if( meshes[i]->intersectsFurthest( ray, &ni ) ) 
      if( !ni.isCloserThan( &fi, ray.startPos ) )
          fi = ni ;  // new one NOT closer than "closest", so use it

  if( mintn ) *mintn = fi ;
  return fi.didHit() ;
}

int MeshGroup::planeSide( const Plane& plane )
{
  // Get the side of the first mesh,
  int side = plane.iSide( meshes[0]->planeSide( plane ) ) ;

  // Compare it with sides of other meshes,
  for( int i = 1 ; i < meshes.size() ; i++ )
    if( side != meshes[i]->planeSide( plane ) )
      return PlaneSide::Straddling ; // if the side differs, you straddle.

  return side ;
}

bool MeshGroup::isEmpty() const { return meshes.empty() ; }

Vector MeshGroup::getCentroid()
{
  Vector c ;
  
  int i ;
  
  // prebaked centroids in the mesh, those get updated every ::transform
  for( i = 0 ; i < meshes.size() ; i++ )
    c += meshes[i]->centroid ;

  c /= (real)i ;
  
  return c ;
}

void MeshGroup::smoothOperator( real AllVertex::* memberName )
{
  vector<real> smoothed( getTotalVertices(), 0.0 ) ;  // the smoothed quantity
  vector<int> numNeighbourCounts( getTotalVertices(), 0 ) ; // count of numneighbours for each vertex.
  
  int vID = 0 ;
  for( int meshNo = 0 ; meshNo < meshes.size() ; meshNo++ )
  {
    printf( "Smooth mesh %d / %zd    \r", meshNo, meshes.size() ) ;
    // Different threads can take different verts to smooth.
    for( int vNo = 0 ; vNo < meshes[meshNo]->verts.size() ; vNo++ )
    {
      AllVertex *vertex = &meshes[meshNo]->verts[ vNo ] ;
      
      // we're at a vertex.  take the value of the vertex as the
      // initial value and increment neighbours
      smoothed[ vID ] = (vertex->*memberName) ;
      numNeighbourCounts[ vID ]++ ;
      
      for( int m = 0 ; m < meshes.size() ; m++ )
      {
        for( int j = 0 ; j < meshes[m]->verts.size() ; j++ )
        {
          AllVertex* other = &meshes[m]->verts[j];
          if( vertex != other && // skip yourself
              vertex->pos.Near( other->pos ) ) // we are near each other
          {
            // then we are neighbours
            smoothed[ vID ] += (other->*memberName) ;
            numNeighbourCounts[ vID ]++ ;
          }
        }
      }
      
      vID++;
    }
  }

  // Now write them.
  vID = 0 ;
  for( int meshNo = 0 ; meshNo < meshes.size() ; meshNo++ )
  {
    for( int vNo = 0 ; vNo < meshes[meshNo]->verts.size() ; vNo++ )
    {
      (meshes[meshNo]->verts[vNo].*memberName) =
        smoothed[ vID ]/numNeighbourCounts[ vID ] ;

      vID++;
    }
  }
}

void MeshGroup::smoothOperator( Vector (AllVertex::* memberName)[10], int index )
{
  vector<Vector> smoothed( getTotalVertices(), 0.0 ) ;
  vector<int> numNeighbourCounts( getTotalVertices(), 0 ) ; // count of numneighbours for each vertex.
  
  int vID = 0 ;
  for( int meshNo = 0 ; meshNo < meshes.size() ; meshNo++ )
  {
    printf( "Smooth mesh %d / %zd    \r", meshNo, meshes.size() ) ;
    for( int vNo = 0 ; vNo < meshes[meshNo]->verts.size() ; vNo++ )
    {
      AllVertex *vertex = &meshes[meshNo]->verts[ vNo ] ;
      
      // we're at a vertex.  take the value of the vertex as the
      // initial value and increment neighbours
      smoothed[ vID ] = (vertex->*memberName)[index] ;
      numNeighbourCounts[ vID ]++ ;
      
      for( int m = 0 ; m < meshes.size() ; m++ )
      {
        for( int j = 0 ; j < meshes[m]->verts.size() ; j++ )
        {
          AllVertex* other = &meshes[m]->verts[j];
          if( vertex != other && // skip yourself
              vertex->pos.Near( other->pos ) ) // we are near each other
          {
            // then we are neighbours
            smoothed[ vID ] += (other->*memberName)[index] ;
            numNeighbourCounts[ vID ]++ ;
          }
        }
      }
      
      vID++;
    }
  }

  // Now write them.
  vID = 0 ;
  for( int meshNo = 0 ; meshNo < meshes.size() ; meshNo++ )
  {
    for( int vNo = 0 ; vNo < meshes[meshNo]->verts.size() ; vNo++ )
    {
      (meshes[meshNo]->verts[vNo].*memberName)[index] =
        smoothed[ vID ]/numNeighbourCounts[ vID ] ;

      vID++;
    }
  }
}

void MeshGroup::smoothOperator( Vector (AllVertex::* memberName)[10], int index, int componentIndex )
{
  vector<real> smoothed( getTotalVertices(), 0.0 ) ;
  vector<int> numNeighbourCounts( getTotalVertices(), 0 ) ; // count of numneighbours for each vertex.
  
  int vID = 0 ;
  for( int meshNo = 0 ; meshNo < meshes.size() ; meshNo++ )
  {
    printf( "Smooth mesh %d / %zd    \r", meshNo, meshes.size() ) ;
    for( int vNo = 0 ; vNo < meshes[meshNo]->verts.size() ; vNo++ )
    {
      AllVertex *vertex = &meshes[meshNo]->verts[ vNo ] ;
      
      // we're at a vertex.  take the value of the vertex as the
      // initial value and increment neighbours
      smoothed[ vID ] = (vertex->*memberName)[index].e[componentIndex] ;
      numNeighbourCounts[ vID ]++ ;
      
      for( int m = 0 ; m < meshes.size() ; m++ )
      {
        for( int j = 0 ; j < meshes[m]->verts.size() ; j++ )
        {
          AllVertex* other = &meshes[m]->verts[j];
          if( vertex != other && // skip yourself
              vertex->pos.Near( other->pos ) ) // we are near each other
          {
            // then we are neighbours
            smoothed[ vID ] += (other->*memberName)[index].e[componentIndex] ;
            numNeighbourCounts[ vID ]++ ;
          }
        }
      }
      
      vID++;
    }
  }

  // Now write them.
  vID = 0 ;
  for( int meshNo = 0 ; meshNo < meshes.size() ; meshNo++ )
  {
    for( int vNo = 0 ; vNo < meshes[meshNo]->verts.size() ; vNo++ )
    {
      (meshes[meshNo]->verts[vNo].*memberName)[index].e[componentIndex] =
        smoothed[ vID ]/numNeighbourCounts[ vID ] ;

      vID++;
    }
  }
}

void MeshGroup::smoothVectorOccludersData()
{
  info( "Averaging vector occluder data shape %s", shape->name.c_str() ) ;
  // must do smoothing in copy structures
  // otherwise you change teh data as you are averaging it
  vector<VectorOccludersData> newFrd( getTotalVertices() ) ;
  vector<int> numNeighbours( getTotalVertices(), 0 ) ;

  int vID = 0 ;
  for( int meshNo = 0 ; meshNo < meshes.size() ; meshNo++ )
  {
    for( int vNo = 0 ; vNo < meshes[meshNo]->verts.size() ; vNo++ )
    {
      AllVertex* vertex = &meshes[meshNo]->verts[vNo];
      VectorOccludersData* voData = vertex->voData;
      
      // write the initial values from voData (current voData we are smoothing).
      newFrd[vID].reflectors = voData->reflectors ; ///copy the whole map

      numNeighbours[vID]++; //myself.

      for( int m = 0 ; m < meshes.size() ; m++ )
      {
        for( int j = 0 ; j < meshes[m]->verts.size() ; j++ )
        {
          AllVertex* other = &meshes[m]->verts[j] ;
          if( vertex != other &&
              vertex->pos.Near( other->pos ) )
          {
            // Found a near vertex.
            numNeighbours[vID]++;

            // take a look at the neighbour's reflectors
            for( map< Mesh*,VOVectors >::iterator neigh = other->voData->reflectors.begin() ;
                 neigh != other->voData->reflectors.end() ; ++neigh )
            {
              // Do I have this emitter shape?
              map< Mesh*,VOVectors >::iterator inAvg = newFrd[vID].reflectors.find( neigh->first ) ;
              
              // it's possible another vertex found a shape we didn't see in our ray cast
              // (for lwo res ray casts).  average it in.
              if( inAvg == newFrd[vID].reflectors.end() )  // shape in neighbour NOT FOUND in avg.
              {
                // Happens a lot for small # verts
                ///info( "%s not in avg vertex %d", neigh->first->name.c_str(), vID ) ;
                newFrd[vID].reflectors.insert( make_pair( neigh->first, neigh->second ) ) ; // copy the first one in there
              }
              else
              {
                // sum it in.
                inAvg->second.Add( neigh->second ) ;
                inAvg->second.normSpecular.w++ ;  // counts # pos/normals added in.
              }
            }
          }
        }
      }

      // Divide out the final values.
      for( map< Mesh*,VOVectors >::iterator iter = newFrd[vID].reflectors.begin() ;
           iter != newFrd[vID].reflectors.end() ; ++iter )
      {
        // the divider is in normSpecular.
        iter->second.Divide( iter->second.normSpecular.w ) ;
        iter->second.normSpecular.w = 1 ;
      }
      vID++ ;
    }
  }

  // WRITE
  vID=0;
  for( int meshNo = 0 ; meshNo < meshes.size() ; meshNo++ )
  {
    for( int vNo = 0 ; vNo < meshes[meshNo]->verts.size() ; vNo++ )
    {
      *meshes[meshNo]->verts[vNo].voData = newFrd[vID++] ;
    }
  }
}

int MeshGroup::getTotalVertices()
{
  int total = 0 ;
  for( int meshNo = 0 ; meshNo < meshes.size() ; meshNo++ )
    total += meshes[meshNo]->verts.size() ;
  return total ;
}

void MeshGroup::calculateNormalsSmoothedAcrossMeshes()
{
  info( "Smoothing %s", shape->name.c_str() ) ;
  for( int meshNo = 0 ; meshNo < meshes.size() ; meshNo++ )
  {
    // actually need to smooth ACROSS MESHES
    for( int vNo = 0 ; vNo < meshes[meshNo]->verts.size() ; vNo++ )
    {
      if( every( vNo, 1000 ) )
        printf( "mesh %d / %zd, vertex %d / %zd     \r",
          meshNo+1, meshes.size(), vNo, meshes[meshNo]->verts.size() ) ;

      Vector avgNormal ;

      // check all tris in EACH mesh
      // find all TRIS that use this vertex,
      // and average their normals
      
      // check WHICH TRIANGLES USE the ith vertex of meshes[meshNo]
      // use THEIR normals to calculate the average normal.
      
      // every mesh,
      for( int m = 0 ; m < meshes.size() ; m++ )
        // check every tri in that mesh,
        for( int j = 0 ; j < meshes[m]->tris.size() ; j++ )
          // if jth triangle uses the ith vertex,
          if( meshes[m]->tris[j].usesVertex( meshes[meshNo]->verts[vNo].pos ) )
            // add in the normal of that triangle
            avgNormal += meshes[m]->tris[j].normal ;

      meshes[meshNo]->verts[vNo].norm = avgNormal.normalize() ;
    }
  }
}

void MeshGroup::addMesh( Mesh* iMesh )
{
  // make sure the mesh's shape owner is set correctly
  if( !iMesh->shape )
    iMesh->shape = shape ;
  meshes.push_back( iMesh ) ;
}

void MeshGroup::setColor( ColorIndex colorIndex, const Vector& color )
{
  for( int i=0 ; i < meshes.size() ; i++ )
    meshes[i]->setColor( colorIndex, color ) ;
}

void MeshGroup::setMaterial( const Material& iMaterial )
{
  for( int i=0 ; i < meshes.size() ; i++ )
    meshes[i]->setMaterial( iMaterial ) ;
}

void MeshGroup::setOwner( Shape *newOwner )
{
  for( int i=0 ; i < meshes.size() ; i++ )
    meshes[i]->shape = newOwner ;
}

void MeshGroup::addTri( const Vector& A, const Vector& B, const Vector& C )
{
  if( meshes.empty() || // empty or
      meshes.back()->tris.size() > maxTrisPerMesh ) // back mesh too big
  {
    Mesh *mesh = new Mesh( shape, defaultMeshType, defaultVertexType ) ;
    meshes.push_back( mesh ) ;
  }

  meshes.back()->addTri( A, B, C ) ;
}

void MeshGroup::addTri( const AllVertex& A, const AllVertex& B, const AllVertex& C )
{
  if( meshes.empty() || // empty or
      meshes.back()->tris.size() > maxTrisPerMesh ) // back mesh too big
  {
    Mesh *mesh = new Mesh( shape, defaultMeshType, defaultVertexType ) ; //!! defaults for now.
    meshes.push_back( mesh ) ;
  }

  meshes.back()->addTri( A, B, C ) ;
}

void MeshGroup::tessellate( real maxArea )
{
  for( int i = 0 ; i < meshes.size() ; i++ )
    meshes[i]->tessellate( maxArea ) ;
}

Vector MeshGroup::getRandomPointFacing( const Vector& dir )
{
  // an acceptable mesh has 2 coordinates:  a mesh# and a triangle # on the mesh
  vector< pair<int,int> > acceptableTris_meshNo_triNo ;

  for( int i = 0 ; i < meshes.size() ; i++ )
  {
    Mesh *mesh = meshes[ i ] ;

    for( int j = 0 ; j < mesh->tris.size() ; j++ )
    {
      Triangle& tri = mesh->tris[j];
      
      if( tri.normal % dir < 0 )
      {
        // this face is acceptable
        acceptableTris_meshNo_triNo.push_back( make_pair( i, j ) ) ;
      }
    }
  }

  // choose a face at random
  int face = randInt( 0, acceptableTris_meshNo_triNo.size() ) ;
  pair<int,int> facepair = acceptableTris_meshNo_triNo[ face ] ;
  Triangle& tri = meshes[ facepair.first ]->tris[ facepair.second ] ;

  // get a random point on the triangle
  return tri.getRandomPointOn() ;
}

