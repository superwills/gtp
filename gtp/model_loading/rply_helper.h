#ifndef RPLY_HELPER_H
#define RPLY_HELPER_H

#include "rply.h"
#include "../math/Vector.h"
#include "../math/Matrix.h"

// PLY
int vertex_recv_x( p_ply_argument argument ) ;
int vertex_recv_y( p_ply_argument argument ) ;
int vertex_recv_z( p_ply_argument argument ) ;
int face_cb( p_ply_argument argument ) ;
bool loadPLY( char* filename, const Matrix & transform ) ;

#endif