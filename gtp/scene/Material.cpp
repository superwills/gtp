#include "Material.h"

function<Vector (const Vector& wi, const Vector& wo)> brdfFunctions[] = {
  
  //Lambertian
  []( const Vector& wi, const Vector& wo ) -> Vector {
    return Vector(1,1,1) ; // constant color
  },

} ;

function<void ( LightVector& data )> bssrdfFunctions[] = {
  []( LightVector& data ) {
    data.color.y = 0 ; // zero green for whatever reason this is just a sample function
    data.direction.rotateZ( 0.2 ) ;
    data.mi.point += 0.01 * (data.mi.normal << Vector::random()) ;
  }
} ;

Material::Material()
{
  defs() ;
  integrity() ;
}

Material::Material( const Vector& diffuseColor )
{
  defs() ;
  kd = diffuseColor ;
  integrity() ;
}

Material::Material( const Vector& diffuseColor, const Vector& specularColor )
{
  defs() ;
  kd = diffuseColor ;
  ks = specularColor ;
  integrity() ;
}

Material::Material( const Vector& diffuseColor,
  const Vector& emissiveColor,
  const Vector& specularColor,
  const Vector& transmissionColor,
  const Vector& fresnelEta )
{
  defs() ;
  kd = diffuseColor;
  ke = emissiveColor;
  ks = specularColor;
  kt = transmissionColor;
  eta = fresnelEta ;
  
  integrity() ;
}

Material::Material( FILE *file )
{
  defs() ;
  fread( &kd, saveSize(), 1, file ) ;
  integrity() ;
}

Material& Material::mult( const Material& other )
{
  kd *= other.kd ;
  ke *= other.ke ;
  ks *= other.ks ;
  kt *= other.kt ;

  return *this ;
}

int Material::save( FILE *file )
{
  int written = 0 ;
  // leave the 'name' member out
  written += saveSize() * fwrite( &kd, saveSize(), 1, file ) ;
  return written ;
}

Material Material::Red( Vector( 1, 0, 0 ) ) ;
Material Material::DkRed( Vector(.5,0,0) ) ;
Material Material::Pink( Vector( 1, .5, 1 ) ) ;

Material Material::Green( Vector(0,1,0) ) ;
Material Material::DkGreen( Vector(0,.5,0) ) ;

Material Material::Blue( Vector(0,0,1) ) ;
Material Material::DkBlue( Vector(0,0,.5) ) ;

Material Material::Yellow( Vector(1,1,0) ) ;
Material Material::DkYellow( Vector(.5,.5,0) ) ;

Material Material::Orange( Vector(.9,.6,.2) ) ;
Material Material::DkOrange( Vector(.6,.4,.1) ) ;

Material Material::Magenta( Vector(1,0,1) ) ;
Material Material::DkMagenta( Vector(.5,0,.5) ) ;

Material Material::Teal( Vector(0,1,1) ) ;
Material Material::DkTeal( Vector(0,.5,.5) ) ;

Material Material::Gray( Vector(.2,.2,.2) ) ;
Material Material::LightGray( Vector(.5,.5,.5) ) ;
Material Material::White( Vector(1,1,1) ) ;
Material Material::WhiteSpecular( 0, 0, Vector(1,1,1,25), 0, 1 ) ;
Material Material::Black( Vector(0,0,0) ) ;
         
