#include "Raycaster.h"
#include "RaytracingCore.h"
#include "RadiosityCore.h"
#include "VizFunc.h"
#include "../scene/Scene.h"
#include "../window/GTPWindow.h"
#include "../threading/ParallelizableBatch.h"
#include "FullCubeRenderer.h"
#include "../math/SHSample.h"
#include "../math/SHVector.h"
#include "../geometry/Intersection.h"
#include "../geometry/Mesh.h"

RayCollection* Raycaster::rc ;

/// 5000 rays is pretty good
Raycaster::Raycaster( int iNumRays )
{
  setNumRays( iNumRays ) ;
  shTex = 0 ;
}

void Raycaster::setNumRays( int iNumRays )
{
  numRaysToUse = iNumRays ;

  if( numRaysToUse > rc->n )
  {
    warning( "Trying to use more rays than rays available, clamped" ) ;
    numRaysToUse = rc->n ;
  }

  // solid angle of each ray I shoot:
  // The integral of the hemispherical cosine lobe is PI.
  // Each ray should cover an AVERAGE solid angle of 4*PI/numRays,
  solidAngleRay = 4.0*PI / numRaysToUse ;

  // but the integral of the cosine lobe over the
  // hemisphere 
  //   > int( int( cos(t)*sin(t), t=0..PI/2, p=0..PI ) )   => PI,
  // 
  // So the sum of the (NORMAL DOT DIRECTION)
  // integrate the cosine lobe,
  // and that integral will be PI, not 1.0.
  
  // Divide the worth of each sample PI 
  // to normalize the integral.

  ///dotScaleFactor = (4.0*PI / numRaysToUse) / PI ; // this is what it is
  dotScaleFactor = 4.0 / numRaysToUse ; // just simplify it
}

void Raycaster::QAO( ParallelBatch* pb1, Scene *scene, int vertsPerJob )
{
  info( "Running AO using %d rays", numRaysToUse ) ;

  for( int i = 0 ; i < scene->shapes.size() ; i++ )
  {
    for( int j = 0 ; j < scene->shapes[i]->meshGroup->meshes.size() ; j++ )
    {
      // Now enqueue jobs
      Mesh *mesh = scene->shapes[i]->meshGroup->meshes[ j ];
      for( int k = 0 ; k < mesh->verts.size() ; k += vertsPerJob )
      {
        int endPt = k+vertsPerJob ;
        clamp<int>( endPt, 0, mesh->verts.size() ) ;
      
        // call shProject_Stage1 on this vertex range.
        pb1->addJob( new CallbackObject3< Raycaster*, void(Raycaster::*)(vector<AllVertex>*,int,int), vector<AllVertex>*,int,int >
          ( this, &Raycaster::ambientOcclusion, &mesh->verts, k, endPt ) ) ;
      }
    } // foreach mesh in meshgroup
  }
}

void Raycaster::QSHPrecomputation( ParallelBatch* pb1, ParallelBatch* pb2, Scene *scene, int vertsPerJob )
{
  // PRE-ALLOCATE THE RAM
  // The reason we pre-allocate the ram is if you're trying
  // to do a scene that's too big to fit in memory, it fails right away
  // (instead of mid-run).

  for( int i = 0 ; i < scene->shapes.size() ; i++ )
  {
    for( int j = 0 ; j < scene->shapes[i]->meshGroup->meshes.size() ; j++ )
    {
      // Now enqueue jobs
      Mesh *mesh = scene->shapes[i]->meshGroup->meshes[ j ];
      for( int k = 0 ; k < mesh->verts.size() ; k++ )
      {
        AllVertex& vertex = mesh->verts[k] ;

        // if material diffuse
        vertex.shDiffuseAmbient = new SHVector() ;
        
        // we only need this if the material is specular
        vertex.shSpecularMatrix = new SHMatrix() ;

        if( window->doInterreflect )
        {
          // we only need these if more than 1 pass
          vertex.shDiffuseInterreflected = new SHVector() ;
          vertex.shDiffuseSummed = new SHVector() ;

          // if specular
          vertex.shSpecularMatrixInterreflected = new SHMatrix() ;
        }
      }
    }
  }



  for( int i = 0 ; i < scene->shapes.size() ; i++ )
  {
    for( int j = 0 ; j < scene->shapes[i]->meshGroup->meshes.size() ; j++ )
    {
      // Now enqueue jobs
      Mesh *mesh = scene->shapes[i]->meshGroup->meshes[ j ];
      for( int k = 0 ; k < mesh->verts.size() ; k += vertsPerJob )
      {
        int endPt = k+vertsPerJob ;
        clamp<int>( endPt, 0, mesh->verts.size() ) ;
      
        // call shProject_Stage1 on this vertex range.
        pb1->addJob( new CallbackObject3< Raycaster*, void(Raycaster::*)(vector<AllVertex>*,int,int), vector<AllVertex>*,int,int >
          ( this, &Raycaster::shPrecomputation_Stage1_Ambient, &mesh->verts, k, endPt ) ) ;

        if( window->doInterreflect )
          pb2->addJob( new CallbackObject3< Raycaster*, void(Raycaster::*)(vector<AllVertex>*,int,int), vector<AllVertex>*,int,int >
            ( this, &Raycaster::shPrecomputation_Stage2_Interreflection, &mesh->verts, k, endPt ) ) ;
      }
    } // foreach mesh in meshgroup
  }
}

void Raycaster::QSHPrecomputation_Stage2_Interreflection_Only( ParallelBatch* pb, Scene *scene, int vertsPerJob )
{
  for( int i = 0 ; i < scene->shapes.size() ; i++ )
  {
    for( int j = 0 ; j < scene->shapes[i]->meshGroup->meshes.size() ; j++ )
    {
      // Now enqueue jobs
      Mesh *mesh = scene->shapes[i]->meshGroup->meshes[ j ];
      int vertsInMesh = mesh->verts.size() ;

      for( int i = 0 ; i < mesh->verts.size() ; i += vertsPerJob )
      {
        int endPt = i+vertsPerJob ;
        clamp<int>( endPt, 0, mesh->verts.size() ) ;
      
        pb->addJob( new CallbackObject3< Raycaster*, void(Raycaster::*)(vector<AllVertex>*,int,int), vector<AllVertex>*,int,int >
          ( this, &Raycaster::shPrecomputation_Stage2_Interreflection, &mesh->verts, i, endPt ) ) ;
      }
    } // foreach mesh in meshgroup
  }
}

void Raycaster::QWavelet_Stage2_Interreflection_Only( ParallelBatch* pb, Scene *scene, int vertsPerJob ) 
{
  for( int i = 0 ; i < scene->shapes.size() ; i++ )
  {
    for( int j = 0 ; j < scene->shapes[i]->meshGroup->meshes.size() ; j++ )
    {
      // Now enqueue jobs
      Mesh *mesh = scene->shapes[i]->meshGroup->meshes[ j ];
      int vertsInMesh = mesh->verts.size() ;

      for( int i = 0 ; i < mesh->verts.size() ; i += vertsPerJob )
      {
        int endPt = i+vertsPerJob ;
        clamp<int>( endPt, 0, mesh->verts.size() ) ;
      
        pb->addJob( new CallbackObject3< Raycaster*, void(Raycaster::*)(vector<AllVertex>*,int,int), vector<AllVertex>*,int,int >
          ( this, &Raycaster::wavelet_Stage2_Interreflection, &mesh->verts, i, endPt ) ) ;
      }
    } // foreach mesh in meshgroup
  }
}

void Raycaster::createSHTexcoords( Scene * scene )
{
  // 1024 means 1M vectors, 2048 means 4M vectors, 4096 means 16M vectors, 8192 means 67M vectors
  // I need (bands*bands) coefficients per vertex
  int verts = scene->countVertices() ;

  // r,g,b   *   #verts  *  bands^2
  int shCoeffsReqd = 3 * verts * window->shSamps->bands*window->shSamps->bands ;
  int shTexSize = NextPow2( sqrt(shCoeffsReqd) ) ;

  info( "SH DIFFUSE requires %d coeffs total, allocating texture of size %dx%d", shCoeffsReqd, shTexSize,shTexSize ) ;

  Texcoording ctex( shTexSize ) ; 

  shTex = new D3D11Surface( window, 0,0, ctex.n,ctex.n, 0,1,
    0, SurfaceType::ColorBuffer, 3,// SLOT 3 because that one is the correct format (read as float3's)
    DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 12 ) ;

  shTex->open() ;

  for( int i = 0 ; i < scene->shapes.size() ; i++ )
  {
    for( int j = 0 ; j < scene->shapes[i]->meshGroup->meshes.size() ; j++ )
    {
      // Now enqueue jobs
      Mesh *mesh = scene->shapes[i]->meshGroup->meshes[ j ];
      
      for( int k = 0 ; k < mesh->verts.size() ; k++ )
      {
        writeSHTexcoords( ctex, mesh->verts[k] ) ;
      }

      mesh->updateVertexBuffer() ;
    } // foreach mesh in meshgroup
  }

  shTex->close() ;
}

void Raycaster::ambientOcclusion( vector<AllVertex>* verts, int startIndex, int endIndex )
{
  if( endIndex > verts->size() ) { error( "OOB error" ) ;  endIndex = verts->size() ; }

  for( int i = startIndex ; i < endIndex ; i++ )
    ambientOcclusion( (*verts)[ i ] ) ;
}

void Raycaster::ambientOcclusion( AllVertex& vertex )
{
  //real dotSum=0.0 ;

  // this is like fog occlusion.
  // if geometry further than 25 units, it DOESN'T block ambient.
  real CUTOFFDIST = 25.0 ;
  vertex.ambientOcclusion = 0 ; // start by assuming it's blocked
  
  int startRay = randInt( 0, rc->n ) ;
  for( int i = 0 ; i < numRaysToUse ; i++ )
  {
    int idx = (startRay + i)%rc->n ;
    Vector dir = rc->v( idx ) ;

    // sample must be +hemisphere
    // Want samples ONLY on the upper hemisphere (with the normal at the center)
    real dot = vertex.norm % dir ; // angle acute, sample in upper hemisphere (centered around normal)
    if( dot < 0 )  continue ; // skip this sample if angle with normal is obtuse
    //dotSum += dot ;

    // Check if the sample hits something
    Ray ray( vertex.pos + EPS_MIN * vertex.norm, dir, 1000, window->scene->mediaEta, 1, 0 ) ;
    MeshIntersection mi ;

    //real passPerc = 1.0 ; // weigh solid angle contrib by passPerc
    if( window->scene->getClosestIntnMesh( ray, 0 ) )
    {
      // blocked from this direction
      // check the distance
      //real dist = mi.getDistanceTo( vertex.pos ) ;
      //passPerc = max( ( ( CUTOFFDIST - dist ) / CUTOFFDIST ), 0.0 ) ; // 1 when dist=0, linear fall to 0 when dist=25
      //passPerc = 0 ;
    }
    else
    {
      vertex.ambientOcclusion += dot * dotScaleFactor ; // reduce the contribution of side-rays
    }  // totally open from this direction

    // "open" from this direction.
    
  }

  // I also write it here, which is where the shader accesses it.
  vertex.texcoord[ TexcoordIndex::VectorOccludersIndex ].w = vertex.ambientOcclusion ;
}

void Raycaster::vectorOccluders( vector<AllVertex>* verts, int startIndex, int endIndex )
{
  if( endIndex > verts->size() ) { error( "OOB error" ) ;  endIndex = verts->size() ; }

  for( int i = startIndex ; i < endIndex ; i++ )
    vectorOccluders( (*verts)[ i ] ) ;
}

void Raycaster::vectorOccluders( AllVertex& vertex )
{
  // cast rays in every which direction, looking for specular reflectors
  // taht are unblocked in the reflection dirctino, and diffuse surfaces
  // that send me light
  vertex.voData = new VectorOccludersData() ;
  //real ffSum = 0.0 ;

  int startRay = randInt( 0, rc->n ) ;
  for( int i = 0 ; i < numRaysToUse ; i++ )
  {
    // Grab ray and make sure it's valid
    int idx = (startRay + i)%rc->n ;
    Vector dir = rc->v( idx ) ;

    // sample must be +hemisphere
    // Want samples ONLY on the upper hemisphere (with the normal at the center)
    real dot = vertex.norm % dir ; // angle acute, sample in upper hemisphere (centered around normal)
    if( dot < 0 )  continue ; // skip this sample if angle with normal is obtuse

    Ray ray( vertex.pos + EPS_MIN * vertex.norm, dir, 1000, window->scene->mediaEta, 1, 0 ) ;
    MeshIntersection mi ;

    if( window->scene->getClosestIntnMesh( ray, &mi ) ) // hit something
    {
      // whadja hit
      Mesh *mesh = mi.getMesh() ;  //if( !mesh ) { error( "No mesh" ) ; }

      // whadda form factor
      real ff = dot * dotScaleFactor ;  // The form factor of the ray,
      //ffSum += ff ;

      real aoff = mi.getAO() * ff ;    // reduced by ambient occlusion at sender

      // if the normal is NOT OBTUSE WITH the casting ray,
      // you need to turn it around because it's physically impossible to hit the
      // "inside" of a surface _first_ with a ray casted towards that surface.
      if( dir % mi.normal > 0 )  mi.normal = - mi.normal ;

      // NORMAL AT POINT OF INTERSECTION
      Vector diffuseNormal = aoff * mi.normal ;
      Vector specularityNormal = ff * mi.normal ; 
      Vector causticDir ; // starts 0 (assuming no caustic)
      
      // COLORS AT POINT OF INTERSECTION
      Vector diffuseColor = mi.getColor( ColorIndex::DiffuseMaterial ) ;
      Vector specularColor = mi.getColor( ColorIndex::SpecularMaterial ) ;

      // Check that the reflection angle is open
      if( window->scene->getClosestIntnMesh( Ray( mi.point + EPS_MIN*mi.normal, dir.reflectedCopy( mi.normal ), 1000 ), 0 ) )
        specularityNormal = Vector(0,0,0) ; // BLOCKED, NO SPECULAR. zero it out.

      // Does this something refract the ray?
      // If so, keep refracting until hit free space (thus creating causticDir),
      // if you CAN'T hit free space by refracting, then this direction is blocked to refraction.
      Vector transColor = mi.getColor( ColorIndex::Transmissive ) ; 
      if( transColor.nonzero() )
      {
        // The surface transmits

        // Going into the shape.
        ray = ray.refract( mi.normal, mi.shape->material.eta.x, mi.point, transColor ) ;

        // the termination conditions are:
        //  - HIT FREE SPACE OR
        //  - BECOME 0 POWER
        //  - BOUNCE TOO MANY TIMES
        while( ray.bounceNum < window->rtCore->maxBounces &&
               ray.power.len2() > window->rtCore->interreflectionThreshold² )
        {
          // diffract the ray again.
          MeshIntersection causticIntn; 
          if( !window->scene->getClosestIntnMesh( ray, &causticIntn ) )
          {
            // You hit nothing, this is the caustic source direction
            // it is weighed down by
            // form factor that it originally came from
            causticDir = ff * ray.direction ; // the caustic is in the last direction of diffraction.
            transColor = ray.power ; // color remaining in ray after passing thru all that geometry
            break ; // done loop
          }
          
          // otherwise, you hit ANOTHER surface.
          // The refract dir goes from current eta to new eta,
          // if the ray exits the material then the rays face the same way (and it will be turned around automatically)
          // the only case I don't detect is if I hit the backside of a 1-ply surface (eg a leaf).  then the diffraction is backwards,
          // but there's no way to fix this unless you _mark_ a surface as being 1-ply (in which case the normal should always be
          // "against" the incoming ray)
          ray = ray.refract( causticIntn.normal, causticIntn.shape->material.eta.x,
            causticIntn.point, ray.power*causticIntn.getColor(ColorIndex::Transmissive) ) ;

          // if the loop conditions are broken then
          // causticDir remains 0.
        }

        // at this point you hit free space or the contribution is nothing
        // (too many diffractions or power=0)
      }


      // I need also to represent shadow color
      // it's 1-transColor, no need to store that.

      // see if there's already an entry in reflectors
      map< Mesh*,VOVectors >::iterator iter = vertex.voData->reflectors.find( mesh )  ;
      if( iter == vertex.voData->reflectors.end() )
      {
        VOVectors posNNormal ;
        
        // say a shadow comes from the DIRECTION the ray was cast.
        // Nothing to do with the sruface normal.
        posNNormal.shadowDir = ff * dir ; // MUCH BETTER / Sharper
        //posNNormal.normShadow = ff * mi.normal ;  // use the normal.
        
        // If the transColor was 0, no point in adding it.
        if( causticDir.nonzero() )
        {
          // Only add this if the causticDir ended up being nonzero.
          posNNormal.causticDir = causticDir ;
          posNNormal.colorTrans = transColor ;
        }
        
        posNNormal.normDiffuse = diffuseNormal ;
        posNNormal.normSpecular = specularityNormal ;
        
        posNNormal.colorDiffuse = diffuseColor ;
        posNNormal.colorSpecular = specularColor ;
        posNNormal.pos = mi.point ;
        posNNormal.pos.w = 1 ; // set the count at 1
        
        vertex.voData->reflectors.insert( make_pair( mesh, posNNormal ) ) ;
      }
      else
      {
        iter->second.shadowDir += ff * dir ; // MUCH BETTER / Sharper
        //iter->second.normShadow += ff * mi.normal ; 
        if( causticDir.nonzero() )
        {
          iter->second.causticDir += causticDir ;
          iter->second.colorTrans += transColor ;
        }
        iter->second.normDiffuse += diffuseNormal ; // length of the vector will be strength of interreflecn
        iter->second.normSpecular += specularityNormal ; // length of the vector will be strength of interreflecn
        
        iter->second.colorDiffuse += diffuseColor ;
        iter->second.colorSpecular += specularColor ;
        iter->second.pos += mi.point ;  // summed position
        iter->second.pos.w++; // increase the count
      }
    }

    //else { }  // open, but ao already computed on previous pass
  }

  //printf( "FFSUm : %f\n", ffSum ) ;

  // done all gathering.  divide
  for( map< Mesh*,VOVectors >::iterator iter = vertex.voData->reflectors.begin() ;
  iter != vertex.voData->reflectors.end() ; ++iter )
  {
    iter->second.pos /= iter->second.pos.w ;
    // Normals are NOT divided, they are scaled by ao and ff.
    iter->second.colorDiffuse /= iter->second.pos.w ;
    iter->second.colorSpecular /= iter->second.pos.w ;
    iter->second.colorTrans /= iter->second.pos.w ;
    iter->second.pos.w = 1;
  }

}


void Raycaster::shPrecomputation_Stage1_Ambient( vector<AllVertex>* verts, int startIndex, int endIndex )
{
  if( endIndex > verts->size() ) { error( "OOB error" ) ;  endIndex = verts->size() ; }

  for( int i = startIndex ; i < endIndex ; i++ )
    shPrecomputation_Stage1_Ambient( (*verts)[ i ] ) ;
}

void Raycaster::shPrecomputation_Stage1_Ambient( AllVertex& vertex )
{
  // Now we have a vertex + normal,
  // test each sample's direction against it:

  // we're finding out the directions this vertex
  // receives ambient light from.  Depending on
  // direction the light comes from, its attenuated
  // a bit based on its incident angle with the surface
  // normal at the vertex.  We approximate in SH.
  Vector& diffuseColorVertex = vertex.color[ ColorIndex::DiffuseMaterial ] ;
  Vector& specularColorVertex = vertex.color[ ColorIndex::SpecularMaterial ] ;
  
  int si = 0;//randInt( 0, window->shSamps->n ) ;//!! IF YOU CHANGE THIS THEN YOU HAVE TO AVERAGE THEM AFTERWARD
  for( int k = 0 ; k < window->shSamps->nU ; k++ ) // walk thru samples..
  {
    SHSample& samp = window->shSamps->samps[(si+k)%window->shSamps->n];
    Vector sampdir = samp.dir() ;

    // sample must be +hemisphere
    // Want samples ONLY on the upper hemisphere (with the normal at the center)
    real dot = vertex.norm % sampdir ; // angle acute, sample in upper hemisphere (centered around normal)
    if( dot < 0 )  continue ; // skip this sample if angle with normal is obtuse

    // Check if the sample hits something
    Ray* r1 = new Ray( vertex.pos + EPS_MIN * vertex.norm, sampdir, 1000, window->scene->mediaEta, 1, 0 ) ;
    MeshIntersection meshIntn ;
    
    list<Ray*> rays;
    rays.push_back( r1 ) ;

    // keep tracing until there are no more rays left to trace.
    // rays get added on specular/translucent re-scattering.
    while( rays.size() )
    {
      // pop a ray and trace it.
      Ray* ray = rays.front() ;
      rays.pop_front() ;

      if( !window->scene->getClosestIntnMesh( *ray, &meshIntn ) )
      {
        // hit free space
        if( ray->bounceNum == 0 ) // if it's the first bounce, use the original shsamp (no bounce)
        {
          if( diffuseColorVertex.nonzero() )
            vertex.shDiffuseAmbient->integrateSample( samp, dot, diffuseColorVertex ) ;  // (ray's power is= 1) when bounceNum=0

          // bounce 0 specular sh, if there is a specular component!
          if( specularColorVertex.nonzero() )
            vertex.shSpecularMatrix->integrateSample( samp, specularColorVertex ) ;//ray is full juice
        }
        else
        {
          // construct a new shsample for this reflected direction
          // ray->power changes the color bands this integrates into,
          // if it was a red diffracted ray, then you only get red power.
          SHSample shsamp2( window->shSamps->bands, ray->direction.toSpherical() ) ;
          if( diffuseColorVertex.nonzero() )
            vertex.shDiffuseAmbient->integrateSample( shsamp2, dot, ray->power*diffuseColorVertex ) ;
          if( specularColorVertex.nonzero() )
            vertex.shSpecularMatrix->integrateSample( shsamp2, ray->power*specularColorVertex ) ;
        }
      }
      else
      {
        // ray hit object.  BOUNCE or TRANSMIT.
        // which bounce is this?  we may not be able to bounce or transmit.
        // if the bounceNum is 0 and we allowed 0 bounces, THEN no bounced light occurs
        // if bounceNum is 0 and we allowed 1 bounce, then bouncing occurs (once).
        if( ray->bounceNum < window->rtCore->maxBounces &&
            ray->power.len2() > window->rtCore->interreflectionThreshold² )
        {
          // ray is good and bounceable
          Vector specColorAtIntn = meshIntn.getColor( ColorIndex::SpecularMaterial ) ;
          if( specColorAtIntn.nonzero() )
          {
            // VIEW INDEPENDENT SPECULAR.
            // there is a specular reflection that is view independent.  It is called
            // a "caustic."  It is when a specular surface acts as a light source,
            // and has a diffuse reflection.  Eg. mirror windows reflecting the sun onto the concrete.
            Ray *specRay = ray->reflectPtr( meshIntn.normal, meshIntn.point, specColorAtIntn ) ; // increments bounces, cuts power by specColorAtIntn
            rays.push_back( specRay ) ;
          }
        
          Vector transColorAtIntn = meshIntn.getColor( ColorIndex::Transmissive ) ;
          if( transColorAtIntn.nonzero() )
          {
            // there is transmission.  BEND.
            // trace a ray for each color band, unless the eta's are all the same
            // 
            if( meshIntn.shape->material.eta.allEqual() )
            {
              // ONE RAY
              Ray *refractedRay = ray->refractPtr( meshIntn.normal, meshIntn.shape->material.eta.x, meshIntn.point, transColorAtIntn ) ;
              rays.push_back( refractedRay ) ;
            }
            else for( int i = 0 ; i < 3 ; i++ )
            {
              // diffract ray on color band we're using
              if( ray->power.e[i] > 0 ) // if that ray has power!  ie if this is blue diffraction of a red ray, obviously will be 0
              {
                Ray *refractedRay = ray->refractPtr( meshIntn.normal, i,
                  meshIntn.shape->material.eta.e[i], meshIntn.point, transColorAtIntn ) ;
                rays.push_back( refractedRay ) ;
              }
            } // 3 band rays
          } // transmissive color
        } // ray is good (bounceable)
      } // no mesh was hit

      DESTROY( ray ) ;
    } // while( rays.size() )
  } // each shsample
    
  // Divide each sample's weight by the factor
  vertex.shDiffuseAmbient->scale( window->shSamps->dotScaleFactor ) ; // incorporates solidAngle+dot with normal
  vertex.shSpecularMatrix->scale( window->shSamps->solidAngleRay ) ; // specular interreflections
  // don't get weighted down by diffuse interreflection component
  
}

void Raycaster::shPrecomputation_Stage2_Interreflection( vector<AllVertex>* verts, int startIndex, int endIndex )
{
  for( int i = startIndex ; i < endIndex ; i++ )
    shPrecomputation_Stage2_Interreflection( (*verts)[ i ] ) ;
} 

void Raycaster::shPrecomputation_Stage2_Interreflection( AllVertex& vertex )
{
  // at each vertex, shoot rays and see what other polygons you hit.
  // work in their direct response into your response.
  // This is PRT.

  int si = 0;//randInt( 0, window->shSamps->n ) ;
  for( int j = 0 ; j < window->shSamps->nU ; j++ )
  {
    SHSample& samp = window->shSamps->samps[(si+j)%window->shSamps->n];
    Vector sampdir = samp.dir() ;

    // it has to be on the + hemisphere to be usable
    real dot = sampdir % vertex.norm ;
    if( dot < 0 )  continue ; // angle is obtuse, don't use it
    
    Ray ray( vertex.pos + EPS_MIN*vertex.norm, sampdir, 1000 ) ;
    
    // shoot the ray in this direction using the raytracer
    Shape* closestShape=0 ;
    MeshIntersection intn ;
        
    // Force use the mesh, never use exactShape intersection.
    // This is important because we need a mesh intersection only,
    // data is parked at the vertices.
    if( window->scene->getClosestIntnMesh( ray, &intn ) )
    {
      // Now that it hits some poly..
      // we know 2 things:
      //   1. VERTEX DOESN'T GET AMBIENT FROM THIS SAMPLE'S DIRECTION (already found in stage 1)
      //   2. BUT VERTEX GETS INTERREFLECTIONS FROM SHAPE HIT BY intn IN THIS DIRECTION
      // (You can get a speedup by "tagging" rays by what they hit in
      //  the first pass, but I didn't do that because it'd take quite a bit of memory overhead)

      // diffuse light has the same strength every dierction.

      // Use the barycentric coordinates of the intersection
      // to produce a NEW SH (temp) function AT THAT POINT, (weight the
      // sh function at the surrounding vertices).
      SHVector newsh ;
      // Get the 3 SH functions that will be blended,
      // FROM THE REMOTE MESH THAT YOU JUST HIT WITH THE FEELER RAY
      SHVector* shA = intn.getMesh()->verts[ intn.tri->iA ].shDiffuseAmbient->scaledCopy( intn.bary.x ) ;
      SHVector* shB = intn.getMesh()->verts[ intn.tri->iB ].shDiffuseAmbient->scaledCopy( intn.bary.y ) ;
      SHVector* shC = intn.getMesh()->verts[ intn.tri->iC ].shDiffuseAmbient->scaledCopy( intn.bary.z ) ;

      newsh.add( shA )->add( shB )->add( shC ) ;
      delete shA; delete shB; delete shC;
      // SCALE DOWN the rgb components by your ability 
      // (at local/sending mesh) to REFLECT LIGHT on each band.
      newsh.scale( dot * window->shSamps->dotScaleFactor * vertex.color[ ColorIndex::DiffuseMaterial ] ) ;
      
      /////window->scene->addVisualization( newsh.generateVisualization( 10, 10, intn->point, ColorBand::R ) ) ;
      /////X vertex.shDiffuseAmbient->add( &newsh ) ; // DO NOT do this, you can't add to the current proj now, 
      // because that will change the first pass results (which we don't want)

      // Use a separate variable, then add the interreflections
      // with the base shading afterwards
      vertex.shDiffuseInterreflected->add( &newsh ) ; // ADD IT, this is the contribution of only ONE ray.

      // Specular addition is expensive, so if the material isn't even specular,
      // can skip this part
      if( vertex.color[ ColorIndex::SpecularMaterial ].notAll( 0 ) )
      {
        SHMatrix* shMA = intn.getMesh()->verts[ intn.tri->iA ].shSpecularMatrix->scaledCopy( intn.bary.x ) ;
        SHMatrix* shMB = intn.getMesh()->verts[ intn.tri->iB ].shSpecularMatrix->scaledCopy( intn.bary.y ) ;
        SHMatrix* shMC = intn.getMesh()->verts[ intn.tri->iC ].shSpecularMatrix->scaledCopy( intn.bary.z ) ;
        SHMatrix newshm ;
        newshm.add( shMA )->add( shMB )->add( shMC ) ;
        delete shMA; delete shMB; delete shMC;
        newshm.scale( window->shSamps->solidAngleRay * vertex.color[ ColorIndex::SpecularMaterial ] ) ;
        vertex.shSpecularMatrixInterreflected->add( &newshm ) ;
      }
    } 
    // else { } // ray didn't hit any shapes
  } // end raycast

  /////info( "sh2::finished %d->%d of %s", startVertex, endVertex, shape->name.c_str() ) ;
}

void Raycaster::writeSHTexcoords( Texcoording & ctex, AllVertex& vertex )
{
  ctex.start() ;
  for( int i = 0 ; i < vertex.shDiffuseSummed->coeffsScale.size() ; i++ )
    shTex->setColorAt( ctex, vertex.shDiffuseSummed->coeffsScale[i] ) ;

  //for( int i = 0 ; i < vertex.shSpecularMatrix->coeff.size() ; i++ )
  //  shTex->setColorAt( ctex, vertex.shSpecularMatrix->coeff[i] ) ;
  
  vertex.texcoord[ TexcoordIndex::SHTexIndex ] = Vector(
    
    ctex.startingCol, ctex.startingRow,
    //ctex.accum, // #total elts
    vertex.shDiffuseSummed->coeffsScale.size(), //#diffuse elts
    vertex.shSpecularMatrix->coeff.size() //#spec elts (NOT IMPLEMENTED ON GPU YET)
    
  ) ;
}

void Raycaster::wavelet_Stage2_Interreflection( vector<AllVertex>* verts, int startIndex, int endIndex )
{
  for( int i = startIndex ; i < endIndex ; i++ )
    wavelet_Stage2_Interreflection( (*verts)[ i ] ) ;
}

void Raycaster::wavelet_Stage2_Interreflection( AllVertex& vertex )
{
  // at each vertex, shoot rays and see what other polygons you hit.
  int si = randInt( 0, rc->n ) ;
  for( int j = 0 ; j < window->shSamps->nU ; j++ )
  {
    Vector &sampdir = rc->vAddr( si+j ) ;

    real dot = sampdir % vertex.norm ;
    if( dot < 0 )  continue ; // angle is obtuse, don't use it

    // Here ray ok to use
    Ray ray( vertex.pos + EPS_MIN*vertex.norm, sampdir, 1000 ) ;
    
    // shoot the ray in this direction using the raytracer
    Shape* closestShape=0 ;
    MeshIntersection intn ;
        
    // Force use the mesh. This is important because we need a mesh intersection only
    if( window->scene->getClosestIntnMesh( ray, &intn ) )
    {
      // Now that it hits some poly..

      // see angle of ray's direction with ___point I hit's___ normal.
      real dot = - (ray.direction % intn.normal) ;

      if( dot > 0 ) // acceptable
      {
        // Use the barycentric coordinates of the intersection
        // to produce a NEW WAVELET (temp) function AT THAT POINT, (weight the
        // wavelet function at the surrounding vertices).
        Eigen::SparseVector<Vector> newWavelet ;

        #if 0
        // Get the 3 wavelet functions that will be blended,
        // FROM THE REMOTE MESH THAT YOU JUST HIT WITH THE FEELER RAY
        CubeMap* senderCubemap = intn.getMesh()->verts[ intn.tri->iA ].cubeMap ;
        
        // this doesn't work, though i think it should: (I hate eigen)
        //Eigen::SparseVector<Vector> waveletA = intn.getMesh()->verts[ intn.tri->iA ].cubeMap->sparseTransformedColor * ( intn.bary.x ) ;
        //Eigen::SparseVector<Vector> waveletB = intn.getMesh()->verts[ intn.tri->iB ].cubeMap->sparseTransformedColor * ( intn.bary.y ) ;
        //Eigen::SparseVector<Vector> waveletC = intn.getMesh()->verts[ intn.tri->iC ].cubeMap->sparseTransformedColor * ( intn.bary.z ) ;

        // SCALE DOWN the rgb components by your ability 
        // (at local/sending mesh) to REFLECT LIGHT on each band.
        Vector m = dot * window->shSamps->dotScaleFactor * vertex.color[ ColorIndex::DiffuseMaterial ] ;
        
        Eigen::SparseVector<Vector> waveletA, waveletB, waveletC ;
        for( Eigen::SparseVector<Vector>::InnerIterator it( senderCubemap->sparseTransformedColor ); it; ++it )
        {
          waveletA.insert( it.index() ) = m*intn.bary.x*it.value() ; //printf( "v1[ %d ] = %f\n", it.index(), it.value() ) ;
          waveletB.insert( it.index() ) = m*intn.bary.y*it.value() ;
          waveletC.insert( it.index() ) = m*intn.bary.z*it.value() ;
        }

        // that's the interreflection
        vertex.cubeMap->newSparseTransformedColor += (waveletA + waveletB + waveletC) ;
        #else
        CubeMap* scA = intn.getMesh()->verts[ intn.tri->iA ].cubeMap ;
        CubeMap* scB = intn.getMesh()->verts[ intn.tri->iB ].cubeMap ;
        CubeMap* scC = intn.getMesh()->verts[ intn.tri->iC ].cubeMap ;

        Vector m = dot * window->shSamps->dotScaleFactor * vertex.color[ ColorIndex::DiffuseMaterial ] ;
        
        Eigen::SparseVector<Vector> waveletA, waveletB, waveletC ;
        for( Eigen::SparseVector<Vector>::InnerIterator it( scA->sparseTransformedColor ); it; ++it )
          waveletA.insert( it.index() ) = m*intn.bary.x*it.value() ;

        for( Eigen::SparseVector<Vector>::InnerIterator it( scB->sparseTransformedColor ); it; ++it )
          waveletB.insert( it.index() ) = m*intn.bary.y*it.value() ;
         
        for( Eigen::SparseVector<Vector>::InnerIterator it( scC->sparseTransformedColor ); it; ++it )
          waveletC.insert( it.index() ) = m*intn.bary.z*it.value() ;

        // that's the interreflection
        vertex.cubeMap->newSparseTransformedColor += (waveletA + waveletB + waveletC) ;
        #endif
        
      } else { } // dot < 0
    } else { } // ray didn't even hit the aabb, so total miss
  } // end raycast

  /////info( "sh2::finished %d->%d of %s", startVertex, endVertex, shape->name.c_str() ) ;
}

void Raycaster::QFormFactors( ParallelBatch* pb, Scene *scene,
  vector<Triangle*>* iTris, int trisPerJob, Eigen::MatrixXf* FFs )
{
  info( "Running formfactor computation using %d rays", numRaysToUse ) ;
  
  tris = iTris ;
  
  // now we can spin off threads here.
  // cut it into groups of trisPerJob.
  for( int i = 0 ; i < tris->size() ; i+=trisPerJob )
  {
    int endPt = i+trisPerJob ;
    clamp<int>( endPt, 0, tris->size() ) ;

    // Create jobs of trisPerJob each.
    pb->addJob( new CallbackObject3< Raycaster*, void(Raycaster::*)(int,int, Eigen::MatrixXf*), int,int, Eigen::MatrixXf* >
        ( this, &Raycaster::formFactors, i, i+trisPerJob, FFs ) ) ;
  }

}

void Raycaster::QVectorOccluders( ParallelBatch* pb, Scene *scene, int vertsPerJob )
{
  info( "Running Vector Occluders using %d rays", numRaysToUse ) ;

  // set up the raycaster to use aoRays
  setNumRays( numRaysToUse ) ;

  for( int i = 0 ; i < scene->shapes.size() ; i++ )
  {
    for( int j = 0 ; j < scene->shapes[i]->meshGroup->meshes.size() ; j++ )
    {
      // Now enqueue jobs
      Mesh *mesh = scene->shapes[i]->meshGroup->meshes[ j ];
      for( int k = 0 ; k < mesh->verts.size() ; k += vertsPerJob )
      {
        int endPt = k+vertsPerJob ;
        clamp<int>( endPt, 0, mesh->verts.size() ) ;
      
        // call shProject_Stage1 on this vertex range.
        pb->addJob( new CallbackObject3< Raycaster*, void(Raycaster::*)(vector<AllVertex>*,int,int), vector<AllVertex>*,int,int >
          ( this, &Raycaster::vectorOccluders, &mesh->verts, k, endPt ) ) ;
      }
    } // foreach mesh in meshgroup
  }
}

void Raycaster::formFactors( int startIndex, int endIndex, Eigen::MatrixXf* FFs )
{
  if( startIndex > tris->size() ){ error( "OOB startIndex" ) ; return ; }
  if( endIndex > tris->size() ) { error( "OOB endIndex" ) ; endIndex = tris->size() ; } ;

  for( int i = startIndex ; i < endIndex ; i++ )
    formFactors( i, FFs ) ; // actually compute
}

// Finds the form factors for 1 tri.
void Raycaster::formFactors( int indexOfTri, Eigen::MatrixXf* FFs )
{
  Triangle *tri = (*tris)[ indexOfTri ] ;
  ///printf( "form factors tri %d            \r", tri->getId() ) ;
  // to find the formfactors for a triangle using raycasting,
  // situate yourself on the tri and just shoot rays, seeing what patches you hit (by id).

  // shoot rays,
  Vector eye = tri->getCentroid() + tri->normal*EPS_MIN ;  // start eye rays slightly off surface
  
  int row = tri->getId() ;
  //real dotSum = 0 ;

  int startRay = randInt( 0, rc->n ) ;
  for( int i = 0 ; i < numRaysToUse ; i++ )
  {
    int idx = (startRay + i)%rc->n ;
    Vector dir = rc->v( idx ) ;

    real dot = tri->normal % dir ;

    if( dot < 0 )  continue ; // this ray shoots into self
    //dotSum += dot ;

    Ray ray( eye, dir, 1000.0 ) ;

    MeshIntersection mi ;
    if( window->scene->getClosestIntnMesh( ray, &mi ) )
    {
      // column is id of patch that was hit
      int col = mi.tri->getId() ;
      
      // INCREMENT THAT PATCHES' RELATION WITH (this patch) BY
      // SOLIDANGLE SUBTENDED BY EACH RAY I SHOOT!
      (*FFs)( row, col ) += dot * dotScaleFactor ; //!! Is this right?
    }
    //else { } // the scene is not closed, so energy is lost.
  }
}



/*
void sphericalWavelet()
{
  // shoot rays around and see what's around you
  int startRay = randInt( 0, rc->n ) ;
  
  for( int i = 0 ; i < numRays ; i++ )
  {
    int idx = (startRay + i)%rc->n ;
    Vector dir = rc->v( idx ) ;
    real dot = dir % vertex.norm ;
    if( dot < 0 ) continue ; // discount this for now, but later you should accept light from behind, if it passes thru a translucent floor for example.
    // but if the floor is transparent, then the light doesn't show up on the surface though.  "backlighting" is really the opacity of the material
    // the light is passing thru.  you can't model it easily.

    Ray ray( vertex.pos, dir, 1000.0 ) ;

    // if that ray hits something in the scene,
    // then you are occluded in that direction.
    if( !scene->getClosestIntnExact( ray, NULL ) )
    {
      // it hit nothing, so integrate this
      viz->rayIndices->push_back( idx ) ;
      viz->rayWeights->push_back( dot ) ; // the strength in this direction depended on the normal.
    }
  }

}
*/