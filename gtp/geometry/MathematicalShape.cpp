#include "MathematicalShape.h"
#include "Mesh.h"
#include "../math/SHVector.h"

#include "../window/GTPWindow.h"

MathematicalShape::MathematicalShape(
  string iname, Vector iPos,
  int magFuncIndex, int colorFuncDiffuseIndex, int colorFuncEmissiveIndex,
  int slices, int stacks, const Material& iMaterial )
{
  material = iMaterial ;
  name = iname ;
  magFunc = magFunctions[ magFuncIndex ] ;
  
  colorFuncDiffuse = colorFunctions[ colorFuncDiffuseIndex ] ;
  colorFuncEmissive = colorFunctions[ colorFuncEmissiveIndex ];

  // there are __2__ meshes:  the emissive mesh, and the diffuse mesh.
  Mesh::visualizeCreateMesh( this, slices, stacks, MeshType::Nonindexed, VertexType::VT10NC10, 0, colorFuncDiffuse, magFunc ) ;
  Mesh::visualizeCreateMesh( this, slices, stacks, MeshType::Nonindexed, VertexType::VT10NC10, 0, colorFuncEmissive, magFunc ) ;

  this->pos = iPos ;
  this->transform( Matrix::Translate( iPos ) ) ;
  
  // sh project it
  ///DESTROY( shProjection ) ;
  ///shProjection = new SHVector( sh, sh->Bands, lightFunction ) ;
}

MathematicalShape::MathematicalShape( string iname, Vector iPos,
    function<real ( real tElevation, real pAzimuth )> iMagFunc,
    function<Vector ( real tElevation, real pAzimuth )> iColorFuncDiffuse,
    function<Vector ( real tElevation, real pAzimuth )> iColorFuncEmissive,
    int slices, int stacks, const Material& iMaterial )
{
  material = iMaterial ;
  name = iname ;
  magFunc = iMagFunc ;
  colorFuncDiffuse = iColorFuncDiffuse ;
  colorFuncEmissive = iColorFuncEmissive ;

  // there are __2__ meshes:  the emissive mesh, and the diffuse mesh.
  Mesh::visualizeCreateMesh( this, slices, stacks, MeshType::Nonindexed, VertexType::VT10NC10, 0, colorFuncDiffuse, magFunc ) ;
  Mesh::visualizeCreateMesh( this, slices, stacks, MeshType::Nonindexed, VertexType::VT10NC10, 0, colorFuncEmissive, magFunc ) ;

  this->pos = iPos ;
  this->transform( Matrix::Translate( iPos ) ) ;
}

MathematicalShape::MathematicalShape( string iname, Vector iPos,
    function<real ( real tElevation, real pAzimuth )> iMagFunc,
    function<Vector ( real tElevation, real pAzimuth )> iColorFuncAll,
    int slices, int stacks, const Material& iMaterial )
{
  material = iMaterial ;
  name = iname ;
  magFunc = iMagFunc ;
  colorFuncDiffuse = iColorFuncAll ;
  colorFuncEmissive = iColorFuncAll ;

  // 1 mesh, because they're the same
  Mesh::visualizeCreateMesh( this, slices, stacks, MeshType::Nonindexed, VertexType::VT10NC10, 0, colorFuncEmissive, magFunc ) ;

  this->pos = iPos ;
  this->transform( Matrix::Translate( iPos ) ) ;
}

Vector MathematicalShape::getColor( const Vector& pointOnLight, ColorIndex colorIndex )
{
  // need (theta,azimuth) angle to POINT ON LIGHT
  Vector sphCoord = pointOnLight - getCentroid() ;
    
  // no need to normalize, only using elevation/azimuth
  SVector sp = sphCoord.toSpherical() ;
    
  // light color for that point we are hitting (matters for cubemaps, math lights)
  return getColor( sp.tElevation, sp.pAzimuth, colorIndex ) ;
}

Vector MathematicalShape::getColor( real tElevation, real pAzimuth, ColorIndex colorIndex )
{
  switch( colorIndex )
  {
    case ColorIndex::DiffuseMaterial:
      return colorFuncDiffuse( tElevation, pAzimuth ) ;
    
    case ColorIndex::Emissive:
      return colorFuncEmissive( tElevation, pAzimuth ) ;

    default:
      error( "MathematicalShape::getColor() only supports diffuse,emissive colors" ) ;
      return 0 ;
  }
}

SHVector* MathematicalShape::shProject( const Vector& scale ) // override
{
  info( "SH projecting the MathematicalShape of %s", name.c_str() ) ;
  // shprojection for a shape is done by
  // integrating the samples against
  // the SHAPE, using radial distances.
  shProjection = new SHVector() ; 
  for( int i = 0 ; i < window->shSamps->n ; i++ )
  {
    SHSample &samp = window->shSamps->samps[ i ] ;
    
    // correlate.
    shProjection->integrateSample( samp, magFunc( samp.tEl(), samp.pAz() ), colorFuncDiffuse( samp.tEl(), samp.pAz() ) ) ;

    printf( "proj %d / %d           \r", i+1, window->shSamps->n ) ;
  }

  shProjection->scale( (4.0*PI)/window->shSamps->n ) ;
  // divide by 4*PI/#samps

  return shProjection ;
}








#pragma region mathematical functions
// These are centered around the origin.
// This makes them easily reusable.
// You can also do pos=(0,0,0) and add
// translation component in the function itself.
function< Vector( real tElevation, real pAzimuth )> colorFunctions[] = {
  
  //ConstantWhite
  []( real tElevation, real pAzimuth ) -> Vector {
    return Vector(1,1,1) ; // constant color
  },

  //Weird test shapes
  []( real tElevation, real pAzimuth ) // arguments
        -> Vector // return type
  { // function
    //real val = sin( tElevation ) *cos( 2*pAzimuth ) ;
    //real val = 1; // sphere
    return Vector (0.25,0.25,0.25); // low white light
    //real val = tElevation ; // apple
    //real val = (tElevation * pAzimuth)/(PI*2*PI) ; // seashell
    //real val = (4+sin(5*(tElevation+pAzimuth)))/(5); // twisted shape
    //real val = sin(2*(tElevation+pAzimuth)); // twisted shape
    //real val = sin(2*tElevation) + cos( tElevation+pAzimuth ); // odd shaped
    //real val = cos(tElevation)+(3/cos(pAzimuth))/100 ; // odd shaped
    //real val = sin( 2*pAzimuth ) + cos( tElevation + pAzimuth ) ;

    //val.clamp( Vector(0,0,0), Vector(1,1,1) ) ;
  },

  //PYRedPZGreenLobe
  []( real tEl, real pAz ) -> Vector {
    return Vector(
      // red is up lobe
      std::max<real>( 0., 5*cos(tEl)-4 ),
          
      // green lower lobe
      std::max<real>( 0., -4*sin(tEl-PI)*cos(pAz-2.5)-3),
          
      0.
    ) ;
  },
  
  //PZBlueLobe
  []( real tEl, real pAz ) -> Vector {
    return Vector(
      0, 0, std::max<real>( 0., -4*sin(tEl-PI)*cos(pAz-2.5)-3)
    ) ;
  },

  //PZLobe
  []( real tEl, real pAz ) -> Vector {
    // this isn't completely right, it affects the alpha
    return std::max<real>( 0., -4*sin(tEl-PI)*cos(pAz-2.5)-3 ) ;
    //return Vector(
    //  std::max<real>( 0., -4*sin(tEl-PI)*cos(pAz-2.5)-3 ),
    //  std::max<real>( 0., -4*sin(tEl-PI)*cos(pAz-2.5)-3 ),
    //  std::max<real>( 0., -4*sin(tEl-PI)*cos(pAz-2.5)-3 )
    //) ;
  },

  //PZLobe2
  []( real tEl, real pAz ) -> Vector {
    return std::max<real>( 0., pow( sin(pAz)*sin(tEl), 7 ) ) ;
    //return Vector(
    //  std::max<real>( 0., pow( sin(pAz)*sin(tEl), 7 ) ),
    //  std::max<real>( 0., pow( sin(pAz)*sin(tEl), 7 ) ),
    //  std::max<real>( 0., pow( sin(pAz)*sin(tEl), 7 ) )
    //) ;
  },

  //NXLobe
  []( real tEl, real pAz ) -> Vector {
    return std::max<real>( 0., pow( cos(PI-pAz)*sin(tEl), 7 ) ) ;
    //return Vector(
    //  std::max<real>( 0., pow( cos(PI-pAz)*sin(tEl), 7 ) ),
    //  std::max<real>( 0., pow( cos(PI-pAz)*sin(tEl), 7 ) ),
    //  std::max<real>( 0., pow( cos(PI-pAz)*sin(tEl), 7 ) )
    //) ;
  },

  //PYLobe
  // No PHI dependence
  // Returns 0 when negative.
  // I use this for glossy reflections
  []( real tEl, real pAz ) -> Vector {
    //return pow( std::max<real>(cos(tEl),0.0), 77 )/4.0 ;
    return Vector(
      8*pow( std::max<real>(cos(tEl),0.0), 77 ),
      8*pow( std::max<real>(cos(tEl),0.0), 77 ),
      8*pow( std::max<real>(cos(tEl),0.0), 77 )
    ) ;
  }



} ;

function<real ( real tElevation, real pAzimuth )> magFunctions[] =
{
  //MagSpherical
  []( real tElevation, real pAzimuth ) -> real {
    return 1 ; // constant radius
  },

  //MagPZLobe
  []( real tEl, real pAz ) -> real {
    return std::max<real>( 0., -4*sin(tEl-PI)*cos(pAz-2.5)-3 ) ;
  },
  
  //MagPZLobe2
  []( real tEl, real pAz ) -> real {
    return std::max<real>( 0., pow( sin(pAz)*sin(tEl), 7 ) ) ;
  },

  //MagNXLobe
  []( real tEl, real pAz ) -> real {
    return std::max<real>( 0., pow( cos(PI-pAz)*sin(tEl), 7 ) ) ;
  },

  //MagPYLobe
  // No PHI dependence
  []( real tEl, real pAz ) -> real {
    return 8*pow( std::max<real>(cos(tEl),0.0), 77 ) ;
  }
  

} ;

#pragma endregion