#ifndef PATCH_ADJACENCY_H
#define PATCH_ADJACENCY_H

struct Triangle ;

struct Adjacency
{
  /// the two tris that are adjacent
  Triangle *tri1, *tri2 ;

  /// The two shared vertices
  /// that are used BOTH on p1 and p2
  Vector sharedVertex1, sharedVertex2 ;

  int sharedIndex1onT1, sharedIndex2onT1,  // sharedIndex1onT1 == shoredIndex1onT2 if
      sharedIndex1onT2, sharedIndex2onT2 ; // using index buffers 

  /// The non-shared vertices on
  /// p1 and p2.
  Vector nonSharedVertexOnT1, nonSharedVertexOnT2 ;
} ;


#endif