#include "D3D11Shader.h"
#include "D3D11Window.h"

const char* D3D11Shader::VERTEX_SHADER_MODEL_5 = "vs_5_0" ;
const char* D3D11Shader::PIXEL_SHADER_MODEL_5 = "ps_5_0" ;

D3D11Shader::D3D11Shader( D3D11Window* iWin, VertexType iVertexType, char* vShaderFilename, char* vShaderFcnName,
              char* pxShaderFilename, char* pxShaderFcnName )
{
  win = iWin ;
  vertexType = iVertexType ;
  if( !loadHLSLBlob( &vblob, vShaderFilename, vShaderFcnName, VERTEX_SHADER_MODEL_5 ) )
    error( "Couldn't load shader %s, %s()", vShaderFilename, vShaderFcnName ) ;
  createVertexShader( &vblob, &vsh ) ;

  if( !loadHLSLBlob( &pxblob, pxShaderFilename, pxShaderFcnName, PIXEL_SHADER_MODEL_5 ) )
    error( "Couldn't load shader %s, %s()", pxShaderFilename, pxShaderFcnName ) ;
  createPixelShader( &pxblob, &psh ) ;
  SAFE_RELEASE( pxblob ) ; // I don't need this one anymore
}

D3D11Shader::~D3D11Shader()
{
  SAFE_RELEASE( vblob ) ;
}

D3D11_INPUT_ELEMENT_DESC D3D11Shader::makeInputElement( char* semantic, int position, DXGI_FORMAT dxgiFormat, int alignedByteOffset )
{
  // there are additional steps to set up the vertex shader
  // you have to set the right vertex layout
  //// TO CREATE A LAYOUT YOU HAVE TO HAVE A SHADER
  // TO BASE IT ON.
    
  D3D11_INPUT_ELEMENT_DESC elt = {
    semantic,  // SemanticName: Case insensitive name for the variable value as received in the shader.
    // The ones you are used to seeing are POSITION, TEXCOORD, COLOR, NORMAL. You can only use the
    // predefined supported type names.
    position,           // SemanticIndex: Is it POSITION0, POSITION1, etc
    dxgiFormat,  // Format: DXGI_FORMAT_R32G32B32_FLOAT is a 3x 32-bit float format
    0,           // InputSlot: Vertex buffer input slot. 
    alignedByteOffset,           // AlignedByteOffSet: which byte does this element start at in the vertex?
    D3D11_INPUT_PER_VERTEX_DATA, // InputSlotClass: per vertex or per instance? see: shader instancing
    0            // UINT InstanceDataStepRate: used for instancing
  } ;

  return elt ;
}

bool D3D11Shader::createVertexShader( ID3D10Blob** blob, ID3D11VertexShader** shader )
{
  return DX_CHECK( win->d3d->CreateVertexShader( (*blob)->GetBufferPointer(), (*blob)->GetBufferSize(), 0, shader ), "Create vertex shader" ) ;
}

bool D3D11Shader::createPixelShader( ID3D10Blob** blob, ID3D11PixelShader** pxShader )
{
  return DX_CHECK( win->d3d->CreatePixelShader( (*blob)->GetBufferPointer(), (*blob)->GetBufferSize(), 0, pxShader ), "Create pixel shader" ) ;
}

void D3D11Shader::activate()
{
  // Tell window ACTIVE SHADER IS ME,
  win->activeShader = this ; // a bit of a loop around, 
  // (reaching into the window class from here),
  // but makes code handling easier (don't have to detect change elsewhere)
  win->gpu->IASetInputLayout( layout ) ;
  win->gpu->VSSetShader( vsh, 0, 0 ) ;
  win->gpu->PSSetShader( psh, 0, 0 ) ;
}

bool D3D11Shader::loadHLSLBlob( ID3D10Blob** blob, char* filename, char* fcnName, const char* shaderModel )
{
  ID3D10Blob *errorblob;

  if( !DX_CHECK( D3DX11CompileFromFileA(
    filename,
    NULL, NULL,
    fcnName,
    shaderModel, // usually "ps_5_0" or "vs_5_0"
    D3D10_SHADER_ENABLE_STRICTNESS, NULL,
    NULL,
    blob, &errorblob,
    NULL ), "compiling fx" ) )
  {
    if( !errorblob )
    {
      error( "%s shader file not found!", filename ) ;
      return false ;
    }
    else
    {
      int size = errorblob->GetBufferSize() ;
      char* errstr = (char*)malloc( size+1 ) ;
      strncpy( errstr, (char*)errorblob->GetBufferPointer(), size ) ;
      errstr[ size ] = 0 ;
      error( errstr );
      free( errstr ) ;
    }
    SAFE_RELEASE( errorblob );
    bail( "Couldn't compile your fx file", true ) ; // (exits)
    return false ;
  }
    
  return true ; // successful load  
}

bool D3D11Shader::createInputLayout( D3D11_INPUT_ELEMENT_DESC *ve, int numElts )
{
  bool ok = DX_CHECK( win->d3d->CreateInputLayout( ve,
                    numElts,
                    vblob->GetBufferPointer(), vblob->GetBufferSize(),
                    &layout ), "Create vertex layout" ) ;
  // now I don't need vblob.
  SAFE_RELEASE( vblob ) ;
  return ok ;
}

void D3D11Shader::setTexture( int slot, ID3D11ShaderResourceView* tex )
{
  win->gpu->VSSetShaderResources( slot, 1, &tex ) ;
  win->gpu->PSSetShaderResources( slot, 1, &tex ) ;
}

// unset the active texture
void D3D11Shader::unsetTexture( int slot )
{
  // TURN OFF the d2d tex, because a lock hasn't been
  // acquired yet and you get a warning if you draw
  // a tex while its still bound from the previous frame
  win->gpu->VSSetShaderResources( slot, 1, win->NO_RESOURCE ) ;
  win->gpu->PSSetShaderResources( slot, 1, win->NO_RESOURCE ) ;
}





VCShader::VCShader( D3D11Window* iWin,
            char* vShaderFilename, char* vShaderFcnName,
            char* pxShaderFilename, char* pxShaderFcnName ) :
            D3D11Shader( iWin, VertexType::PositionColor, vShaderFilename, vShaderFcnName, pxShaderFilename, pxShaderFcnName )
{
  setupInputLayout() ;
}

void VCShader::setupInputLayout() //override
{
  D3D11_INPUT_ELEMENT_DESC ve[2];
    
  // vc
  ve[0] = makeInputElement( "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0 ) ;
  ve[1] = makeInputElement( "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, (3)*sizeof( float ) ) ;
  
  createInputLayout( ve, 2 ) ;
}


VTCShader::VTCShader( D3D11Window* iWin, char* vShaderFilename, char* vShaderFcnName,
            char* pxShaderFilename, char* pxShaderFcnName ) :
            D3D11Shader( iWin, VertexType::PositionTextureColor, vShaderFilename, vShaderFcnName, pxShaderFilename, pxShaderFcnName )
{
  setupInputLayout() ;
}

void VTCShader::setupInputLayout() //override
{
  int accum = 0 ;

  D3D11_INPUT_ELEMENT_DESC ve[3];

  ve[0] = makeInputElement( "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0 ) ;
  ve[1] = makeInputElement( "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, accum += 3*sizeof( float ) ) ;
  ve[2] = makeInputElement( "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, accum += 4*sizeof( float ) ) ;
  
  createInputLayout( ve, 3 ) ;
}



VNCShader::VNCShader( D3D11Window* iWin, char* vShaderFilename, char* vShaderFcnName,
            char* pxShaderFilename, char* pxShaderFcnName ) :
            D3D11Shader( iWin, VertexType::PositionNormalColor, vShaderFilename, vShaderFcnName, pxShaderFilename, pxShaderFcnName )
{
  setupInputLayout() ;
}

void VNCShader::setupInputLayout() //override
{
  int accum = 0 ;

  // vnc
  D3D11_INPUT_ELEMENT_DESC ve[3];
    
  ve[0] = makeInputElement( "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0 ) ; // first one, no offset
  ve[1] = makeInputElement( "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, accum += 3*sizeof( float ) ) ; // there are 3 floats before me
  ve[2] = makeInputElement( "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, accum += 3*sizeof( float ) ) ; // there are 6 floats before me
  
  createInputLayout( ve, 3 ) ;
}

VTTTTNCShader::VTTTTNCShader( D3D11Window* iWin, char* vShaderFilename, char* vShaderFcnName,
                char* pxShaderFilename, char* pxShaderFcnName ) :
                D3D11Shader( iWin, VertexType::VTTTTNC,
                             vShaderFilename, vShaderFcnName, pxShaderFilename, pxShaderFcnName )
{
  setupInputLayout() ;
}

void VTTTTNCShader::setupInputLayout() //override
{
  D3D11_INPUT_ELEMENT_DESC ve[7] ;
  int accum = 0 ;
  ve[ 0 ] = makeInputElement( "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0 ) ;
  ve[ 1 ] = makeInputElement( "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, accum+=3*sizeof(float) ) ;
  ve[ 2 ] = makeInputElement( "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, accum+=4*sizeof(float) ) ;
  ve[ 3 ] = makeInputElement( "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, accum+=4*sizeof(float) ) ;
  ve[ 4 ] = makeInputElement( "TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, accum+=4*sizeof(float) ) ;
  ve[ 5 ] = makeInputElement( "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, accum += 4*sizeof(float) ) ;
  ve[ 6 ] = makeInputElement( "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, accum += 3*sizeof(float) ) ;
  createInputLayout( ve, 7 ) ;
}




VTTTTNCCCCShader::VTTTTNCCCCShader( D3D11Window* iWin, char* vShaderFilename, char* vShaderFcnName,
                char* pxShaderFilename, char* pxShaderFcnName ) :
                D3D11Shader( iWin, VertexType::VTTTTNCCCC,
                             vShaderFilename, vShaderFcnName, pxShaderFilename, pxShaderFcnName )
{
  setupInputLayout() ;
}

void VTTTTNCCCCShader::setupInputLayout() //override
{
  D3D11_INPUT_ELEMENT_DESC ve[10] ;
  int accum = 0 ;
  ve[ 0 ] = makeInputElement( "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0 ) ;
  ve[ 1 ] = makeInputElement( "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, accum+=3*sizeof(float) ) ;
  ve[ 2 ] = makeInputElement( "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, accum+=4*sizeof(float) ) ;
  ve[ 3 ] = makeInputElement( "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, accum+=4*sizeof(float) ) ;
  ve[ 4 ] = makeInputElement( "TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, accum+=4*sizeof(float) ) ;
  ve[ 5 ] = makeInputElement( "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, accum += 4*sizeof(float) ) ;
  ve[ 6 ] = makeInputElement( "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, accum += 3*sizeof(float) ) ;
  ve[ 7 ] = makeInputElement( "COLOR", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, accum += 4*sizeof(float) ) ;
  ve[ 8 ] = makeInputElement( "COLOR", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, accum += 4*sizeof(float) ) ;
  ve[ 9 ] = makeInputElement( "COLOR", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, accum += 4*sizeof(float) ) ;
  createInputLayout( ve, 10 ) ;
}




VT10NC10Shader::VT10NC10Shader( D3D11Window* iWin, char* vShaderFilename, char* vShaderFcnName,
                char* pxShaderFilename, char* pxShaderFcnName ) :
                D3D11Shader( iWin, VertexType::VT10NC10,
                             vShaderFilename, vShaderFcnName, pxShaderFilename, pxShaderFcnName )
{
  setupInputLayout() ;
}

void VT10NC10Shader::setupInputLayout() //override
{
  const int numElts = 1 + 10 + 1 + 10 ;
  D3D11_INPUT_ELEMENT_DESC ve[ numElts ] ;
  int accum = 0 ;

  int j = 0 ;
  ve[ j++ ] = makeInputElement( "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0 ) ;
  
  // 10 texcoord 
  ve[ j++ ] = makeInputElement( "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, accum+=3*sizeof(float) ) ; //3
  for( int k = 1 ; k <= 9 ; k++ )  // LOOP UNTIL TEXCOORD9
    ve[ j++ ] = makeInputElement( "TEXCOORD", k, DXGI_FORMAT_R32G32B32A32_FLOAT, accum+=4*sizeof(float) ) ;  //ve[ 2 ] = makeInputElement( "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, accum+=4*sizeof(float) ) ;

  ve[ j++ ] = makeInputElement( "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, accum += 4*sizeof(float) ) ;
  
  // 10 color
  ve[ j++ ] = makeInputElement( "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, accum += 3*sizeof(float) ) ; //3
  for( int k = 1 ; k <= 9 ; k++ )
    ve[ j++ ] = makeInputElement( "COLOR", k, DXGI_FORMAT_R32G32B32A32_FLOAT, accum += 4*sizeof(float) ) ;
  
  createInputLayout( ve, numElts ) ;
}

