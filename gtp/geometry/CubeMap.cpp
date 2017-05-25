#include "CubeMap.h"
#include "../window/GTPWindow.h"
#include "../math/SHVector.h"
#include "../geometry/MeshGroup.h"
#include "../geometry/Mesh.h"
#include "../geometry/Model.h"
#include "../geometry/Icosahedron.h"
#include "../rendering/FullCubeRenderer.h"
#include "../math/WaveletTransform.h"



RayCollection* CubeMap::rc ;

char* CubeFaceName[6] =
{
  "PX", "NX", "PY", "NY", "PZ", "NZ"
} ;

// DirectX texture tool saves cubemaps
// LEFT HANDED.  This means they are totally backwards,
// and that is the standard.
// This program works exclusively RIGHT HANDED.

// The CONVERTING set of vectors
// are the FWD,RIGHT,UP sets
// and they convert from LH,
// UPSIDE DOWN to RH, right side up.

// FOR A STANDARD D3D11 (LH) ALIGNED cubemap,
// __IN TERMS OF RH SYSTEM COORDINATES__
// FORWARD for the PX face is (1,0,0),
// RIGHT for the PX face is (0,0,-1)
// UP for the PX face is (0,-1,0).

// Whenever I use a cubemap that was saved out
// by a D3D tool (eg a .dds file) or
// was rendered to by D3D (eg in FullCubeRenderer),
// I find the DIRECTION of a texel using THESE
// basis vectors.  What this does is __PRETEND__ that
// the cubemap is RH .. even though the texels in
// the array are stored as if they were LH,
// these vectors convert position to RH.

// would I really mean it if I say I'm sorry? Probably not.
Vector cubeFaceBaseConvertingFwd[6] = {
  Vector( 1, 0, 0 ), // +x
  Vector(-1, 0, 0 ), // -x
  Vector( 0, 1, 0 ), // +y
  Vector( 0,-1, 0 ), // -y
  Vector( 0, 0, 1 ), // +z
  Vector( 0, 0,-1 )  // -z
} ;
Vector cubeFaceBaseConvertingRight[6] = {
  Vector( 0, 0,-1 ), // +x
  Vector( 0, 0, 1 ), // -x
  Vector( 1, 0, 0 ), // +y
  Vector( 1, 0, 0 ), // -y
  Vector( 1, 0, 0 ), // +z
  Vector(-1, 0, 0 )  // -z
} ;
Vector cubeFaceBaseConvertingUp[6] = {
  Vector( 0,-1, 0 ), // +x
  Vector( 0,-1, 0 ), // -x
  Vector( 0, 0, 1 ), // +y
  Vector( 0, 0,-1 ), // -y
  Vector( 0,-1, 0 ), // +z
  Vector( 0,-1, 0 )  // -z
} ;

// FOR A CORRECTLY (RH) ALIGNED cubemap,
// FORWARD for the PX face is (1,0,0),
// RIGHT for the PX face is (0,0,1)
// UP for the PX face is (0,1,0).
Vector cubeFaceBaseRHFwd[6] = {
  Vector( 1, 0, 0 ), // +x
  Vector(-1, 0, 0 ), // -x
  Vector( 0, 1, 0 ), // +y
  Vector( 0,-1, 0 ), // -y
  Vector( 0, 0, 1 ), // +z
  Vector( 0, 0,-1 )  // -z
} ;
Vector cubeFaceBaseRHRight[6] = {
  Vector( 0, 0, 1 ), // +x
  Vector( 0, 0,-1 ), // -x
  Vector( 1, 0, 0 ), // +y
  Vector( 1, 0, 0 ), // -y
  Vector(-1, 0, 0 ), // +z
  Vector( 1, 0, 0 )  // -z
} ;
Vector cubeFaceBaseRHUp[6] = {
  Vector( 0, 1, 0 ), // +x
  Vector( 0, 1, 0 ), // -x
  Vector( 0, 0, 1 ), // +y
  Vector( 0, 0,-1 ), // -y
  Vector( 0, 1, 0 ), // +z
  Vector( 0, 1, 0 )  // -z
} ;

Vector cubeFaceBaseColor[6] = {
  Vector( 1, 0, 0 ), // +x: red
  Vector( 0, 0, 1 ), // -x: blue
  Vector( 0, 1, 0 ), // +y: green
  Vector( 1, 0, 1 ), // -y: magenta
  Vector( 0, 1, 1 ), // +z: cyan
  Vector( 1, 1, 0 )  // -z: yellow
} ;

Vector CubeMap::compressCumulativeRemoved =0;
Vector CubeMap::compressCumulativeKept =0;
Vector CubeMap::compressCumulativeProcessed =0;

CubeMap::CubeMap( string iName, char *iCubeTexFile,
  //int iSlices, int iStacks,
  int icosaSubds,
  bool iBilinear,
  real cubeMapVizRadius )
{
  info( Green, "Loading cubemap '%s'", iCubeTexFile ) ;
  
  // RUN ON MAIN THREAD
  if( ! threadPool.onMainThread() )
  {
    error( "You must open a cubemap on the main thread" );
    return ;
  }

  defInit() ;
  name = iName ;
  radius = cubeMapVizRadius ;
  bilinear = iBilinear ;

  #pragma region load it as a shader resource

  // loads the cube tex as a shader resource.
  // we USED TO stopped doing this because we're sampling the
  // cube tex, sh projecting it, then releasing it.
  D3DX11_IMAGE_LOAD_INFO loadInfoRES ;
  // stock simple load/display behaviour
	loadInfoRES.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
  loadInfoRES.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
  
  // modifiable by CPU
  //
  //      You can't create a dynamic texture cube!
  //      TOO BAD!!!
  //
  //      D3D11: ERROR: ID3D11Device::CreateTexture2D: A D3D11_USAGE_DYNAMIC Resource must have ArraySize equal to 1.  A TextureCube cannot be a D3D11_USAGE_DYNAMIC resource. [ STATE_CREATION ERROR #101: CREATETEXTURE2D_INVALIDDIMENSIONS ]
  //      First-chance exception at 0x000007fefd63cacd in gtp.exe: Microsoft C++ exception: _com_error at memory location 0x001ed750..
  //      D3D11: ERROR: ID3D11Device::CreateTexture2D: Returning E_INVALIDARG, meaning invalid parameters were passed. [ STATE_CREATION ERROR #104: CREATETEXTURE2D_INVALIDARG_RETURN ]
  ////////////loadInfoRES.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE ;
  ////////////loadInfoRES.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  ////////////loadInfoRES.Usage = D3D11_USAGE_DYNAMIC ; // CPU WRITE / GPU READ
  ////////////loadInfoRES.CpuAccessFlags = D3D11_CPU_ACCESS_WRITE ;

  if( !DX_CHECK( D3DX11CreateTextureFromFileA( window->d3d, iCubeTexFile, &loadInfoRES, 0, (ID3D11Resource**)&cubeTex, 0 ), "load texture cube" ) )
  {
    error( "Couldn't load texture cube!" ) ;
    return ;
  }
  
	D3D11_TEXTURE2D_DESC cubeTexDesc ;
	cubeTex->GetDesc(&cubeTexDesc);

  //printf( "MIPLEVELS: %d\n", cubeTexDesc.MipLevels ) ;
  //printf( "ARRAY SIZE: %d\n", cubeTexDesc.ArraySize ) ;
  
	D3D11_SHADER_RESOURCE_VIEW_DESC cubeTexRsvDesc;
	cubeTexRsvDesc.Format = cubeTexDesc.Format;
	cubeTexRsvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	cubeTexRsvDesc.TextureCube.MipLevels = cubeTexDesc.MipLevels;
	cubeTexRsvDesc.TextureCube.MostDetailedMip = 0; //always 0

  DX_CHECK( window->d3d->CreateShaderResourceView( cubeTex, &cubeTexRsvDesc, &cubeTexRsv ), "create shader res view cubetex" ) ;
  #pragma endregion

  // LOAD IT AGAIN as a staged resource,
  // could also copy it, but loading it again is fine
  #pragma region open the cubemap texture
  // load it as a staging texture so you can access
  // the individual texels
    
  // to load it here you actually have to load it AGAIN as a STAGING texture.
  ID3D11Texture2D* stageCubeTex ;

  D3DX11_IMAGE_LOAD_INFO loadInfo ;
	loadInfo.MiscFlags = D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_TEXTURECUBE ;
  loadInfo.Usage          = D3D11_USAGE::D3D11_USAGE_STAGING;
  loadInfo.CpuAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_READ;
  loadInfo.FirstMipLevel = 0 ;
  loadInfo.MipLevels = 1 ;
  loadInfo.BindFlags = 0 ;
  
  DX_CHECK( D3DX11CreateTextureFromFileA(
    window->d3d, iCubeTexFile, &loadInfo, 0, (ID3D11Resource**)&stageCubeTex, 0 ), 
    "load texture cube STAGING" );

  D3D11_TEXTURE2D_DESC stageCubeTexDesc ;
	stageCubeTex->GetDesc(&stageCubeTexDesc);
  info( "Cubemap %s, %d x %d loaded", name.c_str(), stageCubeTexDesc.Width, stageCubeTexDesc.Height ) ;

  pixelsPerSide = stageCubeTexDesc.Width ;
  if( pixelsPerSide != stageCubeTexDesc.Height )
    error( "CubeMap:: width (%d) != height (%d)", stageCubeTexDesc.Width, stageCubeTexDesc.Height ) ;
  
  #pragma endregion

  #pragma region reading the faces
  currentSource = colorValues = new vector<Vector>( 6*pixelsPerSide*pixelsPerSide, Vector(0,0,0,1) ) ;
  
  // FACE must follow enumeration order in CubeMap::Face { PX, NX, PY, NY, PZ, NZ } ;
  // which is the same order as the d3d texture tool.

  // PULL THE TEXELS AND SAVE THEM IN colorValues AS RH.
  for( int FACE = 0 ; FACE < 6 ; FACE++ )
  {
    D3D11_MAPPED_SUBRESOURCE mappedSubRes ;
    DX_CHECK( window->gpu->Map( stageCubeTex, FACE, D3D11_MAP::D3D11_MAP_READ, 0, &mappedSubRes ), "Map the cubemap for cpu read" ) ;

    int bytesPerPixel ;
    switch( stageCubeTexDesc.Format )
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
      bytesPerPixel = 16 ;
      break ;

    case DXGI_FORMAT_R8G8B8A8_UNORM: // default 8-bit format
    default:
      bytesPerPixel = 4 ;
      break ;
    }

    int wastedBytes = mappedSubRes.RowPitch - bytesPerPixel*pixelsPerSide ;
    int wastedIndicesPerRow = wastedBytes/bytesPerPixel ;
    
    // "shoot a ray to each cube map texel center.."
    for( int row = 0 ; row < pixelsPerSide ; row++ )
    {
      printf( "reading face=%d, row=%d/%d     \r", FACE, row,pixelsPerSide ) ;
      for( int col = 0 ; col < pixelsPerSide ; col++ )
      {
        // THE FIRST TEXEL IS THE TOP LEFT.

        // The starting point for a row is USUALLY just 
        // row*stageCubeTexDesc.Width, BUT, there are 'wastedIndex'
        // wasted pixels/indices per row, so,
        int index = ( row * ( pixelsPerSide + wastedIndicesPerRow ) ) + col ; 

        // extract the texel.
        Vector texel ;

        switch( stageCubeTexDesc.Format )
        {
          case DXGI_FORMAT_R8G8B8A8_UNORM: // default 8-bit format
            {
              UINT px = ((UINT*)mappedSubRes.pData)[ index ] ;
        
              // abgr
              int ialpha = (0xff000000 & px)>>24, iblue=(0x00ff0000 & px)>>16, igreen=(0x0000ff00 & px)>>8, ired=0x000000ff & px ;
              texel = Vector::fromByteColor( ired, igreen, iblue, ialpha ) ;
            }
            break ;
          case DXGI_FORMAT_R32G32B32A32_FLOAT:
            {
              float* px = ((float*)mappedSubRes.pData) + 4*index ; 
              texel.x = px[0];
              texel.y = px[1];
              texel.z = px[2];
              texel.w = px[3];
            }
            break ;
          default:
            warning( "Unrecognized texture format, you get black" ) ;
            break ;
        }

        //texel.clamp( 0.0, 1.0 ) ;// compress hdr images non linearly
        //texel /= 20 ;

        int indexTightlyPacked = ( row * pixelsPerSide ) + col ; 
        int texelIndex = (FACE*pixelsPerSide*pixelsPerSide) + (indexTightlyPacked) ;
        (*colorValues)[ texelIndex ] = texel ;  // save the color value
      }// each col
    }// each row
    
    window->gpu->Unmap( stageCubeTex, FACE ) ;
  }
  
  //info( "Done loading the cubemap.. generating visualization.." ) ;
  SAFE_RELEASE( stageCubeTex ) ;
  #pragma endregion
  

  Sphere *sp = Sphere::fromIcosahedron( "temp cubemap sphere", cubeMapVizRadius, icosaSubds, Material(0,1,0,0,1) );
  takeMeshesFrom( sp ) ;  // unlink that mesh from the other object and give it to me
  delete sp ;

  createCubemapMesh() ;
  //info( "Done generating cubemap visualization.." ) ;
  

  shProject( 1 ) ;
}

CubeMap::CubeMap( D3D11Surface* surfaces[6] )
{
  defInit() ;

  pixelsPerSide = surfaces[0]->texDesc.Width ;
  if( pixelsPerSide != surfaces[0]->texDesc.Height )
  {
    error( "Trying to construct a cubemap out of non-square set of surfaces, truncation will occur" ) ;
    pixelsPerSide = min( surfaces[0]->texDesc.Width, surfaces[0]->texDesc.Height ) ; // min to prevent OOB
  }

  // Make enough room for 6 faces
  currentSource = colorValues = new vector<Vector>( 6*pixelsPerSide*pixelsPerSide ) ;
  int trow,tcol;

  for( int FACE = 0 ; FACE < 6 ; FACE++ )
  {
    D3D11Surface *surface = surfaces[ FACE ] ;
    surface->open() ;
    
    for( int row = 0 ; row < pixelsPerSide ; row++ )
    {
      for( int col = 0 ; col < pixelsPerSide ; col++ )
      {
        ByteColor *texel = surface->getColorAt( row, col ) ;
        remapLHToRH( FACE, row,col, trow,tcol ) ;
        (*colorValues)[ dindex(FACE,trow,tcol) ] = texel->toRGBAVector() ;
      } //each col
    } //each row

    surface->close() ;
  } // each face  
}

//ctor must:
//  set up pixelsPerSide
//  create colorValues and set currentSource
CubeMap::CubeMap( string iName, int px, Scene* scene )
{
  info( Green, "Creating cubemap '%s' from scene lights..", iName.c_str() ) ;
  if( ! threadPool.onMainThread() ) {
    error( "You must open a cubemap on the main thread" );
    return ;
  }

  if( !scene->lights.size() )
  {
    error( "No lights" ) ;
    return ;
  }
  defInit() ;

  name = iName ;
  pixelsPerSide = px ;
  
  if( rc->cubemapPixelDirs.find(px) == rc->cubemapPixelDirs.end() )
  {
    warning( "%d x %d not found, pregenning..", px,px ) ;
    pregenDirections( px ) ;
  }

  // create it, and set up currentSource while you're at it.
  currentSource = colorValues = new vector<Vector>( 6*px*px ) ;
  vector<ByteColor> byteColors( 6*px*px ) ;
  
  int FACE,row,col;
  // shoot rays 
  for( int i = 0 ; i < rc->cubemapPixelDirs[px].size() ; i++ )
  {
    Ray ray( Vector(0,0,0), rc->cubemapPixelDirs[px][i].c, 1000 ) ;
    Intersection* intn = 0 ;
    
    if( scene->getClosestIntn( ray, scene->lights, &intn ) )
    {
      // use the emissive color to color the texel.
      (*colorValues)[ i ] = intn->getColor( ColorIndex::Emissive ) ;

      // get the face,row,col and flip
      getFaceRowCol( i, FACE,row,col );
      
      int idx = dindex(FACE,row,col) ;
      byteColors[ idx ] = (*colorValues)[i].clamp(0,1).toByteColor() ;

      DESTROY( intn ) ;
    }
  }

  // Now since this IS a cubemap, and will be used for
  // wavelet lighting, I need to put one on the GPU just
  // to go with the rest of the code.

  // Create the cubemap via staging cubemap first,
  D3D11_TEXTURE2D_DESC cubeTexDesc = { 0 } ;
  cubeTexDesc.Width = px ;
  cubeTexDesc.Height = px ;
  cubeTexDesc.MipLevels = 1 ;
  cubeTexDesc.ArraySize = 6 ;
  cubeTexDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM ;
  cubeTexDesc.Usage  = D3D11_USAGE::D3D11_USAGE_DEFAULT ;
  // D3D11: ERROR: ID3D11Device::CreateTexture2D: Cubemaps do not support multisampling!
  // You must set SampleDesc.Count (value = 0) to 1 and SampleDesc.Quality (value = 1) to 0.
  // [ STATE_CREATION ERROR #93: CREATETEXTURE2D_INVALIDSAMPLES ]
  cubeTexDesc.SampleDesc.Count = 1;
  cubeTexDesc.SampleDesc.Quality = 0;
  cubeTexDesc.CPUAccessFlags = 0 ;
  cubeTexDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE ;
  cubeTexDesc.MiscFlags = D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_TEXTURECUBE ; // it's a texture cube.

  // create the texture cube
  D3D11_SUBRESOURCE_DATA subresData[6] ; // this has to be an ARRAY, one entry for each cubemap face
  for( int FACE = 0 ; FACE < 6 ; FACE++ )
  {
    subresData[FACE].pSysMem = (void*)&byteColors[FACE*px*px] ;
    subresData[FACE].SysMemPitch = 4 * px ; // 4 bpp * #pixels per row = distance in bytes from beginning of one line of texture to next line.
    subresData[FACE].SysMemSlicePitch = 4*px*px ; // bytes on surface.
  }

  // Copies in the data.
  DX_CHECK( window->d3d->CreateTexture2D( &cubeTexDesc, subresData, &cubeTex ), "create cubemap texture" ) ;

	D3D11_SHADER_RESOURCE_VIEW_DESC cubeTexRsvDesc;
	cubeTexRsvDesc.Format = cubeTexDesc.Format;
	cubeTexRsvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	cubeTexRsvDesc.TextureCube.MipLevels = cubeTexDesc.MipLevels;
	cubeTexRsvDesc.TextureCube.MostDetailedMip = 0; //always 0
  
  DX_CHECK( window->d3d->CreateShaderResourceView( cubeTex, &cubeTexRsvDesc, &cubeTexRsv ), "create shader res view cubetex" ) ;

  slices=stacks=64;
  createMesh( MeshType::Indexed, defaultVertexType ) ;
  //Icosahedron* ic = new Icosahedron( "cubemap sp", 100, Material(0,1,0,0,1) ) ;
  //Sphere *sp = Sphere::fromShape( ic, 100, 1 );
  //delete ic ;
  //sp->createMesh( MeshType::Indexed, defaultVertexType ) ;
  //addMesh( sp->meshGroup->meshes[0] ) ;
  
  
  createCubemapMesh() ;

  // Sh project the cubemap
  shProject( 1 ) ;
 
}

CubeMap::CubeMap( const CubeMap& o )
{
  info( "Cloning cubemap %s", o.name.c_str() ) ;

  // default memberwise assignment except for destructible assets,
  // when THIS object gets destroyed don't want original's assets clobbered
  *this = o ;

  // The VIRTUAL dtor actually caused this bug, because before it wasn't virtual
  // so the underlying meshdata awsn't being deconstructed ;)
  //this->cloneMeshesFrom( o ) ;
  meshGroup = o.meshGroup->clone( this ) ;

  if( o.railed )
    railed = new vector<Vector>( *o.railed ) ;
  if( o.colorValues )
    colorValues = new vector<Vector>( *o.colorValues ) ;
  if( o.rotatedColorValues )
    rotatedColorValues = new vector<Vector>( *o.rotatedColorValues ) ;
  currentSource = colorValues ; // (a redundant pointer)

  // If you want these you'll have to make them yourself
  // in your new object
  cubeTex = 0 ;
  cubeTexRsv = 0 ;
  viz = 0 ;
  shViz = 0 ;
}

CubeMap::~CubeMap()
{
  deleteEverythingExceptSparseData() ; // sparse data is local
}

void CubeMap::deleteEverythingExceptSparseData()
{
  // all the raw data gets deleted, and the sparse data is retained
  DESTROY( railed ) ;
  DESTROY( colorValues ) ;
  DESTROY( rotatedColorValues ) ;
  currentSource = 0 ; // (a redundant pointer)

  SAFE_RELEASE( cubeTex ) ;
  SAFE_RELEASE( cubeTexRsv ) ;

  // the shViz is the spherical harmonics viz, which is the one used in SH lighting.
  DESTROY( viz ) ;
  DESTROY( shViz ) ;
}

void CubeMap::defInit()
{
  name = "unnamed cubemap" ;
  isCubeMap = true ; // for base class Shape to know
  shapeType = ShapeType::CubemapShapeLight ;

  previousSource = currentSource = colorValues = rotatedColorValues = railed = 0 ;
  bilinear = true ;
  pixelsPerSide = 0 ;
  cubeTex = 0;
  cubeTexRsv = 0 ;
  viz=shViz=0;

  // sphere class members
  pos = Vector(0,0,0);
  slices = stacks = 32;
  radius = 100 ;
}


void CubeMap::visualizeAO( const Vector& normal )
{
  // draws you a picture of the ao
  real ao = 0 ;
  
  function<Vector (real tEl, real pAz)> colorFunction = [this](real tEl, real pAz) -> Vector {
    //return getColor( tEl, pAz, ColorIndex::Emissive ) ;
    Vector color = getColor( tEl, pAz, ColorIndex::Emissive ) ;
    if( color.all(0) )  // it's __OPEN__
    {
      color = 1 ;
    }
    else
    {
      color = Vector(0,0,0) ; // CLOSED, not setting ks.w=0
    }
    return color ;
  } ;
  function<real (real tEl, real pAz)> magFunction = [this](real tEl, real pAz) -> real {
    return radius ;
  } ;

  // make the drawable mesh.  you don't need to intersect with this,
  // but it is draw as a "visualization" WHEN this cubemap is active
  Mesh::visualizeCreateMesh( this, slices, stacks, MeshType::Indexed, VertexType::VT10NC10, 0, colorFunction, magFunction ) ;

  // create the colormap viz
  viz = Model::visualize( name + " cubem viz", slices, stacks, 0, colorFunction ) ;
  
}

void CubeMap::createCubemapMesh()
{
  function<Vector (real tEl, real pAz)> colorFunction = [this](real tEl, real pAz) -> Vector {
    return getColor( tEl, pAz, ColorIndex::Emissive ) ;
  } ;
  function<real (real tEl, real pAz)> magFunction = [this](real tEl, real pAz) -> real {
    return radius ;
  } ;

  // make the drawable mesh.  you don't need to intersect with this,
  // but it is draw as a "visualization" WHEN this cubemap is active
  /////////Mesh::visualizeCreateMesh( this, slices, stacks, MeshType::Indexed, VertexType::VT10NC10, 0, colorFunction, magFunction ) ;

  // create the colormap viz
  viz = Model::visualize( name + " cubem viz", slices, stacks, 0, colorFunction ) ;
  
}

void CubeMap::pregenDirections( int px )
{
  // if there is no entry for "px", allocate one
  // if there was, return
  map< int, vector<VectorPair> >::iterator p = rc->cubemapPixelDirs.find( px ) ;
  if( p == rc->cubemapPixelDirs.end() )
  {
    // didn't exist, allocate
    rc->cubemapPixelDirs.insert( make_pair( px, vector<VectorPair>() ) ) ;
    rc->cubemapPixelDirs[ px ].resize( 6*px*px ) ; // allocate enough space for 6*px*px vectors
  }
  else
  {
    // existed
    //warning( "%d cubemapPixelDirs already existed, returning", px ) ;
    return ;
  }

  for( int FACE = 0 ; FACE < 6 ; FACE++ )
  {
    for( int row = 0 ; row < px ; row++ )
    {
      for( int col = 0 ; col < px ; col++ )
      {
        int idx = (FACE*px*px) + (( row * px ) + col) ;

        // u and v are the fraction of the way I am
        // over in width and height
        real u = -1 + 2.0*(col + 0.5)/px ;
        real v = -1 + 2.0*(row + 0.5)/px ;
        real r = sqrt( 1 + u*u + v*v ) ;  // radial distance to the pixel on the cube

        // This gets me the direction of the cube map texel, based on which face.
        Vector dir = cubeFaceBaseConvertingFwd[FACE] + u*cubeFaceBaseConvertingRight[FACE] + v*cubeFaceBaseConvertingUp[FACE] ;
        dir.normalize() ;
        
        // put it in the collection
        rc->cubemapPixelDirs[ px ][ idx ] = dir ; // automatically makes and assigns sdir
        
        // So, we have a direction for the texel,
        // and we need to "integrate" this texel
        // into the spherical harmonic function.
        ////SVector sdir = dir.toSpherical() ;
        
      }// each col
    }// each row
  }// each FACE
}

// The scale parameter is not used
SHVector* CubeMap::shProject( const Vector& scale )
{
  if( shProjection ) { warning( "SH projection already existed, returning.." ) ; return shProjection ; }
  pregenDirections( pixelsPerSide ) ; // always call this which checks if exists or not

  // inherited from Sphere (which inherits it from Shape)
  shProjection = new SHVector() ;
  real WSum = 0.0 ;
  for( int FACE = 0 ; FACE < 6 ; FACE++ )
  {
    // "shoot a ray to each cube map texel center.."
    for( int row = 0 ; row < pixelsPerSide ; row++ )
    {
      //printf( "sh projecting face=%d, row=%d/%d     \r", FACE, row,pixelsPerSide ) ;
      for( int col = 0 ; col < pixelsPerSide ; col++ )
      {
        int texelIndex = (FACE*pixelsPerSide*pixelsPerSide) + (( row * pixelsPerSide ) + col) ;
        Vector& texel = (*colorValues)[ texelIndex ] ;
        
        // u and v are the fraction of the way I am
        // over in width and height
        real u = -1 + 2.0*(col + 0.5)/pixelsPerSide ;
        real v = -1 + 2.0*(row + 0.5)/pixelsPerSide ;
        real r = sqrt( 1 + u*u + v*v ) ;  // radial distance to the pixel on the cube

        // I think 4.0 is the TOTAL AREA of the square.
        real dW = 4.0/(r*r*r) ; // NOTE I thought the 4.0 here is wrong, but gets divided out later anyway
        WSum += dW ;
        
        // So, we have a direction for the texel, and we need to "integrate" this texel
        // into the spherical harmonic function.
        SHSample samp( window->shSamps->bands, rc->cubemapPixelDirs[pixelsPerSide][texelIndex].s ) ;
        shProjection->integrateSample( samp, dW, texel ) ;
      }// each col
    }// each row
  }// each FACE

  shProjection->scale( 4.0*PI / WSum ) ; // each had a different solid angle, so you have to use this sum.
  // The factor 4.0 cancels here.  (WSum is 4x as big as it would have been)

  shViz = shProjection->generateVisualization( 32, 32, 0 ) ;

  return shProjection ;
}

void CubeMap::writePx( ByteColor* dstPtr, int pxPerRow )
{
  for( int FACE = 0 ; FACE < 6 ; FACE++ )
  {
    int i,j;
    //      0    1    2    3
    //   +----+----+----+----+
    // 0 |    | +y |    |    |
    //   +----+----+----+----+
    // 1 | -x | +z | +x | -z |
    //   +----+----+----+----+
    // 2 |    | -y |    |    |
    //   +----+----+----+----+
    switch( FACE )
    {
      case CubeFace::PX: i=1,j=2; break ;
      case CubeFace::NX: i=1,j=0; break ;
      case CubeFace::PY: i=0,j=1; break ;
      case CubeFace::NY: i=2,j=1; break ;
      case CubeFace::PZ: i=1,j=1; break ;
      case CubeFace::NZ: i=1,j=3; break ;
    }

    //                      vertical skip         horizontal skip
    int baseAddress = i*pixelsPerSide*pxPerRow + (pixelsPerSide*j) ; 

    for( int row = 0 ; row < pixelsPerSide ; row++ )
    {
      for( int col = 0 ; col < pixelsPerSide ; col++ )
      {
        int indexL = baseAddress + (row*pxPerRow + col) ; // index in the large cubemap (baseAddress+subaddress)
      
        // grab the pixel from this cubemap
        dstPtr[ indexL ] = pxPoint( FACE, row, col ).clamp(0,1).toByteColor() ;
      }
    }
  }
}

void CubeMap::railedWritePx( ByteColor* dstPtr, int pxPerRow )
{
  // write the faces
  for( int FACE = 0 ; FACE < 6 ; FACE++ )
  {
    int i,j;
    //      0    1    2    3
    //   +----+----+----+----+
    // 0 |    | +y |    |    |
    //   +----+----+----+----+
    // 1 | -x | +z | +x | -z |
    //   +----+----+----+----+
    // 2 |    | -y |    |    |
    //   +----+----+----+----+
    switch( FACE )
    {
      case CubeFace::PX: i=1,j=2; break ;
      case CubeFace::NX: i=1,j=0; break ;
      case CubeFace::PY: i=0,j=1; break ;
      case CubeFace::NY: i=2,j=1; break ;
      case CubeFace::PZ: i=1,j=1; break ;
      case CubeFace::NZ: i=1,j=3; break ;
    }

    // DIFFERS FROM PLAIN OL' WRITE PIXEL
    //                      vertical skip         horizontal skip
    int baseAddress = i*pixelsPerSide*pxPerRow + (pixelsPerSide*j)
      // THEN MOVE DOWN _ANOTHER_ ROW AND 1 COL.
      //     ANOTHER ROW            ANOTHER COL
      +      pxPerRow        +      1 ;

    for( int row = 0 ; row < pixelsPerSide-1 ; row++ )
    {
      for( int col = 0 ; col < pixelsPerSide-1 ; col++ )
      {
        int indexL = baseAddress + (row*pxPerRow + col) ; // index in the large cubemap (baseAddress+subaddress)
      
        // grab the pixel from this cubemap
        dstPtr[ indexL ] = railedPxPoint( FACE, row, col ).clamp(0,1).toByteColor() ;
      }
    }
  }

  // write the rails
  // top rail
  // THESE ARE IMAGE BASE ADDRESSES!
  int baseAddressTopRail = pixelsPerSide*pxPerRow ; // the top rail starts at the top left corner of NX
  int baseAddressBotRail = 2*pixelsPerSide*pxPerRow ;
  for( int r = 0 ; r < railTopSize() ; r++ )
  {
    // top rail
    dstPtr[ baseAddressTopRail + r ] = railedPxPoint( CubeFace::RAIL, Rail::TOP_RAIL, r ).clamp(0,1).toByteColor() ;

    // bottom rail
    dstPtr[ baseAddressBotRail + r ] = railedPxPoint( CubeFace::RAIL, Rail::BOT_RAIL, r ).clamp(0,1).toByteColor() ;
  }


  // the sides
  for( int rail = 2 ; rail < 6 ; rail++ )
  {
    for( int r = 0 ; r < railSideSize() ; r++ )
    {
      //                  initial offset                   COL SHIFT                 ROW SHIFT
      int indexL = (pixelsPerSide+1)*pxPerRow +   (rail-2)*(pixelsPerSide)    +   r * pxPerRow ; // INDEX INTO __TEXTURE__ not (railed)
      dstPtr[ indexL ] = railedPxPoint( CubeFace::RAIL, rail, r ).clamp(0,1).toByteColor() ;
    }
  }

}

// bind the texture to the pipeline
void CubeMap::activate( int slotNo )
{
  if( !window->activeShader )
  {
    error( "No active shader" ) ;
    return ;
  }

  window->activeShader->setTexture( slotNo, cubeTexRsv ) ;
}

void CubeMap::downsample( int newPxPerSide )
{
  if( newPxPerSide > pixelsPerSide )
  {
    error( "newPxPerSide %d > pixelsPerSide %d, not downsampling", newPxPerSide, pixelsPerSide ) ;
    return;
  }
  else
    info( Gray, "Downsampling %s from %d x %d to %d x %d", name.c_str(), pixelsPerSide, pixelsPerSide, newPxPerSide, newPxPerSide ) ;

  real shF = pixelsPerSide / newPxPerSide ; // "shrinkFactor"

  int newPxPerSide² = newPxPerSide*newPxPerSide ;

  vector<Vector> * newColors = new vector<Vector>( 6*newPxPerSide² ) ;

  setSource( colorValues ) ; // make sure we're using the original source

  // go thru every pixel of old thang
  for( int FACE = 0 ; FACE < 6 ; FACE++ )
  {
    for( int row = 0 ; row <= pixelsPerSide - shF ; row += shF )
    {
      for( int col = 0 ; col <= pixelsPerSide - shF ; col += shF )
      {
        Vector sumPx ;
        for( int i = 0 ; i < shF ; i++ )
          for( int j = 0 ; j < shF ; j++ )
            sumPx += pxPoint( FACE, row+i, col+j ) ;

        // The indexing here must use
        // pixels per side of the NEW face
        (*newColors)[ CubeMap::index( newPxPerSide, FACE, row/shF, col/shF ) ] = sumPx / (shF*shF) ;
      }
    }
  }

  // now overwrite members
  pixelsPerSide = newPxPerSide ;
  DESTROY( colorValues ) ;
  colorValues = newColors ;
  
  setSource( colorValues ) ; // the old source is dead
}

void CubeMap::compress()
{
  if( FullCubeRenderer::useRailedQuincunx )
  {
    railedMake() ;
    railedCompress( FullCubeRenderer::compressionLightmap ) ;
  }
  else
  {
    // compress it.
    faceCompressRotated( FullCubeRenderer::compressionLightmap ) ; // no sense in dealloc/realloc
  }
}

// compresses using face-based technique
// __mutilates__ current source.
int CubeMap::faceCompress( real threshold )
{
  // track average compression
  compressCumulativeProcessed += currentSource->size() ;

  for( int FACE = 0 ; FACE < 6 ; FACE++ )
  {
    // Do each face separately
    int startIndex = dindex( FACE, 0, 0 ) ;

    // take the 2d image starting at startIndex, for pixelsPerSide*pixelsPerSide elements.
    //compresshere
    Wavelet::transform2d<Vector>( Wavelet::Haar, &(*currentSource)[startIndex], pixelsPerSide, pixelsPerSide ) ;
    //Wavelet::transform2dnonstd<Vector>( Wavelet::Haar, &(*currentSource)[startIndex], pixelsPerSide, pixelsPerSide ) ;
    //Wavelet::latticeLift<Vector>( &(*currentSource)[startIndex], pixelsPerSide, pixelsPerSide ) ;
  }

  // This filters the entire thing
  compressCumulativeRemoved += Wavelet::filterNearZero( &(*currentSource)[0], currentSource->size(), threshold ) ;
  //compressCumulativeRemoved += Wavelet::filterPercentage2d( &(*currentSource)[0], pixelsPerSide, pixelsPerSide, threshold ) ;
  //compressCumulativeRemoved += Wavelet::filterPercentage( &(*currentSource)[0], currentSource->size(), 1, threshold ) ;
  
  
  // clear this out, if it existed
  sparseTransformedColor = Eigen::SparseVector<Vector>() ;
  //sparseM.clear() ;
  
  // now store that in a Eigen::Sparse, which will save memory by only keeping the non-zero elements.
  // insert only the non-zero elements
  for( int i = 0 ; i < currentSource->size() ; i++ )
    if( (*currentSource)[i].notAll( 0 ) ) // all 3 must be 0 to get a 0 elt. Doesn't matter if did rgb breakup, because multiplication would become more complicated.
    {
      sparseTransformedColor.insert( i ) = (*currentSource)[ i ] ;
      //sparseM.insert( i, (*currentSource)[i] ); 
    }


  //info( Magenta, "sparse color has %d elements", sparseTransformedColor.nonZeros() ) ;
  //printf( "sparse color has %d elements, size=%d\n", sparseTransformedColor.nonZeros(), sparseTransformedColor.size() ) ;
  compressCumulativeKept += sparseTransformedColor.nonZeros() ;
  return sparseTransformedColor.nonZeros() ; // return number of components KEPT
  //return sparseM.vals.size() ; // return number of components KEPT
}

int CubeMap::faceCompressOriginal( real threshold, bool keepSource )
{
  if( !colorValues ){ error( "Original doesn't exist, can't compress" ) ; return 0 ; }
  
  int kept ;
  if( keepSource )
  {
    // You have to lift a clone.
    info( "compressing clone, keeping original.." ) ;
    vector<Vector> clone( *colorValues ) ;
    setSource( &clone ) ;
    kept = faceCompress( threshold ) ; // compress the clone.
  }//clone dies
  else
  {
    setSource( colorValues ) ;
    kept = faceCompress( threshold ) ;
    DESTROY( colorValues ) ; // not keeping source
  }
  
  revertToPreviousSource() ;
  return kept ;
}

int CubeMap::faceCompressRotated( real threshold )
{
  if( !rotatedColorValues ){ error( "Rotated doesn't exist, can't compress" ) ; return 0 ; }

  // Here I am going to just lift the rotated function,
  // and the rotated function WILL be destroyed (done in place),
  // but we don't care about that.
  setSource( rotatedColorValues ) ;
  
  int kept = faceCompress( threshold ) ;

  revertToPreviousSource() ;

  return kept ;
}

int CubeMap::railedCompress( real threshold )
{
  // track average compression
  compressCumulativeProcessed += currentSource->size() ;

  // railed compression is done in one go
  Wavelet::latticeLift<Vector>( &(*currentSource)[0], FullCubeRenderer::indexMap ) ;
  
  // This filters the entire thing
  compressCumulativeRemoved += Wavelet::filterNearZero( &(*currentSource)[0], currentSource->size(), threshold ) ;
  //compressCumulativeRemoved += Wavelet::filterPercentage2d( &(*currentSource)[0], pixelsPerSide, pixelsPerSide, threshold ) ;
  //compressCumulativeRemoved += Wavelet::filterPercentage( &(*currentSource)[0], currentSource->size(), 1, threshold ) ;
  
  // clear this out, if it existed
  sparseTransformedColor = Eigen::SparseVector<Vector>() ;
  //sparseM.clear() ;
  
  // now store that in a Eigen::Sparse, which will save memory by only keeping the non-zero elements.
  // insert only the non-zero elements
  for( int i = 0 ; i < currentSource->size() ; i++ )
    if( (*currentSource)[i].notAll( 0 ) ) // all 3 must be 0 to get a 0 elt. Doesn't matter if did rgb breakup, because multiplication would become more complicated.
      sparseTransformedColor.insert( i ) = (*currentSource)[ i ] ;
      //sparseM.insert( i, (*currentSource)[i] ); 

  //info( Magenta, "sparse color has %d elements", sparseTransformedColor.nonZeros() ) ;
  compressCumulativeKept += sparseTransformedColor.nonZeros() ; // more parity than anything
  return sparseTransformedColor.nonZeros() ; // return number of components KEPT
  //return sparseM.vals.size() ; // return number of components KEPT
}

// CONVENIENCE FUNCTION to compress
// the railed lattice.
int CubeMap::railedCompress( real threshold, bool keepSource )
{
  if( !railed ){ error( "RAILED doesn't exist, can't compress" ) ; return 0 ; }
  
  int kept ;
  if( keepSource )
  {
    // You have to lift a clone.
    info( "compressing clone, keeping original.." ) ;
    vector<Vector> clone( *railed ) ;
    setSource( &clone ) ;
    kept = railedCompress( threshold ) ; // compress the clone.
  }//clone dies
  else
  {
    setSource( railed ) ;
    kept = railedCompress( threshold ) ;
    DESTROY( railed ) ; // not keeping source
  }
  
  revertToPreviousSource() ;
  return kept ;
}

void CubeMap::faceUncompress( vector<Vector>* uncompressed )
{
  warning( "Cubemap is being uncompressed" ) ; // we don't usually want this
  
  if( !isCompressed() )
  {
    error( "No compressed data to uncompress, nothin' doin'" ) ;
    return ;
  }

  // I need uncompressed to be big enough to fit
  // the uncompressed vector
  uncompressed->resize( 6 * pixelsPerSide * pixelsPerSide ) ;

  // write in the non-zero elts, at their correct indices
  for( Eigen::SparseVector<Vector>::InnerIterator it( sparseTransformedColor ); it; ++it )
    (*uncompressed)[ it.index() ] = it.value() ; //printf( "v1[ %d ] = %f\n", it.index(), it.value() ) ;
   
  // now you can uncompress them, using whatever cubemap-face-based method you want.
  for( int FACE = 0 ; FACE < 6 ; FACE++ )
  {
    int startIndex = dindex( FACE, 0, 0 ) ;
    Wavelet::untransform2d<Vector>( Wavelet::Haar, &(*uncompressed)[startIndex], pixelsPerSide, pixelsPerSide ) ;
    //Wavelet::unlatticeLift<Vector>( &(*uncompressed)[startIndex], pixelsPerSide, pixelsPerSide ) ;
  }
}

void CubeMap::railedUncompress( vector<Vector>* uncompressed )
{
  //warning( "Railed cubemap is being uncompressed" ) ; // we don't usually want this
  
  if( !isCompressed() )
  {
    error( "No compressed data to uncompress, nothin' doin'" ) ;
    return ;
  }

  uncompressed->resize( railSize() ) ;

  // write in the non-zero elts, at their correct indices
  for( Eigen::SparseVector<Vector>::InnerIterator it( sparseTransformedColor ); it; ++it )
    (*uncompressed)[ it.index() ] = it.value() ; //printf( "v1[ %d ] = %f\n", it.index(), it.value() ) ;
  
  // The unlatticeLift method actually doesn't need to know
  // if your cubemap is railed or not.. it just visits indices.
  Wavelet::unlatticeLift<Vector>( &(*uncompressed)[0], FullCubeRenderer::indexMap ) ;
}

void CubeMap::setSource( vector<Vector> *newSource )
{
  if( !newSource )
    warning( "CubeMap: Setting NULL source!" ) ;
  previousSource = currentSource ;
  currentSource = newSource ;
}
  
// revert to the last used source
void CubeMap::revertToPreviousSource()
{
  currentSource = previousSource;
}

void CubeMap::faceSave( char* filename )
{
  if( !currentSource ){ error( "No face to save" ) ; return ; }

  // before saving, you should make sure alpha's are all 1, to avoid blackout images
  //for( int i = 0 ; i < currentSource->size() ; i++ )
  //  (*currentSource)[i].w = 1;
  //  (*currentSource)[i].clamp(0,1) ;// also clamp it

  // save the cubemap to file,
  int w = 4*pixelsPerSide ;
  int h = 3*pixelsPerSide ;

  // make a texture
  D3D11Surface surf( window, 0,0,  w,h,  0,1,  0.01, SurfaceType::ColorBuffer,  0,DXGI_FORMAT_R8G8B8A8_UNORM, 4 ) ;
  
  ByteColor* b = surf.clear( Vector(1,0,1).toByteColor() ) ;
  
  writePx( b, surf.getPxPerRow() ) ;

  surf.close() ;
  surf.save( filename ) ;
}

void CubeMap::railedSave( char* filename )
{
  if( !currentSource ) { error( "No source to save" ) ; return ; }
  
  int w = 4*pixelsPerSide ;
  int h = 3*pixelsPerSide ; // there is some empty space, but leave it

  // make a texture
  D3D11Surface surf( window, 0,0,  w,h,  0,1,  0.01, SurfaceType::ColorBuffer,  0,DXGI_FORMAT_R8G8B8A8_UNORM, 4 ) ;
  
  // clear everything
  ByteColor* b = surf.clear( Vector(1,0,1).toByteColor() ) ; // does nothing since when you close dynamic surface work is lost.

  // write
  railedWritePx( b, surf.getPxPerRow() ) ;

  surf.close() ;
  surf.save( filename ) ;
}

void CubeMap::faceSaveWaveletRepn( char* filename )
{
  if( !sparseTransformedColor.nonZeros() ){ error( "No compressed" ) ; return ; }

  // write the values in but don't uncompress it.
  vector<Vector> compressed ;

  // copied from "uncompress()"
  compressed.resize( 6 * pixelsPerSide * pixelsPerSide, Vector(0,0,0,1) ) ;// fill 0 (opaque black)
  for( Eigen::SparseVector<Vector>::InnerIterator it( sparseTransformedColor ); it; ++it )
    compressed[ it.index() ] = it.value() ;
  
  setSource( &compressed ) ;
  faceSave( filename ) ;
  revertToPreviousSource() ;
}

void CubeMap::railedSaveWaveletRepn( char* filename )
{
  if( !sparseTransformedColor.nonZeros() ){ error( "No compressed" ) ; return ; }

  // write the values in but don't uncompress it.
  vector<Vector> compressed ;

  // copied from "uncompress()"
  compressed.resize( railSize(), Vector(0,0,0,1) ) ;// fill 0 (opaque black)
  for( Eigen::SparseVector<Vector>::InnerIterator it( sparseTransformedColor ); it; ++it )
    compressed[ it.index() ] = it.value() ;
  
  setSource( &compressed ) ;
  railedSave( filename ) ;
  revertToPreviousSource() ;
}

void CubeMap::faceSaveUncompressed( char* filename )
{
  if( !sparseTransformedColor.nonZeros() ){ error( "No compressed" ) ; return ; }

  // you must uncompress the data first
  vector<Vector> uncompressed ;
  faceUncompress( &uncompressed ) ; // get the wavelet data and uncompress it
  setSource( &uncompressed ) ;
  faceSave( filename ) ;
  revertToPreviousSource() ;
}

void CubeMap::railedSaveUncompressed( char* filename )
{
  if( !sparseTransformedColor.nonZeros() ){ error( "No compressed" ) ; return ; }

  // you must uncompress the data first
  vector<Vector> uncompressed ;
  railedUncompress( &uncompressed ) ; // get the wavelet data and uncompress it
  setSource( &uncompressed ) ;
  railedSave( filename ) ;
  revertToPreviousSource() ;
}

void CubeMap::faceSaveRotated( char* filename )
{
  // save the rotated version
  if( !rotatedColorValues )
  {
    error( "No rotatedColorValues to save!" ) ;
    return ;
  }
  setSource( rotatedColorValues ) ;
  faceSave( filename );
  revertToPreviousSource() ;
}

void CubeMap::draw()
{
  window->shaderCubemap->activate() ;
  activate( 2 ) ;  // the cubeTex is always bound on slot 2
  
  Shape::draw() ;  // invoke base draw
}

#if 0
// the code calling this ctor must have already
// read the header, because that's how it knew to
// make a cubemaplight.
CubeMap::CubeMap( const HeaderLight& head, FILE* file ) : Light( head, file )
{
  // last bit of tie-up
  ///lightFunction = [this]( real tEl, real pAz )->Vector{
  ///  return (*shProjection)(tEl, pAz) ;
  ///};
}

int CubeMap::save( FILE* file ) //override
{
  HeaderLight head ;
  fillDefaults( head ) ;
  
  head.lightFunctionNumber = 0 ;  // NO LIGHT FUNCTION/IRRELEVANT. a cube map won't use
  // one of the math light functions, but it will use the SHPROJECTION as the light function
  // instead.

  head.lightType = LightType::CubemapLight ;

  int written = 0 ;
  written += sizeof(HeaderLight)*fwrite( &head, sizeof(HeaderLight), 1, file ) ;
  Light::save( file ) ;

  return written ;
}
#endif

void CubeMap::prelight( const Vector& vnormal )
{
  real dotSum = 0.0 ;

  // I want this to be done on the gpu, but
  // i need to get the row, col of the fragment at rasterization time (because pos on cube changes its form factor)
  for( int FACE = 0; FACE < 6; FACE++ )
  {
    // this vector stays the same for all the rows, cols on this face.
    Vector& cubeFaceFwd = cubeFaceBaseConvertingFwd[ FACE ] ;
    //////Vector& cubeFaceFwd = cubeFaceBaseRHFwd[ FACE ] ;

    for( int row=0 ; row < pixelsPerSide; row++ )
    {
      for( int col=0 ; col < pixelsPerSide; col++ )
      {
        int m = dindex( FACE, row, col ) ;

        // use the solid angle and dot of each texel
        // to determine weight of this cubemap light.

        Vector cubeMapLightDir = getDirectionConvertedLHToRH( FACE, row, col ) ;
        ////////Vector cubeMapLightDir = getDirectionRH( FACE,row,col ) ;
        
        real costi = cubeMapLightDir.Dot( vnormal ) ;

        // discard condition, full alpha means 100% blockage.
        const static Vector blockedColor( 0,0,0,1 ) ; // I make alpha __1__, because
          // alpha is NOT involved in computations, and the only side effect is
          // that the images are visible when we save them.
        
        if( costi < 0 ) { (*colorValues)[m] = blockedColor ; continue ; } // no light contribution form this vertex, it's in a backwards direction.
        ////dotSum += dot ; // not using the dotSum

        // if pixel was rendered to, then it was occluded
        if( (*colorValues)[ m ].anyGreaterThan( 0 ) ) // only considers rgb, actually should only consider alpha (if it was cleared to 0)
        {
          // there was something rendered here, so, it's 0 actually
          // what REALLY can happen here is partial translucency
          // in RGB, but I'm not modelling that now.
          (*colorValues)[m] = blockedColor ;
        }
        else
        {
          // It was not blocked, so save the form factor here
          // if the vector is coming from a face on the "front" side
          // of the normal
          real costj = cubeFaceFwd.Dot( cubeMapLightDir ) ;

          if( costj < 0 )  error( "costj (%f) < 0", costj ) ;
          
          real dA = 4.0 / (pixelsPerSide*pixelsPerSide) ;
          real u = -1 + 2.0*(col+0.5) / pixelsPerSide ;  // horizontal
          real v = -1 + 2.0*(row+0.5) / pixelsPerSide ;  // VERTICAL ie rows

          real r² = u*u+v*v+1 ; // square length of vector to pt (u,v,1)
          
          real dF = ( costi * costj * dA ) / (PI*r²) ; // correct value .. THIS IS INVISIBLE IF YOU SAVE THE VALUE BECAUSE IT'S SO CLOSE TO 0.
          //real dF = costi * costj ; // JUST TO SEE, BECAUSE ABOVE IS ALWAYS INVISIBLE IF SAVED
          //printf( "dF = %f             \r", dF ) ;
          
          (*colorValues)[m] = Vector(dF,dF,dF,1) ; // MAKES A VECTOR.
          
        }
      }//col
    }//row
  }//FACE
        

  // colorValues should sum to 1.0 MAXIMUM (if they are completely open)
  // They never are really bigger than 1, usually
  // about 1.0013 when they are bigger, so it's acceptable error
  ////real s = 0.0 ;
  ////for( int i = 0 ; i < colorValues->size() ; i++ )
  ////  s += (*colorValues)[i].x ;
  ////if( s <= 1.0 )
  ////  info( Cyan, "Sum was good %f", s ) ;
  ////else
  ////  error( "Sum was %f", s ) ;
}

Vector CubeMap::getColor( real tElevation, real pAzimuth, ColorIndex colorIndex )
{
  // for a cubemap, ColorIndex doesn't matter
  // (but really, it has an emissive component only)
  //if( colorIndex == ColorIndex::Emissive )
  
  return px( SVector( 1.0, tElevation, pAzimuth ).toCartesian() ) ; // this will be
  // a bilinear interpolation
  //else
  //  return 0 ; // cubemap has no specular, no diffuse
}

void CubeMap::wrap( int &face, int &row, int &col )
{
  // both negative, you exitted on both sides
  if( row < 0 && col < 0 )
  {
    // flip the sign of the smaller magnitude one,

    // if they are THE SAME, then you map to a seam.
    if( row == col )
      col = 0 ;  // drop one to 0
    else if( row < col ) // row is BIGGER (mag)
      col = -col ; // flip
    else
      row = -row ;
  }

  // if we walked off the +side,
  // bring the smaller one within bounds
  if( row >= pixelsPerSide && col >= pixelsPerSide )
  {
    if( row == col ) // seam
      row = pixelsPerSide - 1 ;
    else if( row > col ) // row is bigger (mag)
      col = 2*pixelsPerSide - col ;
    else // leave col b/c it's bigger
      row = 2*pixelsPerSide - row ;
  }

  // NegSmallSide2BigSide(x) (pixelsPerSide+x)
  // BigSide2BigSide(x) (2*pixelsPerSide - x - 1)
  // BigSide2SmallSide(x) (x-pixelsPerSide)
  // NegSmallSide2SmallSide(x) ( -x - 1 )

  #define NSS2BS(x) (pixelsPerSide+(x))
  #define BS2BS(x) (2*pixelsPerSide - (x) - 1)
  #define BS2SS(x) ((x)-pixelsPerSide)
  #define NSS2SS(x) ( (-x) - 1 )
  #define FLIPSIDE(x) (pixelsPerSide - (x) - 1)
  // because of the check above,
  // only one will be OOB.
  #define SET(f,x,y) face=f,row=x,col=y
  int orow=row, ocol=col ; // convenience temporaries, so you don't have to see
  // swap temporaries in the code below.
  switch( face )
  {
    case CubeFace::PX:
      if( row < 0 ) // EXIT_TOP
        SET( PY,FLIPSIDE(ocol),NSS2BS(orow) );
      else if( row >= pixelsPerSide ) // EXIT_BOTTOM
        SET( NY, ocol, BS2BS(orow) ) ;
      else if( col < 0 ) // EXIT_LEFT
        SET( PZ, orow, NSS2BS(ocol) ) ;
      else if( col >= pixelsPerSide ) // EXIT_RIGHT
        SET( NZ, orow, BS2SS( ocol ) ) ;
      break;

    case CubeFace::NX:
      if( row < 0 )
        SET( PY, ocol, NSS2SS( orow ) );
      else if( row >= pixelsPerSide )
        SET( NY, FLIPSIDE(ocol), BS2SS( orow ) );
      else if( col < 0 )
        SET( NZ, orow, NSS2BS( ocol ) );
      else if( col >= pixelsPerSide )
        SET( PZ, orow, BS2SS( ocol ) );
      break;

    case CubeFace::PY:
      if( row < 0 )
        SET( NZ, NSS2SS(orow), FLIPSIDE(ocol) ) ;
      else if( row >= pixelsPerSide )
        SET( PZ, BS2SS(orow), ocol ) ;
      else if( col < 0 )
        SET( NX, NSS2SS(ocol), orow ) ;
      else if( col >= pixelsPerSide )
        SET( PX, BS2SS(ocol), FLIPSIDE(orow) ) ;
      break;

    case CubeFace::NY:
      if( row < 0 )
        SET( PZ, NSS2BS(orow), ocol ) ;
      else if( row >= pixelsPerSide )
        SET( NZ, BS2BS(orow), FLIPSIDE(ocol) ) ;
      else if( col < 0 )
        SET( NX, NSS2BS(ocol), FLIPSIDE(orow) ) ;
      else if( col >= pixelsPerSide )
        SET( PX, BS2BS(ocol), orow ) ;
      break;
    
    case CubeFace::PZ:
      if( row < 0 )
        SET( PY, NSS2BS(orow), ocol ) ;
      else if( row >= pixelsPerSide )
        SET( NY, BS2SS(orow), ocol ) ;
      else if( col < 0 )
        SET( NX, orow, NSS2BS(ocol) ) ;
      else if( col >= pixelsPerSide )
        SET( PX, orow, BS2SS(ocol) ) ;
      break;

    case CubeFace::NZ:
      if( row < 0 )
        SET( PY, NSS2SS(orow), FLIPSIDE(ocol) ) ;
      else if( row >= pixelsPerSide )
        SET( NY, BS2BS(orow), FLIPSIDE(ocol) ) ;
      else if( col < 0 )
        SET( PX, orow, NSS2BS( ocol ) ) ;
      else if( col >= pixelsPerSide )
        SET( NX, orow, BS2SS(ocol) ) ;
      break;

    default:
      error( "Invalid cubemap face" ) ;
      break ;
  }
}

int CubeMap::getWrappedIndex( int face, int row, int col )
{
  wrap( face, row, col ) ; // wrap them first
  return dindex( face, row, col ) ; // return the index
}

#pragma region railed functions

// ASSUMES INDEX SAFE (ie no negatives or oob.. it has been wrapped)
int CubeMap::railedIndex( int face, int rowOrRail, int colOrAlong )
{
  if( face == CubeFace::RAIL )
    // you're on the rail.
    // the row is really the rail#
    return railOffsetTo( rowOrRail ) + colOrAlong ; // ADD COL for the additional offset
  else // here you're on a face.
    return face * (pixelsPerSide - 1)*(pixelsPerSide - 1)   +   rowOrRail*(pixelsPerSide - 1) + colOrAlong ;
}

int CubeMap::railedIndexMove( int face, int row, int col, int rowOffset, int colOffset )
{
  // first move/wrap.
  //printf( "%d %d %d (%d,%d) -> ", face,row,col, rowOffset,colOffset ) ;
  railedMove( face, row, col, rowOffset, colOffset ) ;
  //printf( "%d %d %d\n", face,row,col ) ;

  // then give index
  return railedIndex( face, row, col ) ;
}

int CubeMap::railedGetFaceBesideRail( int along )
{
  int railSection = (along / pixelsPerSide) ;

  switch( railSection )
  {
    case 0: return NX ; // NX is beside section 0 (between 0 and (pixelsPerSide-1) of the top and bottom rails)
    case 1: return PZ ;
    case 2: return PX ;
    case 3: return NZ ;

    default:
      error("BAD RAIL" ) ;
      return -1 ;
  }
}

// Gets you the "along" rail offset base address
// for a face.
// Does the opposite of railedGetFaceNearSection
// ADD 2 TO THIS VALUE TO GET THE Rail::LNX etc TO THE
// LEFT OF YOUR FACE.
int CubeMap::railedGetAlongTopRailBaseOffset( int face )
{
  switch( face )
  {
    case NX: return 0 ; // for the NX face, starting at address 0 of the top
      // /bot rails will let you walk beside the nx face.
    
      // I expect you use these responsibly.  If you leave
      // TOP PY you go to a much further offset (near NZ).
    case PZ: 
    case PY:// these are actually aligned with PZ, BOTTOM PY
    case NY:// these are actually aligned with PZ, TOP NY
      return 1 ;

    case PX: return 2 ;
    case NZ: return 3 ;

    default:
      error( "BAD FACE" ) ;
      return -1 ;
  }
}

// IF FACE IS -1 then you are still on the rail
// if FACE is one of the cubemap faces then
// rail=row and along=col.
void CubeMap::railedMoveNWrapRail( int &face, int &rail, int &along, int rowOffset, int colOffset )
{
  if( !rowOffset && !colOffset )
  {
    error( "You are calling step off with 0 offsets" ) ;
    return ;
  }

  if( rail == Rail::TOP_RAIL || rail == Rail::BOT_RAIL )
  {
    // APPLY HORIZONTAL MOTION RIGHT NOW
    if( colOffset )
      along = ( (along + colOffset) + railTopSize() ) % railTopSize() ; // move over left/right ON the rail

    // now apply vertical motion.
    if( !rowOffset )
      return ;// nothing else to do
    
    // if you are a corner, you go down a side rail.
    if( along % pixelsPerSide == 0 ) // CORNER (0, N, 2N, 3N)
    {
      int cornerNumber = along / pixelsPerSide ;
      
      if( rail == Rail::TOP_RAIL )
      {
        if( rowOffset > 0 ) // top rail going DOWN
        {
          // ENTER A SIDERAIL
          rail = cornerNumber+2 ;// LNX rail is Rail=2, and LNX aligns with corner 0.
          // I know this is going to be confusing to look at later, but it makes perfect
          // sense if you draw the diagram ("lattice layout diagram.png" in dropbox).
          along = rowOffset - 1 ; // how far into the rail you are is exactly the row offset -1
        }
        else
        {
          // KLUDGE. Change going up ==> GOING LEFT because a corner only has 3 neighbours.
          // call this function again with new parameters, and end.
          railedMoveNWrapRail( face, rail, along, 0, -rowOffset ) ;
        }
      }
      else if( rail == Rail::BOT_RAIL )
      {
        if( rowOffset < 0 )
        {
          // ENTER SIDERAIL
          rail = cornerNumber+2 ;
          along = railSideSize() + rowOffset ; // minus from the end. if it's -1, you end up at the last entry of the siderail.
        }
        else
          railedMoveNWrapRail( face, rail, along, 0, rowOffset ) ; // for botrail, go right.
      }
    }
    else
    {
      // NOT A CORNER.
      // you are leaving the top/bottom rail
      // and entering a FACE.
      int& row = rail ;  // re-alias these
      int& col = along ; // re-alias these
      // need the section of rail you are on.

      int CALong = ( along % pixelsPerSide ) - 1 ; // "centered along",
      // CALong is the measure of how far you are along a rail,
      // centered between the 2 corners you are in between.
        
      if( rail == Rail::TOP_RAIL )
      {
        if( rowOffset < 0 )
        {
          // TOP RAIL GOING UP enters PY
          face = CubeFace::PY ;

          if( along < pixelsPerSide ) // Will NOT be == pixelsPerSide due to corner check
          {
            // You are entering the LEFT side of PY
            row = CALong ;
            col = -(rowOffset+1) ;
          }
          else if( along < 2*pixelsPerSide )
          {
            // BOTTOM PY
            col = CALong ;
            row = (pixelsPerSide-1) + rowOffset ; // walk into the bottom side of the face
          }
          else if( along < 3*pixelsPerSide )
          {
            // RIGHT PY
            row = (pixelsPerSide-1) - CALong - 1 ; // the -1 is CALONG has index 0 at the corner.
            col = (pixelsPerSide-1) + rowOffset ;
          }
          else
          {
            // TOP PY
            row = -(rowOffset+1) ;
            col = (pixelsPerSide-1) - CALong - 1 ;//FLIP
          }
        }
        else // rowOffset > 0
        {
          // enter the side faces,
          // for all of these the column in CALong,
          // and the row is just rowOffset
          //printf( "TOP RAIL along=%d => ", along );
          if( along < pixelsPerSide )  face = NX ;
          else if( along < 2*pixelsPerSide )  face = PZ ;
          else if( along < 3*pixelsPerSide )  face = PX ;
          else  face = NZ ;
          
          col = CALong ;
          row = rowOffset-1 ; // minus 1 because you are stepping into _0_, not 1

          //printf( "face=%d, row=%d, col=%d\n", face,row,col );
        }
      }
      else if( rail == Rail::BOT_RAIL )
      {
        if( rowOffset > 0 )
        {
          // BOTTOM RAIL GOING DOWN enters NY
          face = NY ;
          if( along < pixelsPerSide )
          {
            // ENTER LEFT NY
            row = (pixelsPerSide - 1) - CALong - 1 ;
            col = rowOffset - 1 ;
          }
          else if( along < 2*pixelsPerSide )
          {
            // ENTER TOP NY
            row = rowOffset - 1 ;
            col = CALong ;
          }
          else if( along < 3*pixelsPerSide )
          {
            // ENTER RIGHT SIDE NY
            row = CALong ;
            col = (pixelsPerSide-1) - rowOffset ;
          }
          else
          {
            // ENTER BOTTOM SIDE NY
            row = (pixelsPerSide-1) - rowOffset ;
            col = (pixelsPerSide-1) - CALong - 1 ;
          }
        }
        else // rowOffset < 0
        {
          // BOTTOM RAIL GOING UP
          
          // enter the side faces,
          // for all of these the column in CALong,
          // and the row is just rowOffset
          if( along < pixelsPerSide )  face = NX ;
          else if( along < 2*pixelsPerSide )  face = PZ ;
          else if( along < 3*pixelsPerSide )  face = PX ;
          else  face = NZ ;

          col = CALong ; // col is aliased with along
          row = (pixelsPerSide-1) + rowOffset ;
        }
      }// END BOT/TOP RAILS
    }// NOT A CORNER
  }// END BOT/TOP RAILS
  // Easy case #2: you are on the side rails and are moving up or down
  // (YOU MAY MOVE ONTO THE TOP OR BOTTOM RAILS!)
  else //DEFINITELY A SIDE RAIL, if( IsSideRail(rail) )
  {
    if( rowOffset ) // there is a row offset
    {
      // up/down movement first
      along += rowOffset ;

      if( along < 0 ) // you entered the top rail
      {
        // the along offset now changes according to which side rail you were FROM.
        int alongOvershoot = along+1 ; // if this is 0 you are ON THE CORNER

        // need the along offset that lines up with this siderail.
        along = pixelsPerSide * (rail-2) ;
        
        rail = Rail::TOP_RAIL; // LAST

        // if along is exactly -1, then you are
        // at the corner.
        // if it is MORE negative than -1, you
        // are walking left along the rail.
        if( alongOvershoot ){
          railedMoveNWrapRail( face, rail, along, alongOvershoot, colOffset ) ;
          return ;
        }
        else if( colOffset ){ // there was no along overshoot, but still must apply colOffset
          railedMoveNWrapRail( face, rail, along, 0, colOffset ) ;
          return ;
        }
      }
      else if( along >= pixelsPerSide-1 )
      {
        int alongOvershoot = pixelsPerSide-1 - along ; // if this is >0, then you have someone going
        // further into botrail.

        along = (pixelsPerSide * (rail-2)) ;// GETS YOU TO THE CORNER BELOW THIS SIDE RAIL
          ///+along%(pixelsPerSide-1)) % railTopSize() ; // in case it goes over

        // you entered the bottom rail.
        rail = Rail::BOT_RAIL; //LAST
        
        if( alongOvershoot ){
          railedMoveNWrapRail( face, rail, along, alongOvershoot, colOffset ) ;
          return ;
        }
        // now, IF THERE IS ALSO A COLUMN OFFSET,
        // call this again (because you are now BOT_RAIL)
        else if( colOffset ){
          railedMoveNWrapRail( face, rail, along, 0, colOffset ) ;
          return ;
        }
      }
      else
      {
        // otherwise the offset stays on the same rail. nothing to add here.
      }

    }

    if( colOffset )
    {
      int& col = along ;
      int& row = rail ; // entering a face, so row is rail.

      // ASSIGN THE FACE BEFORE MODIFYING ROW/RAIL
      switch( rail )
      {
      case Rail::LNX_RAIL:
        if( colOffset < 0 )  face = NZ ;
        else face = NX ;
        break;
      case Rail::LPZ_RAIL:
        if( colOffset < 0 )  face = NX ;
        else face = PZ ;
        break;
      case Rail::LPX_RAIL:
        if( colOffset < 0 )  face = PZ ;
        else face = PX ;
        break;
      case Rail::LNZ_RAIL:
        if( colOffset < 0 )  face = PX ;
        else face = NZ ;
        break;
      }
 
      // here there is a column offset on a side rail, so you enter a face.
      row = along ;  // They all have row=along.
      if( colOffset < 0 )
        col = (pixelsPerSide-1) + colOffset ; // enter from the right side
      else
        col = colOffset - 1 ; // just enter the face by offset pixels - 1 (because start at index0)

   }
  }
}

void CubeMap::railedMoveNWrapFace( int &face, int &row, int &col, int rowOffset, int colOffset )
{
  // wrapping the face.
  
  // SINCE WE KNOW WE START ON A FACE,
  // we can just apply the motions,
  // and IF WE END UP OFF the edges,
  // we wrap to the rail or adjacent faces.

  row += rowOffset ;
  
  int &rail = row ;
  int &along = col ;

  if( row < 0 ) // EXIT_TOP
  {
    // you stepped off the top of the face.
    // for all of these, this lands you on the top rail
    // EXCEPT NY, which lands you on BOT_RAIL.
    // Really, we need to STEP on the top rail, then
    // call the top rail wrap function, so we don't
    // repeat the rail offset code here.

    int rowOvershoot = row + 1 ; // row had the offset applied to it.
    // if row became -1, step on rail and stay there (overshoot=0).
    // if row is now -2, then you should
    // step ONTO the rail, and call railedMoveNWrapRail() with
    // a ROW motion parameter of overshoot

    if( face == NY )
      rail = BOT_RAIL ;
    else
      rail = TOP_RAIL ;

    if( face == PY )
    {
      // leaving top py different.  enter TOP_RAIL near NZ
      along = railedGetAlongTopRailBaseOffset( NZ )*pixelsPerSide + //(3*pxPerSide)
              (pixelsPerSide - col + 1) ; // and flipside the col. (From lattice layout diagram.PNG: col=2 means railOff=4*3+ (4-1 - 2) = 12+ (1))
      face = RAIL ;
      
      // now the direction of the rowOvershoot is reversed (it is now going DOWN not up),
      // and so is colOffset.
      if( rowOvershoot || colOffset )
        railedMoveNWrapRail( face, rail, along, -rowOvershoot, -colOffset ) ;// still need to apply colOffset as well, but it turns around
      return ; //NEEDED.
    }
    else
    {
      // mid faces AND NY are the same.
      
      // how far along the rail are we?
      // face tells the base offset to the rail section
      // add col and you have your position on the rail.
      along = railedGetAlongTopRailBaseOffset( face )*pixelsPerSide + col + 1; // (eg NX returns 0*64, PZ returns 1*64, PX returns 2*64).
      // PLUS ONE to offset from the corner

      face = RAIL ;//CHANGE LAST!
      
      // You are going UP, all directions remain the same on the rail (+col is still right).
      // rowOvershoot will be 0 if there was no overshoot (ie no more row displacement).
      if( rowOvershoot || colOffset )
        railedMoveNWrapRail( face, rail, along, rowOvershoot, colOffset ) ;// still need to apply colOffset as well.
      return ;
    }
  }
  else if( row >= pixelsPerSide-1 ) // EXIT_BOTTOM
  {
    // ALL BOT_RAIL EXCEPT PY.
    int rowOvershoot = row - (pixelsPerSide-1) ; // 63 becomes 0, ie NO overshoot.  64 becomes 1, ie overshoot of 1 (still need to step off rail).

    if( face == NY )
    {
      // handle differently
      along = railedGetAlongTopRailBaseOffset( NZ )*pixelsPerSide +
              (pixelsPerSide-1 - col) ;
      rail = BOT_RAIL ;
      face = RAIL ;
      if( rowOvershoot || colOffset )
        railedMoveNWrapRail( face, rail, along, -rowOvershoot, colOffset ) ; // (down becomes up)
      return ;
    }
    else
    {
      if( face == PY )
        rail = TOP_RAIL ;
      else
        rail = BOT_RAIL ;
      
      along += railedGetAlongTopRailBaseOffset( face ) * pixelsPerSide + 1 ;//adjust along with col+(railedGetFaceAlong)
      // PLUS ONE to offset from the corner

      face = RAIL ; // CHANGE LAST!!

      // now you are on the rail,
      // and need to call railwrapper
      if( rowOvershoot || colOffset )
        railedMoveNWrapRail( face, rail, along, rowOvershoot, colOffset ) ;// still need to apply colOffset as well.
      return ;
    }
  }

  // Here the row offset didn't take you off the face.
  // now you can APPLY THE COL MOTION.
  col += colOffset ;

  // again, the col motion may take you off the face,
  // to a side rail.

  if( col < 0 ) // EXIT_LEFT
  {
    int colOvershoot = col + 1 ; // 
    
    int& rail = row ;
    int& along = col ;

    if( face == PY )
    {
      // enter TOP_RAIL
      along = row + 1 ; //+1 to skip corner
      rail = TOP_RAIL ;
      face = RAIL ;

      // the excess motion is now DOWN.
      if( colOvershoot )
        railedMoveNWrapRail( face, rail, along, -colOvershoot, 0 ) ; // we already applied the rowOffset.
      return ;
    }
    else if( face == NY )
    {
      along = pixelsPerSide - row - 1 ; //(pixelsPerSide-1) - row + 1
      rail = BOT_RAIL ;
      face = RAIL ;

      // excess motion UP.
      if( colOvershoot )
        railedMoveNWrapRail( face, rail, along, colOvershoot, 0 ) ; // we already applied the rowOffset.
      return ;
    }
    else
    {
      // so if you are NX PZ PX or NZ you enter a side rail.
      along = row ;
      rail = railedGetAlongTopRailBaseOffset( face ) + 2 ; // adding 2 gets you the correct LNX etc rail TO YOUR LEFT
      face = RAIL;

      if( colOvershoot )
        railedMoveNWrapRail( face, rail, along, 0, colOvershoot ) ; // remains colOffset
    }
  }
  else if( col >= pixelsPerSide-1 ) // EXIT_RIGHT
  {
    int colOvershoot = col - (pixelsPerSide-1) ; // if this is 0 i am exactly on the rail ( e.g. 3 - 3 = 0 )

    if( face == PY )
    {
      //enter top rail near PX
      along = railedGetAlongTopRailBaseOffset(PX)*pixelsPerSide + (pixelsPerSide-1 - row);
      rail = TOP_RAIL ; // LAST
      face = RAIL ;

      if( colOvershoot )
        railedMoveNWrapRail( face, rail, along, colOvershoot, 0 ) ; // turns into +row overshoot
    }
    else if( face == NY )
    {
      along = railedGetAlongTopRailBaseOffset( PX )*pixelsPerSide + row+1 ; // (row+1) to get away from corner.
      rail = BOT_RAIL ;
      face = RAIL ;
      if( colOvershoot )
        railedMoveNWrapRail( face, rail, along, -colOvershoot, 0 ) ; // turns into -row overshoot
    }
    else
    {
      along = row ;
      rail = ( railedGetAlongTopRailBaseOffset( face ) + 2 + 1 ) % 6; // adding 3 gets you the correct LNX etc rail TO YOUR RIGHT,
      // mod 6 because LNZ is 5+1=6, wraps to LNX..
      if( !rail ) rail = 2 ;
      face = RAIL ;

      if( colOvershoot )
        railedMoveNWrapRail( face, rail, along, 0, colOvershoot ) ; // remains same sign colOffset
    }
  }

}

// Maps you to either on rail or
// off rail.
// You may end up on a rail.
void CubeMap::railedMove( int &face, int &row, int &col, int rowOffset, int colOffset )
{
  if( face == CubeFace::RAIL )
    railedMoveNWrapRail( face, row, col, rowOffset, colOffset ) ;
  else
    railedMoveNWrapFace( face, row, col, rowOffset, colOffset ) ;
}

void CubeMap::railedMake()
{
  const static int RIGHT_COLUMN = pixelsPerSide - 1 ;
  const static int LEFT_COLUMN = 0 ;

  // I figure there should be a way to visualize this, for debugging purposes.
  //           o•o
  //           •o•
  //           o•o
  //  *********TOP RAIL*********
  // L* o•o *L o•o *L o•o *L o•o
  // N* •o• *P •o• *P •o• *N •o•
  // X* o•o *Z o•o *X o•o *Z o•o
  //  *******BOTTOM RAIL********
  //           o•o
  //           •o•
  //           o•o

  // The faces are stored as:
  // 
  // |    PX    |    NX    |    PY    |    NY    |    PZ    |    NZ    |
  //
  // The rails are stored as:
  // | TOP RAIL | BOT RAIL | LNX RAIL | LPZ RAIL | LPX RAIL | LNZ RAIL |
  // 

  // These are all stored in the same vector.

  // the entire shell has rail (N+1)³ - (N-1)³ = 2*(3N² + 1)
  // THE FACES have 6*(N-1)² elts
  //                (all in volume)  -  (all on center faces)  -  (all in center)
  // THE FRAME has      (N+1)³       -        6*(N-1)²         -        (N-1)³
  // 

  

  // one vector
  railed = new vector<Vector>( railSize(), Vector(1,1,0,1) ) ;

  // Take the top (N-1) pixels and make the faces.
  // 
  for( int FACE = 0 ; FACE < 6 ; FACE++ )
    for( int row = 0 ; row < pixelsPerSide - 1 ; row++ )
      for( int col = 0 ; col < pixelsPerSide - 1 ; col++ )
        (*railed)[ railedIndex( FACE, row, col ) ] = (*currentSource)[ dindex( FACE, row, col ) ] ;

  // make the rails.
  // make the SIDE RAILS
  for( int r = 0 ; r < railSideSize() ; r++ )
  {
    // | TOP RAIL | BOT RAIL | LNX RAIL | LPZ RAIL | LPX RAIL | LNZ RAIL |
    // side rails are made by averaging the adjacent original faces.

    /// AddMe4 adds the w component as well (alpha)
    // LNX
    (*railed)[ railOffsetTo(LNX_RAIL) + r ] = //pxPoint( NX, r, LEFT_COLUMN ); //pxPoint( NZ, r, RIGHT_COLUMN );
      ( pxPoint( NZ, r, RIGHT_COLUMN ).AddMe4( pxPoint( NX, r, LEFT_COLUMN ) ) ).DivMe4(2) ;
    // LPZ
    (*railed)[ railOffsetTo(LPZ_RAIL) + r ] = //pxPoint( PZ, r, LEFT_COLUMN ); //pxPoint( NX, r, RIGHT_COLUMN );
      ( pxPoint( NX, r, RIGHT_COLUMN ).AddMe4( pxPoint( PZ, r, LEFT_COLUMN ) ) ).DivMe4( 2 ) ;
    // LPX
    (*railed)[ railOffsetTo(LPX_RAIL) + r ] = //pxPoint( PX, r, LEFT_COLUMN );//pxPoint( PZ, r, RIGHT_COLUMN );
      ( pxPoint( PZ, r, RIGHT_COLUMN ).AddMe4( pxPoint( PX, r, LEFT_COLUMN ) ) ).DivMe4( 2 ) ;
    // LNZ
    (*railed)[ railOffsetTo(LNZ_RAIL) + r ] = //pxPoint( NZ, r, LEFT_COLUMN );pxPoint( PX, r, RIGHT_COLUMN );
      ( pxPoint( PX, r, RIGHT_COLUMN ).AddMe4( pxPoint( NZ, r, LEFT_COLUMN ) ) ).DivMe4( 2 ) ;
  }

  // TOP_RAIL and BOT_RAIL:
  for( int r = 0 ; r < railTopSize() ; r++ )
  {
    // The column you are closest to, on the face.  -1 because
    // of the corners on the rail.
    int colAlong = (r % pixelsPerSide) - 1;
    
    if( r % pixelsPerSide == 0 )
    {
      // CORNER handle differently
      int faceRight = railedGetFaceBesideRail( r+1 ) ; // wrap not needed.
      int faceLeft = railedGetFaceBesideRail( (r-1 + railTopSize()) % railTopSize() ) ; // wrap for when this becomes -1.

      int siderailNearest = r / pixelsPerSide ;

      // Be careful of Add4/AddMe4 usage below.  Add4 generates a copy,
      // (so as not to corrupt object in array), AddMe4 doesn't generate a copy
      // (so it's a bit faster/more efficient).
      // TOP
      (*railed)[ railOffsetTo(TOP_RAIL)  +  r ] =
        (*railed)[ railOffsetTo( siderailNearest ) ].Add4( // FIRST ENTRY IN SIDERAIL NEAREST
          pxPoint( faceLeft, 0, pixelsPerSide-1 ).AddMe4(      // top right from leftface
            pxPoint( faceRight, 0, 0 )                     // top left from rightface
          )
        ).DivMe4( 3 ) ;

      // BOT
      (*railed)[ railOffsetTo(BOT_RAIL)  +  r ] = 
        (*railed)[ railOffsetTo( siderailNearest ) + railSideSize() - 1 ].Add4( // LAST ENTRY IN SIDERAIL NEAREST
          pxPoint( faceLeft, pixelsPerSide-1, pixelsPerSide-1 ).AddMe4( //bot right from leftface
            pxPoint( faceRight, pixelsPerSide-1, 0 )                //bot left from rgihtface
          )
        ).DivMe4( 3 ) ;

    }
    else
    {
      Vector pxFromPYTopRail, pxFromNYBotRail ;
    
      // We have to sample the ORIGINAL cubemap, hence use of pxPoint and
      // not railedPxPoint (railed isn't finished yet, we're making it now!)
      if( r < pixelsPerSide )
      {
        pxFromPYTopRail = pxPoint( PY, colAlong, 0 ) ; // ENTER LEFT
        pxFromNYBotRail = pxPoint( NY, pixelsPerSide-colAlong, 0 ) ;
      }
      else if( r < 2*pixelsPerSide )
      {
        pxFromPYTopRail = pxPoint( PY, pixelsPerSide-1, colAlong ) ;  //enter PY BOT ROW
        pxFromNYBotRail = pxPoint( NY, 0, colAlong ) ;
      }
      else if( r < 3*pixelsPerSide )
      {
        pxFromPYTopRail = pxPoint( PY, pixelsPerSide-colAlong, pixelsPerSide-1 ) ;  // enter PY RIGHT
        pxFromNYBotRail = pxPoint( NY, colAlong, pixelsPerSide-1 ) ;
      }
      else
      {
        pxFromPYTopRail = pxPoint( PY, 0, pixelsPerSide - colAlong ) ;// enter PY TOP
        pxFromNYBotRail = pxPoint( NY, pixelsPerSide-1, pixelsPerSide-colAlong ) ;
      }
    
      // take the color from the original cubemap
      // the face below me,
      int faceAdj = railedGetFaceBesideRail( r ) ;
      (*railed)[ railOffsetTo(TOP_RAIL)  +  r ] =
        ( pxPoint( faceAdj, 0, colAlong ).AddMe4( pxFromPYTopRail ) ).DivMe4( 2 ) ;

      // Do the bottom rail also
      (*railed)[ railOffsetTo(BOT_RAIL)  +  r ] =
        ( pxPoint( faceAdj, pixelsPerSide-1, colAlong ).AddMe4( pxFromNYBotRail ) ).DivMe4( 2 ) ;
    }
  }

  


  // after you make railed you probably want it as the source as well
  setSource( railed ) ;
}

#pragma endregion

/// get cubemap pixel color based on direction vector
Vector CubeMap::px( const Vector& vec )
{
  int FACE = 0 ;
  real rcol, rrow ;

  // this is the only place where this function is used.
  getFaceRowCol( vec, FACE, rrow, rcol ) ;

  // The default is to interpolate bilinearly
  if( bilinear )
    return pxBilinear( FACE, rrow, rcol ) ;
  else
    return pxPoint( FACE, (int)(rrow), (int)(rcol) ) ; //NON interpolated
}

Vector CubeMap::pxBilinear( int face, real row, real col )
{
  // 3.5 IS THE CENTER OF PIXEL 3
  // 3.99 is right edge of pixel 3
  // 3.01 is left edge of pixel 3

  int frow = floor(row) ;
  int fcol = floor(col) ;
  
  real fr = row - frow ;
  real fc = col - fcol ;
  
  Vector TL,TR,BL,BR;
  real wTL,wTR,wBL,wBR;
  
  if( fr >= .5 && fc >= .5 )
  {
    TL = pxPoint( face, frow, fcol ) ;      TR = pxPoint( face, frow, fcol+1 ) ;
    BL = pxPoint( face, frow+1, fcol ) ;    BR = pxPoint( face, frow+1, fcol+1 ) ;
  }
  else if( fr >= .5 && fc < .5 )
  {
    TL = pxPoint( face, frow, fcol-1 ) ;    TR = pxPoint( face, frow, fcol ) ;
    BL = pxPoint( face, frow+1, fcol-1 ) ;  BR = pxPoint( face, frow+1, fcol ) ;
  }
  else if( fr < .5 && fc >= .5 )
  {
    TL = pxPoint( face, frow-1, fcol ) ;    TR = pxPoint( face, frow-1, fcol+1 ) ;
    BL = pxPoint( face, frow, fcol ) ;      BR = pxPoint( face, frow, fcol+1 ) ;
  }
  else if( fr < .5 && fc < .5 )
  {
    TL = pxPoint( face, frow-1, fcol-1 ) ;  TR = pxPoint( face, frow-1, fcol ) ;
    BL = pxPoint( face, frow, fcol-1 ) ;    BR = pxPoint( face, frow, fcol ) ;
  }

  // these are the actual weights
  real p,q,op,oq ;
  
  p = ( fc >= .5 )? fc - .5 : fc + .5 ;
  q = ( fr >= .5 )? fr - .5 : fr + .5 ;

  op = 1 - p ;
  oq = 1 - q ;

  wTL = op*oq ;
  wTR = p*oq ;
  wBR = p*q ;
  wBL = op*q ;

  return wTL*TL + wTR*TR + wBR*BR + wBL*BL ;
}

// This allows out of bounds pixel access.. you will wrap
// to the appropriate pixel.
Vector CubeMap::pxPoint( int face, int row, int col )
{
  // First, we wrap.
  wrap( face, row, col ) ;
  int idx = dindex( face, row, col ) ;  

  #if CUBEMAP_DEBUG_MODE
  if( !currentSource )
  {
    error( "No cubemap" ) ;
    return 0 ;
  }

  // This should almost never happen
  if( idx > currentSource->size() )
  {
    error( "CubeMap::pxPoint: index %d out of range of CubeMap", idx ) ;
    return 0 ;
  }
  #endif

  // This pointer allows us to choose to use the rotated cubemap or not.
  return (*currentSource)[ idx ] ;
}

Vector CubeMap::railedPxPoint( int face, int row, int col )
{
  int idx = railedIndex( face, row, col ) ;  
  return (*currentSource)[ idx ] ;
}

Vector CubeMap::getDirectionConvertedLHToRH( int face, int row, int col )
{
  // get the uv coordinates on the face
  real u = -1 + 2.0*(col + 0.5)/pixelsPerSide ;
  real v = -1 + 2.0*(row + 0.5)/pixelsPerSide ;

  // r is the radial distance to the pixel on the cube
  /////real r = sqrt( 1 + u*u + v*v ) ;

  Vector dir = cubeFaceBaseConvertingFwd[face] + 
          u*cubeFaceBaseConvertingRight[face] +
          v*cubeFaceBaseConvertingUp[face] // the only difference here is BOTH right and up vectors
          // are negated for -x,+x,-z,+z but
          // are the same for +/-y.
        ;
  dir.normalize();
  return dir ;
}

Vector CubeMap::getDirectionRH( int face, int row, int col )
{
  // get the uv coordinates on the face
  real u = -1 + 2.0*(col + 0.5)/pixelsPerSide ;
  real v = -1 + 2.0*(row + 0.5)/pixelsPerSide ;

  Vector dir = cubeFaceBaseRHFwd[face] +  // go out to face
          u*cubeFaceBaseRHRight[face] +   // go right on face
          v*cubeFaceBaseRHUp[face]        // go up face
        ;
  dir.normalize();
  return dir ;
}

Vector CubeMap::getDirectionRH( int index )
{
  int face,row,col;
  getFaceRowCol( index, face, row, col ) ;
  return getDirectionRH( face, row, col ) ;
}

Vector CubeMap::getDirectionConvertedLHToRH( int index )
{
  int face,row,col;
  getFaceRowCol( index, face, row, col ) ;
  return getDirectionConvertedLHToRH( face, row, col ) ;
}

// Get face, row and column based on
// a direction vector
void CubeMap::getFaceRowCol( Vector direction, int& oface, real& orow, real& ocol )
{
  real u,v;

  // For a direction vector from the 2-unit cube,
  // the direction will ALWAYS have a 1 in it, and
  // the side with the 1 is the face side that you're _on_.
  // dividing by 

  // Got inane values when direction.x==direction.y using getDominantAxis()
  char dominantAxis = direction.getDominantAxisEq() ;
  switch( dominantAxis )
  {
  case 0:
    {
      // x dominant
      direction /= fabs(direction.x) ; // sign check must be before divide, because sign will be lost OR USE FABS AS I HAVE HERE
    
      if( direction.x > 0 )
        oface = CubeFace::PX ;
      else
        oface = CubeFace::NX ;
    }
    break ;
  case 1:
    {
      // y dom
      direction /= fabs(direction.y) ;
    
      if( direction.y > 0 )
        oface = CubeFace::PY ;
      else
        oface = CubeFace::NY ;
    }
    break ;
  case 2:
    {
      // z dom
      direction /= fabs(direction.z) ;
    
      if( direction.z > 0 )
        oface = CubeFace::PZ ;
      else
        oface = CubeFace::NZ ;
    }
    break ;
  default: // -1/'n'
    error( "Could not find dominant axis" ) ;
    direction.print() ;
    direction = Vector(1,0,0) ; // avoid further errors.
    break ;
  }
  // choose the u and the v.
  u = direction.Dot( cubeFaceBaseConvertingRight[oface] ) ;
  v = direction.Dot( cubeFaceBaseConvertingUp[oface] ) ;

  // u was -1 at the left edge, +1 at the very right edge.

  // use the reverse formulas
  //real u = -1 + 2.0*(col + 0.5)/W ;
  //real v = -1 + 2.0*(row + 0.5)/H ;

  //int col = round( (u+1) * W/2.0 - 0.5 ) ;
  //int row = round( (v+1) * H/2.0 - 0.5 ) ;

  // will use the fractional portion for interpolation
  // these are wrong!  goes from 0..PixelsPerSide, but should go to pixelsPerSide-1
  //ocol = (u+1) * pixelsPerSide/2.0 ; // NOT subbing back the 0.5. leave it in the center. u,v each between 0 and WIDTH.
  //orow = (v+1) * pixelsPerSide/2.0 ; // 

  // retrieve the row and column.
  // First, u and v convert from [-1..1] to [0..1].
  // Then, we multiply by (px-1) to get [0..63] for example.
  // TOO TIDY.
  //ocol = (u+1)/2.0 * (pixelsPerSide-1) ;
  //orow = (v+1)/2.0 * (pixelsPerSide-1) ;

  // 0 is the LEFT EDGE of the first pixel.
  // 63 is the RIGHT EDGE of the last pixel.

  // 0 is left edge, 3 is last.
  //
  // 0  0.5  1  1.5  2  2.5  3  3.5  4
  //     *       *       *       *

  ocol = (u+1)/2.0 * pixelsPerSide ;
  orow = (v+1)/2.0 * pixelsPerSide ;

  
  // 3.5 IS THE CENTER OF PIXEL 3.
  // 3.999 is THE RIGHT EDGE OF PIXEL 3.
  // 3.001 IS THE LEFT EDGE OF PIXEL 3.

  // that's it. you now have oface, orow, and ocol assigned a value.
  ///window->addDebugLine( Vector(0,0,0), baseColor[FACE], direction, baseColor[FACE] );
  
}

// this doesn't actually work.  you can't
// rotate a wavelet this way, it's mathematically wrong
void CubeMap::rotateWavelet( Matrix matrix )
{
  // FORWARD rotation - don't transpose the matrix

  // now rotate the wavelet repn, if there are values
  // set in the sparse vector
  if( sparseTransformedColor.nonZeros() )
  {
    // I need to erase the old one/create a new one
    Eigen::SparseVector<Vector> rotatedSparseTransformedColor ;

    // now rotate/mutute sparseTransformedColor.
    // rotate only those nonzeros
    for( Eigen::SparseVector<Vector>::InnerIterator it( sparseTransformedColor ); it; ++it )
    {
      int i = it.index() ;
      Vector dir = getDirectionConvertedLHToRH( i ) ;
      // it.index() is the INTEGER INDEX IN THE 1D ARRAY
      // it.value() is the VALUE at the iterator
      ///(*colorValues)[ it.index() ] = it.value() ; // basic copying the vector for uncompression

      dir = dir * matrix ;
      int i2 = index( dir ) ;

      // write the wavelet component AT the index intended
      rotatedSparseTransformedColor.insert( i2 ) = it.value() ;
    }
    
    sparseTransformedColor = rotatedSparseTransformedColor ;
  }
  else
  {
    error( "Trying to rotate wavelet repn but there's no wavelet repn to rotate" ) ;
  }
}

// rotated color values will be stored
// in the rotatedColorValues vector.
// The original always remains in "colorValues" because
// if you rotate multiple times you will lose color values.
void CubeMap::rotate( Matrix matrix )
{
  if( !colorValues )
  {
    error( "Cannot rotate cubemap without the original source." ) ;
    return ;
  }

  // We will take the inverse of the rotation matrix,
  // because for each texel we FETCH the color that was there.
  matrix.transpose() ;

  // if you don't have space, create it
  if( !rotatedColorValues )  rotatedColorValues = new vector<Vector>( colorValues->size() ) ;

  // rotate
  for( int i = 0 ; i < colorValues->size() ; i++ )
  {
    Vector dir = getDirectionConvertedLHToRH( i ) ;
    dir = dir*matrix ;

    // get the vector THAT FETCHES the color for this texel.
    int i2 = index( dir ) ;
    (*rotatedColorValues)[ i ] = (*colorValues)[ i2 ] ;
  }

  // TRY SAVING IT
  //static char b[256];
  //sprintf( b, "C:/vctemp/fullcube/cubemap_rotated_%.1f_deg.png", window->rot ) ;
  //saveRotated( b ) ;
  setSource( rotatedColorValues ) ;

  

}

void CubeMap::rotate( const Vector& axis, real radians )
{
  Matrix matrix = Matrix::Rotation( axis, radians ) ;
  rotate( matrix ) ;
}

void CubeMap::rotate( char axis, real radians )
{
  Matrix matrix ;
  switch( axis )
  {
    case 'x':
      matrix = Matrix::RotationX( radians ) ;
      break ;

    case 'y':
      matrix = Matrix::RotationY( radians ) ;
      break ;

    case 'z':
      matrix = Matrix::RotationZ( radians ) ;
      break ;

    default:
      error( "Invalid rotation axis specified, nothin' doin'" ) ;
      return ;
  }
  
  rotate( matrix ) ;
}

void CubeMap::makePointLightApproximation( int icoSubdivs, int maxLights, 
  int raysPerIcoFace, vector<PosNColor>& pts, bool normalized, real& omaxmag )
{
  // shoot rays at every pixel.
  // 
  Icosahedron* ico = new Icosahedron( "cubemap feeler", 1, Material::White ) ;
  Sphere *sp = Sphere::fromShape( "cubemap feeler", ico, 1, icoSubdivs ) ;

  vector<Triangle>* tris = &sp->meshGroup->meshes[0]->tris;
  bool wasBilinear = bilinear ;
  bilinear = false ; // force bilinear sampling of this cubemap OFF

  list<PosNColor> initialPts ; // will erase from,

  // Just shoot rays within the tri
  for( int i = 0 ; i < tris->size() ; i++ )
  {
    Vector sumDir, sumColor ;
    for( int j = 0 ; j < raysPerIcoFace ; j++ )
    {
      Vector cDir = (*tris)[i].getRandomWithin() ;
      sumDir += cDir ;

      // grab the color
      sumColor += px( cDir ) ;
    }

    // average the colors and dirs you got and push back
    sumDir /= raysPerIcoFace ;
    sumColor /= raysPerIcoFace ;
    
    initialPts.push_back( PosNColor( sumDir, sumColor ) ) ;
  }

  if( initialPts.empty() )
  {
    warning( "Cubemap approximation EMPTY!!" ) ;
    return ;
  }

  // MERGE.
  //info( "%d initialPts", initialPts.size() ) ;
  for( list<PosNColor>::iterator iter = initialPts.begin() ; iter != initialPts.end() ; ++iter )
  {
    for( list<PosNColor>::iterator iter2 = initialPts.begin() ; iter2 != initialPts.end() ; )
    {
      if( iter != iter2 )
      {
        // if the colors are similar and the angles are within 15 degrees, merge.
        // if tjhe directions are within 15 degrees..

        if( iter2->color.Near( iter->color, .1 ) &&  // fairly generous distance allowed
            iter2->pos.angleWith( iter->pos ) < 30 ) // should be close in angle for merge.
        {
          // MERGE
          iter->pos += iter2->pos ;
          iter->color += iter2->color ;
          iter->pos /= 2 ;
          iter->color /= 2 ;

          iter2 = initialPts.erase( iter2 ) ;
          ///info( "Merged" ) ;
        }
        else // no merge
          ++iter2 ;
      }
      else // iter==iter2, so just advance
        ++iter2 ;
    }
  }
  
  //info( "AFTER: %d initialPts", initialPts.size() ) ;

  if( initialPts.size() < maxLights )
  {
    // use all of them
    pts.insert( pts.end(), initialPts.begin(), initialPts.end() ) ;
  }
  else
  {
    // keep only the top k.
    for( int i = 0 ; i < maxLights ; i++ )
    {
      // Pick highest magnitude color indices, remaining, move into results list
      list<PosNColor>::iterator maxIndex = initialPts.begin() ;
      real maxMag = 0 ;
      for( list<PosNColor>::iterator iter = initialPts.begin() ; iter != initialPts.end() ; ++iter )
      {
        real mag = (*iter).color.len2() ;
        if( mag > maxMag ){
          maxMag = mag ;
          maxIndex = iter ;
        }
      }

      // pull out the max index entry and move it to the results list
      pts.push_back( *maxIndex ) ;
      initialPts.erase( maxIndex ) ;
    }
  }
  // Restore the old setting
  bilinear = wasBilinear ;

  DESTROY( sp ) ;

  // find the maximum value
  omaxmag = pts[0].color.len();
  for( int i = 1 ;  i < pts.size(); i++ )
  {
    real c = pts[i].color.len() ;
    if( c > omaxmag )  omaxmag = c;
  }

  // Normalize the remaining
  if( normalized )
    for( int i = 0 ;  i < pts.size(); i++ )
      pts[i].color /= omaxmag ;
  
  info( "%d point lights created to approximate cubemap %s", pts.size(), name.c_str() ) ;
}


