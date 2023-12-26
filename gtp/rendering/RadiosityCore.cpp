#include "RadiosityCore.h"
#include "../window/GTPWindow.h"
#include "../util/StopWatch.h"
#include "Hemicube.h"
#include "../Globals.h"
#include "../geometry/Mesh.h"
#include "../threading/ParallelizableBatch.h"
#include "Raycaster.h"

int RadiosityCore::currentId = -1 ;

char* ColorChannelIndexName[] = {
  "Red",
  "Green",
  "Blue"
} ;

char* SolutionMethodName[] =
{
  "LUDecomposition",
  "AInverse",

  /// Iterative techniques below
  "Jacobi",
  "GaussSeidel",
  "Southwell"
} ;

RadiosityCore::RadiosityCore( D3D11Window* iWin, int iHemicubePixelsPerSide )
{
  win = iWin ;
  hemicubePixelsPerSide = iHemicubePixelsPerSide ;

  NUMPATCHES = 0 ; // no patches registered yet bro
  FFsSizeInBytes = 0 ; // no size yet
  // set defaults
  formFactorDetType = FormFactorDetermination::FormFactorsHemicube ;
  matrixSolverMethod = MatrixSolverMethod::GaussSeidel ;

  iterativeTolerance = ITERATIVETOLERANCE ;
  vectorOccludersTex = 0 ;
}

int RadiosityCore::getNextId()
{
  // XX id=0 is not used (as well it should not be, so black doesn't count as a patch color.)
  // id=0 IS used because row=0 is the first patch row in Eigen.
  // to avoid OBOE's it's easier to use patch 0 = color 0
  currentId++ ;
  
  if( currentId == 0xffffff ) // this is the max # colors we have.
    error( "SOL: Too many patches. Result will be inaccurate." ) ;
  
  return currentId | ALPHA_FULL ; // turn on full alpha-bits ;)
  // The reason the alpha bits need to be on is
  // these patches can disappear if
  // alpha is off.
}

Shape* RadiosityCore::getShapeOwner( int id )
{
  return tris[ id ]->meshOwner->shape ;
}

void RadiosityCore::assignIds( Scene * scene )
{
  tris.clear() ;
  // iterate over triangles in the scene
  for( int i = 0 ; i < scene->shapes.size() ; i++ )
  {
    Shape *shape = scene->shapes[i];
    for( int j = 0 ; j < shape->meshGroup->meshes.size() ; j++ )
    {
      Mesh *mesh = shape->meshGroup->meshes[j] ;
      for( int k = 0 ; k < mesh->tris.size() ; k++ )
      {
        Triangle* tri = &mesh->tris[k];
        tri->setId( getNextId() ) ;
        tris.push_back( tri ) ;

        // assign a crazy radiosity color
        tri->vA()->color[ ColorIndex::RadiosityComp ] = Vector::random() ;
        tri->vB()->color[ ColorIndex::RadiosityComp ] = Vector::random() ;
        tri->vC()->color[ ColorIndex::RadiosityComp ] = Vector::random() ;
      }

      // you have to update the vertex buffer after touching the colors
      mesh->updateVertexBuffer() ;
    }
  }

  NUMPATCHES = tris.size() ;
  //info( "%d tris have been assigned ids", NUMPATCHES ) ;
}

void RadiosityCore::scheduleRadiosity( Scene* scene, FormFactorDetermination iFormFactorDetType, MatrixSolverMethod iMatrixSolverMethod,
    int numShots, real angleIncr )
{
  for( int i = 0 ; i < numShots ; i++ )
  {
    threadPool.createJobForMainThread( "rad", new Callback0( [iMatrixSolverMethod](){
      window->radCore->radiosity( window->scene, FormFactorDetermination::FormFactorsHemicube, iMatrixSolverMethod ) ;
    } ) ) ;

    threadPool.createJobForMainThread( "radcap", new Callback0( [angleIncr](){
      char n[30] ;
      sprintf(n,"radiosity %.0f", window->rot );
      window->screenshot( n ) ;
      window->rot += angleIncr ;
    } ) ) ;
  }
}

void RadiosityCore::radiosity( Scene* scene, FormFactorDetermination iFormFactorDetType, MatrixSolverMethod iMatrixSolverMethod )
{
  
  ////NUMPATCHES = scene->getNumTris() ; // should already be assigned in assignIds.
  if( ! NUMPATCHES )
  {
    error( "NUMPATCHES==0. Uhm, make some shapes bro, and CALL ASSIGN IDS AFTERWARD." ) ;
    return ;
  }

  FFsSizeInBytes = NUMPATCHES*NUMPATCHES*sizeof(float) ;
  info( "Allocating matrix space for %d patches.. (%d MB)", NUMPATCHES, FFsSizeInBytes/(1<<20) ) ;

  formFactorDetType = iFormFactorDetType ;
  matrixSolverMethod = iMatrixSolverMethod ;
  

  // get ffs.  supposedly this is 90% of the processing time.
  // by the time you're done, you will have a
  // matrix of form factors that explains the
  // relationship between any two patches in
  // the scene completely.
  // you have an NXN matrix for the form factors that we are just about to compute
  try
  {
    FFs = new Eigen::MatrixXf() ;
    FFs->resize( NUMPATCHES, NUMPATCHES ) ;
    //info( "Allocated %d MB for form factors matrix", FFsSizeInBytes/( 1 << 20 ) ) ;
  }
  catch(...)
  {
    error( "Not enough memory :(" ) ;
    DESTROY( FFs ) ;
    return ;
  }
  FFs->setZero( NUMPATCHES, NUMPATCHES ) ;
  //FFs->setConstant( 0.0 ) ; // same as above

  // HACKTUALLY, depending on the solution type selected, you need to handle this differently.
  // so now we decide on how to launch the solver.
  if( formFactorDetType == FormFactorsHemicube )
  {
    if( ! threadPool.onMainThread() )
    {
      error( "I cannot run gpu form factor determination on other than main thread." );
      return ;
    }
    
    calculateFormFactorsHemicubes( scene ) ; // NOT MT
    solve() ;   // boot solver when done
  }
  else
  {
    calculateFormFactorsRaytraced( scene ) ; // MT, will boot solver when done
  }
}

void RadiosityCore::calculateFormFactorsHemicubes( Scene* scene )
{
  info( "Using a hemicube" ) ;
  Hemicube* hemicube = new Hemicube( win, 0.5, hemicubePixelsPerSide ) ;

  // VISIT EACH PATCH
  info( "Calculating form factors.. there are %d patches", NUMPATCHES ) ;
  for( int i = 0 ; i < tris.size() ; i++ )
    hemicube->formFactors( scene, *tris[i], // the triangle we're analyzing
      FFs ) ;

  info( "Hemicube form factor determination complete, %d x %d form factors", NUMPATCHES, NUMPATCHES ) ;
}

void RadiosityCore::calculateFormFactorsRaytraced( Scene* scene )
{
  // AFTER raytraced form factors are ready, we can call checkFFs() and then solve().
  info( "Raycasting" ) ;
  
  ParallelBatch *pb = new ParallelBatch( "radiosity raycasting ffs", // rt ff

    // The callback for when done is to call the solve function.
    new CallbackObject0< RadiosityCore*, void(RadiosityCore::*)() >( this, &RadiosityCore::solve )
  ) ;

  // radiosity raycaster will spin off jobs.
  window->raycaster->QFormFactors( pb, scene, &tris, 120, FFs ) ;

  threadPool.addBatch( pb ) ;
}

Texcoording RadiosityCore::createVectorOccludersTex( int size )
{
  Texcoording ctex( size ) ;

  vectorOccludersTex = new D3D11Surface( win, 0,0, ctex.n,ctex.n, 0,1,
    0, SurfaceType::ColorBuffer, 3, // SLOT 3
    DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 4*4 ) ;
  
  {
    Vec4* tex = (Vec4*)vectorOccludersTex->open() ; // I open it here
    // Init random.
    //for( int i= 0 ; i < ctex.n*ctex.n ; i++ )
    //  tex[i] = Vertex( Vector::random() ).pos ;
  }

  return ctex ;
}

void RadiosityCore::smoothVectorOccludersData( Scene* scene )
{
  for( int i = 0 ; i < scene->shapes.size() ; i++ )
    scene->shapes[i]->meshGroup->smoothVectorOccludersData() ;
}

void RadiosityCore::parkAll( Scene* scene )
{
  smoothVectorOccludersData( scene ) ;

  Texcoording ctex = createVectorOccludersTex( VOTextureSize ) ;
  for( int i = 0 ; i < scene->shapes.size() ; i++ )
    for( int j = 0 ; j < scene->shapes[i]->meshGroup->meshes.size() ; j++ )
      for( int k = 0 ; k < scene->shapes[i]->meshGroup->meshes[j]->verts.size() ; k++ )
      {
        AllVertex& vertex = scene->shapes[i]->meshGroup->meshes[j]->verts[k] ;
        park( ctex, vertex ) ;
        //if( k == 47 )
        //  drawDebug( vertex ) ;
        DESTROY( vertex.voData ) ;
      }

  info( "Wrote %d texcoords for VO, which is %d floats total", ctex.totalAccum, ctex.totalAccum*3 ) ;
  vectorOccludersTex->close() ;
}

// park 'vertex's information in the vectorOccludersTex
void RadiosityCore::park( Texcoording& ctex, AllVertex& vertex )
{
  // Park this information at the vertex we are testing.
  ctex.start() ;
        
  // ASSUME EACH REMOTE MESH HAS JUST ONE COLOR
  for( map< Mesh*,VOVectors >::iterator iter = vertex.voData->reflectors.begin() ;
        iter != vertex.voData->reflectors.end() ; ++iter )
  {
    // #1: AVG POS
    //!! SWITCHING TO VEC4's NOW
    vectorOccludersTex->setColorAt( ctex, iter->second.pos ) ;
    //window->addDebugLine( vertex.pos, Vector(1,0,0), iter->second.pos, Vector(1,0,0) ) ;

    // #2: shadow dir
    vectorOccludersTex->setColorAt( ctex, iter->second.shadowDir ) ; // Uses colorTrans actually
    //window->addDebugLine( vertex.pos, Vector(.5), vertex.pos + iter->second.shadowDir, Vector(.5) ) ;

    // #3: caustic dir
    vectorOccludersTex->setColorAt( ctex, iter->second.causticDir ) ;
    //window->addDebugLine( vertex.pos, Vector(1,1,0), vertex.pos + iter->second.causticDir, Vector(1,1,0) ) ;

    // #4: caustic color
    vectorOccludersTex->setColorAt( ctex, iter->second.colorTrans ) ;

    // #5: DIFFUSE NORMAL
    vectorOccludersTex->setColorAt( ctex, iter->second.normDiffuse ) ;

    // #6: DIFFUSE COLOR OF REFLECTOR
    vectorOccludersTex->setColorAt( ctex, iter->second.colorDiffuse ) ;

    // unfortunately left out the W component of the specular reflector.
    // #7: SPECULARITY NORMAL
    vectorOccludersTex->setColorAt( ctex, iter->second.normSpecular ) ;
    //window->addDebugLine( vertex.pos, iter->second.colorSpecular,
    //  vertex.pos + iter->second.normSpecular, iter->second.colorSpecular ) ;

    // #8: SPECULAR COLOR OF REFLECTOR
    vectorOccludersTex->setColorAt( ctex, iter->second.colorSpecular ) ;
  }

  // tell the vertex where to find it's extended information
  // (so it can find it in the shader)
  vertex.texcoord[ TexcoordIndex::VectorOccludersIndex ].x = ctex.startingCol ; //COL
  vertex.texcoord[ TexcoordIndex::VectorOccludersIndex ].y = ctex.startingRow ; //ROW (x,y) of the texture to access
  vertex.texcoord[ TexcoordIndex::VectorOccludersIndex ].z = ctex.accum ;    // # texcoords written
  //Xvertex.texcoord[ TexcoordIndex::VectorOccludersIndex ].w = ambient ;    // put the ambient term here.
  //AMBIENT ALREADY WRITTEN BY RAYCASTER (AO BY HEMICUBE IS BAD/PATCHY)
}

void RadiosityCore::drawDebug( AllVertex& vertex )
{
  // Draw the BLOCKAGE DIRECTIONS at each vertex.
  for( map< Mesh*,VOVectors >::iterator iter = vertex.voData->reflectors.begin() ;
        iter != vertex.voData->reflectors.end() ; ++ iter )
  {
    window->addDebugLine( vertex.pos, 1, vertex.pos + iter->second.shadowDir, .8 ) ;

    window->addDebugLine( vertex.pos, iter->second.colorDiffuse, vertex.pos + iter->second.normDiffuse, .8*iter->second.colorDiffuse ) ;
    
    // Draw CAUSTIC DIR
    //window->addDebugLine( vertex.pos, iter->second.colorTrans, vertex.pos + iter->second.causticDir, iter->second.colorTrans*.8 ) ;

    // draw DIFFUSE NORMAL of reflector
    //window->addDebugLine( vertex.pos, iter->second.colorDiffuse, vertex.pos + iter->second.normDiffuse, iter->second.colorDiffuse*.2 ) ;

    // draw SPECULAR NORMAL
    //window->addDebugLine( vertex.pos, iter->second.colorSpecular, vertex.pos + iter->second.normSpecular, iter->second.colorSpecular*.2 ) ;

    // vector from vertex TO occluder "average point"
    //window->addDebugLine( vertex.pos, Vector(0,1,0), iter->second.pos, Vector(0,.2,0) ) ;
  }
}

void RadiosityCore::vectorOccluders( Scene* scene, FormFactorDetermination iFormFactorDetType )
{
  for( int i = 0 ; i < scene->shapes.size() ; i++ )
  {
    if( scene->shapes[i]->isEmissive() )
    {
      warning( "%s is emissive but it is considered a shape! "
      "VO runs best with point lights only. "
      "Add this shape as a FarLight to stop it "
      "from being considered a collidable shape",
      scene->shapes[i]->name.c_str() ) ;
    }
  }

  // How many vectors do I need to store?
  int vertsInScene = scene->countVertices() ;
  int meshesInScene = scene->countMeshes() ;

  // VO stores 8 vectors per vertex PER mesh seen,
  // WORST case is each vertex SEES each mesh, (but that almost never happens)
  int voVectorsWCReqd = vertsInScene*meshesInScene*(sizeof(VOVectors)/sizeof(Vector)) ; //worst case
  
  VOTextureSize = NextPow2( sqrt(voVectorsWCReqd) ) ;// textures always pow2

  info( "%d vectors (WC) required, allocating %dx%d size vo tex",
    voVectorsWCReqd, VOTextureSize,VOTextureSize ) ;

  formFactorDetType = iFormFactorDetType ;

  // I MUST DO 2 PASSES to factor AO into diffuse interreflection.
  ParallelBatch* pb = new ParallelBatch( "ao", new Callback0( [](){ 
    
    // when done ao, smooth it, 
    info( "Smoothing AO . . . " ) ;
    window->smoothAO() ;

    // then register calls to finish vo
    if( window->radCore->formFactorDetType == FormFactorDetermination::FormFactorsHemicube )
    {
      // NEVER USE THIS, it looks awful
      warning( "Hemicube form factor determination __much__ less accurate "
               "than raycasting, because I'm not going to bounce rays" ) ;
      window->radCore->calculateVectorOccludersHemicubes( window->scene ) ;
    }
    // otherwise the vo comp will be queued already (from outside)
  } ) ) ;
        
  // ao first, always done with ray casting in parallel
  window->raycaster->QAO( pb, window->scene, 120 ) ;
  threadPool.addBatch( pb ) ;

  if( formFactorDetType == FormFactorDetermination::FormFactorsRaycast )
  {
    // Should always raycast. It looks better.
    window->radCore->qVectorOccludersRaytraced( window->scene ) ;
  }



}

void RadiosityCore::calculateVectorOccludersHemicubes( Scene *scene )
{
  // THIS MUST RUN ON MAIN THREAD
  if( !threadPool.onMainThread() )
  {
    warning( "calculateVectorOccludersHemicubes called on NON-main thread.  re-issuing.." ) ;
    threadPool.createJobForMainThread( "Vector Occluders", new Callback0( [](){
      // re-invoke on main thread
      window->radCore->calculateVectorOccludersHemicubes( window->scene ) ;
    } ) ) ;

    return ;
  }

  ////NUMPATCHES = scene->getNumTris() ; // should already be assigned in assignIds.
  if( ! NUMPATCHES )
  {
    error( "NUMPATCHES==0. Uhm, make some shapes bro, and CALL ASSIGN IDS AFTERWARD." ) ;
    return ;
  }

  // only supports hemicubes
  Hemicube* hemicube = new Hemicube( win, 0.5, hemicubePixelsPerSide ) ; // probably want to keep pow2 to avoid texture problems

  // define a super-rough approximation to radiosity as follows:
  // for each vertex in scene:
  //   1)  position hemicube @ vertex, and render to surfaces
  //   2)  SUM TOTAL contribution PER FACE it sees (multiplying by FFcosines).
  //   3)  LOOK UP the BASE DIFFUSE COLOR and NORMAL of the poly.
  //   - store an summed AVERAGE color and AVERAGE normal
  //     for interreflection and use only that.

  Texcoording ctex = createVectorOccludersTex( VOTextureSize ) ;

  // Each Vertex: how much of each remote patch do you see?
  for( int i = 0 ; i < scene->shapes.size() ; i++ )
  {
    for( int j = 0 ; j < scene->shapes[i]->meshGroup->meshes.size() ; j++ )
    {
      Mesh *mesh = scene->shapes[i]->meshGroup->meshes[j];
      
      for( int k = 0 ; k < mesh->verts.size() ; k++ )
      {
        #pragma region INFORMATION GATHER
        AllVertex& vertex = mesh->verts[k] ;

        Vector eye     = vertex.pos ;
        Vector forward = vertex.norm ;

        // The randomness of the perpendicular introduces noise in
        // ambient occlusion in closed environments (hurts the
        // depth parameter)
        Vector up = forward.getRandomPerpendicular() ;

        map<int,real> patchesSeenAndTheirFFs ;
        
        // RENDER TO SURFACES CALL.
        if( every(k,10) )
          printf( "vo: shape %d/%d  vertex %d/%d     \r", i+1, scene->shapes.size(), k, mesh->verts.size() ) ;
        
        hemicube->voFormFactors( scene, eye, forward, up, patchesSeenAndTheirFFs ) ;

        vertex.voData = new VectorOccludersData() ;
        
        for( map<int,real>::iterator iter = patchesSeenAndTheirFFs.begin() ;
             iter != patchesSeenAndTheirFFs.end() ; ++iter )
        {
          int patchId = iter->first ;
          real ff = iter->second ;

          //if( patchId == 0x00ffffff )  ambient += ff ; // precomputed by raycasting (more accurate)
          if( patchId != 0x00ffffff ) // not ambient/miss
          {
            // SECONDARY DIFFUSE
            Triangle* sendingTri = tris[ patchId ] ;
            
            Vector diffuseColor = sendingTri->getAverageColor( ColorIndex::DiffuseMaterial ) ;
            Vector specularColor = sendingTri->getAverageColor( ColorIndex::SpecularMaterial ) ;

            real aoff = sendingTri->getAverageAO() * ff ;
            
            // SECONDARY specular interreflection
            //Shape* reflector = getShapeOwner( patchId ) ;
            Mesh* reflector = sendingTri->meshOwner ;
            
            // (represents "strength" of sent reflection)
            //!! THE DIFFUSE TERM MUST WEAR THIS DOWN BY AO, SO THAT DIFFUSE
            // INTERREFLECTIONS ARE NOT TOO STRONG.
            // BUT THE SPECULAR TERM DOESN'T CARE ABOUT AO.. IF THE
            // DIRECTION IS OPEN
            Vector specularityNormal = ff * sendingTri->normal ; // assumes reflection direction is open
            Vector diffuseNormal = aoff * sendingTri->normal ;

            // see if there's already an entry in reflectors
            map< Mesh*,VOVectors >::iterator iter = vertex.voData->reflectors.find( reflector )  ;
            if( iter == vertex.voData->reflectors.end() )
            {
              VOVectors posNNormal ;
              
              posNNormal.shadowDir = ff * sendingTri->normal ;
              posNNormal.normDiffuse = diffuseNormal ;
              posNNormal.normSpecular = specularityNormal ;

              posNNormal.colorDiffuse = diffuseColor ;
              posNNormal.colorSpecular = specularColor ;
              posNNormal.pos = sendingTri->getCentroid() ; 
              posNNormal.pos.w = 1 ; // set the count at 1
              vertex.voData->reflectors.insert( make_pair( reflector, posNNormal ) ) ;
            }
            else
            {
              iter->second.shadowDir += ff * sendingTri->normal ;
              iter->second.normDiffuse += diffuseNormal ;
              iter->second.normSpecular += specularityNormal ; // length of the vector will be strength of interreflecn
              
              iter->second.colorDiffuse += diffuseColor ;
              iter->second.colorSpecular += specularColor ;
              iter->second.pos += sendingTri->getCentroid() ;  // summed position
              iter->second.pos.w++; // increase the count
            }
          }
        } // each patchSeen

        // Now average out the position vectors (divide by w)
        
        for( map< Mesh*,VOVectors >::iterator iter = vertex.voData->reflectors.begin() ;
             iter != vertex.voData->reflectors.end() ; ++ iter )
        {
          iter->second.pos /= iter->second.pos.w ; // div by # items in summed pos
          iter->second.pos.w = 1 ; // just in case div gets called again somewhere
        }
        // /END INFORMATION GATHER
        #pragma endregion

        // INFORMATION PARK IN TEXTURE
        park( ctex, vertex ) ;

        // don't need voData anymore
        DESTROY( vertex.voData ) ;
      } // each vertex in mesh
    } // each mesh
  } // each shape
  
  vectorOccludersTex->close() ;
  
  //if( !vectorOccludersTex->save( "C:\\radspec.png" ) )
  //vectorOccludersTex->saveDDS( "C:\\radspec.dds" ) ; //R32

  // after done, assign each mesh the same tex
  // becauseof competition with shtex for texture ram,
  // this will have to be reset before rendering.
  // luckily this operation is cheap.
  for( int i = 0 ; i < scene->shapes.size() ; i++ )
    scene->shapes[i]->setAddTexture( vectorOccludersTex ) ;

  // update vertex buffers with the new texcoords AND the AO component
  window->updateVertexBuffers() ;
  
  info( Magenta, "Done Vector Occluders %f seconds", window->timer.getTime() ) ;
  window->programState = ProgramState::Idle ; // return to idle state
  
}

void RadiosityCore::qVectorOccludersRaytraced( Scene * scene )
{
  formFactorDetType = FormFactorDetermination::FormFactorsRaycast ;
  if( ! NUMPATCHES )
  {
    error( "NUMPATCHES==0. Uhm, make some shapes bro, and CALL ASSIGN IDS AFTERWARD." ) ;
    return ;
  }

  // add a parallel batch to do it
  ParallelBatch* pb = new ParallelBatch( "Vector Occluders raycast",
    new Callback0( [](){
      // get on the main thread and write ctexes
      threadPool.createJobForMainThread( "write ctex", new Callback0( [](){
        window->radCore->parkAll( window->scene ) ;

        //Save if you like to see the tex
        //window->radCore->vectorOccludersTex->saveDDS( "C:\\radspec.dds" ) ; //R32

        window->updateVertexBuffers() ;
        window->programState = ProgramState::Idle ; // return to idle state
      }
    )
  ) ;
  } ) ) ;

  window->raycaster->QVectorOccluders( pb, window->scene, 120 ) ;
  threadPool.addBatch( pb ) ;
}

void RadiosityCore::setAveragePatchColor( int channel, AllVertex* vertex, Eigen::VectorXf* fEx, Triangle *patchOwner )
{
  //vertex->color[ ColorIndex::RadiosityComp ] = getPatchColor( patchOwner->getId() ) ;
  vertex->color[ ColorIndex::RadiosityComp ].e[ channel ] = 0 ; //init @ 0
  
  // this is fast enough
  int count = 0 ;
  
  // find all tris that use this vertex
  for( int i = 0 ; i < tris.size() ; i++ )
  {
    Triangle *tri = tris[i];

    if( tri->usesVertex(vertex->pos) )
    {
      // add in it's patch color
      vertex->color[ ColorIndex::RadiosityComp ].e[channel] += (*fEx)( i ) ;
      count++;
    }
  }

  // avg
  vertex->color[ ColorIndex::RadiosityComp ].e[ channel ] /= count ;
}

void RadiosityCore::updatePatchRadiosities( int channel, Eigen::VectorXf* fEx )
{
  for( int i = 0 ; i < tris.size() ; i++ )
  {
    #if 0
    // FLAT
    // Using their patch numbers, add their patch colors
    // sets the 3 vertices to have the same color
    tris[ i ]->vA()->color[ ColorIndex::RadiosityComp ].e[ channel ] = 
    tris[ i ]->vB()->color[ ColorIndex::RadiosityComp ].e[ channel ] = 
    tris[ i ]->vC()->color[ ColorIndex::RadiosityComp ].e[ channel ] = 
      (*fEx)( i ) ;
    #else
    // SMOOTH/averaged
    setAveragePatchColor( channel, tris[ i ]->vA(), fEx, tris[i] ) ;
    setAveragePatchColor( channel, tris[ i ]->vB(), fEx, tris[i] ) ;
    setAveragePatchColor( channel, tris[ i ]->vC(), fEx, tris[i] ) ;
    #endif
  }
}

void RadiosityCore::checkFFs()
{
  //plainFile( "\n****************\nFORM FACTOR MATRIX:" ) ;
  //printMat( FFs ) ;
  //plainFile( "\n****************\n" ) ;

  int goodRows=0, zeroRows=0 ;
  real avgRowSum = 0.0 ;
  for( int row = 0 ; row < FFs->rows() ; row++ )
  {
    real rowSum = FFs->row( row ).sum() ;
    avgRowSum += rowSum ;
    //for( int col = 0 ; col < FFs->cols() ; col++ )
    //  rowSum += FFs( row, col ) ;

    //plains( "sum( FFs[ row=%d ] ) = %f", row, rowSum ) ;
    if( IsNear( rowSum, 1.0, 1e-3 ) )
      goodRows++;//warning( "The row sum (%f) isn't near enough to 1.0, row=%d", rowSum, row ) ;
    else if( IsNear( rowSum, 0.0, 1e-3 ) ) // its near 0
    {
      zeroRows++;
      warning( "Row sum=0.0, patch(%d) cannot see anything", row ) ;
    }
    else
      ;//info( "row %d - ok", row ) ;
  }

  avgRowSum /= FFs->rows() ;
  info( "Check FFS: %d good rows (equal to 1.0), avg=%f", goodRows, avgRowSum ) ;

  //printMat( *FFs ) ;
}

void RadiosityCore::solve()
{
  if( NUMPATCHES == 0 )
  {
    error( "Nothing to solve" ) ;
    return ;
  }
  checkFFs() ;

  #if MULTITHREADED_RADIOSITY_SOLUTION
  // uses 3x memory because must have different A for each form factor matrix rgb channels
  // because of this it isn't worth doing.  A small scene is fast to solve,
  // a large scene is slower, but you need the extra memory to solve the large scene.
  vector<Callback*> cbs ;
  for( int i = 0 ; i < 3 ; i++ )
  {
    // spin off a new thread for each channel
    cbs.push_back( new Callback0( [this,i](){
      solveRadiosity( i ) ;
    }) ) ;
  }
  
  threadPool.createBatch( "radiosity", cbs, new Callback0(
    [this](){
      info( "Radiosity done" ) ;

      threadPool.createJobForMainThread( "update rads",
        new Callback0([]{
          window->radCore->updatePatchRadiosities() ; 
          window->programState = ProgramState::Idle ; // Done computation.
        })
      ) ;
    }
  ) ) ;
  #else
  for( int i = 0 ; i < 3 ; i++ )
    solveRadiosity( i ) ;

  // after done, you don't need FFs anymore
  DESTROY( FFs ) ;

  info( "Done radiosity via GPU, %f seconds", window->timer.getTime() ) ;
  window->updateVertexBuffers() ;
  window->programState = ProgramState::Idle ; // Done computation.
  #endif
}

void RadiosityCore::dropFFsToDisk()
{
  if( !FFs ) { error( "FFs not declared, cannot drop" ) ; return ; }

  info( "Dropping FFs to disk" ) ;
  FILE* out = fopen( "C:/formfactors.dat", "wb" ) ;

  fwrite( (void*)FFs->data(), FFsSizeInBytes, 1, out ) ;
  fclose( out ) ;  //flush

  DESTROY( FFs ) ; //save memory
}

void RadiosityCore::recoverFFsFromDisk()
{
  if( FFs ) { info( "FFs already loaded" ) ; return ; }
  
  info( "Loading FFs from disk.." ) ;
  FILE* in = fopen( "C:/formfactors.dat", "rb" ) ;

  FFs = new Eigen::MatrixXf() ;
  FFs->resize( NUMPATCHES, NUMPATCHES ) ;

  fread( (BYTE*)FFs->data(), FFsSizeInBytes, 1, in ) ;
  // you can delete FFs from disk now

  fclose( in ) ;

  puts( "Loaded." ) ;

  puts( "Deleting file.." ) ;
  DeleteFileA( "C:/formfactors.dat" ) ;
  puts( "Deleted." ) ;
  
}

// ALL THIS HAS TO BE DONE ONCE PER CHANNEL
void RadiosityCore::solveRadiosity( int channel )
{
  Eigen::VectorXf exitance( NUMPATCHES ) ; //initial exitances
  Eigen::VectorXf fEx( NUMPATCHES ) ; // solution vector

  // For each channel, we need to compute
  // 2 things before forming the matrix A:
  //   1 - radiosity and initial exitance vector.  this is
  //       actually physically the EMISSIVE component of
  //       the lighting
  //   2 - reflectivities matrix -- really this is
  //       the diffuse MATERIAL component of each surface
  //       in the scene -- how well each surface reflects
  //       (or absorbs) radiation in each of the RGB bands.

  //1 RADIOSITY IS THE INITIAL EXITANCE VECTOR
  // this is ALSO the same thing as KE
  
  // pull exitance values
  // find the initial lighting conditions
  // by looking into the scene's patches
  // (ultimately came from scene patches).
  info( "Getting initial exitances (EMISSIVE TERM) from tris.." ) ;
  // set up the initial exitance as
  // the initial radiosity of the patch.
  for( int i = 0 ; i < NUMPATCHES ; i++ )
    exitance( i ) = tris[i]->getAverageColor( ColorIndex::Emissive ).e[ channel ] ;

  if( IsNear( exitance.sum(), 0.0, EPS_MIN ) )
    warning( "No %s light source patches in scene", ColorChannelIndexName[ channel ] ) ;

    
  // Form A: Will be destroying A by solving it,
  // so need to declare it here.  Each
  // RGB channel has it's own A.

  // A is the matrix to solve, by inversion or by
  // LU decomposition, or by an iterative method such as
  // Gauss-Seidel.
  
  // There are 3 different A matrices, because although the matrix
  // of form factors does not change per channel,
  // the reflectivities matrix may vary per color.
  // A BLUE object reflects blue and absorbs
  // all other colors.

  // A = I - REFLECTIVITIES .* FFs
  // Can compute by:
  // a)  A = - FFs
  // b)  for( each Patch ) A( patchRow ) *= RED REFLECTIVITY
  // c)  for( diagonal )  A(d,d) += 1 ;
  // D)  SOLVE RED
  // Repeat for Green, Blue
  
  //recoverFFsFromDisk() ;

  // THIS IS APPARENTLY NOT ALLOWED IN EIGEN WHEN FFs IS A POINTER
  ///Eigen::MatrixXf A = -(*FFs) ; //a) EIGENBUG: this screws up and A is a zero matrix
  
  // Explicit copy because the above line makes A a zero matrix. I hate Eigen.
  Eigen::MatrixXf A( NUMPATCHES, NUMPATCHES ) ;
  for( int row = 0 ; row < NUMPATCHES ; row++ )
    for( int col = 0 ; col < NUMPATCHES ; col++ )
      A( row, col ) = - ( (*FFs)( row, col ) ) ;

  //plainFile( "--FFS********************" ) ;
  //printMat( *FFs ) ;
  //plainFile( "--A %d******************* ",channel ) ;
  //printMat( A ) ;
  //dropFFsToDisk() ; // I don't need FFs here
  
  /// EXTRACT THE REFLECTIVITIES FROM THE SCENE GEOMETRY
  // 2 KD IS THE REFLECTIVITY
  for( int p = 0 ; p < tris.size() ; p++ )
  {
    // diffuse color multiplied by this
    // surface reflectivity damping coeff.
    // REFLECTIVITY MULTIPLIES THE ROWS OF FFs
    // the reflectivity in RGB is going to multiply
    // the form factors
    real rflect = tris[ p ]->getAverageColor( ColorIndex::DiffuseMaterial, channel ) * SURFACE_REFLECTIVITIES ;
    A.row(p) *= rflect ; //b)
    
    // Add 1 to that row's diagonal.
    A( p,p ) += 1.0 ; //c)
  }

  ////plainFile( "\n\n********* A=\n" );
  ////printMat( A ) ;
  ////plainFile( "END OF A\n" ) ;

  info( "SOLVING channel %s", ColorChannelIndexName[channel] ) ;
  IterativeSolver* iSolver ;
  ///////////window->stopWatch.start() ;
  if( matrixSolverMethod == LUDecomposition )
  {
    info( "Solve by LU decomposition" ) ;
    Eigen::FullPivLU<Eigen::MatrixXf> lu = A.fullPivLu() ;        // e3
    fEx = lu.solve( exitance ) ;         // e3
  }
  else if( matrixSolverMethod == AInverse )
  {
    // Now you can use the inverse of A to compute the final light at each
    // patch, GIVEN some initial conditions starting emittance vector
    // post multiplying the inverse matrix by
    // the initial exitances vector..
    info( "Solve A^-1" ) ;
    Eigen::MatrixXf AInverse = A.inverse() ;

    // Ax = b
    // so
    // A^-1 * Ax = A^-1 * b8
    // x = A^-1 * b
    fEx = AInverse * exitance ;
  }
  else
  {
    if( matrixSolverMethod == Jacobi )
      iSolver = new IterativeSolverJacobi( &A, &exitance, &fEx, iterativeTolerance ) ;
    else if( matrixSolverMethod == GaussSeidel )
      iSolver = new IterativeSolverSeidel( &A, &exitance, &fEx, iterativeTolerance ) ;
    else
      iSolver = new IterativeSolverSouthwell( &A, &exitance, &fEx, iterativeTolerance ) ;

    iSolver->solve() ;
  }

  //window->stopWatch.stop() ;
  info( "Solution complete" ) ;

  #pragma region solution accuracy/num stability check
  // check the solution vector,
  // by multiplying
  // B = Ax
  // here exitance was B,
  // x = fEx

  // so compute B, and you should get the original "exitance" vector
  Eigen::VectorXf B = A * fEx ;
  int correct = 0 ;
  for( int i = 0 ; i < B.size() ; i++ )
  {
    if( fEx(i) < 0 )
    {
      error( "Exitance value NEGATIVE, (%d)=%f", i, fEx( i ) ) ;
      fEx(i) = fabs( fEx( i ) ) ; // correct it.
    }
    else if( !IsNear( B(i), exitance(i), 0.1 ) )
      error( "B(%d) not within 0.1 of exitance(%d), diff=%f", i, i, ( B(i) - exitance(i) ) ) ;
    else
      correct++ ;
  }
  info( "%d / %d correct exitance values", correct, NUMPATCHES ) ;
  #pragma endregion

  //// Now we need to pass back
  // the patch radiosities,
  // will sit in TEXCOORD1 of
  // the GeneralVertex format.
  // So the result of the radiosity
  // computation is considered a texture.

  updatePatchRadiosities( channel, &fEx ) ;
} //A, exitances, fEx die
