#include "Model.h"

// Used for visualization and Light objects
Model::Model( string iModelNameNOTAFilename )
{
  name = iModelNameNOTAFilename ;
  aabb = 0 ;
}

Model::Model( string iModelNameNOTAFilename, Material iMaterial )
{
  name = iModelNameNOTAFilename ;
  material = iMaterial ;
  
}

Model::Model( char* filename, FileType fileType, MeshType iMeshType, Material iMaterial )
{
  // defaults to indexed.
  Load( filename, fileType, iMeshType, defaultVertexType, iMaterial ) ;
}

// You can only load a model from a file
Model::Model( char* filename, FileType fileType, MeshType iMeshType, VertexType iVertexType, Material iMaterial )
{
  Load( filename, fileType, iMeshType, iVertexType, iMaterial ) ;
}

void Model::Load( char* filename, FileType fileType, MeshType iMeshType, VertexType iVertexType, Material iMaterial )
{
  name = filename ;
  material = iMaterial ;
  
  if( Parser::Load( this, filename, fileType, iMeshType, iVertexType, iMaterial ) )  // load filename into THIS.
  {
    for( int i = 0 ; i < meshGroup->meshes.size() ; i++ )
    {
      if( !meshGroup->meshes[i]->createVertexBuffer() )
      {
        // if vertex buffer creation fails,
        // then it was a bad mesh (had 0 verts), so delete it and remove it
        // from the collection.  this happens if you edit a mesh and
        // delete all the faces from a group
        DESTROY( meshGroup->meshes[i] ) ;
        meshGroup->meshes.erase( meshGroup->meshes.begin() + i-- ) ; // decrement i after deletion
      }
    }
  }
}

Model::Model( FILE* binaryFile ) : Shape( binaryFile )
{
  // there is no model-specific stuff
  // the Shape base class loader loads all
}

int Model::save( FILE* binaryFile ) // override, because need a MODEL* header.
{
  HeaderShape head ;
  head.ShapeType = ModelShape ;
  int written = 0 ;
  written += sizeof(HeaderShape)*fwrite( &head, sizeof( HeaderShape ), 1, binaryFile ) ;
  written += Shape::save( binaryFile ) ;
  return written ;
}

Model::~Model()
{
  DESTROY( aabb ) ;
}

void Model::addTri( const Vector& a, const Vector& b, const Vector& c )
{
  // if there is no meshgroup, create one
  if( !meshGroup )  meshGroup = new MeshGroup( this, defaultMeshType, defaultVertexType, 1200 ) ;

  // if there is no mesh at entry 0, add one
  if( !meshGroup->meshes.size() )
    meshGroup->addMesh( new Mesh( this, defaultMeshType, defaultVertexType ) ) ;

  // now add a tri to mesh 0.
  meshGroup->meshes[0]->addTri( a, b, c ) ;
}

#pragma region model viz production
Model* Model::visualize( string iName, int slices, int stacks, const Vector& center,
    function<Vector (real tElevation, real pAzimuth)> colorFunc, function<real (real tElevation, real pAzimuth)> magFunc )
{
  Model* model = new Model( iName ) ;
  Mesh::visualizeCreateMesh( model, slices, stacks, MeshType::Indexed, VertexType::VT10NC10, center, colorFunc, magFunc ) ;
  return model ;
}

Model* Model::visualize( string iName, int slices, int stacks,
    const Vector& center,
    const Vector& baseColorPlus, const Vector& baseColorMinus,
    function<real (real tElevation, real pAzimuth)> magFunc )
{
  Model *model = new Model( iName ) ;
  function<Vector (real tElevation, real pAzimuth)> colorFunc =
    [magFunc, baseColorPlus, baseColorMinus](real tElevation, real pAzimuth) -> Vector 
    {
      // you can also scale by square of magnitude,
      // or by the normal of some artificial light source
      real m = magFunc(tElevation,pAzimuth) ;
      return m >= 0 ? m*baseColorPlus : -m*baseColorMinus ; // flip sign.. color should never be returned _as_ "negative"
    } ;
  
  Mesh::visualizeCreateMesh( model, slices, stacks, MeshType::Indexed, VertexType::VT10NC10, center, colorFunc, magFunc ) ;
  return model ;
}

Model* Model::visualize( string iName, int slices, int stacks, const Vector& center,
  function<Vector (real tElevation, real pAzimuth)> colorFuncUsedAsMagAsWell )
{
  Model *model = new Model( iName ) ;
  
  // magnitude determines shape
  function<real (real tElevation, real pAzimuth)> fMags = [colorFuncUsedAsMagAsWell]( real tElevation, real pAzimuth ) -> real {
    return colorFuncUsedAsMagAsWell( tElevation, pAzimuth ).len() ;
  } ;

  Mesh::visualizeCreateMesh( model, slices, stacks, MeshType::Indexed, VertexType::VT10NC10, center, colorFuncUsedAsMagAsWell, fMags ) ;
  return model ;
}

Model* Model::visualize( string iName, int slices, int stacks, const Vector& center,
  function<Vector (real tElevation, real pAzimuth)> colorFunc, real fixedRadius )
{
  Model *model = new Model( iName ) ;
  
  // magnitude determines shape
  function<real (real tElevation, real pAzimuth)> fMags = [fixedRadius](real tEl, real pAz)->real{ return fixedRadius ; } ; // no dep on theta, phi

  Mesh::visualizeCreateMesh( model, slices, stacks, MeshType::Indexed, VertexType::VT10NC10, center, colorFunc, fMags ) ;
  return model ;
}


#pragma endregion

