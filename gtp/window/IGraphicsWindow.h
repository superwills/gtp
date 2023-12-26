#ifndef GRAPHICSWINDOW_H
#define GRAPHICSWINDOW_H

#include "WindowClass.h"

class Scene ;
struct Mesh ;
union Vector ;
union Matrix ;
union ByteColor ;

// universal (xplatform) vertex types
enum VertexType
{
  PositionColor, // used for debug elements
  PositionTextureColor, // used for 
  PositionNormalColor,  // most common format for lighted stuff
  VTTTTNC,
  VTTTTNCCCC,
  VT10NC10
} ;

enum FillMode{ Solid, Wireframe, Points } ;

// cull the..
enum CullMode{ CullNone, CullClockwise, CullCounterClockwise } ;

enum TextureSamplingMode { Point, Linear, AnisotropicSampling } ;



// the interface contract a IGraphicsWindow derivate
// must provide
class IGraphicsWindow : public Window
{
protected:
  bool deviceDirty ;  // if the device is dirty, then
  // the SUBCLASSED (GTPWindow) needs to reload all its
  // resources and recreate all its vertex buffers.

  FillMode fillmode ;

  CullMode cullmode ;

public:
  IGraphicsWindow( HINSTANCE hInst, TCHAR* windowTitleBar,
                   int windowXPos, int windowYPos,
                   int windowWidth, int windowHeight ) ;
  
  /// Every IGraphicsWindow must provide::
  // these are partially implemented in class Window
  virtual void activate() PURE ;
  virtual void deactivate() PURE ;
  virtual bool setSize( int width, int height, bool fullScreen ) PURE ;
  virtual bool fullScreenInMaxResolution() PURE ; // go fullscreen, in max resolution
  virtual int getWidth() PURE ;
  virtual int getHeight() PURE ;

  
  ////////////////////////////
  ////////////////////////////
  ////// PURE FUNCTIONS //////
  ////////////////////////////
  
  // you have to be able to Render a scene
  virtual void render( Scene * scene ) PURE ;

  // re-bind the main screen backbuffer as the rendertarget
  virtual void bindScreenRenderTarget() PURE ;
  virtual void step() PURE ;

  // Must complete all initialization here
  virtual bool init( int width, int height ) PURE ;
  virtual void screenshot() PURE ;
  virtual void screenshot( char* additional ) PURE ;
  virtual void clear( ByteColor toColor ) PURE ;
  
  virtual void alphaBlend( bool on ) PURE ;

  virtual void setBackgroundColor( ByteColor color ) PURE ;
  virtual void toggleFillMode() PURE ;
  virtual void setFillMode( FillMode iFillMode ) PURE ;
  virtual void setTextureSamplingMode( TextureSamplingMode texSampMode ) PURE ;
  
  // A general undo( statename ) would be helpful
  virtual void revertPointSize() PURE ;
  virtual void setPointSize( float newPtSize ) PURE ;

  virtual bool createFont( int fontId, int height, int boldness, char* fontName ) PURE;
  virtual bool loadTexture( int textureId, char* filename, ByteColor backgroundColor ) PURE;

  virtual void cullMode( CullMode cullThese ) PURE ;

  virtual void drawText( int fontId,
    const char *str, ByteColor color,
    float x, float y, float boxWidth, float boxHeight,
    DWORD formatOptions ) PURE ;

  virtual bool beginText() PURE ;
  virtual bool endText() PURE ;

  // The first thing you call before drawing anything. Checks everything's a-ok with
  // gpu.
  virtual bool beginDrawing() PURE ;
  virtual bool endDrawing() PURE ;
  virtual bool present() PURE ; //flips the backbuffer

} ;

#endif