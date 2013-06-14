#include "rply_helper.h"
#include <vector>
using namespace std ;

#include "../main.h"

#define PLY_FAIL 1

static vector<Vector> nverts ;
static Matrix transformation ;

static float bx, by, bz ;
int vertex_recv_x( p_ply_argument argument )
{
  bx = ply_get_argument_value( argument ) ;
  return 1; // shouldn't this be 0, for "success?"
}

int vertex_recv_y( p_ply_argument argument )
{
  by = ply_get_argument_value( argument ) ;
  return 1 ;
}

int vertex_recv_z( p_ply_argument argument )
{
  // finish the triangle
  bz = ply_get_argument_value( argument ) ;
  nverts.push_back( Vector(bx, by, bz) ) ;
  return 1 ;
}

int index1, index2, index3 ;
int face_cb( p_ply_argument argument )
{
  long length, value_index;
  char buf[120];
  ply_get_argument_property( argument, NULL, &length, &value_index ) ;


  

  switch( value_index )
  {
    case 0:
      index1 = round( ply_get_argument_value( argument ) ) ;

    case 1:
      index2 = round( ply_get_argument_value( argument ) ) ;
      //printf("%.3f ", ply_get_argument_value(argument));
      break;

    case 2:
      {
        index3 = round( ply_get_argument_value( argument ) ) ;
        // This is the THIRD face..
        sprintf( buf, "plymodel_face%d%d%d", index1, index2, index3 ) ;

        Vector v1 = nverts[ index1 ] * transformation ;
        Vector v2 = nverts[ index2 ] * transformation ;
        Vector v3 = nverts[ index3 ] * transformation ;


        //window->scene->addDisplayTriangle(
        //  v1, v2, v3,
        //  Material( ByteColor( 255, 128, 128 ) ),
        //  buf
        //) ;

        //printf("%.3f\n", ply_get_argument_value(argument));
        // Finish the face
      }
      break;

    default:
      // not a triangle.. its a square
      // This is the THIRD face..
      //printf("NOT A TRIANGLE!! %d ", value_index );
      break;
  }

  return 1;
}

bool loadPLY( char* filename, const Matrix& transform )
{
  transformation = transform ;
  nverts.clear() ;

  long nvertices, ntriangles;
  p_ply ply = ply_open( filename, NULL ) ;
  if( !ply )
    return PLY_FAIL;
  if( !ply_read_header( ply ) )
    return PLY_FAIL;
    
  // Set up the naming of what funcion gets called when
  // something comes in.
  nvertices = ply_set_read_cb( ply, "vertex", "x", vertex_recv_x, NULL, 0 ) ;
  ply_set_read_cb( ply, "vertex", "y", vertex_recv_y, NULL, 0 ) ;
  ply_set_read_cb( ply, "vertex", "z", vertex_recv_z, NULL, 0 ) ;
  
  ntriangles = ply_set_read_cb( ply, "face", "vertex_indices", face_cb, NULL, 0 ) ;
  
  
  printf( "Model has %ld vertices, %ld triangles\n", nvertices, ntriangles ) ;
  
  if( !ply_read(ply) )
    return PLY_FAIL;
    
  ply_close(ply);
  return 0;
}
