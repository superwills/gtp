#include "Perlin.h"

// http://local.wasp.uwa.edu.au/~pbourke/texture_colour/perlin/

// Coherent noise function over 1, 2 or 3 dimensions
// (copyright Ken Perlin)

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "../geometry/Model.h"
#include "../window/GTPWindow.h"

Vector PerlinGenerator::CLOUD_WHITE( 1.0 ) ;
Vector PerlinGenerator::SKY_BLUE( .168, .612, 1 ) ;

Vector PerlinGenerator::WOOD1( .545, .251, .035 ) ;
Vector PerlinGenerator::WOOD2( .859, .667, .325 ) ;
Vector PerlinGenerator::WOOD3( .918, .753, .380 ) ;
Vector PerlinGenerator::WOOD4( .961, .792, .447 ) ;

Vector PerlinGenerator::MARBLE1( .1, .05, 0 ) ;
Vector PerlinGenerator::MARBLE2( .25, .125, 0 ) ;
Vector PerlinGenerator::MARBLE3( .67, .1, 0 ) ;
Vector PerlinGenerator::MARBLE4( .9, .77, .64 ) ;

PerlinGenerator::PerlinGenerator()
{
  reset( 0 ) ;
  worldNormalizer = 1 ;
}

PerlinGenerator::PerlinGenerator( PerlinNoiseType iNoiseType, int seed, const Vector& iWorldNormalizer )
{
  reset( seed ) ;
  worldNormalizer = iWorldNormalizer ;
  noiseType = iNoiseType ;
}

PerlinGenerator::PerlinGenerator( int seed, real iPersistence, real iLacunarity, int iOctaves, const Vector& iWorldNormalizer,
    const Vector& iColor1, const Vector& iColor2 ):
  persistence(iPersistence),
  lacunarity(iLacunarity),octaves(iOctaves),
  worldNormalizer(iWorldNormalizer),
  color1(iColor1),color2(iColor2)
{
  reset( seed ) ;
  noiseType = CustomLinear ;
}

PerlinGenerator::PerlinGenerator( int seed, real iPersistence, real iLacunarity, int iOctaves, const Vector& iWorldNormalizer, 
  const Vector& iColor1, const Vector& iColor2, const Vector& iColor3 ):
  persistence(iPersistence),
  lacunarity(iLacunarity),octaves(iOctaves),
  worldNormalizer(iWorldNormalizer),
  color1(iColor1),color2(iColor2),color3(iColor3)
{
  reset( seed ) ;
  noiseType = CustomQuadratic ;
}

PerlinGenerator::PerlinGenerator( int seed, real iPersistence, real iLacunarity, int iOctaves, const Vector& iWorldNormalizer,
  const Vector& iColor1, const Vector& iColor2, const Vector& iColor3, const Vector& iColor4 ):
  persistence(iPersistence),
  lacunarity(iLacunarity),octaves(iOctaves),
  worldNormalizer(iWorldNormalizer),
  color1(iColor1),color2(iColor2),color3(iColor3),color4(iColor4)
{
  reset( seed ) ;
  noiseType = CustomCubic ;
}

PerlinGenerator::PerlinGenerator( int seed, real iPersistence, real iLacunarity, int iOctaves, const Vector& iWorldNormalizer,
  const Vector& iColor1, const Vector& iColor2, const Vector& iColor3, const Vector& iColor4, const Vector& iColor5 ):
  persistence(iPersistence),
  lacunarity(iLacunarity),octaves(iOctaves),
  worldNormalizer(iWorldNormalizer),
  color1(iColor1),color2(iColor2),color3(iColor3),color4(iColor4),color5(iColor5)
{
  reset( seed ) ;
  noiseType = CustomQuartic ;
}

PerlinGenerator::PerlinGenerator( int seed, real iPersistence, real iLacunarity, int iOctaves, const Vector& iWorldNormalizer,
  const Vector& iColor1, const Vector& iColor2, const Vector& iColor3, const Vector& iColor4, const Vector& iColor5, const Vector& iColor6 ) :
  persistence(iPersistence),
  lacunarity(iLacunarity),octaves(iOctaves),
  worldNormalizer(iWorldNormalizer),
  color1(iColor1),color2(iColor2),color3(iColor3),color4(iColor4),color5(iColor5),color6(iColor6)
{
  reset( seed ) ;
  noiseType = CustomQuintic ;
  
}

double PerlinGenerator::noise1(double arg)
{
  int bx0, bx1;
  double rx0, rx1, sx, t, u, v, vec[1];

  vec[0] = arg;
  setup(0,t,vec,bx0,bx1,rx0,rx1);

  sx = s_curve(rx0);
  u = rx0 * g1[ p[ bx0 ] ];
  v = rx1 * g1[ p[ bx1 ] ];

  return(perlin_lerp(sx, u, v));
}

double PerlinGenerator::noise2(double vec[2])
{
  int bx0, bx1, by0, by1, b00, b10, b01, b11;
  double rx0, rx1, ry0, ry1, *q, sx, sy, a, b, t, u, v;
  int i, j;

  setup(0, t,vec, bx0,bx1, rx0,rx1);
  setup(1, t,vec, by0,by1, ry0,ry1);

  i = p[ bx0 ];
  j = p[ bx1 ];

  b00 = p[ i + by0 ];
  b10 = p[ j + by0 ];
  b01 = p[ i + by1 ];
  b11 = p[ j + by1 ];

  sx = s_curve(rx0);
  sy = s_curve(ry0);

  q = g2[ b00 ] ; u = at2(rx0,ry0);
  q = g2[ b10 ] ; v = at2(rx1,ry0);
  a = perlin_lerp(sx, u, v);

  q = g2[ b01 ] ; u = at2(rx0,ry1);
  q = g2[ b11 ] ; v = at2(rx1,ry1);
  b = perlin_lerp(sx, u, v);

  return perlin_lerp(sy, a, b);
}

double PerlinGenerator::noise3(double vec[3])
{
  int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
  double rx0, rx1, ry0, ry1, rz0, rz1, *q, sy, sz, a, b, c, d, t, u, v;
  int i, j;

  setup(0, t,vec, bx0,bx1, rx0,rx1);
  setup(1, t,vec, by0,by1, ry0,ry1);
  setup(2, t,vec, bz0,bz1, rz0,rz1);

  i = p[ bx0 ];
  j = p[ bx1 ];

  b00 = p[ i + by0 ];
  b10 = p[ j + by0 ];
  b01 = p[ i + by1 ];
  b11 = p[ j + by1 ];

  t  = s_curve(rx0);
  sy = s_curve(ry0);
  sz = s_curve(rz0);

  q = g3[ b00 + bz0 ] ; u = at3(rx0,ry0,rz0);
  q = g3[ b10 + bz0 ] ; v = at3(rx1,ry0,rz0);
  a = perlin_lerp(t, u, v);

  q = g3[ b01 + bz0 ] ; u = at3(rx0,ry1,rz0);
  q = g3[ b11 + bz0 ] ; v = at3(rx1,ry1,rz0);
  b = perlin_lerp(t, u, v);

  c = perlin_lerp(sy, a, b);

  q = g3[ b00 + bz1 ] ; u = at3(rx0,ry0,rz1);
  q = g3[ b10 + bz1 ] ; v = at3(rx1,ry0,rz1);
  a = perlin_lerp(t, u, v);

  q = g3[ b01 + bz1 ] ; u = at3(rx0,ry1,rz1);
  q = g3[ b11 + bz1 ] ; v = at3(rx1,ry1,rz1);
  b = perlin_lerp(t, u, v);

  d = perlin_lerp(sy, a, b);

  return perlin_lerp(sz, c, d);
}

void PerlinGenerator::normalize2(double v[2])
{
  double s;

  s = sqrt(v[0] * v[0] + v[1] * v[1]);
  v[0] = v[0] / s;
  v[1] = v[1] / s;
}

void PerlinGenerator::normalize3(double v[3])
{
  double s;

  s = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  v[0] = v[0] / s;
  v[1] = v[1] / s;
  v[2] = v[2] / s;
}

void PerlinGenerator::reset( int seed )
{
  srand( seed ) ;
  int i, j, k;

  for (i = 0 ; i < perlin_B ; i++) {
    p[i] = i;
    g1[i] = (double)((rand() % (perlin_B + perlin_B)) - perlin_B) / perlin_B;

    for (j = 0 ; j < 2 ; j++)
      g2[i][j] = (double)((rand() % (perlin_B + perlin_B)) - perlin_B) / perlin_B;
    normalize2(g2[i]);

    for (j = 0 ; j < 3 ; j++)
      g3[i][j] = (double)((rand() % (perlin_B + perlin_B)) - perlin_B) / perlin_B;
    normalize3(g3[i]);
  }

  while (--i) {
    k = p[i];
    p[i] = p[j = rand() % perlin_B];
    p[j] = k;
  }

  for (i = 0 ; i < perlin_B + 2 ; i++)
  {
    p[perlin_B + i] = p[i];
    g1[perlin_B + i] = g1[i];
    for (j = 0 ; j < 2 ; j++)
      g2[perlin_B + i][j] = g2[i][j];
    for (j = 0 ; j < 3 ; j++)
      g3[perlin_B + i][j] = g3[i][j];
  }
}

// --- My harmonic summing functions - PDB --------------------------
///
// "alpha" is the weight when the sum is formed.
// Typically it is 2, As this approaches 1 the function is noisier.
// "beta" is the harmonic scaling/spacing, typically 2.
// alpha is the scale reducer.
// 

double PerlinGenerator::PerlinNoise1D( double x, double divisionFactor, double freqMult, int numFreqs )
{
  double sum = 0;
  double scale = 1;
  for( int i = 0 ; i < numFreqs ; i++ )
  {
    sum += noise1(x) / scale ; // get noise, reduce magnitude by scale (so "higher freq" noise has less effect)
    scale *= divisionFactor ; // increase division factor, for next iteration
    x *= freqMult ; // "up the frequency" of next noise value.
  }
  return sum ;
}

// To wrap on a sphere:  use theta and phi for x and y
// Brushed metal: vary only ONE of x,y
//   (eg PerlinNoise2D( uv.x, _1_, 2, 2, 8 ) // (the 1 stays constant across the 2d face)
//   (OR you could just generate a line of 1d noise and copy it)
double PerlinGenerator::PerlinNoise2D( double x, double y, double divisionFactor, double freqMult, int numFreqs )
{
  double sum = 0;
  double p[2],scale = 1;

  p[0] = x;
  p[1] = y;
  for( int i=0 ; i < numFreqs ; i++ )
  {
    sum += noise2(p) / scale;
    scale *= divisionFactor ;
    p[0] *= freqMult ;
    p[1] *= freqMult ;
  }
  return sum ;
}

double PerlinGenerator::PerlinNoise3D(double x, double y, double z, double divisionFactor, double freqMult, int numFreqs )
{
  double sum = 0;
  double p[3],scale = 1;

  p[0] = x;
  p[1] = y;
  p[2] = z;
  for( int i = 0 ; i < numFreqs ; i++ )
  {
    sum += noise3(p) / scale;
    scale *= divisionFactor ;
    p[0] *= freqMult ;
    p[1] *= freqMult ;
    p[2] *= freqMult ;
  }
  return sum ;
}

Vector PerlinGenerator::linearSpline( real t )
{
  return linearSpline( t, color1, color2 ) ;
}

Vector PerlinGenerator::linearSpline( real t, const Vector& c1, const Vector& c2 )
{
  return (t*c1 + (1-t)*c2).clamp(0,1) ;
}

Vector PerlinGenerator::quadraticSpline( real t )
{
  return quadraticSpline( t, color1, color2, color3 ) ;
}

Vector PerlinGenerator::quadraticSpline( real t, const Vector& c1, const Vector& c2, const Vector& c3 )
{
  real _t = 1-t ;
  return (t*t*c1 + 2*t*_t*c2 + _t*_t*c3).clamp(0,1) ;
}

Vector PerlinGenerator::cubicSpline( real t )
{
  return cubicSpline( t, color1, color2, color3, color4 ) ;
}

Vector PerlinGenerator::cubicSpline( real t, const Vector& c1, const Vector& c2, const Vector& c3, const Vector& c4 )
{
  real _t = 1-t ;
  return (t*t*t*c1 + 
    3*t*t*_t*c2 + 
    3*t*_t*_t*c3 +
    _t*_t*_t*c4).clamp(0,1) ;
}

Vector PerlinGenerator::quarticSpline( real t )
{
  return quarticSpline( t, color1, color2, color3, color4, color5 ) ;
}

Vector PerlinGenerator::quarticSpline( real t, const Vector& c1, const Vector& c2, const Vector& c3, const Vector& c4, const Vector& c5 )
{
  real _t = 1-t;
  return (t*t*t*t*c1 + 
    4*t*t*t*_t*c2 + 
    6*t*t*_t*_t*c3 +
    4*t*_t*_t*_t*c4 + 
    _t*_t*_t*_t*c5).clamp(0,1) ;
}

Vector PerlinGenerator::quinticSpline( real t )
{
  return quinticSpline( t, color1, color2, color3, color4, color5, color6 ) ;
}

Vector PerlinGenerator::quinticSpline( real t, const Vector& c1, const Vector& c2, const Vector& c3, const Vector& c4, const Vector& c5, const Vector& c6 )
{
  real _t = 1-t;
  return (t*t*t*t*t*c1 + 
    5 *t*t*t*t*_t*c2 + 
    10*t*t*t*_t*_t*c3 +
    10*t*t*_t*_t*_t*c4 + 
    5 *t*_t*_t*_t*_t*c5 +
    _t*_t*_t*_t*_t*c6).clamp(0,1) ;
}

Vector PerlinGenerator::wood( double x, double y, double z )
{
  return marble( x,y,z,x*x + y*y, 32, 40, WOOD1, WOOD2, WOOD3, WOOD4 ) ;
}

Vector PerlinGenerator::sky( double x, double y, double z, const Vector& skyColor, const Vector& cloud )
{
  //real val = PerlinNoise3D( x,y,z, 1.2, 2, 12 ) ; // rocky and pixellated
  
  ///////real a=.1, // a is the size of the sides.  a=big means THICK DONUT
  ///////     c=.6; // c is the size of the ring.  c=big means LARGE CIRCUMFERENCE DONUT
  ///////x *= 2*PI ;
  ///////y *= 2*PI ;
  ///////real xt = (c+a*cos(y))*cos(x);
  ///////real yt = (c+a*cos(y))*sin(x);
  ///////real zt = a*sin(y);
  ///////real val = PerlinNoise3D( xt,yt,zt, 1.5, 2, 12 ) ; // torus

  real val = PerlinNoise3D( x,y,z, 1.5, 2, 12 ) ; // "very good" cloud

  //real val = PerlinNoise3D( x,y,z, 2, 4, 12 ) ; // smoother than "very good"
  //real val = PerlinNoise3D( x,y,z, 2, 2, 12 ) ; // too smooth

  // force x,y,z input values to REPEAT, making it tileable
  // travel in a torus.
  //real c=4, a=1;
  //real x1 = (c+a*cos(y))*cos(x);
  //real y1 = (c+a*cos(y))*sin(x);
  //real val = PerlinNoise2D( x,y, 2, 2, 12 ) ; // test
  
  real t = (1+val)/2.0 ;
  
  //return (t*t*skyColor + (1-t)*(1-t)*cloud).clamp(0,1) ; // DARKER clouds
  //return (t*skyColor + (1-t)*(1-t)*cloud).clamp(0,1) ; // lighter clouds
  //return linearSpline( t, skyColor, cloud ) ; //very smooth, light clouds day
  
  //return cubicSpline( t, 1, .5, Vector( 0.570, 0.710, 1.000 ), Vector( .168, .612, 1 ) ) ; // another nice cloud
  //return cubicSpline( t, cloud, .5, Vector(0.2016, 0.7344, 1), skyColor ) ;
  //return cubicSpline( t, cloud, cloud*.5, (skyColor*1.2).clamp(0,1), skyColor ) ;
  //return cubicSpline( t, cloud, 0,.5, skyColor ) ; // dark/ominous.
  //return quinticSpline( t, cloud, .5,Vector(.2,.4,1),-.2,1, skyColor ) ; // abstract/painterly
  //return quinticSpline( t, cloud, .5, .5, 0, Vector(.2,.4,1), skyColor ) ; // ok
  
  return quadraticSpline( t, cloud, .6, skyColor ) ; // Very good.
  
}

Vector PerlinGenerator::marble( double x, double y, double z, double grainVariable )
{
  return marble( x,y,z,grainVariable,32,16, MARBLE1,MARBLE4 ) ;
}

Vector PerlinGenerator::marble( double x, double y, double z, double grainVariable,
  int freckling, double freqBoost, const Vector& c1, const Vector& c2 )
{
  // sin( x + |noise(p)| + .5*|noise(2p)| + ... )
  real n = PerlinNoise3D( x,y,z, 2, 2, freckling ) ;
  real val = sin( freqBoost*(grainVariable + n) ) ;
  
  // has black rifts between colors,
  ////if( val < 0 )  return (-val)*colorMinus;
  ////else  return val*colorPlus;
  
  real t = (1+val)/2.0 ;
  return linearSpline( t, c1, c2 ) ;
}

Vector PerlinGenerator::marble( double x, double y, double z, double grainVariable,
  int freckling, double freqBoost,
  const Vector& c1, const Vector& c2, const Vector& c3, const Vector& c4 )
{
  // sin( x + |noise(p)| + .5*|noise(2p)| + ... )
  real n = PerlinNoise3D( x,y,z, 2, 2, freckling ) ;
  real val = sin( freqBoost*(grainVariable + n) ) ;
  
  // You can also blend using a bezier type spline,
  real t = ( 1 + val ) / 2.0 ;

  return cubicSpline( t,c1,c2,c3,c4 ) ;
}

Vector PerlinGenerator::at( const Vector& pos )
{
  //return sky( pos.x,pos.y,pos.z, SKY_BLUE, CLOUD_WHITE ) ;
  Vector n = pos/worldNormalizer ;

  switch( noiseType )
  {
    case PerlinNoiseType::Sky:
      return sky( n.x, n.y, n.z, SKY_BLUE, CLOUD_WHITE ) ;
      
    case PerlinNoiseType::Wood:
      return wood( n.x, n.y, n.z ) ;
      
    case PerlinNoiseType::Marble:
    default:
      return marble( n.x,n.y,n.z, n.x ) ;

    case PerlinNoiseType::CustomLinear:
      return linearSpline( PerlinNoise3D( n.x,n.y,n.z, persistence, lacunarity, octaves ) ) ;
      
    case PerlinNoiseType::CustomQuadratic:
      return quadraticSpline( PerlinNoise3D( n.x,n.y,n.z, persistence, lacunarity, octaves ) ) ;

    case PerlinNoiseType::CustomCubic:
      return cubicSpline( PerlinNoise3D( n.x,n.y,n.z, persistence, lacunarity, octaves ) ) ;

    case PerlinNoiseType::CustomQuartic:
      return quarticSpline( PerlinNoise3D( n.x,n.y,n.z, persistence, lacunarity, octaves ) ) ;

    case PerlinNoiseType::CustomQuintic:
      return quinticSpline( PerlinNoise3D( n.x,n.y,n.z, persistence, lacunarity, octaves ) ) ;

  }
}

real PerlinGenerator::mountainHeight( real u, real v )
{
  //return PerlinNoise2D( u, v, persistence, lacunarity, octaves ) ;
  return PerlinNoise2D( u, v, 2, 2, 4 ) ;
}

Model* PerlinGenerator::heightmapNonindexed( int udivs, int vdivs, real uSize, real vSize, real normalFindStepRadians )
{
  Model* model = new Model( "heightmap" ) ;

  Mesh* mesh = new Mesh( model, MeshType::Nonindexed, VertexType::VT10NC10 ) ;
  model->addMesh( mesh ) ;

  real uStep = 1.0 / udivs ;
  real vStep = 1.0 / vdivs ;

  // u and v must be normalized to sample the perlin noise function properly

  // do not use non-integral loop counters, because termination condition is finicky
  for( int u = 0 ; u < udivs ; u++ ) //x
  {
    for( int v = 0 ; v < vdivs ; v++ ) //z
    {
      real fu = (real)u/udivs ;
      real fv = (real)v/vdivs ;

      // 1- 4
      // |/ |
      // 2- 3
      Vector ns[4], cs[4];
      Vector c=1;//Vector::random();
      for( int i = 0 ; i < 4 ; i++ ) cs[i]=c ;

      Vector ps[4] = {
        Vector( u, 0, v ),
        Vector( u, 0, v+vStep ),
        Vector( u+uStep, 0, v+vStep ),
        Vector( u+uStep, 0, v )
      } ;

      for( int i = 0 ; i < 4 ; i++ )
      {
        ps[i].x *= uSize ;
        ps[i].z *= vSize ;
      }


      for( int i = 0 ; i < 4 ; i++ )
        ps[i].y = mountainHeight( ps[i].x, ps[i].z ) ;
      
      // calculate normals.
      int nCount = 0 ;
      for( real ro = 0 ; ro < 2*PI ; ro += normalFindStepRadians )
      {
        nCount++;
        
        // rotate about the y-axis, to get points around y1
        Vector base1( uStep*uSize/32,0,0 ), base2( uStep*uSize/32,0,0 ) ;

        // rotation only changes x and z.
        base1.rotateY( ro ) ;
        base2.rotateY( ro+normalFindStepRadians ) ;

        Vector s(1,1,1);
        //window->addDebugLine( s, Vector(1,0,0), s+base1, Vector(1,0,0) ) ;
        //window->addDebugLine( s, Vector(1,0,0), s+base2, Vector(1,0,0) ) ;

        for( int i = 0 ; i < 4 ; i++ )
        {
          // Get the heights 
          base1.y = mountainHeight( ps[i].x + base1.x,    ps[i].z + base1.z ) ;
          base2.y = mountainHeight( ps[i].x + base2.x,    ps[i].z + base2.z ) ;
          
          // base1 always "lags" base2
          ns[i] += base1 << base2 ;
          window->addDebugLine( ps[i] + base1, Vector(1,0,0), ps[i] + base2, Vector(.5,0,0) ) ;
        }
      }

      for( int i = 0 ; i < 4 ; i++ )
      {
        ns[i] /= nCount ;
        ns[i].normalize();

        // multiply these 
        //ps[i].x *= uSize ;
        //ps[i].z *= vSize ;
      }

      // 0- 3
      // |/ |
      // 1- 2
      // add to mesh
      AllVertex fvs[4] = {
        AllVertex( ps[0], ns[0], cs[0], 0, 0 ),
        AllVertex( ps[1], ns[1], cs[1], 0, 0 ),
        AllVertex( ps[2], ns[2], cs[2], 0, 0 ),
        AllVertex( ps[3], ns[3], cs[3], 0, 0 )
      } ;

      mesh->addTri( fvs[0],fvs[1],fvs[3] ) ;
      mesh->addTri( fvs[1],fvs[2],fvs[3] ) ;

    }
  }

  return model ;
}

Model* PerlinGenerator::heightmapIndexed( int udivs, int vdivs, real uSize, real vSize, real normalFindStepRadians )
{
  Model* model = new Model( "heightmap" ) ;

  Mesh* mesh = new Mesh( model, MeshType::Indexed, VertexType::VT10NC10 ) ;
  model->addMesh( mesh ) ;

  real us = 1.0 / udivs ;

  // u and v must be normalized to sample the perlin noise function properly
  for( int u = 0 ; u < udivs ; u++ ) //x
  {
    for( int v = 0 ; v < vdivs ; v++ ) //z
    {
      real fu = (real)u/udivs ;
      real fv = (real)v/vdivs ;
      
      Vector p(fu,0,fv), c=1, n;
      
      p.y = mountainHeight( p.x, p.z ) ;
      
      p.x *= uSize ;
      p.z *= vSize ;
      
      mesh->verts.push_back( AllVertex( p, n, c, 0, 0 ) ) ;
    }
  }

  // face tie up
  for( int i = 0 ; i < udivs-1 ; i++ )
  {
    for( int j = 0 ; j < vdivs-1 ; j++ )
    {
      // 3- 2
      // |/ |
      // 0- 1
      int v0 = (i*vdivs) + j,
          v1 = (i*vdivs) + j+1,
          v2 = ((i+1)*vdivs) + j+1,
          v3 = ((i+1)*vdivs) + j ;
      
      mesh->addTriByIndex( v0, v1, v3 ) ;
      mesh->addTriByIndex( v1, v2, v3 ) ;
    }
  }

  mesh->calculateNormals();
  return model ;
}
