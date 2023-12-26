#ifndef PARSER_H
#define PARSER_H

#include <string>
using namespace std ;
#include "../util/StdWilUtil.h"
#include "../window/D3DDrawingVertex.h"
#include "../scene/Material.h"
#include "../geometry/Mesh.h"

struct Model ;

bool argCheck( char *fnName, char* str, int numArgsGot, int numArgsExpected ) ;

enum FileType
{
  // the gtp binary file format
  BinaryGTP,

  OBJ,
  PLY,
  ThreeDS,
  COLLADA,
  MD2,
  MD3,
  FBX

} ;

#pragma region defs
// Define the dictionary words
// and their lengths
// In case you're wondering
// why I define *_LEN for each
// its to avoid hard-coding numbers
// like 2, or 6, and also to avoid
// unnecessary calls to strlen (efficiency)
#define MTL_LIBRARY     "mtllib"
#define MTL_LIBRARY_LEN 6
#define MTL_NEW         "newmtl"
#define MTL_NEW_LEN     6
#define MTL_AMBIENT     "Ka"
#define MTL_AMBIENT_LEN 2
#define MTL_DIFFUSION   "Kd"
#define MTL_DIFFUSION_LEN 2
#define MTL_SPECULARITY "Ks"
#define MTL_SPECULARITY_LEN 2
#define MTL_ILLUM       "illum"
#define MTL_ILLUM_LEN   6
#define MTL_NSPEC       "Ns"
#define MTL_NSPEC_LEN   2
#define MTL_D           "d"
#define MTL_D_LEN       1
#define MTL_TEXTURE_NAME "map_Kd"
// There could be others used by other formats
#define MTL_TEXTURE_NAME_LEN 6

/// Switch material 
#define OBJ_CHANGE_MATERIAL "usemtl"
#define OBJ_CHANGE_MATERIAL_LEN 6


#ifdef SINGLE_PRECISION
#define OBJ_VERTEX "v %f %f %f"
#define OBJ_NORMAL "vn %f %f %f"
#define OBJ_TEXCOORD "vt %f %f"
#define OBJ_NS "Ns %f"
#define THREE_FLOATS "%f %f %f"
#else
#define OBJ_VERTEX "v %lf %lf %lf"
#define OBJ_NORMAL "vn %lf %lf %lf"
#define OBJ_TEXCOORD "vt %lf %lf"
#define OBJ_NS "Ns %lf"
#define THREE_FLOATS "%lf %lf %lf"
#endif


// Turn this on to see obj parser verbose output
#define OBJ_PARSER_VERBOSE 0

enum FaceSpecMode
{
  NOFACE    = 0, // ie no face spec mode
  Vertices  = 1 << 0,
  Normals   = 1 << 1,
  Texcoords = 1 << 2
} ;

extern
char * FaceSpecModeName[] ;
#pragma endregion

// abstract base class for a parser
class Parser
{
protected:
  FILE *file ;
  char *origFile ;
  
  MeshType meshType ; // INDEXED OR NONINDEXED meshes to generate.
  VertexType vertexType ; // TYPE OF VERTEX TO PUT IN THE VERTEX BUFFER.
  
  vector<int> faces ;
  Model *model;  // the model object that
  // this Parser will fill
  
  static char* DEFAULT_GROUP ;
  static char* DEFAULT_MATERIAL ;

public:
  Parser() ;

  // You can force indexbuffer / 1 normal / vertex if you like.
  Parser( Model *iModel, MeshType iMeshType, VertexType iVertexType, Material defaultMat ) ;

  ~Parser() ;

  bool open( char* filename ) ;

  // reads the file
  virtual void read() PURE ;

  static bool Load( Model *iModel, char *filename, FileType fileType, MeshType iMeshType, VertexType iVertexType, Material defaultMat ) ;
} ;

class OBJParser : public Parser
{
  friend class Parser ;

  // master base arrays verts, normals, texcoords
  vector<Vector> verts ;
  vector<Vector> normals ;
  vector<Vector> diffuseColor ;
  vector<Vector> texcoords ;

  map<string, Material*> materials ;
  Material* currentMaterial ;
  
  /// This is the total # triangular faces
  /// in the model, after triangulation.
  /// 3*this number is the size of the
  /// ACTIVE combinedVertices? array.
  int numTriangleFacesTOTAL ; // This will be tallied up
  // on this pass, and verified on the second pass.

  // RAW # of vertices, normals and texcoords specified
  // in the OBJ file.
  int numVerts, numNormals, numTexCoords ; // these are very important to keep track of
  // as you are reading the file because v/vt/vn indices can be negative!

  /// An OBJGroup is:
  ///   - face windings that visit the
  ///     3 arrays in OBJParser,
  ///   - different groups would be different
  ///     solid bodies in the same "scene" described by an obj file,
  ///   - different materials within a _group_ mean
  ///     the texture is changing, or the per-vertex color is changing.
  ///   - distinct groups are made
  ///     EACH TIME the material changes.
  ///     The reason for this is
  ///     when you go to render, you want to batch
  ///   - different faces in a group use different materials.
  ///     
  /// they are collections of faces
  /// The file switches between groups as it parses thru
  /// -- while running thru the file faces will be added to different groups
  /// at different times, not necessarily in order.
  struct OBJGroup
  {
  public:
    // vector<Texture*> textures ; // the textures used by _any_ mtlthis group
    string name ;  // The name of the group as it appears in the obj file

    /// How faces are specified in this file.
    /// It can be any combination of these 3
    FaceSpecMode faceSpecMode ;
    // FaceSpecMode used to be an attribute of
    // the entire model file, until model files
    // were discovered that had v//n format for
    // some groups but v/t/n format for other groups,
    // in the same file.

    /// Tells you if this group is
    /// in a specific FaceSpecMode.
    bool isFaceSpecMode( int queryFSM )
    {
      // its just straight equals.
      return faceSpecMode == queryFSM ;
    }
  
    // These 3 faces* arrays are
    // temporary because when we
    // draw we'll need to have
    // pre-assembled the faces
    // into a single array.

    /// The faces array is an index buffer
    /// indexing the vertices and normals
    /// arrays
    vector<int> facesVertexIndices ;

    /// The tie-together for normals is
    /// completely independent of the
    /// tie-together for vertices.. hence
    /// why the #normals != #vertices necessarily.
    vector<int> facesNormalIndices ;
    vector<int> facesTextureIndices ;
    
    // Material can change PER FACE, so we store
    // the material for each face.  Really here,
    // it's stored as a separate material EACH VERTEX,
    // but practically we only allow the material to change per-face.
    vector<Material*> facesMaterialIndices ;

    /// indexCount*:  A running tally of the total number
    /// of indices that will be in 
    /// each of facesVertexIndices, facesNormalIndices, facesTextureIndices
    /// when the parse is complete.
    int indexCountPass1 ;
    int indexCountPass2 ;

    /// vertCount*:  A running tally of the total number
    /// of vertices that will be in
    /// the model when the parse is complete
    /// REDUNDANT: This is A COMBINED COUNT,
    /// it isn't the total vertices specified in the obj file.
    /// It's the COMBINED total vertices
    int vertCountPass1 ;
    int vertCountPass2 ;

    /// The final array of vertex coords.
    //vector<AllVertex> combinedVertices ;

    OBJGroup( string iname )
    {
      name=iname ;
      indexCountPass1 = 0 ;
      indexCountPass2 = 0 ;
      vertCountPass1 = 0 ;
      vertCountPass2 = 0 ;

      faceSpecMode   = NOFACE ; // NO FACE SPEC MODE YET
    }
    
    // no dynamic alloc
    ~OBJGroup(){}
  } ;

  // These are the groups in the objfile.
  map<string, OBJGroup*> groups ;


public:
  OBJParser( Model *iModel, MeshType iMeshType, VertexType iVertexType, Material defaultMat ) ;

  // string parse routines
  int getFaceSpecMode( char *str ) ;
  
  // counts numbers items in "f 3/5/70 82/9/10 10/4/9   55/2/69"
  int countVerts( char *buf ) ;
  
  bool setK( Material* mat, char *str ) ;

  // parse vertex and extract faces
  bool parseVertex( char *buf, Vector *v ) ;
  
  bool parseNormal( char *buf, Vector *v ) ;

  bool parseTexcoord( char *buf, Vector *v ) ;

  void extractFaces( char *buf, int numVerts, OBJGroup *group ) ;
  
  void loadMtlFile( char *mtllibName ) ;
  
  Material* getMaterial( string materialName ) ;
  
  OBJGroup* getGroup( string groupName ) ;

  void pass1() ;

  void pass2() ;

  void setupMeshes() ;

  void read() override ;
} ;

class PLYParser : public Parser
{
  friend class Parser ;

public:
  PLYParser( Model*iModel, MeshType iMeshType, VertexType iVertexType, Material iMat ) ;

  void read() override ;
} ;

#endif
