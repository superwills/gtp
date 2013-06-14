#ifndef D3D11SHADER_H
#define D3D11SHADER_H

#include <D3D11.h>
#include <D3DX11.h>

class D3D11Window ;
enum VertexType ;

class D3D11Shader abstract
{
protected:
  VertexType vertexType ;
  ID3D11InputLayout *layout ;
  ID3D10Blob *vblob, *pxblob ;
  ID3D11VertexShader *vsh ;
  ID3D11PixelShader *psh ;
  D3D11Window *win ;

  const static char* VERTEX_SHADER_MODEL_5 ;
  const static char* PIXEL_SHADER_MODEL_5 ;

public:
  D3D11Shader( D3D11Window* iWin, VertexType iVertexType, char* vShaderFilename, char* vShaderFcnName,
               char* pxShaderFilename, char* pxShaderFcnName ) ;

  ~D3D11Shader() ;

  D3D11_INPUT_ELEMENT_DESC makeInputElement( char* semantic, int position, DXGI_FORMAT dxgiFormat, int alignedByteOffset ) ;

  bool createVertexShader( ID3D10Blob** blob, ID3D11VertexShader** shader ) ;

  bool createPixelShader( ID3D10Blob** blob, ID3D11PixelShader** pxShader ) ;

  void activate() ;

  bool loadHLSLBlob( ID3D10Blob** blob, char* filename, char* fcnName, const char* shaderModel ) ;

  bool createInputLayout( D3D11_INPUT_ELEMENT_DESC *ve, int numElts ) ;

  // the function derived instances must
  // provide and that makes this class abstract
  virtual void setupInputLayout() = 0 ;

  void setTexture( int slot, ID3D11ShaderResourceView* tex ) ;

  // unset the active texture
  void unsetTexture( int slot ) ;
} ;





class VCShader : public D3D11Shader
{
public:
  VCShader( D3D11Window* iWin, char* vShaderFilename, char* vShaderFcnName,
             char* pxShaderFilename, char* pxShaderFcnName ) ;

  void setupInputLayout() override ;
} ;

class VTCShader : public D3D11Shader
{
public:
  VTCShader( D3D11Window* iWin, char* vShaderFilename, char* vShaderFcnName,
              char* pxShaderFilename, char* pxShaderFcnName ) ;

  void setupInputLayout() override ;
} ;

class VNCShader : public D3D11Shader
{
public:
  VNCShader( D3D11Window* iWin, char* vShaderFilename, char* vShaderFcnName,
              char* pxShaderFilename, char* pxShaderFcnName ) ;

  void setupInputLayout() override ;
} ;

class VTTTTNCShader : public D3D11Shader
{
public:
  VTTTTNCShader( D3D11Window* iWin, char* vShaderFilename, char* vShaderFcnName,
                  char* pxShaderFilename, char* pxShaderFcnName ) ;

  void setupInputLayout() override ;
} ;

class VTTTTNCCCCShader : public D3D11Shader
{
public:
  VTTTTNCCCCShader( D3D11Window* iWin, char* vShaderFilename, char* vShaderFcnName,
                  char* pxShaderFilename, char* pxShaderFcnName ) ;

  void setupInputLayout() override ;
} ;

class VT10NC10Shader : public D3D11Shader
{
public:
  VT10NC10Shader( D3D11Window* iWin, char* vShaderFilename, char* vShaderFcnName,
                  char* pxShaderFilename, char* pxShaderFcnName ) ;

  void setupInputLayout() override ;
} ;


#endif