#ifndef MODEL_H
#define MODEL_H

#include "Shape.h"
#include "AABB.h"
#include "Mesh.h"
#include "../model_loading/Parser.h"


struct Model : public Shape
{
  friend class Parser ; // establish the relationship between Parser and Model.
  // Parsers load the Model, but after they LOAD the model the Parser can
  // be destroyed, so by its nature Parsers live very temporarily.
  // after construction of the object, the Parser can be destroyed. (which is why
  // the Parser class does NOT extend Model).
  // creates a plain model. used for visualizations
  // and manually constructed models. does not load from file.
  Model( string iModelNameNOTAFilename ) ;
  Model( string iModelNameNOTAFilename, Material iMaterial ) ;

  Model( char* filename, FileType fileType, MeshType iMeshType, Material iMaterial ) ;

  // You can only load a model from a file
  Model( char* filename, FileType fileType, MeshType iMeshType, VertexType iVertexType, Material iMaterial ) ;
private:
  // helper to ctors to avoid redundant code
  void Load( char* filename, FileType fileType, MeshType iMeshType, VertexType iVertexType, Material iMaterial ) ;
public:
  Model( FILE* binaryFile ) ;

  int save( FILE* binaryFile ) override ;

  virtual ~Model() ;

  // Used to assemble a mesh, triangle by triangle, using code
  void addTri( const Vector& a, const Vector& b, const Vector& c ) ;


  // creates a mesh with magnitude determined somecow and color too
  static Model* visualize( string iName, int slices, int stacks, const Vector& center,
    function<Vector (real tElevation, real pAzimuth)> colorFunc, function<real (real tElevation, real pAzimuth)> magFunc ) ;

  // radius determined by function, colors determined by sign of function.
  static Model* visualize( string iName, int slices, int stacks, const Vector& center,
    const Vector& baseColorPlus, const Vector& baseColorMinus,
    function<real (real tElevation, real pAzimuth)> R ) ;

  // Feed it a color function only, which is
  // used as magnitude as well (.len()).
  static Model* visualize( string iName, int slices, int stacks, const Vector& center,
    function<Vector (real tElevation, real pAzimuth)> colorFuncUsedAsMagAsWell ) ;

  // creates a SPHERE with colors you want
  static Model* visualize( string iName, int slices, int stacks, const Vector& center,
    function<Vector (real tElevation, real pAzimuth)> colorFunc, real fixedRadius ) ;


} ;

#endif