#ifndef PERLIN_H
#define PERLIN_H

#include "Vector.h"

struct Model ;

// http://local.wasp.uwa.edu.au/~pbourke/texture_colour/perlin/
#define perlin_B 0x100
#define perlin_BM 0xff
#define perlin_N 0x1000
/////#define perlin_NP 12   // 2^N //(not used)
/////#define perlin_NM 0xfff // (not used)

#define s_curve(t) ( t * t * (3. - 2. * t) )
#define perlin_lerp(t, a, b) ( a + t * (b - a) )
#define at2(rx,ry) ( rx * q[0] + ry * q[1] )
#define at3(rx,ry,rz) ( rx * q[0] + ry * q[1] + rz * q[2] )


enum PerlinNoiseType
{
  Sky,
  Wood,
  Marble,
  CustomLinear,
  CustomQuadratic,
  CustomCubic,
  CustomQuartic,
  CustomQuintic
} ;


struct PerlinGenerator
{
  // perlin state variables
  int p[perlin_B + perlin_B + 2];
  double g3[perlin_B + perlin_B + 2][3];
  double g2[perlin_B + perlin_B + 2][2];
  double g1[perlin_B + perlin_B + 2];

  PerlinNoiseType noiseType ;

  // my predefined color values
  static Vector CLOUD_WHITE,SKY_BLUE,
    WOOD1,WOOD2,WOOD3,WOOD4,
    MARBLE1,MARBLE2,MARBLE3,MARBLE4 ;
  real persistence;
  real lacunarity;
  int octaves; 
  Vector color1,color2,color3,color4,color5,color6;
  
  Vector worldNormalizer ;

  PerlinGenerator() ;
  PerlinGenerator( PerlinNoiseType iNoiseType, int seed, const Vector& iWorldNormalizer ) ;

  // Lacunarity: A multiplier that determines how quickly the frequency increases for each successive octave in a Perlin-noise function.
  // Persistence: A multiplier that determines how quickly the amplitudes diminish for each successive octave in a Perlin-noise function.
  PerlinGenerator( int seed, real iPersistence, real iLacunarity, int iOctaves, const Vector& iWorldNormalizer,
    const Vector& iColor1, const Vector& iColor2 ) ;

  PerlinGenerator( int seed, real iPersistence, real iLacunarity, int iOctaves, const Vector& iWorldNormalizer, 
    const Vector& iColor1, const Vector& iColor2, const Vector& iColor3 ) ;

  PerlinGenerator( int seed, real iPersistence, real iLacunarity, int iOctaves, const Vector& iWorldNormalizer,
    const Vector& iColor1, const Vector& iColor2, const Vector& iColor3, const Vector& iColor4 ) ;

  PerlinGenerator( int seed, real iPersistence, real iLacunarity, int iOctaves, const Vector& iWorldNormalizer,
    const Vector& iColor1, const Vector& iColor2, const Vector& iColor3, const Vector& iColor4, const Vector& iColor5 ) ;

  PerlinGenerator( int seed, real iPersistence, real iLacunarity, int iOctaves, const Vector& iWorldNormalizer,
    const Vector& iColor1, const Vector& iColor2, const Vector& iColor3, const Vector& iColor4, const Vector& iColor5, const Vector& iColor6 ) ;
  
  __forceinline void setup( int i, double&t, double*vec, int& b0, int& b1, double& r0, double& r1 )
  {
    t = vec[i] + perlin_N;
    b0 = ((int)t) & perlin_BM;
    b1 = (b0+1) & perlin_BM;
    r0 = t - (int)t;
    r1 = r0 - 1.;
  }

  double noise1(double);
  double noise2(double *);
  double noise3(double *);
  void normalize2(double *);
  void normalize3(double *);
  void reset( int seed );

  /// "alpha" is the weight when the sum is formed.  Typically it is 2,
  /// as alpha approaches 1 the function is noisier
  /// "beta" is the harmonic scaling/spacing, typically 2
  double PerlinNoise1D(double x, double divisionFactor, double freqMult, int numFreqs ) ;


  // divisionFactor: amount to reduce scale (by division) of higher frequencies by
  // freqMult:  Inter-frequency spacing
  // numFreqs:  Number of frequencies to add
  // Eg if you pass 2 as division factor, 3 as freqMult, and 4 as numFreqs,
  // you get
  //   noise(x) + noise(3*x)/2 + noise(3^2*x)/2^2 + noise(3^3*x)/2^3 + noise(3^4*x)/2^4
  // These functions are INTENTIONALLY pass by value because
  // you need to modify x,y,z as you iterate anyway
  // (so you need a copy of the variables anyway).
  double PerlinNoise2D( double x, double y, double divisionFactor, double freqMult, int numFreqs ) ;

  // divisionFactor aka persistence, freqMult aka lacunarity, numFreqs aka octaves
  double PerlinNoise3D( double x, double y, double z, double divisionFactor, double freqMult, int numFreqs ) ;

  Vector linearSpline( real t ) ;
  Vector linearSpline( real t, const Vector& c1, const Vector& c2 ) ;
  Vector quadraticSpline( real t ) ;
  Vector quadraticSpline( real t, const Vector& c1, const Vector& c2, const Vector& c3 ) ;
  Vector cubicSpline( real t ) ;
  Vector cubicSpline( real t, const Vector& c1, const Vector& c2, const Vector& c3, const Vector& c4 ) ;
  Vector quarticSpline( real t ) ;
  Vector quarticSpline( real t, const Vector& c1, const Vector& c2, const Vector& c3, const Vector& c4, const Vector& c5 ) ;
  Vector quinticSpline( real t ) ;
  Vector quinticSpline( real t, const Vector& c1, const Vector& c2, const Vector& c3, const Vector& c4, const Vector& c5, const Vector& c6 ) ;

  Vector wood( double x, double y, double z ) ;
  Vector sky( double x, double y, double z, const Vector& skyColor, const Vector& cloud ) ;

  // Use internal variable parameters
  Vector marble( double x, double y, double z, double grainVariable ) ;

  // freqBoost increases freq of the underlying sinusoid
  // freckling is how "smooth" the marbling is.  If 
  // you set freckling to like, 1, you get very smooth,
  // distinct bands.  If you set it to 32 or so, you get
  // a very nice speckly marble thing.
  Vector marble( double x, double y, double z, double grainVariable,
    int freckling, double freqBoost,
    const Vector& c1, const Vector& c2 ) ;
  
  Vector marble( double x, double y, double z, double grainVariable,
    int freckling, double freqBoost,
    const Vector& c1, const Vector& c2, const Vector& c3, const Vector& c4 ) ;

  //Vector mountain( double x, double y, 

  // use internally stored parameters to generate noise
  Vector at( const Vector& pos ) ;

  real mountainHeight( real u, real v ) ;

  // defaults to using (x,z) for floor plane and
  // y for height, you can rotate the model after generation
  // if this is a problem.
  Model* heightmapNonindexed( int udivs, int vdivs, real uSize, real vSize, real normalFindStepRadians ) ;

  Model* heightmapIndexed( int udivs, int vdivs, real uSize, real vSize, real normalFindStepRadians ) ;
} ;

#endif