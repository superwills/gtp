#ifndef MATH_SHAPE_H
#define MATH_SHAPE_H

#include "Shape.h"

enum ColorFunctions
{
  ConstantWhite,
  TestShape,
  PYRedPZGreenLobe,
  PZBlueLobe,
  PZLobe,
  PZLobe2,
  NXLobe,
  PYLobe
} ;

enum MagFunctions
{
  MagSpherical,
  MagPZLobe,
  MagPZLobe2,
  MagNXLobe,
  MagPYLobe
} ;

extern function<Vector( real tElevation, real pAzimuth )> colorFunctions[] ;
extern function<real ( real tElevation, real pAzimuth )> magFunctions[] ;

struct MathematicalShape : public Shape
{
  // So, a MathematicalShape is based on a function
  // that specifies the surface wrt the origin.
  // YOU CAN embed the pos in the magFunction, 
  // but that's so inflexible
  Vector pos ;

  // The reason I had MagFunc and ColorFunc separately was
  // so that the shape's GEOMETRY could be separate from
  // the color values on the geometry
  // This only makes sense WHEN A MATHEMATICAL SHAPE
  // SPECIFIES __GEOMETRY__, NOT A LIGHT SOURCE.
  function<real ( real tElevation, real pAzimuth )> magFunc ;

  // Color functions.
  // returns an RGB color on any theta/phi.
  // need separate functions for emissive/diffuse/specular behaviour.
  // these directly replace the idea of a "material"
  function<Vector ( real tElevation, real pAzimuth )>
    colorFuncDiffuse, colorFuncEmissive, colorFuncSpecular, colorFuncTransmissive ;

  MathematicalShape( string iname, Vector iPos,
    int magFuncIndex, int colorFuncDiffuseIndex, int colorFuncEmissiveIndex,
    int slices, int stacks, const Material& iMaterial ) ;

  MathematicalShape( string iname, Vector iPos,
    function<real ( real tElevation, real pAzimuth )> iMagFunc,
    function<Vector ( real tElevation, real pAzimuth )> iColorFuncDiffuse,
    function<Vector ( real tElevation, real pAzimuth )> iColorFuncEmissive,
    int slices, int stacks, const Material& iMaterial ) ;

  MathematicalShape( string iname, Vector iPos,
    function<real ( real tElevation, real pAzimuth )> iMagFunc,
    function<Vector ( real tElevation, real pAzimuth )> iColorFuncAll,
    int slices, int stacks, const Material& iMaterial ) ;

  // just say YES for now
  bool isTransmitting() override { return true ; }

  // if there IS an emissive function set, then
  // you have to assume it emits light, far
  // as the raytracer is concerned
  bool isEmissive() override { return true ; } // i always say this is true
  // because that's the only thing i use mathematicalshape for (lights)
  
  // you must override both functions,
  Vector getColor( const Vector& pointOnLight, ColorIndex colorIndex ) override ;

  // get the light "strength" as a function of
  // theta and phi.
  // this just passes off to lightFunction,
  // whatever that is.
  Vector getColor( real tElevation, real pAzimuth, ColorIndex colorIndex ) override ;

  // Evaluating a SHAPE at tElevation, pAzimuth
  // means shooting a ray with that angle and seeing
  // where it intersects the shape.
  // for a sphere, this is trivial (you get back r*color),
  // but for an arbitrary mesh, it is a ray-mesh intersection,
  // shot from centroid.
  // could be const but intersectMesh isn't const.
  // radial distance to mesh from centroid at this angle
  real getRadialDistance( real tEl, real pAz, bool fromCentroid, bool outerShell ) override
  {
    // fromCentroid always true, outerShell always true
    return magFunc( tEl, pAz ) ;
  }

  // overrides so the color function is used.
  //!! USES colorFuncDiffuse! Not Emissive!
  SHVector* shProject( const Vector& scale ) override ;
} ;

#endif
