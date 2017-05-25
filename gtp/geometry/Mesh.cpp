#include "Mesh.h"
#include "../window/GTPWindow.h" 
#include "../Globals.h"
#include "../math/SHVector.h"

char* MeshTypeName[] ={
  "Indexed",
  "Nonindexed"
} ;

MeshType defaultMeshType = MeshType::Nonindexed ;
VertexType defaultVertexType = VertexType::VT10NC10 ;

// DOES NOT add the mesh as a child of the mesh!
Mesh::Mesh( Shape *iShape, MeshType iMeshType, VertexType iVertexType )
{
  vb = 0 ;
  aabb=0;
  
  shape = iShape ;
  meshType = iMeshType ;
  vertexType = iVertexType ;
}

Mesh::Mesh( const Mesh& mesh )
{
  error( "A Mesh is being copied BUT NOT REALLY!!" ) ;
}

Mesh::Mesh( const Mesh* mesh, Shape* newOwner )
{
  //error( "A Mesh is being copied BUT NOT REALLY!!" ) ;
  puts( "Mesh clone" ) ;
  printf( "Mesh from %s being copied, just thought you should know.", mesh->shape->name.c_str() ) ;

  world = mesh->world ;
  centroid = mesh->centroid ;

  meshType = mesh->meshType ;
  vertexType = mesh->vertexType ; 

  // These all get copied
  tris = mesh->tris ;
  verts = mesh->verts ;
  faces = mesh->faces ;

  vb=0;

  createVertexBuffer() ;
  
  shape = newOwner ;
  aabb = new AABB( shape ) ;
  
}

Mesh::Mesh( FILE* file, Shape* iShape )
{
  vb = 0 ;
  aabb=0;
  shape = iShape ;
  
  HeaderMesh head ;
  
  fread( &head, sizeof(HeaderMesh), 1, file ) ;
  meshType = (MeshType)head.meshType ;
  vertexType = (VertexType)head.vertexType ;
  
  
  // construct the tris
  // this is the simplest way to do it: feed the required
  // MESH* pointer at construction (below) and read each
  // Triangle with a FREAD (copy construct + read).
  // The other way to do it is to
  // FREAD into 3 vecs / indices at the same time, and
  // then construct a triangle one time (read + copy).
  // ((You cannot construct the triangle and FREAD
  // at the same time, due to how FREAD works - unless you write a ctor for Triangle that accepts a FILE*.))
  tris.resize( head.numTris, Triangle( Vector(), Vector(), Vector(), this ) ) ;
  for( int i = 0 ; i < head.numTris ; i++ )
    tris[i].load( file ) ; // reduce copy construction by NOT using a ctor

  verts.resize( head.numVerts ) ;
  for( int i = 0 ; i < verts.size() ; i++ )
    verts[i].load( file, head.shNumCoeffsPerVertex ) ;
  
  if( head.meshType == MeshType::Indexed )
  {
    faces.resize( head.numIndices );
    fread( &faces[0], sizeof(unsigned int), head.numIndices, file ) ;
  }

  createVertexBuffer() ; // you're going to have to create the vb here, once the verts are in
}

int Mesh::save( FILE *file )
{
  int written = 0 ;
  HeaderMesh head ;
  
  head.meshType = meshType ;
  head.vertexType = vertexType ;
  head.numTris = tris.size() ;
  head.numVerts = verts.size() ;
  head.numIndices = faces.size() ;

  if( !verts[0].shDiffuseAmbient )
    head.shNumCoeffsPerVertex = 0 ;
  else
    head.shNumCoeffsPerVertex = verts[0].shDiffuseAmbient->coeffsScale.size() ;

  if( head.numIndices == 0 && head.meshType == MeshType::Indexed )
    warning( "A mesh had 0 indices and used MeshType::Indexed!" ) ;

  int trisDataLen = head.numTris * ( 3*sizeof(Vector) + (3*sizeof(int)) ) ;  // TRIOWNER FIELD CAN BE SET UP AT RECON TIME
  int vertDataLen = head.numVerts * AllVertex::NumElementsTotal * sizeof(Vector) ;
  int coeffsDataLen = head.numVerts * head.shNumCoeffsPerVertex  // num coeffs
    * sizeof(real) //size per coeff
    *3 ; // 3 channels
  int indexDataLen = head.numIndices * sizeof( unsigned int ) ;

  // so this mesh will take up head+TOTALDATALEN bytes.
  head.dataLen = trisDataLen + vertDataLen + coeffsDataLen + indexDataLen ;
  
  
  
  // write the header
  written += sizeof(HeaderMesh)*fwrite( &head, sizeof(HeaderMesh), 1, file ) ;

  // write the tris
  for( int i = 0 ; i < tris.size() ; i++ )
    written += (3*sizeof(Vector)+3*sizeof(int)) * fwrite( &tris[i].a, (3*sizeof(Vector)+3*sizeof(int)), 1, file ) ;

  // write each vertex (by calling its method, so the coeffs get written)
  for( int i = 0 ; i < verts.size() ; i++ )
    written += verts[i].save( file ) ;

  // if there's an index buffer, write it too
  if( head.meshType == MeshType::Indexed )
    written += sizeof(unsigned int)*fwrite( &faces[0], sizeof(unsigned int), faces.size(), file ) ;
  
  if( written != sizeof( HeaderMesh ) + head.dataLen )
  {
    error( "The calculated data len (%d) != written datalen (%d)", sizeof(HeaderMesh)+head.dataLen, written ) ;
  }
  return written ;
}

////Mesh& Mesh::operator=( const Mesh& mesh )
////{
////  
////}

Mesh::~Mesh()
{
  // when a mesh dies, destroy the aabb and the vb
  DESTROY( aabb ) ;
  DESTROY( vb ) ;
}

int Mesh::planeSide( const Plane& plane )
{
  int side = plane.iSide( verts[0].pos ) ;
  
  for( int i = 1 ; i < verts.size() ; i++ )
    if( plane.iSide( verts[i].pos ) != side )
      return 0 ;

  return side ;
}

void Mesh::updateTrisFromVertsAndFaces()
{
  for( int i = 0 ; i < tris.size() ; i++ )
    tris[i].refresh() ;
}

void Mesh::transform( const Matrix& m, const Matrix& nT )
{
  for( int i = 0 ; i < verts.size() ; i++ )
  {
    verts[i].pos  = verts[i].pos * m ;
    verts[i].norm = transformNormal(verts[i].norm, nT).normalize() ;
  }

  // The tris need to be updated as well
  for( int i = 0 ; i < tris.size() ; i++ )
    tris[i].transform( m ) ;

  // Refresh the centroid in case of rotation.
  computeCentroid() ;
}

void Mesh::addVertex( const AllVertex& v )
{
  verts.push_back( v ) ;
}

void Mesh::addTri( const Vector& A, const Vector& B, const Vector& C )
{
  if( Plane::areCollinear( A, B, C ) )
  {
    //warning( "You tried to add a degenerate triangle, not adding" ) ;
    return ;
  }
  if( meshType == Indexed )
    addIndexedTri( A, B, C ) ;
  else
    addNonindexedTri( A, B, C ) ;
}

void Mesh::addTri( const AllVertex& A, const AllVertex& B, const AllVertex& C )
{
  if( Plane::areCollinear( A.pos, B.pos, C.pos ) || Triangle::isDegenerate( A.pos, B.pos, C.pos ) )
  {
    //warning( "You tried to add a degenerate triangle, not adding" ) ;
    //char b[255];
    //char *p=b;
    //p+=A.pos.printInto( p ) ; p+=B.pos.printInto( p ) ; p+=C.pos.printInto( p )  ;
    //warning( b ) ;
    return ;
  }
  if( meshType == Indexed )
    addIndexedTri( A, B, C ) ;
  else
    addNonindexedTri( A, B, C ) ;
}

void Mesh::addQuad( const Vector& A, const Vector& B, const Vector& C, const Vector& D )
{
  addTri( A, B, C ) ;
  addTri( A, C, D ) ;
}

void Mesh::addTessQuad( const Vector& A, const Vector& B, const Vector& C, const Vector& D, int uPatches, int vPatches )
{
  Vector uaxis = B - A ;
  Vector vaxis = D - A ;

  for( int iu = 0 ; iu < uPatches ; iu++ )
  {
    for( int iv = 0 ; iv < vPatches ; iv++ )
    {
      real u1 = (real)iu / uPatches ;
      real v1 = (real)iv / vPatches ;

      real u2 = ( iu + 1.0 ) / uPatches ;
      real v2 = ( iv + 1.0 ) / vPatches ;

      // u2  u1
      // p2  p1
      // B---A v1
      // | _/|
      // |/  |
      // C---D v2
      // p3  p4

      Vector p1 = A + u1*uaxis + v1*vaxis ;
      Vector p2 = A + u2*uaxis + v1*vaxis ;
      Vector p3 = A + u2*uaxis + v2*vaxis ;
      Vector p4 = A + u1*uaxis + v2*vaxis ;

      Triangle tri1( p1, p2, p3, this ) ;
      Triangle tri2( p1, p3, p4, this ) ;

      // Create AllVertex'es so that we can work in tex coord
      AllVertex vertex1( p1, tri1.normal, shape->material ) ;
      AllVertex vertex2( p2, tri1.normal, shape->material ) ;
      AllVertex vertex3( p3, tri1.normal, shape->material ) ;
      AllVertex vertex4( p4, tri2.normal, shape->material ) ;
      
      vertex1.texcoord[ TexcoordIndex::Decal ].x = u1 ;
      vertex1.texcoord[ TexcoordIndex::Decal ].y = v1 ;
      vertex2.texcoord[ TexcoordIndex::Decal ].x = u2 ;
      vertex2.texcoord[ TexcoordIndex::Decal ].y = v1 ;
      vertex3.texcoord[ TexcoordIndex::Decal ].x = u2 ;
      vertex3.texcoord[ TexcoordIndex::Decal ].y = v2 ;
      vertex4.texcoord[ TexcoordIndex::Decal ].x = u1 ;
      vertex4.texcoord[ TexcoordIndex::Decal ].y = v2 ;

      addTri( vertex1, vertex2, vertex3 ) ;
      addTri( vertex1, vertex3, vertex4 ) ;
    }
  }
}

void Mesh::addQuad( const AllVertex& A, const AllVertex& B, const AllVertex& C, const AllVertex& D )
{
  addTri( A, B, C ) ;
  addTri( A, C, D ) ;
}

void Mesh::addTessQuad( const AllVertex& A, const AllVertex& B, const AllVertex& C, const AllVertex& D, int uPatches, int vPatches )
{
  // scales all properties
  AllVertex uaxis = B - A ;
  AllVertex vaxis = D - A ;

  for( int iu = 0 ; iu < uPatches ; iu++ )
  {
    for( int iv = 0 ; iv < vPatches ; iv++ )
    {
      real u1 = (real)iu / uPatches ;
      real v1 = (real)iv / vPatches ;

      real u2 = ( iu + 1.0 ) / uPatches ;
      real v2 = ( iv + 1.0 ) / vPatches ;

      // u2  u1
      // p2  p1
      // B---A v1
      // | _/|
      // |/  |
      // C---D v2
      // p3  p4

      AllVertex p1 = A + u1*uaxis + v1*vaxis ;
      AllVertex p2 = A + u2*uaxis + v1*vaxis ;
      AllVertex p3 = A + u2*uaxis + v2*vaxis ;
      AllVertex p4 = A + u1*uaxis + v2*vaxis ;

      addTri( p1, p2, p3 ) ;
      addTri( p1, p3, p4 ) ;
    }
  }
}

void Mesh::setVertexBuffer( const vector<AllVertex>& sourceVerts )
{
  verts = sourceVerts ; // copy
}
  
// avoid the long vertex add times by manually indicating the
// INDICES of the vertices you want to add.  Used for when
// constructing triangles for an object from a vertex buffer YOU ALREADY HAVE SET
void Mesh::addTriByIndex( int ia, int ib, int ic )
{
  if( ia >= verts.size() )
    error( "index %d out of bounds", ia ) ;
  if( ib >= verts.size() )
    error( "index %d out of bounds", ib ) ;
  if( ic >= verts.size() )
    error( "index %d out of bounds", ic ) ;
  
  faces.push_back( ia ) ;
  faces.push_back( ib ) ;
  faces.push_back( ic ) ;
  tris.push_back( Triangle( verts[ia].pos, verts[ib].pos, verts[ic].pos, ia, ib, ic, this ) ) ;
}

void Mesh::addNonindexedTri( const Vector& A, const Vector& B, const Vector& C )
{
  Triangle tri( A, B, C, this ) ;
  
  AllVertex vA( A, tri.normal, shape->material ) ;
  AllVertex vB( B, tri.normal, shape->material ) ;
  AllVertex vC( C, tri.normal, shape->material ) ;

  addNonindexedTri( vA, vB, vC ) ;
}

void Mesh::addNonindexedTri( const AllVertex& A, const AllVertex& B, const AllVertex& C )
{
  addVertex( A ) ;
  addVertex( B ) ;
  addVertex( C ) ;

  // GEOMETRIC REP_N
  tris.push_back(
    Triangle( A.pos, B.pos, C.pos,
              verts.size() - 3, verts.size() - 2, verts.size() - 1, this )
  ) ;
}

// IF you're adding INDEXED TRIS then ids are
// assigned to the triangle faces, but 
// you can only raytrace for hemicube, NOT
// rasterize, because the sharing of vertices
// will not allow unique colors to adjacent faces
// (it will be blended).  proceeding anyway,
// this may yield an interesting result.
void Mesh::addIndexedTri( const Vector& A, const Vector& B, const Vector& C )
{
  // construct a tri.
  Triangle tri( A, B, C, this ) ; // use next available ID.
  AllVertex vA( A, tri.normal, shape->material ) ;
  AllVertex vB( B, tri.normal, shape->material ) ;
  AllVertex vC( C, tri.normal, shape->material ) ;
  addIndexedTri( vA, vB, vC ) ;  // IF THE VERTEX ALREADY EXISTS.. YOU WILL LOSE THE ID COLORS
}


//!! YOU CAN USE A TEMPORARY OCTREE TO SPEED THIS UP.
void Mesh::addIndexedTri( const AllVertex& A, const AllVertex& B, const AllVertex& C )
{
  int iA=-1, iB=-1, iC=-1 ; // indices of 3 vertices.
  
  //char b1[30], b2[30];
  // See if there's already a vertex "close enough"
  // to the triangle you're pushing in the vertex array
  for( int i = 0 ;
    i < verts.size() &&
    ( iA==-1||iB==-1||iC==-1 ) ; // done early condition is NONE are -1, condn to enter is AT LEAST ONE is -1
    i++ ) 
  {
    // check all 3 at the same time
    if( iA == -1 && verts[i].pos.Near( A.pos ) ) // A not found yet AND A near this vertex
    {
      iA = i ;
      //verts[i].pos.getVector().printInto( b1 ) ;
      //A.pos.getVector().printInto( b2 ) ;
      //info( "I found that A is near %s=%s", b1, b2 ) ;

      // AVERAGE THE NORMALS OF THE VERTEX THAT MATCHED
      // AND THE EXISTING VERTEX
      verts[i].norm = ( verts[i].norm + A.norm ).normalize() ;
    }
    if( iB == -1 && verts[i].pos.Near( B.pos ) )
    {
      iB = i ;
      //verts[i].pos.getVector().printInto( b1 ) ;
      //B.pos.getVector().printInto( b2 ) ;
      //info( "I found that B is near %s=%s", b1, b2 ) ;
      verts[i].norm = ( verts[i].norm + B.norm ).normalize() ;
    }
    if( iC == -1 && verts[i].pos.Near( C.pos ) )
    {
      iC = i ;
      //verts[i].pos.getVector().printInto( b1 ) ;
      //C.pos.getVector().printInto( b2 ) ;
      //info( "I found that C is near %s=%s", b1, b2 ) ;
      verts[i].norm = ( verts[i].norm + C.norm ).normalize() ;
    }
  }

  if( iA == -1 ) // iA is -1, so a suitable index for A was not found
  {
    // didn't find a near one, so add it and iA is the last vertex
    //A.pos.getVector().printInto( b1 ) ;
    //info( "Found a unique vertex %s", b1 ) ;
    
    addVertex( A ) ;
    iA = verts.size() - 1 ;
  }

  if( iB == -1 )
  {
    //B.pos.getVector().printInto( b1 ) ;
    //info( "Found a unique vertex %s", b1 ) ;

    addVertex( B ) ;
    iB = verts.size() - 1 ;
  }
  
  if( iC == -1 )
  {
    //C.pos.getVector().printInto( b1 ) ;
    //info( "Found a unique vertex %s", b1 ) ;

    addVertex( C ) ;
    iC = verts.size() - 1 ;
  }
  
  faces.push_back( iA ) ;
  faces.push_back( iB ) ;
  faces.push_back( iC ) ;

  // GEOMETRIC REP_N
  tris.push_back(
    Triangle( A.pos, B.pos, C.pos,
              iA,    iB,    iC, this )
  ) ;
}

void Mesh::addIndexedTri( const AllVertex& A, const AllVertex& B, const AllVertex& C,
                          int iA, int iB, int iC )
{
  faces.push_back( iA ) ;
  faces.push_back( iB ) ;
  faces.push_back( iC ) ;

  // GEOMETRIC REP_N
  tris.push_back(
    Triangle( A.pos, B.pos, C.pos,
              iA,    iB,    iC, this )
  ) ;
}

// call this after you've added all your tris
bool Mesh::createVertexBuffer()
{
  bool ret ;
  switch( vertexType )
  {
    case VertexType::PositionColor:
      ret = createVertexBufferVC() ;
      break ;

    case VertexType::PositionNormalColor:
      ret = createVertexBufferVNC() ;
      break ;

    case VertexType::PositionTextureColor:
      error( "This vertex type unsupported, making vnc.." ) ;
      ret = createVertexBufferVTC() ;
      break ;

    case VertexType::VTTTTNC:
      ret = createVertexBufferVTTTTNC() ;
      break ;

    case VertexType::VTTTTNCCCC:
      ret = createVertexBufferVTTTTNCCCC() ;
      break ;
    
    case VertexType::VT10NC10:
      ret = createVertexBufferVT10NC10() ;
      break ;

    default:
      warning( "Undefined vertex format %d, you get vttttncccc", vertexType ) ;
      ret = createVertexBufferVTTTTNCCCC() ;
      break ;
  }

  // Now that staged changes are complete,
  // compute the centroid.
  if( ret )
    computeCentroid() ;

  return ret ;
}

bool Mesh::createVertexBufferVC()
{
  DESTROY( vb ) ;  // if there already was one, destroy it.

  TVertexBuffer<Vertex> *tvb ;

  vector<Vertex> vertsOnly( verts.size() ) ;
  for( int i = 0 ; i < verts.size() ; i++ ) {
    vertsOnly[i].pos = verts[i].pos ;
  }

  // so uh, use faces only if in indexed mode
  if( meshType == MeshType::Indexed )
    tvb = new TVertexBuffer<Vertex>( window, &vertsOnly[0], vertsOnly.size(), &faces[0], faces.size() ) ;
  else
    tvb = new TVertexBuffer<Vertex>( window, &vertsOnly[0], vertsOnly.size() ) ;

  vb = tvb ;
  return true ;
}

bool Mesh::createVertexBufferVNC()
{
  DESTROY( vb ) ;  // if there already was one, destroy it.

  TVertexBuffer<VertexNC> *tvb ;

  if( !verts.size() )
  {
    error( "vnc no vertices to make a vertex buffer out of" ) ;
    return false ;
  }

  vector<VertexNC> vertsOnly( verts.size() ) ;
  for( int i = 0 ; i < verts.size() ; i++ ) {
    vertsOnly[i].pos = verts[i].pos ;
    vertsOnly[i].norm = verts[i].norm ;
    vertsOnly[i].color = verts[i].color[ ColorIndex::DiffuseMaterial ] ;
  }
  
  // so uh, use faces only if in indexed mode
  if( meshType == MeshType::Indexed )
    tvb = new TVertexBuffer<VertexNC>( window, &vertsOnly[0], vertsOnly.size(), &faces[0], faces.size() ) ;
  else
    tvb = new TVertexBuffer<VertexNC>( window, &vertsOnly[0], vertsOnly.size() ) ;

  vb = tvb ;
  return true ;
}

bool Mesh::createVertexBufferVTC()
{
  error( "Not implemented" ) ;
  return false ;
}

bool Mesh::createVertexBufferVTTTTNC()
{
  DESTROY( vb ) ;  // if there already was one, destroy it.

  TVertexBuffer<VertexTTTTNC> *tvb ;

  if( !verts.size() )
  {
    error( "vttttnc no vertices to make a vertex buffer out of" ) ;
    return false ;
  }

  vector<VertexTTTTNC> vertsOnly( verts.size() ) ;
  for( int i = 0 ; i < verts.size() ; i++ ) {
    vertsOnly[i].pos = verts[i].pos ;
    ///memcpy( &vertsOnly[i], &verts[i], 4*sizeof(Vector) ) ;
    vertsOnly[i].tex[0] = verts[i].texcoord[ 0 ] ;
    vertsOnly[i].tex[1] = verts[i].texcoord[ 1 ] ;
    vertsOnly[i].tex[2] = verts[i].texcoord[ 2 ] ;
    vertsOnly[i].tex[3] = verts[i].texcoord[ 3 ] ;
    vertsOnly[i].norm = verts[i].norm ;
    vertsOnly[i].color = verts[i].color[ 0 ] ;
  }
  
  // so uh, use faces only if in indexed mode
  if( meshType == MeshType::Indexed )
    tvb = new TVertexBuffer<VertexTTTTNC>( window, &vertsOnly[0], vertsOnly.size(), &faces[0], faces.size() ) ;
  else
    tvb = new TVertexBuffer<VertexTTTTNC>( window, &vertsOnly[0], vertsOnly.size() ) ;

  vb = tvb ;
  return true ;
}

bool Mesh::createVertexBufferVTTTTNCCCC()
{
  DESTROY( vb ) ;  // if there already was one, destroy it.

  TVertexBuffer<VertexTTTTNCCCC> *tvb ;

  if( !verts.size() )
  {
    error( "vttttncccc no vertices to make a vertex buffer out of" ) ;
    return false ;
  }

  vector<VertexTTTTNCCCC> vertsOnly( verts.size() ) ;
  for( int i = 0 ; i < verts.size() ; i++ ) {
    ///memcpy( &vertsOnly[i], &verts[i], 4*sizeof(Vector) ) ;
    vertsOnly[i].pos = verts[i].pos ;
    vertsOnly[i].tex[0] = verts[i].texcoord[ 0 ] ;
    vertsOnly[i].tex[1] = verts[i].texcoord[ 1 ] ;
    vertsOnly[i].tex[2] = verts[i].texcoord[ 2 ] ;
    vertsOnly[i].tex[3] = verts[i].texcoord[ 3 ] ;
    vertsOnly[i].norm = verts[i].norm ;
    vertsOnly[i].color[0] = verts[i].color[ 0 ] ;
    vertsOnly[i].color[1] = verts[i].color[ 1 ] ;
    vertsOnly[i].color[2] = verts[i].color[ 2 ] ;
    vertsOnly[i].color[3] = verts[i].color[ 3 ] ;
  }
  
  // so uh, use faces only if in indexed mode
  if( meshType == MeshType::Indexed )
    tvb = new TVertexBuffer<VertexTTTTNCCCC>( window, &vertsOnly[0], vertsOnly.size(), &faces[0], faces.size() ) ;
  else
    tvb = new TVertexBuffer<VertexTTTTNCCCC>( window, &vertsOnly[0], vertsOnly.size() ) ;

  vb = tvb ;
  return true ;
}

bool Mesh::createVertexBufferVT10NC10()
{
  DESTROY( vb ) ;  // if there already was one, destroy it.

  TVertexBuffer<VertexT10NC10> *tvb ;

  if( !verts.size() )
  {
    error( "vt10nc10 no vertices to make a vertex buffer out of" ) ;
    return false ;
  }

  vector<VertexT10NC10> vertsOnly( verts.size() ) ;
  for( int i = 0 ; i < verts.size() ; i++ ) {
    vertsOnly[i].pos = verts[i].pos ;
    for( int j = 0 ; j < 10 ; j++ ) {
      vertsOnly[i].tex[j] = verts[i].texcoord[j] ;
      vertsOnly[i].color[j] = verts[i].color[j] ;
    }
    vertsOnly[i].norm = verts[i].norm ;
  }
  
  // so uh, use faces only if in indexed mode
  if( meshType == MeshType::Indexed )
    tvb = new TVertexBuffer<VertexT10NC10>( window, &vertsOnly[0], vertsOnly.size(), &faces[0], faces.size() ) ;
  else
    tvb = new TVertexBuffer<VertexT10NC10>( window, &vertsOnly[0], vertsOnly.size() ) ;

  vb = tvb ;
  return true ;
}

void Mesh::updateVertexBuffer()
{
  if( !vb )
  {
    warning( "You tried to update vb but no vertexbuffer existed! Creating.." ) ;
    createVertexBuffer() ;
    return ;
  }
  switch( vertexType )
  {
    case VertexType::PositionColor:
      updateVertexBufferVC() ;
      break ;

    case VertexType::PositionNormalColor:
      updateVertexBufferVNC() ;
      break ;

    case VertexType::PositionTextureColor:
      error( "This vertex type unsupported, making vnc.." ) ;
      updateVertexBufferVTC() ;
      break ;

    case VertexType::VTTTTNC:
      updateVertexBufferVTTTTNC() ;
      break ;

    case VertexType::VTTTTNCCCC:
      updateVertexBufferVTTTTNCCCC() ;
      break ;
    
    case VertexType::VT10NC10:
      updateVertexBufferVT10NC10() ;
      break ;

    default:
      error( "Undefined vertex format %d, no update performed", vertexType ) ;
      //updateVertexBufferVTTTTNCCCC() ;
      break ;
  }
}

void Mesh::updateVertexBufferVC()
{
  TVertexBuffer<VertexC>* tvb = (TVertexBuffer<VertexC>*)vb ;
  VertexC* p = (VertexC*)tvb->lockVB() ;
  
  if( !p ){  error( "CANNOT LOCK VB; ALL YOUR VERTICES ARE INVALID" ) ; return ;  }
  for( int i = 0 ; i < verts.size() ; i++ )
  {
    p[i].pos = verts[i].pos ;
    p[i].color = verts[i].color[ 0 ] ;
  }
  
  tvb->unlockVB() ;
}

// you must update the entire buffer. because dynamic buffers
// must use D3D11_MAP_WRITE_DISCARD.
void Mesh::updateVertexBufferVNC()
{
  TVertexBuffer<VertexNC>* tvb = (TVertexBuffer<VertexNC>*)vb ;
  VertexNC* p = (VertexNC*)tvb->lockVB() ;
  
  if( !p ){  error( "CANNOT LOCK VB; ALL YOUR VERTICES ARE INVALID" ) ; return ;  }
  for( int i = 0 ; i < verts.size() ; i++ )
  {
    p[i].pos = verts[i].pos ;
    p[i].norm = verts[i].norm ;
    p[i].color = verts[i].color[ 0 ] ;
  }
  
  tvb->unlockVB() ;
}

void Mesh::updateVertexBufferVTC()
{
  TVertexBuffer<VertexTC>* tvb = (TVertexBuffer<VertexTC>*)vb ;
  VertexTC* p = (VertexTC*)tvb->lockVB() ;
  
  if( !p ){  error( "CANNOT LOCK VB; ALL YOUR VERTICES ARE INVALID" ) ; return ;  }
  for( int i = 0 ; i < verts.size() ; i++ )
  {
    p[i].pos = verts[i].pos ;
    p[i].tex = verts[i].texcoord[0] ;
    p[i].color = verts[i].color[ 0 ] ;
  }
  
  tvb->unlockVB() ;
}

// you must update the entire buffer. because dynamic buffers
// must use D3D11_MAP_WRITE_DISCARD.
void Mesh::updateVertexBufferVTTTTNC()
{
  TVertexBuffer<VertexTTTTNC>* tvb = (TVertexBuffer<VertexTTTTNC>*)vb ;
  VertexTTTTNC* p = (VertexTTTTNC*)tvb->lockVB() ;
  
  if( !p ){  error( "CANNOT LOCK VB; ALL YOUR VERTICES ARE INVALID" ) ; return ;  }
  for( int i = 0 ; i < verts.size() ; i++ )
  {
    p[i].pos = verts[i].pos ;
    p[i].tex[0] = verts[i].texcoord[ 0 ] ;
    p[i].tex[1] = verts[i].texcoord[ 1 ] ;
    p[i].tex[2] = verts[i].texcoord[ 2 ] ;
    p[i].tex[3] = verts[i].texcoord[ 3 ] ;
    p[i].norm = verts[i].norm ;
    p[i].color = verts[i].color[0] ;
  }
  
  tvb->unlockVB() ;
}

void Mesh::updateVertexBufferVTTTTNCCCC()
{
  TVertexBuffer<VertexTTTTNCCCC>* tvb = (TVertexBuffer<VertexTTTTNCCCC>*)vb ;
  VertexTTTTNCCCC* p = (VertexTTTTNCCCC*)tvb->lockVB() ;
  
  if( !p ){  error( "CANNOT LOCK VB; ALL YOUR VERTICES ARE INVALID" ) ; return ;  }
  for( int i = 0 ; i < verts.size() ; i++ )
  {
    p[i].pos = verts[i].pos ;
    p[i].tex[0] = verts[i].texcoord[ 0 ] ;
    p[i].tex[1] = verts[i].texcoord[ 1 ] ;
    p[i].tex[2] = verts[i].texcoord[ 2 ] ;
    p[i].tex[3] = verts[i].texcoord[ 3 ] ;
    p[i].norm = verts[i].norm ;
    p[i].color[0] = verts[i].color[0] ;
    p[i].color[1] = verts[i].color[1] ;
    p[i].color[2] = verts[i].color[2] ;
    p[i].color[3] = verts[i].color[3] ;
  }
  
  tvb->unlockVB() ;
}

void Mesh::updateVertexBufferVT10NC10()
{
  TVertexBuffer<VertexT10NC10>* tvb = (TVertexBuffer<VertexT10NC10>*)vb ;
  VertexT10NC10* p = (VertexT10NC10*)tvb->lockVB() ;
  
  if( !p ){  error( "CANNOT LOCK VB; ALL YOUR VERTICES ARE INVALID" ) ; return ;  }

  // Here you could split into a couple of different paths,
  // and only update the elements that the ACTIVE shader
  // requires
  for( int i = 0 ; i < verts.size() ; i++ )
  {
    p[i].pos = verts[i].pos ;
    for( int j = 0 ; j < 10 ; j++ ) {
      p[i].tex[j] = verts[i].texcoord[j] ;
      p[i].color[j] = verts[i].color[j] ;
    }
    p[i].norm = verts[i].norm ;
  }
  
  tvb->unlockVB() ;
}

void Mesh::setColor( ColorIndex colorIndex, const Vector& newColor )
{
  for( int i = 0 ; i < verts.size() ; i++ )
    verts[i].color[ colorIndex ] = newColor ;
}

void Mesh::setMaterial( const Material& iMaterial )
{
  for( int i = 0 ; i < verts.size() ; i++ )
    verts[i].setMaterial( iMaterial ) ;
}

bool Mesh::intersects( const Ray& ray )
{
  // he doesn't want detailed intersection information or
  // even the nearest polygon, just whether it HITS THE MESH or not.
  for( int i = 0 ; i < tris.size() ; i++ )
    if( tris[i].intersects( ray, NULL ) )
      return true ; // it hit a tri and detailed intn information was not requested
  return false ;    // it didn't hit any tris
}

// brute force intersection on a mesh, with no aux information
// about which tris in the mesh are candidates for the hit
bool Mesh::intersects( const Ray& ray, MeshIntersection* intn )
{
  if( !intn )  return intersects( ray );
  
  // go thru each Tri in the model, adding intersections as you find them
  MeshIntersection ni, ci=MeshIntersection::HugeMeshIntn ;
  for( int i = 0 ; i < tris.size() ; i++ )
    if( tris[i].intersects( ray, &ni ) )  // this tri intersects with the ray,
      if( ni.isCloserThan( &ci, ray.startPos ) )
        ci = ni ;  // the new one's closer
  
  if( intn )  *intn = ci ;
  return ci.didHit() ;
}

bool Mesh::intersectsFurthest( const Ray& ray, MeshIntersection* intn )
{
  // go thru each Tri in the model, adding intersections as you find them
  //!!! USE A LIST AND RETURN INTNS BACK TO FRONT.
  MeshIntersection ni, fi=MeshIntersection::SmallMeshIntn ;
  for( int i = 0 ; i < tris.size() ; i++ )
    if( tris[i].intersects( ray, &ni ) )  // this tri intersects with the ray,
      if( !ni.isCloserThan( &fi, ray.startPos ) )
        fi = ni ;  // the new one's NOT closer.. SO USE IT!!
  
  if( intn )  *intn = fi ;
  return fi.didHit() ;
}

void Mesh::computeCentroid()
{
  centroid = 0 ;
  for( int i = 0 ; i < verts.size() ; i++ )
    centroid += verts[i].pos ;
  centroid /= verts.size() ;
}

void Mesh::calculateNormals()
{
  // now, you need the adjacency to find the smoothed normals
  // as a rudimentary calc, just use the one already calculated
  
  if( meshType == MeshType::Indexed )
  {
    // indexed mesh is much easier.  for each vertex,
    // find the tris 
    
    // clear the normals
    for( int i = 0 ; i < verts.size() ; i++ )  verts[i].norm=0;

    for( int i = 0 ; i < tris.size() ; i++ )
    {
      verts[ tris[i].iA ].norm += tris[i].normal ;
      verts[ tris[i].iB ].norm += tris[i].normal ;
      verts[ tris[i].iC ].norm += tris[i].normal ;
    }

    // normalize it now
    for( int i = 0 ; i < verts.size() ; i++ )  verts[i].norm.normalize() ;
  }
  else
  {
    // find all the normals as just the face normals
    for( int i = 0 ; i < tris.size() ; i++ )
    {
      verts[ tris[i].iA ].norm = tris[i].normal ;
      verts[ tris[i].iB ].norm = tris[i].normal ;
      verts[ tris[i].iC ].norm = tris[i].normal ;
    }

    // now smooth

    // THis is very slow code, unnacceptably so for large models.
    // the modelling package should calculate the normals
    // and indexed meshes should be used as much as possible
    for( int i = 0 ; i < verts.size() ; i++ )
    {
      Vector avgNormal ;

      for( int j = 0 ; j < tris.size() ; j++ )
        if( tris[j].usesVertex( verts[i].pos ) )
          avgNormal += tris[j].normal ;

      verts[i].norm = avgNormal.normalize() ;
    }
  }
}

// You can just BUILD A NEW MODEL, and make this a SHAPE level tessellation.
void Mesh::tessellate( real maxArea )
{
  
  // Use the same format as this MESH
  Mesh *newMesh = new Mesh( this->shape,
    meshType,  // when tessellating, using indexing is a good idea.
    vertexType ) ;
  
  // the tris come from the existing collection of tris.
  for( int i = 0 ; i < tris.size() ; i++ )
    tris[i].recursivelySubdivideAndAddToMeshWhenSmallEnough( 0, 2000, maxArea, newMesh )  ; // you add them to the new mesh.
  
  // yeah but NEWMESH is referenced AS THE PARENT to these new tris.
  // later NEWMESH will be destroyed, so you need to actually .. update each tri
  // so it thinks THIS is its parent.
  for( int i = 0 ; i < newMesh->tris.size() ; i++ )
    newMesh->tris[i].meshOwner = this ; // re-assign the meshOwner
  
  // The MESHGROUP containing this mesh will be out of date now.
  // that's why its better to UPDATE `this` with the relevant
  // info, and NOT replace the members of this with the members of newMesh (which I was doing previously)
  this->verts = newMesh->verts ;
  
  DESTROY(vb); // ok. that's the only piece of the old mesh remaining
  
  (*this) = *newMesh ; // invokes operator=

  this->createVertexBuffer() ;

  DESTROY( newMesh ) ; // we don't need the auxiliary mesh anymore.
}

void Mesh::draw()
{
  if( !vb )
  {
    warning( "Mesh belonging to shape '%s' had no vertex buffer", shape->name.c_str() ) ;
    //return ; //makeVertexBuffer() ;
    createVertexBuffer() ;

    // if there is STILL no vb here, it means thta
    // vb creation falied (there were no verts)
    if( !vb )
    {
      error( "Tried to create vb but failed" ) ;
      return ;
    }
  }

  // each mesh must be able to set the texture they want to use
  for( int i = 0 ; i < texes.size() ; i++ )
    texes[i]->activate() ;

  // apply my world matrix
  window->setWorld( world ) ;

  vb->drawTris() ;

  // it's rude to leave your world set
  window->setWorld( Matrix() ) ;
}

// SETS DIFFUSE __AND__ EMISSIVE COLOR
// TO colorFunc.
void Mesh::visualizeCreateMesh(
  Shape* shapeToAddMeshTo,
  int slices, int stacks,
  MeshType iMeshType, VertexType iVertexType,
  const Vector& center,
  function<Vector (real tElevation, real pAzimuth)> colorFunc,
  function<real (real tElevation, real pAzimuth)> magFunc )
{
  Mesh *mesh = new Mesh( shapeToAddMeshTo, iMeshType, iVertexType ) ;

  for( int t = 0 ; t < stacks ; t++ ) // stacks are ELEVATION so they count theta
  {
    real theta1 = ( (real)(t)/stacks )*PI ;
    real theta2 = ( (real)(t+1)/stacks )*PI ;

    for( int p = 0 ; p < slices ; p++ ) // slices are ORANGE SLICES so the count azimuth
    {
      real phi1 = ( (real)(p)/slices )*2*PI ; // azimuth goes around 0 .. 2*PI
      real phi2 = ( (real)(p+1)/slices )*2*PI ;
      
      //phi2   phi1
      // |      |
      // 2------1 -- theta1
      // |\ _   |
      // |    \ |
      // 3------4 -- theta2
      //

      SVector sv1( magFunc( theta1, phi1 ), theta1, phi1 ),
              sv2( magFunc( theta1, phi2 ), theta1, phi2 ),
              sv3( magFunc( theta2, phi2 ), theta2, phi2 ),
              sv4( magFunc( theta2, phi1 ), theta2, phi1 ) ;    
      
      // The colors can be less than 0.
      Vector color1 = colorFunc( theta1, phi1 ) ;
      Vector color2 = colorFunc( theta1, phi2 ) ;
      Vector color3 = colorFunc( theta2, phi2 ) ;
      Vector color4 = colorFunc( theta2, phi1 ) ;
      
      // abs (enforce non-negative radius)
      sv1.r = abs(sv1.r) ;
      sv2.r = abs(sv2.r) ;
      sv3.r = abs(sv3.r) ;
      sv4.r = abs(sv4.r) ;

      //printf( "%f %f %f %f   ", sv1.r, sv2.r, sv3.r, sv4.r ) ;

      // get cartesian coordinates (+r valued)
      Vector c1 = sv1.toCartesian() ;
      Vector c2 = sv2.toCartesian() ;
      Vector c3 = sv3.toCartesian() ;
      Vector c4 = sv4.toCartesian() ;

      // compute the normals
      Vector n1 = ( (c2 - c1) << (c4 - c1) ).normalize() ;
      Vector n2 = ( (c3 - c2) << (c1 - c2) ).normalize() ;
      Vector n3 = ( (c4 - c3) << (c2 - c3) ).normalize() ;
      Vector n4 = ( (c1 - c4) << (c3 - c4) ).normalize() ;

      // here the diffuse color is the same
      // as the emissive color.  However,
      // for a light, it really should be
      // emissive color only.

      // MODULATE source material by color values found here.
      AllVertex vertex1( center + c1, n1, Material( color1, color1, color1, color1, 1 ).mult( shapeToAddMeshTo->material ) ) ;
      AllVertex vertex2( center + c2, n2, Material( color2, color2, color2, color2, 1 ).mult( shapeToAddMeshTo->material ) ) ;
      AllVertex vertex3( center + c3, n3, Material( color3, color3, color3, color3, 1 ).mult( shapeToAddMeshTo->material ) ) ;
      AllVertex vertex4( center + c4, n4, Material( color4, color4, color4, color4, 1 ).mult( shapeToAddMeshTo->material ) ) ;

      // facing out
      if( t == 0 ) // top cap
        mesh->addTri( vertex1, vertex3, vertex4 ) ; //t1p1, t2p2, t2p1
      else if( t + 1 == stacks ) //end cap
        mesh->addTri( vertex3, vertex1, vertex2 ) ; //t2p2, t1p1, t1p2
      else
      {
        // body, facing OUT:
        mesh->addTri( vertex1, vertex2, vertex4 ) ;
        mesh->addTri( vertex2, vertex3, vertex4 ) ;

        // body, FACING IN
        //mesh->addTri( vertex1, vertex4, vertex2 ) ;
        //mesh->addTri( vertex2, vertex4, vertex3 ) ;
      }
    }
  }

  mesh->calculateNormals() ;
  mesh->createVertexBuffer() ;

  if( !shapeToAddMeshTo->meshGroup )
    shapeToAddMeshTo->meshGroup = new MeshGroup( shapeToAddMeshTo, iMeshType, iVertexType, 1000 ) ;

  shapeToAddMeshTo->meshGroup->addMesh( mesh ) ;
}
