#include "RaytracingCore.h"
#include "../window/GTPWindow.h"
#include "../math/perlin.h"
#include "../math/SH.h"
#include "../geometry/CubeMap.h"
#include "../geometry/Mesh.h"
#include "../threading/ParallelizableBatch.h"
#include "../math/SHVector.h"

Fragment Fragment::Zero ;

int FrameBuffer::fogMode ;
Vector FrameBuffer::fogColor ;
real FrameBuffer::fogFurthestDistance ; // REALLY_FAR.
real FrameBuffer::fogDensity ;

RayCollection* RaytracingCore::rc ;

  
const char* TraceTypeName[] = {
  "Whitted", "Distributed", "Path"
} ;


RaytracingCore::RaytracingCore( int iNumRows, int iNumCols,
    int iRaysPerPixel,
    bool iStratified,
    string iTraceType,
    int iRaysDistributed,
    int iRaysCubeMapLighting,
    int iMaxBounces, real iInterreflectionThreshold,
    bool iShowBg
  )
{
  cubeMap = 0 ;
  realTimeMode = false ;
  rows = iNumRows ;
  cols = iNumCols ;
  
  raysPerPixel = iRaysPerPixel ;
  sqrtRaysPerPixel = sqrt( (float)raysPerPixel ) ;
  stratified = iStratified ;
  
  if( iTraceType[0] == 'W' || iTraceType[0] == 'w' ) // whit
    traceType = TraceType::Whitted ;
  else if( iTraceType[0] == 'D' || iTraceType[0] == 'd' ) // dist
    traceType = TraceType::Distributed ;
  else
    traceType = TraceType::Path ;
  
  info( "%s tracing", TraceTypeName[traceType] ) ;

  raysDistributed = iRaysDistributed ; // #rays to average when using area lights.
  // if this value is __1__, it means to treat the light as a POINT SOURCE.
  if( raysDistributed > rc->n/8 && traceType==Path )
  {
    warning( "Path tracing: You're trying to distribute %d rays, "
      "but only %d rays in collection.  Must be _at least_ 8x as many rays in collection "
      "as dist rays. Reducing to %d rays",
      raysDistributed, rc->n, rc->n/8 ) ;
    raysDistributed = rc->n/8 ;
  }
  raysCubeMapLighting = iRaysCubeMapLighting ;  // BOOLEAN whether to use cubemap or not. need roughly 200,000 directions for 256x256 cubemap
  maxBounces = iMaxBounces ;
  interreflectionThreshold² = SQUARE( iInterreflectionThreshold ) ; // light must have 10% energy left.
  
  showBg = iShowBg ;

  //pixels.resize( rows*cols ) ;
  frameBuffer = new FrameBuffer( rows, cols ) ;
  bytePixels.resize( rows*cols ) ;

  viewingPlane = new ViewingPlane( rows, cols, raysPerPixel ) ;
}

RaytracingCore::~RaytracingCore()
{
  DESTROY( viewingPlane ) ;
}

void RaytracingCore::clear( Vector color )
{
  // clear out the pixels.
  frameBuffer->clear() ; // clear out the old fragments

  //memset( &bytePixels[ 0 ], 128, pixels.size()*sizeof(ByteColor) ) ;//not using this.
  ByteColor bc = color.toByteColor() ;
  for( int i = 0 ; i < bytePixels.size() ; i++ )
    bytePixels[i] = bc ;
}

void RaytracingCore::writePixelsToByteArray()
{
  // update the colors array with the
  // colorization procedure you want
  //frameBuffer->colorRaw( raysPerPixel ) ;
  //frameBuffer->colorByNumFragments( raysPerPixel ) ;
  //frameBuffer->colorNormalized( raysPerPixel ) ;

  bool normalizeColors = false ;
  
  if( normalizeColors )
  {
    real divby = 0 ;
    
    // This divides by the largest color, but does not spoil the color balance
    real largest = 0 ;
    for( int i = 0 ; i < frameBuffer->colors.size() ; i++ )
      // "push" largestColor up by max component, 
      clamp<real>( largest, frameBuffer->colors[i].max(), 1e6 ) ;

    real YD = window->expos * ( (window->expos/largest) + 1 ) / ( window->expos+1 ) ;
    printf( "largest=%f, YD=%f     \r", largest, YD ) ;

    for( int i = 0 ; i < bytePixels.size() ; i++ )
    {
      // tone mapping operator:
      Vector finalColor = YD * frameBuffer->colors[i] ;
      finalColor.w = frameBuffer->colors[i].w ; // line above loses the alpha
      // clamp them now so you don't get weird colors
      bytePixels[i] = finalColor.clamp(0,1).toByteColor() ;

    }
    
  }
  else
  {
    // no normalization
    for( int i = 0 ; i < bytePixels.size() ; i++ )
    {
      Vector finalColor = frameBuffer->colors[i]  ;
      bytePixels[i] = finalColor.clamp(0,1).toByteColor() ;
    }
  }
  

}


// ray traced spherical harmonic solution
Vector RaytracingCore::castRTSH( Ray& ray, Scene *scene )
{
  if( ray.bounceNum > 3 ) return 0 ; // don't let 2 bounces happen
  MeshIntersection mi ;
  
  if( !scene->getClosestIntnMesh( ray, &mi ) )
  {
    Vector finalColor ;
    
    if( cubeMap )
    {
      SVector raySphDir = ray.direction.toSpherical() ;
      finalColor = ray.power * cubeMap->getColor( raySphDir.tElevation, raySphDir.pAzimuth, ColorIndex::Emissive ) ;
    }
    else if( scene->perlinSkyOn ) // perlin sky
    {
      // sample the perlin sky!
      finalColor = ray.power*scene->perlin.sky( ray.direction.x, ray.direction.y, ray.direction.z,
                         PerlinGenerator::SKY_BLUE,
                         PerlinGenerator::CLOUD_WHITE ) ;
    }
    else
      finalColor = scene->bgColor ; // use the lame bg color

    return finalColor ;
  }


  // pick up the SH color
  // get the weighted sum of the sh projections at intn
  SHVector newsh ;
        
  // Get the 3 SH functions that will be blended,
  // FROM THE REMOTE MESH THAT YOU JUST HIT WITH THE FEELER RAY
  Mesh* m = mi.getMesh() ;
  SHVector* shA = m->verts[ mi.tri->iA ].shDiffuseSummed->scaledCopy( mi.bary.x ) ;
  SHVector* shB = m->verts[ mi.tri->iB ].shDiffuseSummed->scaledCopy( mi.bary.y ) ;
  SHVector* shC = m->verts[ mi.tri->iC ].shDiffuseSummed->scaledCopy( mi.bary.z ) ;
  newsh.add( shA )->add( shB )->add( shC ) ;
  Vector color = newsh.dot( scene->spRotated ) ; // use the last projected lighting function
  
  if( mi.shape->material.ks.len2() > 0 )
  {
    // Use the reflection direction to pick up the specular color.
    Ray specRay = ray.reflect( mi.normal, mi.point, mi.shape->material.ks ) ;

    // does this hit more geometry or free space?
    color += castRTSH( specRay, scene ) ;
  }

  if( mi.shape->material.kt.len2() > 0 )
  {
    // do all 3 rays.
    for( int i = 0 ; i < 3 ; i++ )
    {
      Ray transRay = ray.refract( mi.normal, i, mi.shape->material.eta.e[i], mi.point, mi.shape->material.kt ) ;
      color += castRTSH( transRay, scene ) ;
    }
  }
  
  return ray.power * color ;
}


// There are 4 types of rays:
//   1.  Pixel Rays or Eye Rays - Carry light directly to the eye thru a pixel on the screen (surface color - UNREFLECTED)
//   2.  Illumination or  Shadow rays - carry light from a light source directly to an object surface
//   3.  Reflection rays  - carry light reflected by an object
//   4.  Transparency rays - carry light passing thru an object
Vector RaytracingCore::cast( Ray& ray, Scene *scene )
{
  // RECURSION TERMINATION:
  // If the bounceNum is 1 and you allowed 0 bounces, then you get 0 back.
  // (if you allowed 0 bounces the scene will always be black).
  // if the bounceNum is 1 and you allowed 1 bounce, you will bounce then.
  if( ray.bounceNum > maxBounces || // cutoff bounce number, even if still high energy (lots of purely specular refln)
      ray.power.len2() < interreflectionThreshold² ) // it means you're NOT going to be reflecting much energy. when the magnitude of rgbJuice left is getting really small,
                                                     // don't trace, because the contrib will be really weak.
  {
    return 0 ;//BLANK
  }

  Intersection *intn ;
  
  // Shoot ray into the scene.  "See" CLOSEST surface we hit.
  if( !scene->getClosestIntn( ray, &intn ) )
  {
    // hits nothing (a base case)
    Vector finalColor ;
    
    // If not showing bg, then just return 0 when
    // a 0-bounce ray hits the background.
    if( !showBg && !ray.bounceNum )  return 0 ; // 0 alpha too
    
    if( cubeMap )
    {
      finalColor = ray.power * cubeMap->px( ray.direction ) ;
    }
    else if( scene->perlinSkyOn ) // perlin sky
    {
      // sample the perlin sky!
      finalColor = ray.power*scene->perlin.sky( ray.direction.x, ray.direction.y, ray.direction.z,
                         PerlinGenerator::SKY_BLUE,
                         PerlinGenerator::CLOUD_WHITE ) ;
    }
    else
      finalColor = scene->bgColor ; // use the lame bg color

    return finalColor ;  // if this was a "total miss" 
    // (missed all scene geometry), then w is 0 and 
    // we know to leave it out of the averaging.
    // This prevents the edges of objects from "glowing"
    // when against an intense cubemap that has SOME
    // hits on the object but many hits on the cubemap.
  }

  // Ok, we hit something.
  // DID WE HIT A LIGHT SOURCE WITH OUR RAY?
  // In this program light sources are Shapes.
  // So sample the light source at the location we hit it (the color it sends).
  Vector emittedColor = intn->getColor( ColorIndex::Emissive ) ;
  
  Vector diffuseColor ;
  
  // surface color at intn of eye->object
  Vector diffuseColorAtIntn = intn->getColor( ColorIndex::DiffuseMaterial ) ;
  
  // gather the diffuse color.
  if( diffuseColorAtIntn.nonzero() )  // only do if the surface IS actually diffuse
  {
    #pragma region direct lighting
    // Whitted: looks lamest
    if( traceType == Whitted )
    {
      for( int i=0 ; i < scene->lights.size() ; i++ )
      {
        // 1 ray to each light.
        Vector lightPos = scene->lights[i]->getCentroid() ; // using POINT LIGHTS (hard shadows) 
        // (even if they are area light sources use their centroid)

        Vector toLight = ( lightPos - intn->point ).normalize() ;
        real dot = intn->normal % toLight ;
        if( dot < 0 )  continue ; // ray shoots INTO surface
        
        // cast ray towards the light.  no change in eta, just a power loss due to diffuse reflection.
        Ray shadowRay( intn->point + EPS_MIN*intn->normal, toLight, 1000, ray.eta, ray.power*diffuseColorAtIntn, ray.bounceNum+1 ) ;
        shadowRay.isShadowRay = true ; // flag it as a shadow ray, so it can't reflect
        diffuseColor += dot * cast( shadowRay, scene ) ;
      }
    }
    else if( traceType == Distributed )
    {
      for( int i=0 ; i < scene->lights.size() ; i++ )
      {
        for( int li = 0 ; li < raysDistributed ; li++ )
        {
          Vector lightPos = scene->lights[i]->getRandomPointFacing( intn->normal ) ; // fuzzy shadows

          Vector toLight = ( lightPos - intn->point ).normalize() ;
          real dot = intn->normal % toLight ;
          if( dot < 0 )  continue ; // ray shoots INTO surface
        
          // cast ray towards the light.  no change in eta, just a power loss due to diffuse reflection.
          Ray shadowRay( intn->point + EPS_MIN*intn->normal, toLight, 1000, ray.eta, ray.power*diffuseColorAtIntn, ray.bounceNum+1 ) ;
          shadowRay.isShadowRay = true ; // flag it as a shadow ray, so it can't reflect
          diffuseColor += dot * cast( shadowRay, scene ) ;
        } // end each light, i
      }

      // average the color by dividing by rays shot towards the light.
      diffuseColor /= raysDistributed ; // average it.
    }
    #pragma endregion

    #pragma region cubemap lighting
    // be sure to limit bounce number to 1 or 2 if you DON'T want
    // this to take forever.
    if( raysCubeMapLighting && cubeMap )
    {
      int skip = cubeMap->pixelsPerSide/raysCubeMapLighting ;

      // Consider each cube map pixel as a point light.
      real dotSum = 0.0 ;
      for( int FACE = 0 ; FACE < 6 ; FACE++ )
      {
        for( int row = 0 ; row < cubeMap->pixelsPerSide ; row+=skip )
        {
          for( int col = 0 ; col < cubeMap->pixelsPerSide ; col+=skip )
          {
            // form a vector that points to the face
            Vector rayCubeDir = cubeMap->getDirectionConvertedLHToRH( FACE, row, col ) ;
            rayCubeDir.normalize();

            real dot = intn->normal % rayCubeDir ;
            if( dot < 0 )  continue ; // can't use this ray, because it's going to hit own face anyway.

            dotSum += dot ;
            Ray cubeRay( intn->point + intn->normal * EPS_MIN, rayCubeDir, 1000, ray.eta, ray.power*diffuseColorAtIntn, ray.bounceNum+1 ) ;
            
            // cast the ray towards that cubemap direction
            diffuseColor += dot * cast( cubeRay, scene ) ;
          } //each col
        } // each row
      } // end each face
    
      diffuseColor /= dotSum ;
    }
    #pragma endregion

    #pragma region path tracing
    if( traceType == Path )
    {
      // shows caustics, only looks good with immense (>200) number of rays
      // and high (>4) rays per pixel
      if( raysDistributed == 1 )
      {
        // "path tracing"
        // get random direction 
        Vector& dir = rc->vAboutAddr( intn->normal ) ; // retrieves a ray that doesn't shoot into the surface
        real dot = dir % intn->normal ;

        // shoot a diffuse ray feeler out.
        Ray pathRay( intn->point + EPS_MIN*intn->normal, dir, 1000.0, ray.eta, ray.power*diffuseColorAtIntn, ray.bounceNum+1 ) ;
      
        // cast a ray off and retrieve the color.
        // You have to hit a LIGHT SOURCE to actually
        // add some color.
        diffuseColor += dot * cast( pathRay, scene ) ;

        ////Vector dColor = cast( pathRay, scene ) ;
        ////diffuseColor += dot * dColor * (dColor.w > 0) ; // if dColor.w is 0, DON'T COUNT IT
        // (glowy edges problem)
      }
      else // more than 1 ray distributed for diffuse color gather
      {
        Vector gatheredColor ;
        real dotSum = 0.0 ;

        int startSamp = randInt( 0, rc->n ) ;

        // shoot "raysDistributed" rays in random directions
        // to estimate the color at pixel.
        for( int i = 0 ; i < raysDistributed ; i++ )
        {
          // On average, you only can use half the rays offered here,
          Vector& dir = rc->vAddr( startSamp+i ) ; // avoid copy, requires direction ok check
          real dot = intn->normal % dir ;
          if( dot < 0 ) // ray shoots into surface
          {
            i--; // didn't count
            startSamp = (startSamp+1) % rc->n ; // move startSamp up one, so we don't get the same samp again
            // mod around in case exceed rc->n.
            // This will only result in an infinite loop if raysDistributed is large
            // while rc->n is small.  Over-protected for in startup (on average rc->n must be >2x 
            // sample sze, but made it >8x).
            continue ;
          }
          
          Ray distRay( intn->point + EPS_MIN*intn->normal, dir, 1000.0, ray.eta, ray.power*diffuseColorAtIntn, ray.bounceNum+1 ) ;  // eta stays the same as where ray came from.
          diffuseColor += dot * cast( distRay, scene ) ;
        }

        diffuseColor /= raysDistributed ;
      }
    }
    #pragma endregion
  }// end diffuse lighting
  
  Vector specularColor ;
  Vector specularColorAtIntn = intn->getColor( ColorIndex::SpecularMaterial ) ;
  if( specularColorAtIntn.nonzero() && !ray.isShadowRay ) // shadow rays can't reflect.
  {
    // 3. Reflection Ray.
    //int a = 20 ;
    //for( int i = 0 ; i < a ; i++ )
    {
      Ray reflRay = ray.reflect( intn->normal, intn->point, specularColorAtIntn ) ;

      // jitter only for distributed ray tracing.
      if( traceType!=Whitted && intn->shape->material.specularJitter )
        reflRay.direction.jitter( intn->shape->material.specularJitter ) ; // jitter it.
      specularColor += cast( reflRay, scene ) ;
    }
    //specularColor /= a ;
  }

  #pragma region transmitted
  Vector txColorAtIntn = intn->getColor( ColorIndex::Transmissive ) ;

  // 4. Transparency rays
  // the direction of the transparency ray depends on
  // the refraction constant (snell/fresnel)
  // and the surface normal.
  Vector transmittedColor ;
  
  // the only time to change ETA is when you refract.
  Vector toEta ;

  // if the angle is OBTUSE, then the ray FACES the normal,
  // and so you are ENTERING A MEDIA, not free space
  if( ray.direction.obtuse( intn->normal ) ) // enter new media
    toEta = intn->shape->material.eta ;
  else // exit the surface, enter free space
    toEta = scene->mediaEta ;

  // Note that you only split 3 ways THE FIRST TIME you hit this function,
  // with full juice light.  subsequent calls that reach here actually
  // only refract in R, G or B (because the other 2 channels will be 0 then).

  // IF THE TX COMPONENTS ARE THE SAME THEN NEVER SPLIT THE RAY
  if( toEta.allEqual() )
  {
    // don't do transmission if the material hit can't transmit
    if( txColorAtIntn.notAll( 0 ) )
    {
      // They're all the same, so use x component
      Ray refractedRay = ray.refract( intn->normal, toEta.x, intn->point, ray.power*txColorAtIntn ) ;
      if( traceType!=Whitted && intn->shape->material.transmissiveJitter )
        refractedRay.direction.jitter( intn->shape->material.transmissiveJitter ) ;
      transmittedColor += cast( refractedRay, scene ) ;   
    }
  }
  else for( int i = 0 ; i < 3 ; i++ ) // SPLIT 3
  {
    // IF this component is active:
    if( txColorAtIntn.e[i] > 0 )
    {
      Vector txP ;
      txP.e[i] = txColorAtIntn.e[i];// only use the color band this diffraction reps.
      Ray refractedRay = ray.refract( intn->normal, i, toEta.e[i], intn->point, ray.power*txP ) ;
      if( traceType!=Whitted && intn->shape->material.transmissiveJitter )
        refractedRay.direction.jitter( intn->shape->material.transmissiveJitter ) ;
      transmittedColor += cast( refractedRay, scene ) ;   
    }
  }
  #pragma endregion

  // The color of the fragment is the sum of contributions
  // final result
  Vector finalColor = ray.power * (
      emittedColor +
      diffuseColor +
      specularColor +
      transmittedColor
    ) ;

  #define NOFOG 1
  #if NOFOG
  // NO FOG
  DESTROY( intn ) ;  // Now we don't need intn.
  return finalColor ;
  #else
  // YES FOG
  // If fog is being applied, you need the distance to this fragment.
  real dist = distanceBetween(ray.startPos, intn->point) ;
  //finalColor.w = dist ; // DON'T PUT DISTANCE INFO IN w!  WHY?
  // you could fog blend/participating media here.  This would also
  // add fog blending to interreflections, which would be cool.

  Fragment f( finalColor, intn->normal, dist, ray.bounceNum, false ) ;
  DESTROY( intn ) ;  // Now we don't need intn.
  return f.foggedColor( FogMode::FogLinear, Vector(1,1,1,1), 0.5, 500 ) ;
  #endif
}


/// THIS IS INCOMPLETE
Vector RaytracingCore::brdfCast( Ray& ray, Scene *scene )
{
  warning( "BRDFCast incomplete, go work on it" ) ;

  if( ray.bounceNum > maxBounces )  return 0 ;

  // you take the ray, and the surface it hits, and use the brdf
  // to determine (with probability) the output power and direction.

  // there is a possibility of complete absorption, (black). Roulette.

  Intersection intn ;
  
  if( !scene->getClosestIntnExact( ray, &intn ) )
  {
    Vector finalColor ;
    
    if( cubeMap )
    {
      SVector raySphDir = ray.direction.toSpherical() ;
      finalColor = ray.power * cubeMap->getColor( raySphDir.tElevation, raySphDir.pAzimuth, ColorIndex::Emissive ) ;
    }
    else if( scene->perlinSkyOn ) // perlin sky
    {
      // sample the perlin sky!
      finalColor = ray.power*scene->perlin.sky( ray.direction.x, ray.direction.y, ray.direction.z,
                         PerlinGenerator::SKY_BLUE,
                         PerlinGenerator::CLOUD_WHITE ) ;
    }
    else
      finalColor = scene->bgColor ; // use the lame bg color

    return finalColor ;  // if this was a "total miss" 
    // (missed all scene geometry), then w is 0 and 
    // we know to leave it out of the averaging.
  }

  // Here we hit an object.
  Vector finalColor ;

  // if the object emits light, take it
  finalColor += intn.getColor( ColorIndex::Emissive ) ;

  // reflect in arbitrary directions, weigh by brdf.

  // (specular dir weighs high, etc).

  // The OUTPUT direction is actually the REVERSE of
  // the direction vector we shot.
  Vector out = -ray.direction ;

  // the INPUT direction is the random direction we are gathering from.
  
  // choose a start pos
  int soff = randInt( 0, rc->n ) ;
  
  for( int i = 0 ; i < raysDistributed ; i++ )
  {
    Vector& in = rc->vAddr( soff+i ) ;
    
    // I'll make a BRDF up.  Say it's a strange material that 
    // reflects light specularly along a circle

    if( in % intn.normal < 0 ) continue ; // skip directions that are obtuse to the normal

    // Get the angles
    real tOut = out.angleWithNormalized( intn.normal ) ;
    real tIn = in.angleWithNormalized( intn.normal ) ;

    // Now the closer they are, the stronger the refln.
    real reflStr = 1.0 - fabs( tOut-tIn ) ;

    // let it also vary with space position, in X
    real sMult = 1+sin( 40*intn.point.x ) ;
    reflStr *= sMult ;

    // cast again, in new direction,
    Ray newRay( intn.point+intn.normal*EPS_MIN, in, 1000, ray.eta, ray.power, ray.bounceNum+1 ) ;

    // Cast again, looking for the base case to return a final color.
    finalColor += brdfCast( newRay, scene ) ;
  }
  
  return finalColor/raysDistributed ;
}

// traces a rectangle of pixels
void RaytracingCore::traceRectangle( int startRow, int endRow, int startCol, int endCol, Scene * scene )
{
  // for each pixel, fill the frame buffer
  for( int row = startRow ; row < endRow ; row++ )
  {
    for( int col = startCol ; col < endCol ; col++ )
    {
      Vector pxColor(0) ;
      int idx = row*cols+col ;

      #if 0
      // RTSH
      // right now this will bug out if
      // the sh projected light source is touched while rt'ing
      // (ie if you are still updating the SH object by rotating the
      // light source in the rasterize loop)
      Ray ra = viewingPlane->getRay( row, col, randFloat( -.5, .5 ), randFloat( -.5, .5 ) ) ;
      ra.eta = scene->mediaEta ;
      ra.power = 1 ;
      ra.bounceNum = 0 ;
      pxColor = castRTSH( ra, scene ) ;
      #else

      // try an adaptive method
      // first do RPP, then do more until you converge on a color.
      if( stratified )
      {
        // stratified sampling: jitter the rays evenly
        int nbins = sqrtRaysPerPixel * sqrtRaysPerPixel ;

        for( int c = 0 ; c < raysPerPixel ; c++ )
        {
          // subrow and subcol are the mini bins within the pixel
          int subrow,subcol ;
          
          if( c < nbins )
          {
            // cast nbins rays at least 1 per bin
            subrow = c / sqrtRaysPerPixel ;
            subcol = c % sqrtRaysPerPixel ;
          }
          else // for nbins to rpp
          {
            // cast the rest in random bins
            subrow = rand() % sqrtRaysPerPixel ;
            subcol = rand() % sqrtRaysPerPixel ;
          }

          Ray r = viewingPlane->getRay( row, col,
            randFloat( -1 + subrow*(2./sqrtRaysPerPixel), -1 + (subrow+1)*(2./sqrtRaysPerPixel) ),
            randFloat( -1 + subcol*(2./sqrtRaysPerPixel), -1 + (subcol+1)*(2./sqrtRaysPerPixel) )
          ) ;

          ///window->addDebugRayLock( r, Vector(1,0,0), Vector(1,1,1) ) ;
          r.eta = scene->mediaEta ;
          r.power = 1 ;
          r.bounceNum = 0 ;

          // RETURN COLORS FROM CAST AND THEN STORE THEM (OR NOT)
          // cast a full-juice ray on it's 0th bounce
          pxColor.AddMe4( cast( r, scene ) ) ;
          //pxColor += cast( r, scene ) ;
          ////pxColor += brdfCast( r, scene ) ;
        }
      }
      else
      {
        // NOT stratified
        // jitters randomly
        for( int ray = 0 ; ray < raysPerPixel ; ray++ )
        {
          // cast a full-juice ray on it's 0th bounce
          Ray r = viewingPlane->getRay( row, col, randFloat( -.5, .5 ), randFloat( -.5, .5 ) ) ;
          ///////window->addDebugRayLock( r, Vector(1,0,0), Vector(1,1,1) ) ;
          r.eta = scene->mediaEta ;
          r.power = 1 ;
          r.bounceNum = 0 ;
          //pxColor += cast( r, scene ) ;
          pxColor.AddMe4( cast( r, scene ) ) ;
          ////pxColor += brdfCast( r, scene ) ;
        }
      }
      #endif

      //pxColor /= raysPerPixel ; // 
      pxColor.DivMe4( raysPerPixel ) ;
      frameBuffer->colors[ idx ] = pxColor ;

    }//for col
  }//for row

  doneSingleJob() ; // call done to increment completed threads count
}

void RaytracingCore::doneSingleJob()
{
  numJobsDone++ ;
  
  if( numJobs == numJobsDone )
    doneCompletely() ;
}

void RaytracingCore::doneCompletely()
{
  // COMPLETELY done here.
  info( Green, "Done RT, %.2f seconds", window->timer.getTime() ) ;
  window->programState = ProgramState::Idle ;
  DESTROY( cubeMap ) ; //!!BUG kill this now
}

// actual raytracing algorithm
void RaytracingCore::raytrace( const Vector& eye, const Vector& look, const Vector& up, Scene * scene )
{
  clear( Vector( .5,.5,.5,.25 ) ) ;

  info( "Tracing stratified=%s "
    "rpp=%d, rpl=%d, "
    "raysmonte=%d, "
    "maxBounces=%d, term energy=%f",
    stratified?"true":"false",
    raysPerPixel, raysDistributed,
    raysDistributed,
    maxBounces, interreflectionThreshold² ) ;

  viewingPlane->persp( RADIANS(45), (real)cols / rows, 1, 1000 ) ;
  viewingPlane->orient( eye, look, up ) ;

  // Show the 
  //window->addDebugLineLock( viewingPlane->a, Vector(1,0,0), viewingPlane->b, Vector(0,0,1) ) ;
  //window->addDebugLineLock( viewingPlane->b, Vector(0,0,1), viewingPlane->c, Vector(1,1,0) ) ;
  //window->addDebugLineLock( viewingPlane->c, Vector(1,1,0), viewingPlane->d, Vector(0,1,0) ) ;
  //window->addDebugLineLock( viewingPlane->d, Vector(0,1,0), viewingPlane->a, Vector(1,0,0) ) ;

  // spin off several jobs to do this
  numJobs=numJobsDone=0;

  // last chance to check valid state
  if( traceType != Whitted && raysDistributed == 0 )
  {
    error( "Doing a distributed or path trace, but dist rays=0, setting=1" ) ;
    raysDistributed = 1 ;
  }

  // creates tiles
  /*
  int n = 5 ;
  numJobs = n*n ;
  numJobsDone = 0;
  for( int i = 0 ; i < n ; i++ )
    for( int j = 0 ; j < n ; j++ )
      threadPool.createJob(
        "rt tile", 
        new CallbackObject5<RaytracingCore*, void (RaytracingCore::*)( int, int, int, int, const Scene * ), // This is readable, right?
        int,int,int,int,const r33Scene*>( this, &RaytracingCore::traceRectangle,
        i*rows/n, j*cols/n, (i+1)*rows/n, (j+1)*cols/n, scene )
      ) ;
  */

  // creates strips
  
  // first count how many there will be (or make a batch)
  // the reason you have to do this is actually BETWEEN
  // submission of first job, and
  //   BEFORE the next job is queued,
  //   the first job can COMPLETE
  // meaning the raytracer thinks it's completely done
  // raytracing, which causes this false "ok all done"
  // response. (condition to be all done is numJobs==numJobsDone)


  // If you're planning to raytrace,
  // and there's a cubemap, make sure it's uncompressed.
  CubeMap* cm = scene->getActiveCubemap() ;
  cubeMap = 0 ;

  if( cm ) // validity
  {
    if( !cm->colorValues )
      info( Magenta, "RT: Had a cubemap, but it has no values! I can't sample it, forcing cubemap off." ) ;
    else if( !cm->isActive ) // activity
      info( Magenta, "RT: Cubemap inactive, not using" ) ;
    else
    {
      // make a clone and give it to the ray tracer, destroyed at end of RT run or beginnign of next
      cubeMap = new CubeMap( *cm ) ; // clone
      // rotate it according to window
      cubeMap->rotate( window->rotationMatrix ) ;
      cubeMap->setSource( cubeMap->rotatedColorValues ) ; // use the rotated color values
    }
  }
  
  if( !cubeMap )
  {
    info( Magenta, "Not using cubemap" ) ;
    window->rtCore->raysCubeMapLighting = 0 ;
  }

  // If there are FAR LIGHTS, but no LIGHTS, then
  // it means you forgot to add them to th elights collection
  if( scene->farLights.size() )
  {
    warning( "You're tracing a scene with FAR LIGHTS "
      "but the FAR LIGHTS won't be seen by the ray tracer." );

  }

  window->timer.reset() ;

  for( int row = 0 ; row < rows ; row+=2 )
    numJobs++ ;

  ParallelBatch *rtBatch = new ParallelBatch( "raytracing", NULL ) ;
  
  for( int row = 0 ; row < rows ; row+=2 )
  {
    //for( int col = 0 ; col < cols ; col++ )
    {
      // now create them
      Callback *cb = new CallbackObject5<RaytracingCore*, void (RaytracingCore::*)( int, int, int, int, Scene * ), // This is readable, right?
        int,int,int,int,Scene*>( this, &RaytracingCore::traceRectangle,
        
        row, row+2,  // 2 rows
        
        0, cols,     // all cols
        
        scene ) ;

      rtBatch->addJob( cb ) ;
      // threadPool.createJob( "rt tile", cb ) ; // don't add immediately
    }
  }
  
  threadPool.addBatch( rtBatch ) ;
  
}

void RaytracingCore::copyToSurface( D3D11Surface * surf )
{
  if( !surf )
  {
    error( "RTCORE cannot copy to NULL surface" ) ;
    return ;
  }

  BYTE* pBits = surf->open() ;
  
  ///////if( !pBits )
  ///////  error( "RTCORE could not open surface for writing" ) ;
  ///////else
  ///////{
  ///////  ///memcpy( pBits, &pixels[0], 4*getNumPixels() ) ;   // HYPERILLEGAL IN D3D11!! RowPitch=64 bytes.
  ///////  for( int row = 0 ; row < rows ; row++ )
  ///////  {
  ///////    for( int col = 0 ; col < cols ; col++ )
  ///////    {
  ///////      int index = row*cols + col ;
  ///////
  ///////      UINT rowStart = row * 64 ;  // WRONG!! I THINK. RowPitch is the number of bytes in a block of 4x4 texels. (ie 64 bytes here)
  ///////      UINT colStart = col * 4;
  ///////      int i1 = rowStart + colStart ;
  ///////      ((UINT*)pBits)[ i1 ] = pixels[ index ].intValue ;
  ///////    }
  ///////  }
  ///////}
  
  surf->close() ;
}

