#include "Hemicube.h"
#include "../window/GTPWindow.h"

char* SidesNames[] =
{
  "FRONT", "Top", "Right", "Bottom", "Left"
} ;

Hemicube::Hemicube( D3D11Window* iWin, real iSideLen, int iPixelsPerSide ) :
  sideLen( iSideLen ),
  pixelsPerSide( iPixelsPerSide )
{
  win = iWin ;

  surfaces[ FRONT ] =  new D3D11Surface( win, 0,0, pixelsPerSide, pixelsPerSide,   0,1,0, SurfaceType::RenderTarget, 0, DXGI_FORMAT_R8G8B8A8_UNORM, 4 ) ;
  surfaces[ Top ] =    new D3D11Surface( win, 0,0, pixelsPerSide, pixelsPerSide/2, 0,1,0, SurfaceType::RenderTarget, 0, DXGI_FORMAT_R8G8B8A8_UNORM, 4 ) ;
  surfaces[ Right ] =  new D3D11Surface( win, 0,0, pixelsPerSide, pixelsPerSide/2, 0,1,0, SurfaceType::RenderTarget, 0, DXGI_FORMAT_R8G8B8A8_UNORM, 4 ) ;
  surfaces[ Bottom ] = new D3D11Surface( win, 0,0, pixelsPerSide, pixelsPerSide/2, 0,1,0, SurfaceType::RenderTarget, 0, DXGI_FORMAT_R8G8B8A8_UNORM, 4 ) ;
  surfaces[ Left ] =   new D3D11Surface( win, 0,0, pixelsPerSide, pixelsPerSide/2, 0,1,0, SurfaceType::RenderTarget, 0, DXGI_FORMAT_R8G8B8A8_UNORM, 4 ) ;
  
  // because this is a hemicube, we want the viewport to render AS IF
  // the aspect ratio were 1.0
  for( int i = 1 ; i < 5 ; i++ )
    surfaces[i]->viewport.Height *= 2 ;

  // generate vertices of the hemicube,
  // about the origin.

  real s = sideLen/2.0 ;

  // half cube in y>0 halfspace
  /*
  A = Vector( -s, 0, -s ) ;
  B = Vector( -s, 0,  s ) ;
  C = Vector( -s, s, -s ) ;
  D = Vector( -s, s,  s ) ;

  E = Vector(  s, 0, -s ) ;
  F = Vector(  s, 0,  s ) ;
  G = Vector(  s, s, -s ) ;
  H = Vector(  s, s,  s ) ;
  //*/
  
  // in z<0 half space
  ///*
  A = Vector( -s, -s, -s ) ;
  B = Vector( -s, -s,  0 ) ;
  C = Vector( -s,  s, -s ) ;
  D = Vector( -s,  s,  0 ) ;
                  
  E = Vector(  s, -s, -s ) ;
  F = Vector(  s, -s,  0 ) ;
  G = Vector(  s,  s, -s ) ;
  H = Vector(  s,  s,  0 ) ;
  //*/

  // in z>0 half space
  /*
  A = Vector( -s, -s, 0 ) ;
  B = Vector( -s, -s, s ) ;
  C = Vector( -s,  s, 0 ) ;
  D = Vector( -s,  s, s ) ;
                  
  E = Vector(  s, -s, 0 ) ;
  F = Vector(  s, -s, s ) ;
  G = Vector(  s,  s, 0 ) ;
  H = Vector(  s,  s, s ) ;
  //*/

  computeFormFactorCosines() ;
}

////Vector& Hemicube::formFactorDirectionVector( int face, int row, int col )
////{
////  ///////return formFactorDirectionVectors[ face ][ row*pixelsPerSide + col ] ;
////}

void Hemicube::computeFormFactorCosines()
{
  info( "Computing form factor cosines.." ) ;

  //////for( int i = 0 ; i < 5 ; i++ )
  //////  formFactorDirectionVectors[ i ].resize( pixelsPerSide*pixelsPerSide ) ;

  // you only need one front and one side.
  formFactorCosines[ Hemicube::FRONT ].resize( pixelsPerSide, pixelsPerSide ) ;

  // assuming sideLength of 2.0, area=2*2=4.0
  // each pixel's area is then 4/(px^2)
  real dA = 4.0 / (pixelsPerSide*pixelsPerSide) ;
  // so each pixel occupies an area of 4/(w*h)

  for( int row = 0 ; row < pixelsPerSide ; row++ )
  {
    for( int col = 0 ; col < pixelsPerSide ; col++ )
    {
      // D3D gives you the texture with the
      // top right corner first.  So, the transformation is
      // (pp 283-284 Radiosity Ashdown), (p85 Cohen&Wallace)
      real u = -1 + 2.0*(col+0.5) / pixelsPerSide ;  // horizontal
      real v = -1 + 2.0*(row+0.5) / pixelsPerSide ;  // VERTICAL ie rows

      real c = u*u+v*v+1 ; // square length of vector to pt (u,v,1)
      real dF = dA / (PI*c*c) ;
      formFactorCosines[ Hemicube::FRONT ]( row, col ) = dF;
    }
  }
  //plainFile( "Form factor cosines, FRONT" ) ;
  //printMat( formFactorCosines[ Hemicube::FRONT ] ) ;

  /////////////
  /// sides ///
  // the renders are done in landscape, with the bottom row of pixels corresponding
  // to the bottom of the hemicube (stick your head in the hemicube and rotate around)
  formFactorCosines[ Hemicube::Top ].resize(
    pixelsPerSide / 2, //rows
    pixelsPerSide ) ;//cols

  for( int row = 0 ; row < pixelsPerSide/2 ; row++ )
  {
    for( int col = 0 ; col < pixelsPerSide ; col++ )
    {
      real u = -1 + 2*( (col+0.5) / pixelsPerSide ) ;

      // n is inverted.
      // THE TOP OF THE TEXTURE IS THE FIRST ROW.
      //real n = 0 + 1*( row / (pixelsPerSide/2.0) ) ;
      real n = 1 - 1*( (row+0.5) / (pixelsPerSide/2.0) ) ; // @ row=0, n = NEAR 1
      // n is vertical distance from surface.

      // @ row = pixelsPerH/2.0, n = NEAR 0.
      real c = u*u+n*n+1 ;
      real dF = (dA*n) / (PI*c*c) ;
      // at each row and column, 
      formFactorCosines[ Hemicube::Top ]( row, col ) = dF ;
    }
  }

  //plainFile( "Form factor cosines, Top" ) ;
  //printMat( formFactorCosines[ Hemicube::Top ] ) ;
}

void Hemicube::renderToSurface( HemiSides surfaceSide, Scene *scene, const Vector& eye, const Vector& lookDir, const Vector &upDir )
{
  surfaces[ surfaceSide ]->rtBind() ;

  // Clear WHITE.  This is the "no patch" color
  surfaces[ surfaceSide ]->clear( ByteColor( 255,255,255,255 ) ) ;

  if( !win->beginDrawing() )
  {
    error( "Hemicube could not begin drawing" ) ;
    return ;
  }
  
  // Set matrices in shader (globals)
  Matrix proj = Matrix::ProjPerspectiveRH( RADIANS( HEMI_FOV ), 1.0, HEMI_NEAR, HEMI_FAR ) ; // always use 1.0 aspect, even on sides
  Matrix view = Matrix::LookAt( eye, lookDir, upDir ) ;
  
  win->setModelviewProjection( view*proj ) ;

  //win->cullMode( CullMode::CullNone ) ;
  win->setFillMode( FillMode::Solid ) ;
  
  // activate the id shader:
  win->alphaBlend( false ) ;
  
  #pragma region right render code
  // To calculate form factors,
  // I need the scene rendered
  // with patch ids (Hence use the "shaderID" shader.)
  win->shaderID->unsetTexture( 0 ) ; // DO NOT draw with a texture! (if there was bound previously)
  win->shaderID->activate() ;
  win->render( scene ) ;  
  #pragma endregion

  // TESTING AND DEBUGGING
  /////win->shaderPhongLit->activate() ;//! DEBUG
  /////window->rasterize() ;//! DEBUG

  win->endDrawing() ;
  win->present() ;

  surfaces[ surfaceSide ]->rtUnbind() ;
}

void Hemicube::renderToSurfaces( Scene *scene, const Vector& eye, const Vector& forward, const Vector& up, real offset )
{
  // Forward is the vector that points OUT of the
  // hemicube -- same vector as the surface normal
  // of the surface this hemicube is built on top of.
  /////Vector up = forward.getPerpendicular() ;  // what's UP to the hemicube.  `out` and
  // `up` are always orthogonal
  Vector right = forward << up ; // what's "right" to the hemicube

  // Before we begin get vectors in the direction of
  // TOP, RIGHT, BOTTOM, LEFT first.
  Vector outFront = forward ;
  Vector outTop = up ;
  Vector outRight = right ;
  Vector outBottom = -outTop ;
  Vector outLeft = -outRight ;
  
  //// RENDERING
  // Generate the projection matrix
  // CHANGE THE ASPECT RATIO OF THE PROJECTION MATRIX
  //!!ASPECT RATIO check
  
  // Front:
  // The LOOK vector is always,
  // EYE + direction (its a point in
  // space that you are looking at,
  // while the up vector is not like that.
  
  Vector offEye = eye + outFront*offset; // patchiness and pixellation almost always caused
  // by hemicube renders CATCHING THE BACK OF NEIGHBOURING POLYGONS,
  // OR CATCHING THEMSELVES.
  
  // Front: UP is the direction of TOP
  // look out in front of you 
  renderToSurface( FRONT, scene, offEye, eye + outFront, outTop ) ; // 0

  // Top:
  renderToSurface( Top, scene, offEye, eye+outTop, outFront ) ;   // 1

  // Right:
  renderToSurface( Right, scene, offEye, eye+outRight, outFront ) ; // 2

  // Bottom:
  renderToSurface( Bottom, scene, offEye, eye+outBottom, outFront ) ;// 3

  // Left:
  renderToSurface( Left, scene, offEye, eye+outLeft, outFront ) ;  // 4
}

/// Trigger a render to the Hemicube face surfaces,
// from the vantage point "eye", in direction "forward"
void Hemicube::formFactors( Scene *scene, const Triangle& tri, Eigen::MatrixXf* FFs )
{
  UINT sender = tri.getId() ; // this patch is the sender
  if( every( sender, 10 ) )
    printf( "Positioning patch %d/%d\r", sender, FFs->rows() ) ; // rows in FFS is total patches.
  //printf( ".. computing form factor .. %c\r", spinner = nextSpin( spinner, true ) ) ;

  Vector eye = tri.getCentroid() ;

  // forward=NORMAL to vertex/patch
  // up/right doesn't matter, it just has to be
  // perpendicular to forward, so
  // up = forward.getPerpendicular() and
  // right = forward << up
  Vector forward = tri.normal ;  // tri.normal is actually the FWD vector

  if( forward.len2() == 0 )
  {
    error( "normal was 0" ) ;
    forward = Vector(0,1,0) ;
  }

  Vector up = forward.getRandomPerpendicular() ;

  renderToSurfaces( scene, eye, forward, up, 1e-6 ) ; //1e-9 seems to work fine for PATCHES
  ///window->scene->extractCubeVertices() ; //debug
  //saveSurfacesToFiles() ; // debug

  // the information from renderToSurfaces IS ONE ROW
  // of the form factor matrix.

  #pragma region form factor extraction
  ////////////////////
  // ANALYSIS AND FORM FACTOR EXTRACTION
  // now, walk through the hemicube's surfaces,
  // and count all the pixels that get colored.

  // we want to know patch i's relationship..
  // with all other scene patches.

  // so, go through what got rendered.
  // everytime the color changes, that's a
  // different patch that this patch sees.
  // we'll call THIS patch the "RECIPIENT",
  // and the "other" patch the "SENDER"
  //     
  // to COUNT this, just walk through each
  // surface, and INCREMENT form factor matrix @
  //   ffs[ RECIPIENT ][ SENDER ] += (cosine multiplier for pixel)*(1 pixel) ;

  // these numbers will be the same as the counter variable 'i'
  for( int side = 0 ; side < 5 ; side++ )
  {
    D3D11Surface *surface = surfaces[ side ] ;
    surface->open() ;

    // now walk through each pixel of the surface.
    // if you cast the pointer to UINT, each
    // entry pointed to is a complete 32 bit pixel,
    // and so, you can simply read the value there as
    // the ID.
    int numPx = surface->getNumPixels() ;
    int ffFaceIndex = 0 ;
    
    if( side > 0 )
    {
      // it's a side, not the front
      ffFaceIndex = 1 ;

      // only go through top half of pixels
      // in the side faces.  the bottom half remains unused
      ////numPx /= 2 ; // ACTUALLY the textures are half width
    }
    
    // the patches this patch "sends" energy to
    // are the patches it can "see"
    int white = 0 ;
    //for( int pixel = 0 ; pixel < numPx ; pixel++ )
    for( int row = 0 ; row < surface->getHeight() ; row++ )
    {
      for( int col = 0 ; col < surface->getWidth() ; col++ )
      {
        //int pixel = row * surface->getWidth() + col ;
        //UINT rawColor = surface->getColorAt( pixel )->intValue ;
        UINT rawColor = surface->getColorAt( row, col )->intValue ;

        // turn off the alpha bits to get the ID of
        // the receiver patch
        UINT receiver = rawColor & ~ALPHA_FULL ; // Here you must turn the alpha bits off all the time
        // because the rendered pixels will always have alpha off.

        //printf( "Patch %08x receives from %08x\n", sender, receiver ) ;
        if( receiver == 0x00ffffff )
        {
          white++ ;   /// LOST ENERGY
        }
        else if( sender > FFs->rows() ||
                 receiver > FFs->rows() )
        {
          // this is where patches that "see nothing" (look into space)
          // go. (would see 0x00ffffff, (w/alpha off mask))
          // this happens a lot for patches that
          // are positioned on a border, just so
          // they face out of the scene.
          error( "Patch %08x->%08x larger than max patches, should not exist", sender, receiver ) ;
        }
        else
        {
          /// the side surfaces only use 
          /// "top" determined form factors.. symmetry
          //FFs( sender, receiver ) += formFactorCosines[ ffCosIndex ]( pixel ) ;

          (*FFs)( sender, receiver ) += formFactorCosines[ ffFaceIndex ]( row,col ) ;
        }
      } // foreach col
    } // foreach row


    //if( white )
    //  printf( "%d/%d px were white", white, numPx ) ;
    surface->close() ;
  } // each hemicube side
  #pragma endregion
}

void Hemicube::voFormFactors( Scene *scene, const Vector& eye, const Vector& forward, const Vector& up,
     map<int,real> &patchesSeenAndTheirFFs )
{
  // render the scene
  renderToSurfaces( scene, eye, forward, up, 1e-6 ) ;// need to offset furter for per-vertex than for patches
  //saveSurfacesToFiles() ; // debug

  // push the ambient, because you always need this
  // it should be the first entry (index 0) so it's easy to add to
  ////patchesSeenAndTheirFFs.insert( make_pair( 0x00ffffff, 0.0 ) ) ;
  ////if( patchesSeenAndTheirFFs.size() != 1 )
  ////  warning( "FFs size isn't 1" ) ;

  // now examine
  for( int side = 0 ; side < 5 ; side++ )
  {
    D3D11Surface *surface = surfaces[ side ] ;
    surface->open() ;
    ///surface->openDepthTex() ;

    int ffFaceIndex = 0 ;
    if( side > 0 )  ffFaceIndex = 1 ; // means a side face

    // the patches this patch "sends" energy to
    // are the patches it can "see"
    for( int row = 0 ; row < surface->getHeight() ; row++ )
    {
      for( int col = 0 ; col < surface->getWidth() ; col++ )
      {
        UINT rawColor = surface->getColorAt( row, col )->intValue ;
        /////float depth = *surface->getDepthAt( row, col ) ;
        
        // SENDER here means something different than the other loop.
        // here we're talking about
        UINT SENDINGPATCHID = rawColor & ~ALPHA_FULL ; // turn off the alpha-bits

        // if IN patchesSeen use that entry
        map<int,real>::iterator pEntry = patchesSeenAndTheirFFs.find( SENDINGPATCHID ) ;
        if( pEntry == patchesSeenAndTheirFFs.end() ) // no entry
          patchesSeenAndTheirFFs.insert( make_pair( SENDINGPATCHID, formFactorCosines[ ffFaceIndex ]( row,col ) ) ) ;
        else
          pEntry->second += formFactorCosines[ ffFaceIndex ]( row,col ) ;

        // ambient only gets blocked if the geometry is really close
        // of course this means form factors WILL ADD UP TO MORE THAN 1.0,
        // but that's supposed to happen .. this __isn't__ a radiosity computation,
        // there is no matrix solution.. we're just approximating the AMOUNT
        // of interreflected light we'd get from the remote surfaces.  because non static emissive
        // patches _aren't_ factored in here, we have to do it this way.

        // depth values are between 0 and 1, so we only cut the
        // the ambient contribution when the value is REALLY small
        // (ie the fragment is REALLy close)
        
        // condition to be far enough away NOT to block light //(CHUNKY/BLOCKY)
        //if( depth > .99 )  ffs[ 0 ] += formFactorCosines[ ffFaceIndex ]( row,col ) ; // add to ambient index

        // "BIG DEPTH" means FAR AWAY GEOMETRY
        // which means "HIGH AMBIENT OCCLUSION".

        // If the scene is indoors (geometry is closed
        // and you will get __0__ ambient), use depth*ff
        // for the ao term (would always get 0 otherwise)
        /////if( indoorsScene )
        /////  patchesSeenAndTheirFFs[ 0 ] += depth*formFactorCosines[ ffFaceIndex ]( row,col ) ; // add to ambient index

        ///else printf( "Depth %f\n", depth ) ;
      } // foreach col
    } // foreach row

    surface->close() ;
    ////surface->closeDepthTex() ;
  } // each hemicube side
}

real Hemicube::getArea( HemiSides surfaceSide )
{
  if( surfaceSide == HemiSides::FRONT ) // the only full size surface
    return sideLen * sideLen ;
  else
    return sideLen * (sideLen/2.0) ;
}

void Hemicube::saveSurfacesToFiles()
{
  char surfaceFilename[ 100 ] ;

  static int filesavecallnumber = 0 ;

  for( int i = 0 ; i < 5 ; i++ )
  {
    sprintf( surfaceFilename, "C:/vctemp/builds/gtp/fullcube/%08d_Hemicube_%s.png",
      filesavecallnumber++,
      //currentPatch->id,//id removed, needs be added back
      //"", // names removed
      SidesNames[ i ]
    ) ;
    
    surfaces[i]->save( surfaceFilename ) ;
  }

  info( "Saved surfaces to files" ) ;
}


