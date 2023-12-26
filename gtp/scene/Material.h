#ifndef MATERIAL_H
#define MATERIAL_H

#include "../math/Vector.h"
#include "../math/perlin.h"
#include "../geometry/Intersection.h"
#include <functional>
using namespace std ;

enum BRDF
{
  Lambertian,  // perfect diffuse
  LommelSeeliger,
  Phong,
  BlinnPhong,
  TorranceSparrow,
  CookTorrance,
  OrenNayar,
  AshikhminShirley,
  HTSG,
  Lafortune,
  Lebedev,
  Anisotropic


  //Skin,
  //Wood, // can't you color by perlin noise? no there's no reason to have angle dependence - wood is a texture not a brdf
  //Marble
};

// incoming pos, incoming angle, color
struct LightVector
{
  MeshIntersection mi ;  // for the bssrdf to exit somewhere on the
  // intersected TRIANGLE of the model it is hitting.
    
  Vector direction ;
  Vector color ;

  LightVector()
  {
    // i need to be able to query the
    // SHAPE for a point (u,v) distance
    // away from the entrypoint
    // THAT IS STILL ON THE SURFACE

  }
} ;

extern function<Vector (const Vector& wi, const Vector& wo)> brdfFunctions[] ;
extern function<void ( LightVector& data )> bssrdfFunctions[] ;

struct Material
{
  // they have names for when being loaded from an OBJ file
  string name ;

  // When light hits a material, 3 things can happen:
  // 1. It transmits a percentage of the ray
  // 2. It sends diffuse reflected light back in the direction of the ray
  // 3. specularly reflected light
  // 4. It "emits" or sends light towards the source (because it is a light source)
  Vector
    //ka, // "ambient" behaviour is false and should be subsumed under diffuse
    kd,   // "diffuse" color is "soft color"
    ke,   // "emissive color"
    ks,   // "specular color"
    kt,   // "transmission wavelengths" - how MUCH does r,g,b transmit?  If =1, then it
    // transmits 100% of incident light (after refraction) thru the object.  Now,
    // if kd.r=1 and kt.r=1, you are violating the law of conservation of
    // energy (unless its iridescent, (ie converts some green light energy
    // to RED).. or possibly converts incident gamma rays to visible color
    // spectrum)
    
    eta ;// the fresnel coefficients for r,g and b
  

  /// A perlin noise generator
  /// Really this should be abstracted to
  /// a "Texturizer" object, but for now
  /// all textures will be perlin and procedural.
  PerlinGenerator* perlinGen[5] ; // room for 5 perlin generators,
  // to modulate each kd, ks, kt, eta, ke

  // These are ALL still just approximating BRDFs.

  real Ns ;// specular power

  real specularJitter, transmissiveJitter ;
  
  // conservation of energy says transmission%, diffuse% and specular%
  // should add up to 1.

  // if EMISSION is included, then the surface counts AS A LIGHT SOURCE
  // as well and so it will emit light.

  // eventually lights should be all Material'd objects with a high emissive term.
 
  // a brdf takes BOTH INCOMING direction AND outgoing direction,
  // and returns RATIO of incoming radiance to outgoing radiance
  // (for rgb)
  // ie THAT IS WHAT PHONG MODEL COLORS ARE, they are ratios
  // for RGB to reflect in, 
  // Note that kt+ks+kd <= 1 ALL ANGLES, because you cannot
  // transmit 50% light in the direction you are specularly
  // reflecting light at 100%.  It makes no sense to generate
  // 150% light out of 100% light.
  function<Vector (const Vector& wi, const Vector& wo)> brdf ;

  // outgoing POS, ANGLE, COLOR
  // change in the pos allows for
  // subsurface scattering.

  // modifies incoming light vector,
  //   - incoming pos,
  //   - angle (TO NORM) [ assumes normal is (0,1,0) ]
  //   - color (allows for irridescent type effects)
  //function<void ( LightVector& data )> bssrdf ;
  // Problems:
  // - the exit position MUST BE on some point
  //   of the material __surface__, NOT outside
  //   and not 
  // - should the brdf accept a normal or no?
  // 

  Material() ;

  // http://stackoverflow.com/questions/8440474/mark-constructor-as-explicitly-requiring-an-object-type
  explicit Material( const Vector& diffuseColor ) ;

  explicit Material( const Vector& diffuseColor, const Vector& specularColor ) ;

  Material( const Vector& diffuseColor,
    const Vector& emissiveColor,
    const Vector& specularColor,
    const Vector& transmissionColor,
    const Vector& fresnelEta ) ;

  Material( FILE *file ) ;

  Material& mult( const Material& other ) ;

  int save( FILE *file ) ;

  inline int saveSize()
  {
    return (5*sizeof(Vector) + 1*sizeof(real));
  }

  static Material randomSolid( real low, real high )
  {
    Material m ;
    m.kd = Vector::random( low, high ) ;
    m.integrity() ;
    return m ;
  }

  static Material randomTranslucent( real low, real high )
  {
    Material m ;
    m.kd = Vector::random( low, high ) ;
    m.kt = 1 - m.kd/2 ;
    m.integrity() ;
    return m ;
  }

  // defaults
  void defs()
  {
    kd = Vector(.5,.5,.5) ;
    ke = Vector(0,0,0) ;
    ks = Vector(1,1,1,25) ;
    kt = Vector(0,0,0) ;
    eta = Vector(1.3,1.3,1.3) ;
    Ns = 25 ;
    specularJitter = 0 ;
    transmissiveJitter = 0 ;
  }

  // the reality is, a surface cannot have kt+kd+ks>1
  // FOR ANY ONE GIVEN ANGLE. The BRDF would tell you
  // kt, kd, ks different for different angles, but
  // with LAMBERTIAN materials, you are stuck with ONE
  // kd, kt, ks for ALL angles.
  void integrity()
  {
    memset( perlinGen, 0, 5*sizeof(PerlinGenerator*) ) ;

    if( eta.all(0) )
    {
      warning( "Zero eta screws up the raycaster when diffracting, correcting to 1's" ) ;
      eta.setxyz( 1 ) ;
    }

    // you cannot diffuse/specular reflect or transmit more than 100% of
    // the incident light (
    //Vector ktkdks = kt + kd + ks ;

    // This warning is annoying
    ///if( ktkdks.x > 1 || ktkdks.y > 1 || ktkdks.z > 1 )
    ///  warning( "Material %s, has ktkdks=(%f,%f,%f) transmits+reflects more than 100% of incident light energy, unrealistic surface",
    ///  name.c_str(), ktkdks.x, ktkdks.y, ktkdks.z ) ;
  }

  // Does the material have a texture on this index?
  inline bool hasPerlinTexture( ColorIndex colorIndex ) { return perlinGen[ colorIndex ] ; }

  static Material randomSolid()
  {
    return Material::randomSolid( 0, 1 );
  }

  static Material randomTranslucent()
  {
    return Material::randomTranslucent( 0, 1 );
  }

  // not sampling the perlin texture
  inline Vector getColor( ColorIndex colorIndex )
  {
    return (&kd)[ colorIndex ] ;
  }

  // Usually you use this.
  inline Vector getPerlinTextureColor( const Vector& point, ColorIndex colorIndex )
  {
    if( perlinGen[ colorIndex ] )
      return perlinGen[ colorIndex ]->at( point ) ;
    
    warning( "Tried to get texture color but no texture exists" ) ;
    return 1 ; // this is an error
  }

  static Material Red ;
  static Material DkRed ;
  static Material Pink ;

  static Material Green ;
  static Material DkGreen ;

  static Material Blue ;
  static Material DkBlue ;

  static Material Yellow ;
  static Material DkYellow ;

  static Material Orange ;
  static Material DkOrange ;

  static Material Magenta ;
  static Material DkMagenta ;

  static Material Teal ;
  static Material DkTeal ;

  static Material Gray;
  static Material LightGray;
  static Material White ;
  static Material WhiteSpecular ;
  static Material Black ;
} ;


#endif