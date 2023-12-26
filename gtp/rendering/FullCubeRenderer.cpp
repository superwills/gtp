#include "FullCubeRenderer.h"
#include "../geometry/MeshGroup.h"
#include "../geometry/Mesh.h"
#include "../geometry/Model.h"
#include "../window/GTPWindow.h"
#include "../util/StdWilUtil.h"
#include "Hemicube.h" // for HEMI_NEAR, HEMI_FAR
#include "../threading/ParallelizableBatch.h"
#include "Raycaster.h"
#include "../math/SHVector.h"

RayCollection* FullCubeRenderer::rc ;
real FullCubeRenderer::compressionLightmap = 0.02 ;
real FullCubeRenderer::compressionVisibilityMaps = 0.02 ;
bool FullCubeRenderer::useRailedQuincunx = false ;
IndexMap FullCubeRenderer::indexMap ;

FullCubeRenderer::FullCubeRenderer( Scene* iScene, int pxPerSide )
{
  compressionMode = CompressionMode::HaarWaveletCubemap ; // defaults to haar wavelet cubemap compression

  scene = iScene ;
  pixelsPerSide = pxPerSide ;
  for( int FACE = 0 ; FACE < 6 ; FACE++ )
    surfaces[ FACE ] = 0 ;
}

#if 0
void FullCubeRenderer::pregenFFCos()
{
  // precompute the form factor cosines
  formFactorCosines.resize( pixelsPerSide*pixelsPerSide ) ; // front

  // assuming sideLength of 2.0, area=2*2=4.0
  // each pixel's area is then 4/(px^2)
  real dA = 4.0 / (pixelsPerSide*pixelsPerSide) ;
  // so each pixel occupies an area of 4/(w*h)

  for( real row = 0 ; row < pixelsPerSide ; row++ )
  {
    for( real col = 0 ; col < pixelsPerSide ; col++ )
    {
      // D3D gives you the texture with the
      // top right corner first.  So, the transformation is
      // (pp 283-284 Radiosity Ashdown)
      real u = -1 + 2.0*(col+0.5) / pixelsPerSide ;  // horizontal
      real v = -1 + 2.0*(row+0.5) / pixelsPerSide ;  // VERTICAL ie rows

      real c = u*u+v*v+1 ; // square length of vector to pt (u,v,1)
      real dF = dA / (PI*c*c) ;
      formFactorCosines[ row*pixelsPerSide + col ] = dF ;
    }
  }
}
#endif

real FullCubeRenderer::ffCos( int pxPerSide, int row, int col )
{
  warning( "You shouldn't be using this function, use the RayCollection::cubemapPixelDirs collection instead" ) ;

  // assuming sideLength of 2.0, area=2*2=4.0
  // each pixel's area is then 4/(px^2)
  real dA = 4.0 / (pxPerSide*pxPerSide) ;
  // so each pixel occupies an area of 4/(w*h)

  // D3D gives you the texture with the
  // top right corner first.  So, the transformation is
  // (pp 283-284 Radiosity Ashdown)
  real u = -1 + 2.0*(col+0.5) / pxPerSide ;  // horizontal
  real v = -1 + 2.0*(row+0.5) / pxPerSide ;  // VERTICAL ie rows

  real c = u*u+v*v+1 ; // square length of vector to pt (u,v,1)
  real dF = dA / (PI*c*c) ;
  return dF ;
}

void FullCubeRenderer::initSurfaces()
{
  // i need cubes
  for( int FACE = 0 ; FACE < 6 ; FACE++ )
  {
    surfaces[ FACE ] = new D3D11Surface(
      window,
      0,0,
      pixelsPerSide,pixelsPerSide,
      0,1,
      0,
      SurfaceType::RenderTarget,
      0,DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,
      4 ) ;
  }
}


void FullCubeRenderer::renderSetup()
{
  window->setFillMode( FillMode::Solid ) ; // MUST be solid fill mode
  
  window->cullMode( CullMode::CullNone ) ; // turn off culling for full cube rendering.

  window->alphaBlend( false ) ; // maybe TRUE to enable partial pass of cubemap light
  window->shaderPlain->unsetTexture( 0 ) ; // DO NOT draw with a texture! (if there was bound previously)
  window->shaderPlain->activate() ;
  /////window->shadingMode = ShadingMode::HemicubeID ; // this would be eq to above

  ////window->shaderPhongLit->activate() ; //! DEBUG
}

// All this does is perform the rendering.
// Assumes you have init'd the surfaces and called renderSetup().
void FullCubeRenderer::renderAt( const Vector& eye )
{
  // YOU MUST HAVE ALREADY CALLED renderSetup()
  UINT clearColor = 0 ;
  
  // RENDER 6 FACES
  for( int FACE = 0 ; FACE < 6 ; FACE++ )
  {
    surfaces[ FACE ]->rtBind() ;
    surfaces[ FACE ]->clear( clearColor ) ;
    
    Matrix proj = Matrix::ProjPerspectiveRH( RADIANS( HEMI_FOV ), 1.0, HEMI_NEAR, HEMI_FAR ) ;
    
    // RENDER FROM MY VIEWPOINT.
    Matrix view = Matrix::Translate( -eye ) * 
      Matrix::ViewingFace( cubeFaceBaseRHRight[FACE], cubeFaceBaseRHUp[FACE], cubeFaceBaseRHFwd[FACE] )  ;

    window->setModelviewProjection( view*proj ) ;
    window->render( scene ) ;  // Don't call "window->rasterize()" because that picks a shader

    window->endDrawing() ;
    window->present() ;

    surfaces[ FACE ]->rtUnbind() ;
    // saving the faces
    //char b[255];
    //sprintf( b, "C:/vctemp/fullcube/vertex_%d_side_%d.png", k, FACE ) ;
    //surfaces[ FACE ]->save( b ) ;
  } // FACE
}


void FullCubeRenderer::renderCubesAtEachVertex( int iCompressionMode )
{
  window->timer.reset() ;

  compressionMode = iCompressionMode ;

  if( !surfaces[0] ) // check if init'd 
    initSurfaces() ;

  UINT clearColor = 0;//ByteColor( 0,0,0,0 ).intValue ;

  ULONGLONG cumulativeUncompressedComponents = 0, cumulativeCompressedComponents = 0 ;

  renderSetup() ;
  
  // boot up the processor
  FINISHED_SUBMITTING_JOBS = false ;

  // wavelet compression is fast compared to
  // the actual hemicube finding, 
  // so you don't need that many threads
  //////threadsCreated = 4 ;
  //////if( compressionMode == CompressionMode::SphericalHarmonics )
  threadsCreated = 7 ; // actually this should be the same as "threadPool.threads"
  // (always make full threads.)

  // This creates "threadsCreated" 
  for( int i = 0 ; i < threadsCreated ; i++ )
    threadPool.createJob( "compressor", new CallbackObject0< FullCubeRenderer*, void(FullCubeRenderer::*)() >
      ( this, &FullCubeRenderer::greedyCompressor ) ) ;

  for( int i = 0 ; i < scene->shapes.size() ; i++ )
  {
    Shape *shape = scene->shapes[ i ] ;
    for( int j = 0 ; j < shape->meshGroup->meshes.size() ; j++ )
    {
      printf( "fcr shape %d / %d, mesh %d / %d\n",
        i+1, scene->shapes.size(),
        j+1, shape->meshGroup->meshes.size() ) ;

      Mesh *mesh = shape->meshGroup->meshes[j];
      cumulativeUncompressedComponents += mesh->verts.size() * 6 * pixelsPerSide * pixelsPerSide ;  // the mesh has verts*px*px*6 elements to compress

      for( int k = 0 ; k < mesh->verts.size() ; k++ )
      {
        if( !k || every(k,10) )
          printf( "fcr vertex %d / %d       \r", k+1, mesh->verts.size() );

        AllVertex& vertex = mesh->verts[ k ] ;
        renderAt( vertex.pos + vertex.norm*1e-3 ) ;

        // After rendering all faces, extract the information
        vertex.cubeMap = new CubeMap( surfaces ) ;

        #if 0
        // Just testing viz of cubemap
        // this shows a mesh
        if( shape->name=="plane floor" && k == 12 ) // jsut do it for 1 vertex on the plane floor
        {
          ////vertex.cubeMap->createCubemapMesh() ;
          vertex.cubeMap->visualizeAO( vertex.norm ) ;
          vertex.cubeMap->viz->transform( Matrix::Translate( vertex.pos ) ) ;
          
          // The data at each vertex is destroyed after transformation,
          // so you need to clone this here.
          Model* vizShape = new Model( "viz k=12" ) ;
          vizShape->takeMeshesFrom( vertex.cubeMap->viz ) ;

          window->scene->addVisualization( vizShape ) ;
        }
        #endif
        // lock queue and submit this vertex for compression on the worker thread
        // the lock introduces a bit of slowdown, but not as much as
        // actually running all the processing.
        { MutexLock lock( scene->mutexFCR, INFINITE ) ;
          readyVerts.push_back( &vertex ) ; }

        // must compress HERE to save memory.
        //
        // to save the cubemaps you need to run the
        // processing on the main thread.
        //////compressVizInfo( &vertex ) ; // commented out
        // b/c normally use multithreading
      } // verts
    } // meshes
  } // each shape

  {
    MutexLock lock( scene->mutexFCR, INFINITE ) ;
    info( "SENDING DONE SIGNAL.." ) ;
    FINISHED_SUBMITTING_JOBS = true ; // here we're done submitting jobs.
  }
  
  // at the end here you have a rough avg of compression ratios
  /////ULONGLONG numLost = cumulativeUncompressedComponents - cumulativeCompressedComponents ;
  /////info( Cyan, "CubeMap average compression: (dropped) %.2f", 100.0*numLost/cumulativeUncompressedComponents ) ;
}

void FullCubeRenderer::compressVizInfo( AllVertex *vertex )
{
  // need to "bake-in" the form factors NOW,
  // so at light time, they don't have to be computed
  vertex->cubeMap->prelight( vertex->norm ) ;
      
  switch( compressionMode )
  {
    case CompressionMode::SphericalHarmonics:
      vertex->shDiffuseAmbient = vertex->cubeMap->shProject( 1 ) ; // project cubemap to SH.
      // also create the newSH now because we will interreflect in a moment
      vertex->shDiffuseInterreflected = new SHVector() ;
      vertex->shDiffuseSummed = new SHVector() ;
      break ;

    case CompressionMode::HaarWaveletCubemap:
      int lost, compressedComponents, uncompressedComponents ;

      //char b[255];
      if( useRailedQuincunx )
      {
        vertex->cubeMap->setSource( vertex->cubeMap->colorValues ) ;
        vertex->cubeMap->railedMake() ;
        
        // TRY SAVING IT, only works if called from main thread
        //sprintf( b, "C:/vctemp/fullcube/cubemap_for_vertex_%d_orig.png", k ) ;
        //vertex->cubeMap->railedSave( b ) ;
        compressedComponents = vertex->cubeMap->railedCompress( compressionVisibilityMaps, false ) ;
        uncompressedComponents = vertex->cubeMap->railSize() ;

        // TRY SAVING IT, only works if called from main thread
        //sprintf( b, "C:/vctemp/fullcube/cubemap_for_vertex_%d_wave.png", k ) ;
        //vertex->cubeMap->railedSaveWaveletRepn( b ) ;
        //sprintf( b, "C:/vctemp/fullcube/cubemap_for_vertex_%d_uncomp.png", k ) ;
        //vertex->cubeMap->railedSaveUncompressed( b ) ;
      }
      else
      {
        // TRY SAVING IT, only works if called from main thread
        ///sprintf( b, "C:/vctemp/fullcube/cubemap_for_vertex_%d_orig.png", k ) ;
        ///vertex->cubeMap->faceSave( b ) ;
        compressedComponents = vertex->cubeMap->faceCompressOriginal( compressionVisibilityMaps, false ) ; // must compress HERE to save memory.
        uncompressedComponents = 6*pixelsPerSide*pixelsPerSide ;// UNRAILED
      }
  
      lost = uncompressedComponents - compressedComponents ;
      
      // 100*lost/originally had = percent compressed (100% means lost all)
      real compPerc = 100.0 * lost / uncompressedComponents ; // lost and unc are INT
      //printf( "compression = %f percent %d components\n", compPerc, vertex->cubeMap->sparseTransformedColor.nonZeros() ) ;
      break ;
  }

  // DO NOT DELETE THE CUBEMAP ENTIRELY YOU NEED THE SPARSE DATA FOR LIGHTING
  // You must clean up the raw data here however or you will
  // very quickly run out of memory
  vertex->cubeMap->deleteEverythingExceptSparseData() ;
  
}

void FullCubeRenderer::greedyCompressor()
{
  bool exitting=false;
  while( !exitting )
  {
    vector<AllVertex*> myJobs ;
    
    // Try and get jobs
    {
      // mutex lock the queue and see if there's something ready
      MutexLock lock( scene->mutexFCR, INFINITE ) ;

      // if there are fewer than 100 jobs, and it's NOT the last batch, then wait, so you don't get a small job size
      if( readyVerts.size() < 100 && !FINISHED_SUBMITTING_JOBS ) continue ;

      if( threadsCreated == 1 || readyVerts.size() < 100 ) // 1 thread take em all, or less than 100 AND DONE ALL JOBS take 'em all
      {
        // otherwise, copy the references to process for ME
        myJobs = readyVerts;
        // then clear the main list
        readyVerts.clear() ;
      }
      else
      {
        // Take up to 100 jobs max.  case less than 100 handled above
        myJobs.insert( myJobs.end(), readyVerts.begin(), readyVerts.begin() + 100 ) ; // there are at least 100 ready

        // remove 100 jobs from readyVerts
        readyVerts.erase( readyVerts.begin(), readyVerts.begin()+100 ) ;
      }

      if( FINISHED_SUBMITTING_JOBS && readyVerts.size()==0 ) // are all jobs released, and no more will come (ie all picked up by other threads?)
        exitting = true ; // exit after processing this group, because we just took the last of them
    }
    
    // process myJobs
    for( int i = 0 ; i < myJobs.size() ; i++ )
    {
      //printf( "job %d / %d    \r", i, myJobs.size() ) ;
      compressVizInfo( myJobs[i] ) ;
    }
  }


  
  
  info( Cyan, "A FCR compressor thread is exitting.." ) ;
  {
    MutexLock lock( scene->mutexFCR, INFINITE ) ;
    --threadsCreated;
    info( Cyan, "%d threads remain..", threadsCreated ) ;
    if( threadsCreated == 0 )
    {
      // LAST THREAD IS DONE AND EXITTING
      info( Red, "Last thread!!" ) ;

      // You don't need to know which pass.  greedyCompressor
      // will only ever be used to process stuff as the gpu puts it out,
      // not for anything else.
      // if we were on pass1, then we need to launch pass2
      // (which is always ray casted)
      if( compressionMode == CompressionMode::SphericalHarmonics )
      {
        if( !window->doInterreflect )
        {
          // just stop
          window->shDidFinishRunning = true ;
        }
        else
        {
          // do a second pass
          ParallelBatch *pb = new ParallelBatch( "sh interreflection", new Callback0( [](){
            // when it's done you would set this flag
            info( "Done pass #2 sh, pass #1 was rasterized, pass 2 was raycasted" ) ;
            window->scene->shAddInterreflections() ;
            window->shDidFinishRunning = true ;
          } ) ) ;
          
          window->raycaster->QSHPrecomputation_Stage2_Interreflection_Only( pb, scene, 120 ) ;

          threadPool.addBatch( pb ) ;
        }
      }
      else
      {
        if( !window->doInterreflect )
        {
          window->waveletDidFinishRunning = true ;
          window->programState = ProgramState::Idle ; // done now.
          info( Red, "WAVELET::Pass 1 wavelet lighting took %f seconds", window->timer.getTime() ) ;
          info( Red, "WAVELET::Average compression: kept %f %f %f percent", 
            100.0*CubeMap::compressCumulativeKept.x/CubeMap::compressCumulativeProcessed.x,
            100.0*CubeMap::compressCumulativeKept.y/CubeMap::compressCumulativeProcessed.y,
            100.0*CubeMap::compressCumulativeKept.z/CubeMap::compressCumulativeProcessed.z ) ;

          int totalVerts = window->scene->countVertices() ;
          info( Red, "There are %f %f %f components on average at each vertex",
            CubeMap::compressCumulativeKept.x/totalVerts,
            CubeMap::compressCumulativeKept.y/totalVerts,
            CubeMap::compressCumulativeKept.z/totalVerts ) ;
        }
        else
        {
          // now kick off the 2nd pass using rays
          ParallelBatch *pb = new ParallelBatch( "wavelet interreflection", new Callback0( [](){
            
            info( Red, "WAVELET::INTERREFLECTED: wavelet lighting + interreflection took %f seconds", window->timer.getTime() ) ;
            info( Red, "WAVELET::INTERREFLECTED: Kept %f %f %f percent",
              100.0*CubeMap::compressCumulativeKept.x/CubeMap::compressCumulativeProcessed.x,
              100.0*CubeMap::compressCumulativeKept.y/CubeMap::compressCumulativeProcessed.y,
              100.0*CubeMap::compressCumulativeKept.z/CubeMap::compressCumulativeProcessed.z ) ;
            int totalVerts = window->scene->countVertices() ;
            info( Red, "There are %f %f %f components on average at each vertex",
              CubeMap::compressCumulativeKept.x/totalVerts,
              CubeMap::compressCumulativeKept.y/totalVerts,
              CubeMap::compressCumulativeKept.z/totalVerts ) ;

            window->scene->waveletAddInterreflections() ;
            window->waveletDidFinishRunning = true ;
            window->programState = ProgramState::Idle ;
          } ) ) ;
          
          window->raycaster->QWavelet_Stage2_Interreflection_Only( pb, scene, 120 ) ;

          threadPool.addBatch( pb ) ;
        }
      }
      ///////else // This is done PASS2
      ///////{
      ///////  if( compressionMode == CompressionMode::SphericalHarmonics )
      ///////    window->shDidFinishRunning = true ;  // now shLight in the main loop can get called
      ///////  else
      ///////    window->waveletDidFinishRunning = true ;
      ///////}
    }
  }

  
}






