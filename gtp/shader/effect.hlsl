//#pragma pack_matrix( column_major ) // COLUMN MAJOR.  Your matrix is packed as column major by default.
#pragma pack_matrix( row_major )
#define MAX_LIGHTS 32
#define SH_MAX_BANDS 10 // i'm allowing up to 10 band SH
#define VOVECTORSPERBLOCKER 8  // there are 8 vo vectors

cbuffer Globals : register( b0 )
{
  int4 activeLights ; //ambient,diffuse,specular,unused
  
  float4 ambientLightColor[MAX_LIGHTS] ; 
  // Ambient: ACTUALLY NOT USED, it looks too bad.

  // diffuseLightPos: position of the diffuse light.
  float4 diffuseLightPos[MAX_LIGHTS] ; 
  float4 diffuseLightColor[MAX_LIGHTS] ;
  
  // Specular lights can have a direction if you wish
  // I guess this is most useful for vpls.
  float4 specularLightPos[MAX_LIGHTS] ;
  float4 specularLightDir[MAX_LIGHTS] ;
  float4 specularLightColor[MAX_LIGHTS] ;
  
  float4 eyePos ;
  float4 expos ;
  float4 vo ; // x=mode,y=blockage str,z=caustic str
  float4 voI ;
  // the normal to the surface, to work in form factors (cubemap renderer)
  ////  float4 vnormal ; //(unused)
} ;

cbuffer Globals1 : register( b1 )
{
  float4x4 modelViewProj ;
};

cbuffer Globals2 : register( b2 )
{
  float4x4 world ;
};

cbuffer Globals3 : register( b3 )
{
  float4x4 textureRotation ;
} ;

cbuffer Globals4 : register( b4 )
{
  // i need rgb coeffs anyway, i waste 1 float
  // per coeff triplet for padding,
  float4 shLightingCoeffs[ SH_MAX_BANDS*SH_MAX_BANDS ] ;
  float4 shOptions ;
} ;

cbuffer Globals5 : register( b5 )
{
  // for waves.
  float4 timePhaseFreqAmp[ 50 ] ; // these 4 packed into a float4
  float4 longAxis[ 50 ] ;
  float4 transverseAxis[ 50 ] ;
} ;

// 2x float4 textures
Texture2D REG0TEX : register( t0 );
Texture2D REG1TEX : register( t1 );

TextureCube REG2CubeTex : register( t2 ) ;

// SH DATA TEXTURE
Texture2D<float3> SHTEX : register( t3 );

// VO DATA TEXTURE
Texture2D<float4> VOTEX : register( t3 );

// this one is variable (changes with user key press)
SamplerState texSampler : register( s0 );

// this one is ALWAYS linear
SamplerState texSamplerLinear : register( s1 );

SamplerState texSamplerAnisotropic : register( s2 );




/// apply both world and modelview transforms
float4 transform( float3 pt )
{
  // Apply WORLD, then MODELVIEWProj.
  return mul( float4(pt,1), mul(world,modelViewProj) ) ;
}

float avg( float3 val )
{
  return ( val.x + val.y + val.z ) / 3 ;
}

float3 tonemap( float3 color )
{
  //expos.x = .12 ; // "exposure"
  //expos.y = 70 ; // supposed to be the "max value"
  float yd = expos.x * ( (expos.x/expos.y) + 1 ) / ( expos.x + 1 ) ;
  
  return color * yd ;
}

// YOU COULD move the lights in the shader
///float3 moveLight( float3 pos ) { }

// a vertex shader has input and output formats
// px shader always outputs a color
// 
// vvnc - accepts vnc and outputs vnc to shader
// vvnc_phong_vc - accepts vnc and outputs vc to px shader (phong lighting per-vertex)
// vG_vnc - accepts


/////////////////////////////////////vvc
struct vvcIn
{
  float3 pos : POSITION ;
  float4 color : COLOR0 ; //!!float psize : PSIZE ;
} ;
struct vvcOut
{
  float4 color : COLOR0 ;
  float4 pos : SV_Position ;
} ;
struct pxvcIn
{
  float4 color : COLOR0 ; // pxvc has
  // no business with SV_POSITION, so
  // it's not included in the input
  // to this pixel shader
} ;

// Transformation by modelViewProj only,
//   passes color
// FOR DEBUG LINES, DEBUG SURFACES
vvcOut //->TO pxvc
vvc( vvcIn input )
{
  vvcOut output ;
  output.pos = transform(input.pos) ;
  output.color = input.color ;
  return output ;
}

// Passes the input color (from
// vertex shader) directly to the output (screen)
float4 pxvc( pxvcIn input ) : SV_TARGET // FROM vvc
{
  return input.color ;
}
///////////////////////////////////// /vvc



///////////////////////////////////// vvnc
struct vvncIn
{
  float3 pos : POSITION ;
  float3 norm : NORMAL ;
  float4 color : COLOR0 ;
} ;
struct vvncOut
{
  float4 color : COLOR0 ; // only the pixel shader can output sv_target
  float3 untransformedPos : TEXCOORD0 ;
  float3 norm : TEXCOORD1 ;  
  float4 pos : SV_POSITION ;
} ;
struct pxvncIn
{
  float4 color : COLOR0 ;
  float3 untransformedPos : TEXCOORD0 ;
  float3 norm : TEXCOORD1 ;  
} ;

// Transformation by modelViewProj only,
//   passes color0
//   passes normal
//   passes untransformed position
// FOR DOING N DOT L (PHONG) SHADING IN A PIXEL SHADER
vvncOut //->TO pxvncUnlit
vvnc( vvncIn input )
{
  vvncOut output;

  output.pos = mul( float4(input.pos,1), modelViewProj ) ;
  output.untransformedPos = input.pos ; // send the ORIGINAL position to the pixel shader
  output.norm = input.norm ; // pass the normal
  output.color = input.color ; // Pass the original color
  return output ;
}

// Transformation by modelViewProj,
//   compute diffuse and specular color (per-vertex)
//   output final color
vvcOut //->TO pxvc
vvncPhong( vvncIn input )
{
  vvcOut output ;
  output.pos = transform(input.pos) ;
  output.color = input.color ;
  return output ;
} 

float4 pxvncUnlit( pxvncIn input ) : SV_TARGET
{
  return saturate( input.color ) ; // don't color it with anything special
}

float4 pxvncWavelet( pxvncIn input ) : SV_TARGET
{
  // vnormal is set BEFORE calling this render call,
  // each pixel compares with that normal
  // 
  // I need the actual ROW and COLUMN of the current fragment to do this.
  ///if( input.color == 0 )
  ///{
  ///  input.color += ffCos( 
  ///}
}

// the environment map is sampled by using
// the specular reflection.
float4 pxvncEnvMap( pxvncIn input ) : SV_TARGET
{
  float3 incident = normalize( input.untransformedPos - eyePos.xyz ) ;
  
  //float3 R = incident - 2 * dot( incident, input.norm ) * input.norm ;
  float3 R = reflect( incident, input.norm ) ;

  float4 textureColor = REG2CubeTex.Sample( texSamplerLinear, R );
  
  return saturate( textureColor + input.color ) ;
}

// just projects the cube map onto a sphere (plasters it)
float4 pxvncCubeMap( pxvncIn input ) : SV_TARGET
{
  // use normalized coordinates of the sphere vertex for texcoord
  // rotate the vector by the "texture matrix"
  float4 tr = mul( float4( normalize( input.untransformedPos.xyz ), 1 ), textureRotation ) ;
  float4 textureColor = REG2CubeTex.Sample( texSamplerLinear, tr.xyz );
  return textureColor ;
}
///////////////////////////////////// /vvnc



///////////////////////////////////// vvtc
struct vvtcIn {
  float3 pos : POSITION;
  float4 tex : TEXCOORD0;
  float4 color : COLOR0;
} ;
struct vvtcOut {
  float4 color : COLOR0;
  float4 tex : TEXCOORD0;
  float4 pos : SV_POSITION;
} ;
struct pxvtcIn{
  float4 color : COLOR0;
  float4 tex : TEXCOORD0;
};

// transforms geometry
//   -passes texcoord0
//   -passes color0, INCLUDING ALPHA
// for plastering a texture onto geometry,
// must send to a texturing fragment shader
// so it can actually SAMPLE the texture
vvtcOut //-> TO pxvtc
vvtc( vvtcIn input )
{
  vvtcOut output ;
  output.pos = transform(input.pos) ;
  output.tex = input.tex ;
  output.color = input.color ;
  return output ;
}

// sample the texture. modulate with input color (usually white)
float4 pxvtc( pxvtcIn input ) : SV_TARGET // FROM vvtc, OR vGTexDecal_vtc
{
  return input.color * REG0TEX.Sample( texSampler, input.tex.xy ) ;
}
///////////////////////////////////// /vvtc





//////////// FULL PHONG
struct vvnccccFullPhongOut
{
  // the 4 phong components are included here.
  float4 colorDiffuse : COLOR0 ;
  float4 colorSpecular : COLOR1 ;
  float4 colorEmissive : COLOR2 ;
  float4 colorTransmissive : COLOR3 ;
  float3 untransformedPos : TEXCOORD0 ;
  float3 norm : TEXCOORD1 ;  
  float4 pos : SV_POSITION ;
} ;
struct pxvnccccFullPhongIn {
  // I take in 4 colors: diffuse, specular, emissive, transmissive
  float4 colorDiffuse : COLOR0 ;
  float4 colorSpecular : COLOR1 ;
  float4 colorEmissive : COLOR2 ;
  float4 colorTransmissive : COLOR3 ;
  float3 untransformedPos : TEXCOORD0 ;
  float3 norm : TEXCOORD1 ;  
} ;

// diffuse: color depends on angle normal makes with vector to light position
// the diffuse lighting is rgb proportional
// to the cosine of the angle of SURFACE_NORMAL
// with VECTOR_SURFACE_TO_LIGHT source
float diffusePoint( float3 surfacePt, float3 surfaceN, float3 lightPos )
{
  float3 L = normalize( lightPos - surfacePt ) ;// pt to light // vector from original space pos to diffuse light pos, world space
  float nDotL = max( dot( surfaceN,L ), 0 ) ; // clamp below by 0 - can't be negative
  return nDotL ;
}

float diffuseDirectional( float3 surfaceN, float3 lightDirNormalized )
{
  // LIGHTDIR,surfaceN SHOULD BE NORMALIZED.
  return max( dot( surfaceN, lightDirNormalized ), 0 ) ;
}

// specular: has view dependence
float specularPower( float3 eye, float specPower, float3 surfacePt, float3 surfaceN, float3 lightPos )
{
  float3 V = normalize( surfacePt - eye ) ;// eyeToSurface
  float3 R = reflect( V, surfaceN ) ;//want to see if this lines up w/ toLight
  float3 L = normalize( lightPos - surfacePt ) ;

  float sdot = max( dot( R, L ), 0 ) ; //%reflection
  float spower = pow( sdot, specPower ) ; //raise the fractional dot product
  // to a power to reduce size of highlight

  return spower ;
}

// if you already have the vector TO the lightPos,
// WHEN the viewing angle LINES UP with
// the REFLECTION of surfaceToLight, you get
// a highlight.
//////float specularDir( float3 viewDir, float exponent, float3 surfaceN, float3 surfaceToLight )
//////{
//////  float3 R = reflect( viewDir, surfaceN ) ;
//////  float sdot = max( dot( R, surfaceToLight ),0 ) ;// want this to line up/be collinear for spec highlight.
//////  return pow( sdot, exponent ) ;
//////}


float4 pxvnccccFullPhong( pxvnccccFullPhongIn input ) : SV_TARGET // FROM vGFullPhong_vncccc
{
  /////float3 ambient = input.colorAmbient.rgb * ambientLightColor[0].rgb ; // removed

  // the interpolated normal is not normalized
  input.norm = normalize( input.norm ) ;

  float3 diffuse = { 0,0,0 } ;
  float3 specular = { 0,0,0 } ;

  //int i = 0 ;  // only want to see 1 light
  for( int i = 0 ; i < activeLights[1] ; i++ ) // switch the for loop on to cover all lights.
  {
    // treat as point lights
    float diffPower = diffusePoint( input.untransformedPos, input.norm, diffuseLightPos[i].xyz ) ;

    // treat as directional lights (no position, only direction)
    //float diffPower = diffuseDirectional( input.norm, diffuseLightPos[i].xyz ) ;

    diffuse += diffPower * diffuseLightColor[i].rgb * input.colorDiffuse.rgb ;

    float specPower = specularPower( eyePos.xyz, input.colorSpecular.w, input.untransformedPos, input.norm, specularLightPos[i].xyz ) ;
    specular += specPower * specularLightColor[i].rgb * input.colorSpecular.rgb ;
  }
  
  float3 color = tonemap( input.colorEmissive.rgb + diffuse + specular ) ; // * float3(0,.023,.07) ; //laem water fx
  
  // HIGH transmissive means translucent, ie use 1-trans.
  float alpha = 1-avg(input.colorTransmissive.rgb) ;
  return float4( (1-input.colorTransmissive.rgb)*color, alpha ) ;

  // Could also do per-channel trans, by doing separately for each channel
}
//////////// /FULL PHONG





///////////////////////////////////// GENERALVERTEX (all inputs saved in the vertex buffer)
struct GENERALVERTEX
{
  float3 pos          : POSITION ;
  float4 tDecal       : TEXCOORD0 ;   // DECAL
  float4 tRadiosityId : TEXCOORD1 ;   // id color for hemicube approx
  float4 tVOIndex     : TEXCOORD2 ;   // Specular INDEX texcoord, u=u tex, v=v tex, w=NUM SPECULAR COMPONENTS
  float4 tSHTexIndex  : TEXCOORD3 ;   // indices for the spherical harmonic texture coordinates.
  float4 tUnused4   : TEXCOORD4 ;
  float4 tUnused5   : TEXCOORD5 ;
  float4 tUnused6   : TEXCOORD6 ;
  float4 tUnused7   : TEXCOORD7 ;
  float4 tUnused8   : TEXCOORD8 ;
  float4 tUnused9   : TEXCOORD9 ;
  float3 norm       : NORMAL ;
  float4 cDiffuse   : COLOR0 ;  // diffuse "natural" BASE material color
  float4 cEmissive  : COLOR1 ;  // EMISSIVE (initial exitance.. most 0 except LOCAL area light sources)
  float4 cSpecular  : COLOR2 ;  // specular material
  float4 cTrans     : COLOR3 ;  // transmissive material
  float4 cRadiosity : COLOR4 ;  // radiosity computation result (ss final exitance)
  float4 cSHLit     : COLOR5 ;  // shlighted color, cpu
  float4 cWavelet   : COLOR6 ;  // wavelet
  float4 cUnused7   : COLOR7 ;
  float4 cUnused8   : COLOR8 ;
  float4 cUnused9   : COLOR9 ;
} ;

// advances your texture index in a texture with width w
// (jumps to next row when you walk off the + side of the texture)
void nextIndexOffset( inout float2 index, float w )
{
  index.x++ ;
  if( index.x >= w ) // if( w - index.x < 0.01 )
  {
    index.x = 0 ;
    index.y++ ;
  }
}

// each of these is a mapping.
// incoming vertex colors can be treated as
// anything: 
vvncOut //->TO pxvncUnlit, pxvncPhong, pxvncCubeMap
vGDiffuse_vnc( GENERALVERTEX input ) {
  vvncOut output;
  output.pos = transform(input.pos) ;
  output.untransformedPos = input.pos ;
  output.norm = input.norm ;
  output.color = input.cDiffuse + input.cEmissive ; // DIFFUSE + Emissive
  return output ;
}


float3x3 rotation( float3 u, float rads )
{
  float c, s ;
  sincos( rads, s, c ) ;
  float l_c = 1 - c ;
  
  return float3x3(
    u.x*u.x + (1 - u.x*u.x)*c,   u.x*u.y*l_c - u.z*s,   u.x*u.z*l_c + u.y*s,
    u.x*u.y*l_c + u.z*s,   u.y*u.y+(1 - u.y*u.y)*c,   u.y*u.z*l_c - u.x*s,  
    u.x*u.z*l_c - u.y*s,   u.y*u.z*l_c + u.x*s,   u.z*u.z + (1 - u.z*u.z)*c
  ) ;
}

void wave( float time, float phase, float freq, float amp,
  float3 longAxis, float3 transverseAxis, inout float3 pos, inout float3 norm )
{
  // the displacement to the pos variable is function of amp and angle
  // in the direction of the transverseAxis
  pos += amp*sin( time*freq + phase ) * transverseAxis ;
  //pos += amp*sin( time*freq + phase ) * longAxis ; // Displace along longitudinal axis as well.

  // the change to the normal components
  float slope = freq*amp*cos( time*freq + phase ) ;
  float t = atan( slope ) ;
  
  float3 zAxis = cross( longAxis, transverseAxis ) ;

  // rotation about "z" axis 
  norm = normalize( mul( rotation( zAxis, t ), norm ) ) ;
}

// Bends 2 normals (shadow,caustic) instead of just 1
void wave2( float time, float phase, float freq, float amp,
  float3 longAxis, float3 transverseAxis, inout float3 pos, inout float3 norm1, inout float3 norm2 )
{
  // the displacement to the pos variable is function of amp and angle
  // in the direction of the transverseAxis
  pos += amp*sin( time*freq + phase ) * transverseAxis ;
  //pos += amp*sin( time*freq + phase ) * longAxis ; // Displace along longitudinal axis as well.

  // the change to the normal components
  float slope = freq*amp*cos( time*freq + phase ) ;
  float t = atan( slope ) ;
  
  float3 zAxis = cross( longAxis, transverseAxis ) ;

  // rotation about "z" axis 
  norm1 = normalize( mul( rotation( zAxis, t ), norm1 ) ) ;
  norm2 = normalize( mul( rotation( zAxis, t ), norm2 ) ) ;
}

// this just moves vncccc values to the outputs
vvnccccFullPhongOut //->TO pxvnccccFullPhong
vGFullPhong_vncccc( GENERALVERTEX input )
{
  vvnccccFullPhongOut output ;

  if( vo.x > 1 )
  {
    for( int i = 0 ; i < 50 ; i++ )
    {
      wave( vo.z, dot( input.pos.xyz, longAxis[i].xyz ), timePhaseFreqAmp[i].z, timePhaseFreqAmp[i].w,
            longAxis[i].xyz, transverseAxis[i].xyz, input.pos.xyz, input.norm ) ;
    }
  }

  output.pos = transform(input.pos) ;
  output.untransformedPos = input.pos ;
  output.norm = input.norm ;
  output.colorDiffuse = input.cDiffuse ;
  output.colorEmissive = input.cEmissive ;
  output.colorSpecular = input.cSpecular ;
  output.colorTransmissive = input.cTrans ;
  
  //output.colorAmbient = float4(0,0,0,0) ;
  
  return output ;
}

// this just moves vncccc values to the outputs
vvncOut //->TO pxvncUnlit
vGFullPhong_vnc( GENERALVERTEX input )
{
  vvncOut output ;

  output.pos = transform(input.pos) ;

  /////float3 ambient = input.colorAmbient.rgb * ambientLightColor[0].rgb ; // removed

  float3 diffuse = { 0,0,0 } ;
  float3 specular = { 0,0,0 } ;

  for( int i = 0 ; i < activeLights[1] ; i++ ) // switch the for loop on to cover all lights.
  {
    // treat as point lights
    float diffPower = diffusePoint( input.pos, input.norm, diffuseLightPos[i].xyz ) ;

    // treat as directional lights (no position, only direction)
    //float diffPower = diffuseDirectional( input.norm, diffuseLightPos[i].xyz ) ;

    diffuse += diffPower * diffuseLightColor[i].rgb * input.cDiffuse.rgb ;

    float specPower = specularPower( eyePos.xyz, input.cSpecular.w, input.pos, input.norm, specularLightPos[i].xyz ) ;
    specular += specPower * specularLightColor[i].rgb * input.cSpecular.rgb ;
  }
  
  float3 color = tonemap( input.cEmissive.rgb + diffuse + specular ) ;
  
  output.color = float4( (1-input.cEmissive.rgb)*color, 1-avg(input.cTrans.rgb) ) ; // HIGH transmissive means translucent, ie use 1-trans.

  return output ;
}


// For applying a decal texture
vvtcOut //->TO pxvtc
vGTexDecal_vtc( GENERALVERTEX input ) {
  vvtcOut output;
  output.pos = transform(input.pos) ;
  output.tex = input.tDecal ;
  output.color = input.cDiffuse ;
  return output ;
}

// SH shader - sh computed light
vvncOut //->TO pxvncUnlit
vGSHCPU_vnc( GENERALVERTEX input )
{
  vvncOut output;
  output.pos = transform(input.pos) ;
  output.untransformedPos = input.pos ;
  output.norm = input.norm ;
  output.color = input.cSHLit ; // cpu computed, PASSES ALPHA
  return output ;
}

vvncOut //->TO pxvncUnlit
vGSHGPU_vnc( GENERALVERTEX input )
{
  vvncOut output;
  output.pos = transform(input.pos) ;
  output.untransformedPos = input.pos ;
  output.norm = input.norm ;
  output.color = input.cSHLit ; // cpu computed, PASSES ALPHA

  
  //sh lighting computation,
  float width,height;
  SHTEX.GetDimensions( width, height ) ;

  // Gets the right starting position for this vertex
  float2 texIndex = input.tSHTexIndex.xy ; 
  
  float3 shColor = { 0,0,0 } ;

  //DIFFUSE
  for( int i = 0 ; i < input.tSHTexIndex.z && 
                   i < shOptions.x ;  // shOptions.x cuts off evaluation early if using fewer bands
                   i++ )
  {
    shColor += SHTEX[ texIndex ] * shLightingCoeffs[ i ].rgb ;
    nextIndexOffset( texIndex, width ) ;
  }

  //SPECULAR
  //for( int i = 0 ; i < input.tSHTexIndex.w ; i++ )
  //{
  //  shColor += SHTEX[ texIndex ] * shLightingCoeffs[ i ].rgb ;
  //  nextIndexOffset( texIndex, width ) ;
  //}

  output.color = float4( tonemap( shColor ), 1-avg(input.cTrans.rgb) ) ; // use alpha from the diffuse material

  //clamp the max color below by this color
  ////clamp( maxColor, output.color, float4( 1e6,1e6,1e6,1e6 ) ) ;

  return output ;
}

// ID shader
vvncOut //->TO pxvncUnlit
vGID_vnc( GENERALVERTEX input )
{
  vvncOut output;
  output.pos = transform(input.pos) ;
  output.untransformedPos = input.pos ;
  output.norm = input.norm ;
  output.color = input.tRadiosityId ; // must have full alpha, otherwise doesn't appear
  
  return output ;
}

vvncOut //->TO pxvncUnlit
vGRadiosity_vnc( GENERALVERTEX input )
{
  vvncOut output ;
  output.pos = transform(input.pos) ;
  output.untransformedPos = input.pos ;
  output.norm = input.norm ;
  //output.color = input.cRadiosity ; // RADIOSITY comp result
  output.color = float4( tonemap( input.cRadiosity.rgb ), input.cRadiosity.w ) ; // RADIOSITY comp result
  return output ;
}

vvncOut //->TO pxvncWavelet-NO XX (pxvncUnlit)
vGWavelet_vnc( GENERALVERTEX input )
{
  vvncOut output ;
  output.pos = transform(input.pos) ;
  output.untransformedPos = input.pos ;
  output.norm = input.norm ;
  output.color = input.cWavelet ; // WAVELET comp result
  return output ;
}













// Between 0 and 1
// If the angle is obtuse you get 1.0
// Function works this way on purpose
float getSin2AcuteAngle( float3 v1, float3 v2 )
{
  // if angle >90 deg then you get just 1 back.
  float cosAngle = max( dot( v1, v2 ), 0 ) ; 
  float cos2Angle = cosAngle*cosAngle ;
  return 1 - cos2Angle ; // will be 1.0 if no blockage
}

// Just a waving shader
float4 pxvncWave( pxvncIn input ) : SV_TARGET
{
  float3 vToLight = normalize( diffuseLightPos[0].xyz - input.untransformedPos ) ;
  
  float3 dummy ;
  for( int i = 0 ; i < 50 ; i++ )
  {
    // bend the normal due to caustic
    wave( vo.z, dot( input.untransformedPos, longAxis[i].xyz ), timePhaseFreqAmp[i].z, timePhaseFreqAmp[i].w,
          longAxis[i].xyz, transverseAxis[i].xyz, dummy, input.norm ) ;
  }

  float sin2Angle = getSin2AcuteAngle( input.norm, vToLight ) ;
  float3 color = pow( sin2Angle, -vo.y ) * float3(.2,.23,.6) ;
  
  return float4( tonemap( color ), 1 ) ;
}

// struct pxvncIn{
//   float4 color : COLOR0 ;
//   float3 untransformedPos : TEXCOORD0 ;
//   float3 norm : TEXCOORD1 ;
// };

// This is per-vertex VO
float4 pxvncVO( pxvncIn input ) : SV_TARGET
{
  return saturate( input.color ) ;
}

// Per vertex vo shader
vvncOut //->TO pxvncVO
vGVectorOccluders_vnc( GENERALVERTEX input )
{
  vvncOut output ;
  output.pos = transform(input.pos) ;
  output.untransformedPos = input.pos ;
  output.norm = input.norm ;

  // Need these vectors everywhere, compute them now
  float3 eyeToV = normalize( input.pos.xyz - eyePos.xyz ) ;
  float3 eyeRefl = reflect( eyeToV, input.norm ) ;

  //  AO ONLY || THERE ARE NO BLOCKERS
  // I tested it and this if statement does not cost much.
  if( vo.x==0 || input.tVOIndex.z == 0 )
  {
    // primary diffuse and specular only
    float3 primaryDiffuseColor = {0,0,0} ;
    float3 primarySpecularColor = {0,0,0} ;

    for( int i = 0 ; i < activeLights[1] ; i++ )
    {
      float3 vToLight = normalize( diffuseLightPos[i].xyz - input.pos ) ;
      primaryDiffuseColor += max( dot( input.norm, vToLight ), 0 ) * 
        input.cDiffuse.rgb * diffuseLightColor[i].rgb * 
        pow( input.tVOIndex.w, vo.y ) ; // exaggerate AO with vo.y.
        // if AO is 1, then nothing happens.

      float spower = max( dot( eyeRefl, vToLight ), 0 ) ;
      spower = pow( spower, input.cSpecular.w ) ;
      primarySpecularColor += spower * specularLightColor[i].rgb * input.cSpecular.rgb ;
    }

    output.color = float4( (1-input.cTrans.rgb)*tonemap(primaryDiffuseColor + primarySpecularColor), 1-avg( input.cTrans.rgb ) ) ;
    return output ;
  }

  

  ///////////
  // VO
  float3 fColor = {0,0,0};

  float width,height;
  VOTEX.GetDimensions( width, height ) ;
  float2 texIndex = input.tVOIndex.xy ; 
  
  int nBlockers = input.tVOIndex.z / VOVECTORSPERBLOCKER ;  // VOVECTORS is # vectors used per blocker, default 8
  float3 p[ MAX_LIGHTS ] ; // save primary colors for EVERY LIGHT,
  // which we will attenuate later with VO shadows
  // max # lights=default 32

  // First save-in the primary (1st bounce/direct illum) colors for EVERY LIGHT on vertex
  for( int lNo = 0 ; lNo < activeLights[1] ; lNo++ )
  {
    float3 vToLight = normalize( diffuseLightPos[lNo].xyz - input.pos ) ;
    p[lNo] = max( dot( input.norm, vToLight ), 0 ) * input.cDiffuse.rgb * diffuseLightColor[lNo].rgb ;
    p[lNo] += pow( max( dot( eyeRefl, vToLight ), 0 ), input.cSpecular.w ) * specularLightColor[lNo].rgb * input.cSpecular.rgb ;
  }

  // Next look at all sets of VO vectors at this vertex (1 set of 8 per occluder seen)
  for( int k = 0 ; k < input.tVOIndex.z ; k += VOVECTORSPERBLOCKER )
  {
    // 1 POS
    float3 slPos = VOTEX[ texIndex ] ;
    nextIndexOffset( texIndex, width ) ;

    // 2 SHADOW DIR
    float3 slSurfNShadow = VOTEX[ texIndex ] ;
    nextIndexOffset( texIndex, width ) ;

    // 3 CAUSTIC DIR
    float3 slCausticDir = VOTEX[ texIndex ] ; // different from shadow due to light bending
    nextIndexOffset( texIndex, width ) ;

    // 4 CAUSTIC COLOR
    float3 slTransColor = VOTEX[ texIndex ] ; // rgb translucency/opacity
    nextIndexOffset( texIndex, width ) ;

    // 5 DIFFUSE NORMAL
    float3 slSurfNDiffuse= VOTEX[ texIndex ] ; // remoteSurfaceDiffuseNormal
    nextIndexOffset( texIndex, width ) ;

    // 6 DIFFUSE COLOR
    float3 slSurfDiffuseColor = VOTEX[ texIndex ] ; // remoteSurfaceDiffuseColor
    nextIndexOffset( texIndex, width ) ;
    
    // 7 SPECULAR NORMAL also represents THE DIRECTION OF THE NORMAL OF THE REFLECTOR SURFACE
    float3 slSurfNSpecular = VOTEX[ texIndex ] ; // remoteSurfaceSpecularNormal
    float slSurfNSpecularPower = length( slSurfNSpecular ) ;
    slSurfNSpecular /= slSurfNSpecularPower ; // any vector you reflect about must be normalized.
    nextIndexOffset( texIndex, width ) ;

    // 8 SPECULAR COLOR
    float3 slSurfSpecularColor = VOTEX[ texIndex ] ;
    nextIndexOffset( texIndex, width ) ;

    // If using 4-component vectors, can put this in .w
    float blockageFF = length( slSurfNShadow ) ;  // this is the FORM FACTOR of the occluding object.
    float3 blockageDirection = slSurfNShadow / blockageFF ;
    float causticFF = length( slCausticDir ) ; 
    float3 causticDirection = slCausticDir / causticFF ;
    
    // I tested it and this if statement does not cost much.
    if( vo.x > 1 ) {
      float3 dummy;
      for( int waveNo = 0 ; waveNo < 50 ; waveNo++ )
        wave2( vo.z, dot( input.pos.xyz, longAxis[waveNo].xyz ), timePhaseFreqAmp[waveNo].z, timePhaseFreqAmp[waveNo].w,
              longAxis[waveNo].xyz, transverseAxis[waveNo].xyz, dummy, causticDirection, blockageDirection ) ;
    }
    
    float3 secondaryDiffuseFromDiffuse   = {0,0,0} ;
    float3 secondaryDiffuseFromSpecular  = {0,0,0} ;
    float3 secondarySpecularFromSpecular = {0,0,0} ;

    for( int lNo = 0 ; lNo < activeLights[1] ; lNo++ )
    {
      float3 vToLight = ( diffuseLightPos[lNo].xyz - input.pos ) ;
      float distToLight = length( vToLight ) ;
      vToLight /= distToLight ;
      
      // sin^2 angle will be 1.0 if no blockage
      float blockageStrength = pow( getSin2AcuteAngle( blockageDirection, vToLight ), vo.y*blockageFF*distToLight ) ;
      float causticStrength  = pow( getSin2AcuteAngle( causticDirection, vToLight ), -vo.w*causticFF ) ;
      float3 shadowAtten = blockageStrength * (1-slTransColor) ; // atten. by blockage*(OPACITY). If not opaque no blockage.
      float3 causticAmp  = causticStrength * slTransColor ;      // mult by amp*TRANSLUCENCY. If not trans no amp.

      // Amplification/attenuation due to THIS BLOCKER.
      // caustic+shadow fight each other
      p[lNo] *= ( shadowAtten + causticAmp ) ;   // amp/reduce primary color

      // SECONDARY, DD
      float3 surfToLight = normalize( diffuseLightPos[lNo].xyz - slPos ) ;
      float dpower = max( dot( surfToLight, slSurfNDiffuse ), 0 ) ; // find power remote surface can send me
      secondaryDiffuseFromDiffuse += dpower * diffuseLightColor[lNo].rgb * slSurfDiffuseColor * input.cDiffuse.rgb ;
      
      // DS
      float3 vToSurf = normalize( slPos - input.pos.xyz ) ;
      float3 vToSurfRefl = reflect( vToSurf, slSurfNSpecular ) ;
      float spower = slSurfNSpecularPower * max( dot( vToSurfRefl, surfToLight ), 0 ) ; // original
      //float spower = max( dot( vToSurfRefl, surfToLight ), 0 ) ; // don't depend on ff (BAD)
      spower = pow( spower, voI.w ) ; // "hurt it" (reduce) proportionally to
      // material exponent.  spower is already proportional to "how much
      // vToSurfRefl lines up with surfToLight."
      secondaryDiffuseFromSpecular += spower * slSurfSpecularColor * input.cDiffuse.rgb ;
      
      // SS:
      // want eyeRefl==vToSurf, and eyeReflRefl==surfToLight
      // eyeRefl must POINT TO slPos.
      float ssLineup = max( dot( vToSurf, eyeRefl ), 0.0 ) ;
      ssLineup = pow( ssLineup, 1-slSurfNSpecularPower ) ; // WIDEN (0.4 nominal.. use 1-ff) smaller=WIDER band
      // forgiveness in lineup depends on SIZE of remote surface.

      // eyeReflRefl must POINT TO LIGHT to get SS refln
      float3 eyeReflRefl = reflect( eyeRefl, slSurfNSpecular ) ; // eye Reflect vertex Reflect remoteSurface
      ///spower = slSurfNSpecularPower * max( dot( eyeReflRefl, surfToLight ), 0 ) ; // original
      spower = ssLineup * max( dot( eyeReflRefl, surfToLight ), 0 ) ;
      spower = pow( spower, voI.w ) ; // voI.w is the exponent to be used in SS reflections, parameter
      secondarySpecularFromSpecular += spower * slSurfSpecularColor * input.cSpecular.rgb ;
    }

    // add in the secondary contributions FROM ALL LIGHTS due to this blocker
    fColor +=
      +secondaryDiffuseFromDiffuse * voI.x
      +secondaryDiffuseFromSpecular * voI.y
      +secondarySpecularFromSpecular * voI.z
    ;    
  }

  // Add in all 8 p lights
  for( int lNo = 0 ; lNo < activeLights[1] ; lNo++ )
  {
    fColor += p[lNo] ;
  }

  output.color = float4( input.cEmissive.rgb + fColor, 1-avg(input.cTrans.rgb) ) ;
  output.color.rgb = tonemap( (1-input.cTrans.rgb)*output.color.rgb ) ;

  return output ;
}










































///////////////////////////////////// /GENERALVERTEX




