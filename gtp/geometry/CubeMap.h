#ifndef CUBEMAPLIGHT_H
#define CUBEMAPLIGHT_H

#include "Sphere.h"
#include <D3D11.h>
#include <D3DX11.h>

//Eigen for SparseVector/SparseMatrix
#ifdef real
#undef real
#endif
#include <Eigen/Eigen> // for the sparse vector representation
#include <Eigen/Sparse>
#define real double
#include "../math/SparseMatrix.h"

struct Model ;
struct D3D11Surface ;
struct RayCollection ;

enum CubeFace { 
  RAIL=-1,// not on a face
       // RIGHT    UP
  PX=0,//  -Z      -Y
  NX,  //  +Z      -Y
  PY,  //  +X      +Z
  NY,  //  +X      -Z
  PZ,  //  +X      -Y
  NZ   //  -X      -Y
} ;


#pragma region railing
// a railed cubemap is a different type of cubemap, with
// N a power of 2, there are
// (N-1)*(N-1) pixels on each face, and
// a "rail" of pixels that are SHARED between
// two adjacent faces.  It looks like this, for N=4:
//
//      ☺☻☺
//      ☻☺☻
//      ☺☻☺
// O•O•O•O•O•O•O•O•
// •☺☻☺•☺☻☺•☺☻☺•☺☻☺
// O☻☺☻O☻☺☻O☻☺☻O☻☺☻
// •☺☻☺•☺☻☺•☺☻☺•☺☻☺
// O•O•O•O•O•O•O•O•
//      ☺☻☺
//      ☻☺☻
//      ☺☻☺
//
// ☺ - even on face
// ☻ - odd on face
// O - even on rail
// • - odd on rail
//
// NAMING (this is extremely important!):
//
//       ....
//       .PY.
//       ....
// ....TOP_RAIL.......
// L    L    L    L
// N NX P PZ P PX N NZ
// X    Z    X    Z
// ....BOT_RAIL.......
//       ....
//       .NY.
//       ....
//
// Total size of each face is (N+1)*(N+1),
// but the "rails" are shared between faces.
// Note when you fold the cubemap the rail gets
// shared with the top/bottom faces as well

enum Rail
{
  TOP_RAIL=0,  BOT_RAIL,  LNX_RAIL,
  LPZ_RAIL,    LPX_RAIL,  LNZ_RAIL
} ;
enum RailCorner
{
  // named in order of order in rails array.
  LeftBack, LeftFront, RightFront, RightBack
} ;

#define IsSideRail(rail) (rail > Rail::BOT_RAIL)
#pragma endregion

// For point light approximation extraction
struct PosNColor{ Vector pos, color ;
  PosNColor(){}
  PosNColor( const Vector& iP, const Vector& iC ) : pos(iP), color( iC ) {}
} ;

extern char* CubeFaceName[6] ;

// Cubemaps, as specified in DDS files, are
// LH.  The FWD set (converting, RH) are identical,
// but the directions considered as RIGHT and UP
// are different.
extern Vector cubeFaceBaseConvertingFwd[6] ;
extern Vector cubeFaceBaseConvertingRight[6] ;
extern Vector cubeFaceBaseConvertingUp[6] ;

extern Vector cubeFaceBaseRHFwd[6] ;
extern Vector cubeFaceBaseRHRight[6] ;
extern Vector cubeFaceBaseRHUp[6] ;

extern Vector cubeFaceBaseColor[6] ;

#define CUBEMAP_DEBUG_MODE 1

// A cubemap is _intersected_ with as a sphere,
// which is cheaper than treating it as a huge mesh.
struct CubeMap : public Sphere
{
  static RayCollection* rc ;

  // linear vector used, so multiplication
  // by another cubemap is simplified.
  // currentSource is the cubemap you WANT TO USE.
  // Because 
  vector<Vector> *colorValues, *rotatedColorValues, *currentSource, 
    *railed ; // use 'railed' only used if
    // you're doing a quincunx lattice transform
  
  Eigen::SparseVector<Vector> sparseTransformedColor ;
  Eigen::SparseVector<Vector> newSparseTransformedColor ; // when interreflecting, this is the
  // sum of functions you see

  // Here is another imp
  //SparseMatrix sparseM ;

  int pixelsPerSide ;
  bool bilinear ; // use bilinear interpolation when calling getColor

  ID3D11Texture2D* cubeTex ;
  ID3D11ShaderResourceView* cubeTexRsv ;

  // the shViz is the spherical harmonics viz, which is the one used in SH lighting.
  Model *viz, *shViz ;
  
  CubeMap( string iName, char* iCubeTexDDSFile,
    //int iSlices, int iStacks,
    int icosaSubds,
    bool iBilinear,
    real cubeMapVizRadius ) ; // loads a DDS from file

  // You can also make a cubemap out of 6 d3d11surfaces,
  // ordered px,nx,py,ny,pz,nz
  CubeMap( D3D11Surface* surfaces[6] ) ;

  // I need to know WHO called this method
  // and a fcr for the precomputed form factor cosines
  ////CubeMap( AllVertex& vertex, FullCubeRenderer* fcr, D3D11Surface* surfaces[6] ) ;
  // Constructs a cubemap from 
  // a group of shapes by casting rays
  // from the origin in the direction of every pixel
  // and saving the emissive term
  // px: is pixels per side of the cubemap.
  // Creates a cubemap from a scene->lights collection.
  // http://www.parashift.com/c++-faq-lite/named-ctor-idiom.html
  CubeMap( string iName, int px, Scene* scene ) ;

  CubeMap( const CubeMap& o ) ;
  
  virtual ~CubeMap() ;

  void deleteEverythingExceptSparseData() ;

  void defInit() ;

  void visualizeAO( const Vector& normal ) ;

  void createCubemapMesh() ;

private:
  // WRITES PIXELS FROM currentSource INTO dstPtr.
  void writePx( ByteColor* dstPtr, int pxPerRow ) ;
  void railedWritePx( ByteColor* dstPtr, int pxPerRow ) ;

public:
  // converts this cubemap to SH, right handed coordinate system.
  static void pregenDirections( int px ) ;

  SHVector* shProject( const Vector& scale ) override ;

  void activate( int slotNo ) ;
  void downsample( int newPxPerSide ) ;
  //void ↓( int newPxPerSide ) ; //downsample!

  // allow compression scheme to pass as a parameter (Spherical wavelet etc)
  // The FACTOR is % of maximummm signal value MUST BE in order to
  // be RETAINED.  So if you set this value extremely high (like near 1.0),
  // then you'll only retain the MAXIMUM difference.
  // If you set it extremely low (0.0), then there will be
  // NO COMPRESSION AT ALL.
  // members specific to the compress() function:
  static Vector compressCumulativeRemoved ; // running count of cumulative components cut.
  static Vector compressCumulativeKept ;
  static Vector compressCumulativeProcessed ; // running count of components processed by compress();

  // keepSource means to KEEP the original data AS WELL
  // as generate compressed data.  Usually you want to destroy
  // the original for the visibility maps, and keep the
  // entire original for the source light cubemap (in case
  // you need to use it to generate a rotated copy)
  // this compresses 
public: 
  #pragma region compression
  // runs currently selected compression routine
  void compress() ;

  // base compression/uncompression functions
  int faceCompress( real threshold ) ; 
  int faceCompressOriginal( real threshold, bool keepSource ) ;//CONV
  int faceCompressRotated( real threshold ) ;//CONV

  // operates on currentSource, assuming it's
  // railed. You are responsible to DELETE
  // currentSource if you do not want to keep it.
  int railedCompress( real threshold ) ;
  int railedCompress( real threshold, bool keepSource ) ;//CONV
  // you don't have a railedCompressOriginal
  // because whenever you 

  // Specify the target to uncompress into.
  void faceUncompress( vector<Vector>* uncompressed ) ;

  // the railed function is a different size
  void railedUncompress( vector<Vector>* uncompressed ) ;

  // tells you if the cubemap has been compressed or not
  // (if the colorValues array exists, it WAS compressed previously)
  bool isCompressed()
  {
    return sparseTransformedColor.nonZeros() ; 
    //return sparseM.vals.size() ;
  }

  // The source is usually stored within the class
  // It would be better to setSource using a 
  // MEMBER_POINTER, but that just makes the syntax
  // so much worse.
  void setSource( vector<Vector> *newSource ) ;

  // revert to the last used source
  void revertToPreviousSource() ;
 
private:
  /// You don't have to juggle this yourself - 
  /// named member functions do this.
  vector<Vector> *previousSource ;
  #pragma endregion

  #pragma region saving
public:
  // saves a face
  void faceSave( char* filename ) ;
  void railedSave( char* filename ) ;

  //saves out the wavelet representation
  void faceSaveWaveletRepn( char* filename ) ;
  void railedSaveWaveletRepn( char* filename ) ;

  void faceSaveUncompressed( char* filename ) ;
  void railedSaveUncompressed( char *filename ) ; 

  void faceSaveRotated( char* filename ) ;
  #pragma endregion

  ////CubeMap( const HeaderLight& head, FILE* file ) ;
  ////int save( FILE* file ) override ;

  // Cubemaps ALWAYS emit light. that's why they're there.
  bool isEmissive() override { return true ; }

  void draw() override ;

  // get linear array index from face#, row# and col#
  // When intel intrinsics were used, this function caused
  // a compiler error.
  inline int dindex( int face, int row, int col )
  {
    // just really long to type, so use it.
    return face*pixelsPerSide*pixelsPerSide + row*pixelsPerSide + col ;
  }
  //#define dindex( face, row, col ) (face*pixelsPerSide*pixelsPerSide + row*pixelsPerSide + col)

  // get the nearest index based on a direction
  int index( const Vector& direction )
  {
    // retrieve the texel
    int face ;
    real row,col;
    getFaceRowCol( direction, face, row, col ) ;
    return dindex( face, (int)(row), (int)(col) ) ; //crude rounding
  }

  // get linear array index from pixels per side, face#, row# and col#
  static int index( int pxPerSide, int face, int row, int col )
  {
    return face*pxPerSide*pxPerSide + row*pxPerSide + col ;
  }
  
  // UNINDEX: get face#, row# and col# from linear array INDEX
  void getFaceRowCol( int index, int& face, int& row, int& col ) 
  {
    // take the index and translate that back to a face, row, col
    // does the opposite of "index"
    face = index / (pixelsPerSide*pixelsPerSide) ;

    // get down to the row/col indexing
    int i = index - face*pixelsPerSide*pixelsPerSide ; // alternatively
    // now sub off multiples of (pxPerSide*pxPerSide) until you are in
    // the space 0..pxPerSide*pxPerSide
    //int i2 = index % (pxPerSide*pxPerSide) ;

    row = i / pixelsPerSide ;
    col = i % pixelsPerSide ;
  }

  // Remaps an rh coordinate system face,row,col to
  // d3d11's LH coordinate system (flips where needed)
  // doesn't change the original variables since those
  // are typically being iterated on when this function is called.
  void remapLHToRH( int FACE, int row, int col, int &trow, int& tcol ) 
  {
    // we are about to convert this LH cubemap to RH
    static int FlipHorizontal=1, FlipVertical=2 ;
    int flipping=0;

    flipping = 0;
    switch( FACE )
    {
      case CubeFace::PX:
      case CubeFace::NX:
      case CubeFace::PZ:
      case CubeFace::NZ:
        flipping = FlipHorizontal ;
        break ;
      case CubeFace::PY:
      case CubeFace::NY:
        flipping = FlipVertical ;
        break ;
    }

    trow = row ;
    tcol = col ;
    if( flipping&FlipHorizontal )
      tcol = pixelsPerSide - col - 1;
    if( flipping&FlipVertical )
      trow = pixelsPerSide - row - 1;
  }

  void remapRHToLH( int FACE, int row, int col, int &trow, int& tcol ) 
  {
    // we are about to convert this LH cubemap to RH
    static int FlipHorizontal=1, FlipVertical=2 ;
    int flipping=0;

    flipping = 0;
    switch( FACE )
    {
      case CubeFace::PX:
      case CubeFace::NX:
      case CubeFace::PZ:
      case CubeFace::NZ:
        flipping = FlipHorizontal ;
        break ;
      case CubeFace::PY:
      case CubeFace::NY:
        flipping = FlipVertical ;
        break ;
    }

    trow = row ;
    tcol = col ;
    if( flipping&FlipHorizontal )
      tcol = pixelsPerSide - col - 1;
    if( flipping&FlipVertical )
      trow = pixelsPerSide - row - 1;
  }

  void prelight( const Vector& vnormal ) ;

  // provides a color value for
  // tElevation, pAzimuth
  // overrides getColor thru Shape->Sphere
  Vector getColor( real tElevation, real pAzimuth, ColorIndex colorIndex ) override ;

  // wraps out of bounds row/col access
  // on a particular face to
  // the correct row and column on the correct face.
  // So (face=0, row=-1, col=0) might map to
  // (face=1, row=255,col=0) or something like that.
  void wrap( int &face, int &row, int &col ) ;

  int getWrappedIndex( int face, int row, int col ) ;
  
  #pragma region railed functions
  // If FACE==RAIL, then row is actually the railNumber.
  int railedIndex( int face, int rowOrRail, int colOrAlong ) ;

  // topsize=bottomsize
  inline int railTopSize() { return 4*pixelsPerSide; }
  inline int railSideSize() { return pixelsPerSide-1; }

  inline int railFacesSize(){
    return 6 * SQUARE( pixelsPerSide - 1 ) ;
  }
  inline int railRailsSize(){
    return CUBE( pixelsPerSide+1 ) - railFacesSize() - CUBE( pixelsPerSide-1 ) ;
  }

  // retrieves size of entire railed cube
  inline int railSize(){
    return railFacesSize() + railRailsSize() ;
  }

  // like railIndex, but with FACE=-1, and col=0 all the time
  int railOffsetTo( int rail ){
    // the starting index in the rail
    // The rails are stored as:
    // | TOP RAIL | BOT RAIL | LNX RAIL | LPZ RAIL | LPX RAIL | LNZ RAIL |
    switch( rail )
    {
    case Rail::TOP_RAIL:
    case Rail::BOT_RAIL:
      return railFacesSize() + rail*railTopSize() ; // 0 for top rail,
    
    case Rail::LNX_RAIL:
    case Rail::LPZ_RAIL:
    case Rail::LPX_RAIL:
    case Rail::LNZ_RAIL:
      return railFacesSize() + 2*railTopSize() + (rail-2)*railSideSize() ;

    default:
      error( "Bad rail index" ) ;
      return 0 ;
    }
  }
  
  int railGetRailSize( int rail ) {
    if( IsSideRail(rail) ) return railSideSize() ;
    else return railTopSize() ;
  }
  
private:
  // Gets you the cubemap face number
  // CLOSEST to the "along" value.
  // For TOP_RAIL these are BELOW you,
  // for BOT_RAIL these are ABOVE you.
  // You will never get PY or NY here,
  // even those are also ALONG TOP,BOT_RAIL.

  // I tried to name these carefully, because
  // they are very simple concepts that come up again
  // and again and need to have names.
  // ALONG is how far along the top/bottom rails you are.
  int railedGetFaceBesideRail( int along ) ;

  // Gets you the "along" value for
  // the left side corner of a face.
  int railedGetAlongTopRailBaseOffset( int face ) ;

  // MOVING on a rail is hard.
  // This function MOVES YOU on a rail.
  // With the normal move (on just the cubemap)
  // you adjust ROW and COL and then call WRAP,
  // if row/col became negative, you would wrap
  // to the neighbouring faces.
  // Now, there is a MOVE function which resolves
  // WHERE YOU ARE NOW. If you are on the rail,
  // you move differently than if you are on a face.
  // a helper function to railedWrap,
  // this steps you OFF the rail, in the direction
  // of row, col.
  // along is how far along the rail you are.
  void railedMoveNWrapRail( int &face, int &rail, int &along, int rowOffset, int colOffset ) ;

  // helper to railed wrap
  void railedMoveNWrapFace( int &face, int &row, int &col, int rowOffset, int colOffset ) ;

public:
  // adjusts face,row,col to make your move
  // in rowOffset, colOffset.
  // calls railedWrapRail or railedWrapFace depending on
  // You CAN'T just move the row,col like you could in
  // the regular cubemap motion BECAUSE
  // row,col mean something different and must be
  // displaced differently for
  // the RAIL positions than they are for
  // the face positions.
  void railedMove( int &face, int &row, int &col, int rowOffset, int colOffset ) ;

  // gets you the index YOU WILL MOVE TO for a rowOffset, colOffset.
  // it calls railedMove() to modify face,row,col by rowOffset,colOffset,
  // then immediately calls railedIndex(), so you don't have to.
  int railedIndexMove( int face, int row, int col, int rowOffset, int colOffset ) ;

  void railedMake() ;

  // because the railed cubemap has both
  // a "rails" array and a "faces" array,
  // look up can be tedious (require if stmt split)
  // If FACE==RAIL, then row is actually the railNumber.
  // A little bit complicated, but this always uses
  // CURRENTSOURCE, so you need to setsource before you call it.
  Vector railedPxPoint( int face, int row, int col ) ;
  #pragma endregion

  // samples the cube map.
  // v should be on the unit sphere
  // gets cubemap pixel color based on direction vector
  Vector px( const Vector& v ) ;

  // sampling the cubemap using this "px" function
  Vector pxBilinear( int face, real row, real col ) ;

  // point sampling: integral
  Vector pxPoint( int face, int row, int col ) ;

  // don't use these *RH functions. They are bad.
  Vector getDirectionRH( int face, int row, int col ) ;
  // get a direction based on the INDEX in the cubemap
  // vector
  Vector getDirectionRH( int index ) ;
  
  // gets you the direction for a face,row,col
  // inside the vector you get back is U,V,LEN (r).
  Vector getDirectionConvertedLHToRH( int index ) ;
  Vector getDirectionConvertedLHToRH( int face, int row, int col ) ;
  
  // __3__ outputs
  void getFaceRowCol( Vector direction, int& oface, real& orow, real& ocol ) ;

  ///void createMesh( MeshType meshType, VertexType vertexType ) ;

  // rotates the wavelet representation of the cubemap
  void rotateWavelet( Matrix matrix ) ;

  void rotate( Matrix matrix ) ;

  void rotate( const Vector& axis, real radians ) ;

  /// axis rotation is a bit faster than general
  void rotate( char axis, real radians ) ;

  void makePointLightApproximation( int icoSubdivs, int maxLights, 
    int raysPerIcoFace, vector<PosNColor>& pts, bool normalized, real& omaxmag ) ;
} ;


#endif