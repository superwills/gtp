#include "Parser.h"
#include "../geometry/Model.h"
#include "../window/D3DDrawingVertex.h"
#include "../util/StopWatch.h"
#include "../geometry/Bezier.h"

bool argCheck( char *fnName, char* str, int numArgsGot, int numArgsExpected )
{
  if( numArgsGot != numArgsExpected )
  {
    warning( "%s failed to parse %d args from `%s`, parsed %d", fnName, numArgsExpected, str, numArgsGot ) ;
    return false ;
  }
  else
    return true ;
}

char * FaceSpecModeName[] =
{
  "FACESPECMODE UNDEFINED ERROR", // 0
  "v",                // 1
  "n / error",        // 2
  "v//n",             // 3
  "t / error",        // 4
  "v/t",              // 5
  "n/t / error",      // 6
  "v/t/n"             // 7
} ;


char* Parser::DEFAULT_GROUP = "__DEFAULT_GROUP__" ;
char* Parser::DEFAULT_MATERIAL = "__DEFAULT_MATERIAL__" ;

Parser::Parser( Model *iModel, MeshType iMeshType, VertexType iVertexType, Material defaultMat ) :
  model(iModel), meshType(iMeshType), vertexType( iVertexType )//just saves these
{
  
}

Parser::~Parser()
{
  
}

bool Parser::open( char* filename )
{
  origFile = strdup(filename);

  info( "opening %s\n", filename ) ;
  file = fopen( filename, "r" ) ;
  if( !file )  error( "Could not open %s\n", filename ) ;

  return (bool)file ;
}

bool Parser::Load( Model *iModel, char *filename, FileType fileType, MeshType iMeshType, VertexType iVertexType, Material defaultMat )
{
  const char* ext = strrchr( filename, '.' ) ;
  if( !ext )
  {
    info( "No extension" ) ;
    ext = "" ;
  }
  else
  {
    ext += 1 ;
    //info( "ext is %s", ext ) ;
  }
  Parser *parser ;

  if( !strcmpi( ext, "obj" ) )
  {
    //info( "OBJ" ) ;

    // confirm with fileType
    if( fileType != FileType::OBJ )
      warning( "Detected OBJ, but you specified %d in fileType", fileType ) ;
    
    parser = new OBJParser( iModel, iMeshType, iVertexType, defaultMat ) ;
  }
  else if( !strcmpi( ext, "ply" ) )
  {
    info( "PLY" ) ;

    if( fileType != FileType::PLY )
      warning( "Detected PLY, but you specified %d in fileType", fileType ) ;

    parser = new PLYParser( iModel, iMeshType, iVertexType, defaultMat ) ;
  }
  else
  {
    // use fileType
    error( "Unrecognized filetype or not programmed for" );
  }
  
  if( !parser->open( filename ) )  return false ; // parse file failed
  parser->read() ;
  DESTROY( parser ) ;

  return true ; // success
}

OBJParser::OBJParser( Model *iModel, MeshType iMeshType, VertexType iVertexType, Material defaultMat ):
Parser(iModel,iMeshType,iVertexType,defaultMat)
{
  // make the default group/material
  OBJGroup *defaultGroup = new OBJGroup( DEFAULT_GROUP ) ;
  materials.insert( make_pair( DEFAULT_MATERIAL, new Material( defaultMat ) ) ) ;
}

#pragma region string parse routines
int OBJParser::getFaceSpecMode( char *str )
{
  /// The general format is
  /// f v/vt/vn v/vt/vn v/vt/vn v/vt/vn
  
  /// For Vertices only, we use something like
  /// f v v v
  /// f 3 5 1
  
  /// For Vertices|Normals (no texture coords), we use
  /// f v//vn v//vn v//vn
  /// f 3//2 1//5 6//1

  /// For Vertices|Texcoords, we use
  /// f v/vt v/vt v/vt
  /// f 4/1 3/5 2/2
  
  char *pos = str+2 ; // skip "f "
    
  while( !isspace(*pos) )
  {
    if( *pos == '/' )  // Hit first slash.
    {
      // Check if we have a slash right next to it,
      ++pos ;
      if( *pos == '/' )
        return Vertices | Normals ;  // "v//n"

      // If we didn't have a slash right next to it,
      // it'll either be DIGITS then white space (v/t)
      // or there'll be ANOTHER white slash before
      // the next white space.

      while( !isspace(*pos) )
      {
        if( *pos == '/' )
          return Vertices | Texcoords | Normals ; // "v/t/n"
        ++pos ;
      }

      // if it WAS a space next (before
      // hitting a /) then we have v/t
      return Vertices | Texcoords ; //"v/t"
    }

    // next char.
    ++pos;
  }
    
  // If didn't return above, means hit space
  // before hitting slash at all
  return Vertices ; // "v"
}
  
// counts numbers items in "f 3/5/70 82/9/10 10/4/9   55/2/69"
int OBJParser::countVerts( char *buf )
{
  int count = 0 ;
  char *line = buf+2 ;

  // advance until the first nonwhitespace character
  // past the "f "
  while( isspace( *line ) )  line++ ;

  int len = strlen( line ) ; // length from first vertex to end string
  bool onWord = true ;
    
  for( int i = 0 ; i < len; i++ )
  {
    if( isspace( line[i] ) )
    {
      // INcrement count only when
      // stepping OFF vertex spec to whitespace
      if( onWord )
      {
        count++ ;
        onWord = false ;
      }
    }
    else
      onWord = true ;
  }

  // If you ended while still on word
  // then increment count here to count
  // the "last" word
  if( onWord )
    count++ ;

  return count ;
}

bool OBJParser::setK( Material* mat, char *str )
{
  Vector *vec ;
  // can be T also
  //if( str[0] != 'K' && str[0] != 'T' ) { error( "Not a material parameter" ) ; return false ; }
  bool kt=false;
  switch( str[1] )
  {
  //case 'a': vec = &mat->ka ; break ;
  case 'a': warning( "ka term IGNORED" ) ; return false ; // if Ka is 0 and is read after Kd with values, you lose Kd
  case 'd': vec = &mat->kd ; break ;
  case 's': vec = &mat->ks ; break ;
  case 'e': vec = &mat->ke ; break ;
  case 'f': vec = &mat->kt ; kt=true; break ; //"Tf"

  default:
    error( "Not valid material" ) ;
    return false ;
  } 
  
  

  str+=2; // move past K?
  while( isspace(*str) )  str++ ; // move past all white space
  int res = sscanf( str, THREE_FLOATS, &vec->x, &vec->y, &vec->z ) ;
  if( kt )
  {
    if( mat->kt.all( 1.0 ) )
    {
      warning( "KT WAS ALL 1.0, WHICH MEANS THE MATERIAL IS FULLY TRANSMISSIVE" ) ;
      warning( "Change Tf to 0 or the shape will be entirely INVISIBLE" ) ;
      warning( "Believed to be a Maya obj export bug, because Tf is supposed to be "
      "transmissive color, but it is 1-transmissive color" ) ;
      mat->kt = Vector( 1,1,1 ) - mat->kt ;
    }
  }


  return argCheck( "setK", str, res, 3 ) ;
}
#pragma endregion

#pragma region parse vertex and extract faces
bool OBJParser::parseVertex( char *buf, Vector *v )
{
  int res = sscanf( buf, OBJ_VERTEX, &v->x, &v->y, &v->z ) ;
  return argCheck( "parseVertex", buf, res, 3 ) ;
}

bool OBJParser::parseNormal( char *buf, Vector *v )
{
  int res = sscanf( buf, OBJ_NORMAL, &v->x, &v->y, &v->z ) ;
  return argCheck( "parseNormal", buf, res, 3 ) ;
}

bool OBJParser::parseTexcoord( char *buf, Vector *v )
{
  int res = sscanf( buf, OBJ_TEXCOORD, &v->x, &v->y ) ;
  return argCheck( "parseTexcoord", buf, res, 2 ) ;
}

void OBJParser::extractFaces( char *buf, int numVerts, OBJGroup *group )
{
  int index = group->vertCountPass2 ;

  // We know how many vertices to expect,
  // so we're going to save each in a temporary buffer
  // The reason for the temporary buffer
  // is we need to triangulate
  vector<int> verts( numVerts ) ;
  vector<int> normals( numVerts ) ;
  vector<int> texcoords( numVerts ) ;

  // Extract numVerts from buf
  // f 2 4 1 5
  // f 8/5 2/7 1/3 7/7 1/9 3/7
  // f 2//6 3//5 1//8 8//3 9//7
  // f 9/2/5 8/6/7 9/1/3

  // Assume we start here.  So we know everything about the line.  

  FaceSpecMode ff = (FaceSpecMode)getFaceSpecMode( buf ) ; // E.g. "v/t" ;

  char *line = buf + 2 ;
  while( isspace( *line ) ) ++line ; // skip past any whitespace in front

  // now, based on the line format, use the correct format string
  if( (ff & Texcoords) && (ff & Normals) )
  {
    // v/t/n
    int v,t,n;
    char *pos = line ;
    int vReadCount = 0 ;

    while( pos ) // while we haven't reached the null terminator yet
    {
      int res = sscanf( pos, "%d/%d/%d", &v, &t, &n ) ;
      
      if( v < 0 )  v = numVerts + v ;
      if( t < 0 )  t = numTexCoords + t ;
      if( n < 0 )  n = numNormals + n ;

      // Make a vertex.
      verts[ vReadCount ] = v ;
      texcoords[ vReadCount ] = t ;
      normals[ vReadCount ] = n ;

      vReadCount++ ;

      cstrNextWord( pos ) ;
    }
  }
  else if( ff & Normals )
  {
    // v//n
    int v,n;

    char *pos = line ;
    int vReadCount = 0 ;

    while( pos )
    {
      int res = sscanf( pos, "%d//%d", &v, &n ) ;
      
      if( v < 0 )  v = numVerts + v ;
      if( n < 0 )  n = numNormals + n ;  
      
      verts[ vReadCount ] = v ;
      normals[ vReadCount ] = n ;

      vReadCount++ ;

      // Advance to the next "word"
      cstrNextWord( pos ) ;
    }
  }
  else if( ff & Texcoords )
  {
    // v/t
    int v,t ;

    char *pos = line ;
    int vReadCount = 0 ;

    while( pos )
    {
      int res = sscanf( pos, "%d/%d", &v, &t ) ;
      
      if( v < 0 )  v = numVerts + v ;
      if( t < 0 )  t = numTexCoords + t ;
      
      verts[ vReadCount ] = v ;
      texcoords[ vReadCount ] = t ;
      
      vReadCount++ ;
      cstrNextWord( pos ) ;
    }
  }
  else
  {
    // v
    int v;

    char *pos = line ;
    int vReadCount = 0 ;

    while( pos )
    {
      sscanf( pos, "%d", &v ) ;
      if( v < 0 )  v = numVerts + v ;
      verts[ vReadCount ] = v ;
      vReadCount++ ;
      cstrNextWord( pos ) ;
    }

    /*
    // DEBUG
    for( int i = 0 ; i < group->facesVertexIndices.size() ; i+=3 )
    {
      info( "FACE: %d %d %d", group->facesVertexIndices[i],
        group->facesVertexIndices[i+1], group->facesVertexIndices[i+2] ) ;
    }
    */
    #pragma endregion
  }

  // Triangulate 
  // NOW we have numVerts verts.
  // So now, if this number is larger than 3,
  // we need to triangulate.

  // The first face is just the first 3,
  // as they were
  int i = 0 ;
  for( int c = 1 ; c < (numVerts-1) ; c++ )
  {
    // always start with 0th vertex.
    group->facesVertexIndices[ index + i ] = verts[ 0 ] ;
    group->facesTextureIndices[ index + i ] = texcoords[ 0 ] ;
    group->facesNormalIndices[ index + i ] = normals[ 0 ] ;

    // tie with c'th vertex
    group->facesVertexIndices[ index + i+1 ] = verts[ c ] ;
    group->facesTextureIndices[ index + i+1 ] = texcoords[ c ] ;
    group->facesNormalIndices[ index + i+1 ] = normals[ c ] ;
    // and c'th+1 vertex
    group->facesVertexIndices[ index + i+2 ] = verts[ c+1 ] ;
    group->facesTextureIndices[ index + i+2 ] = texcoords[ c+1 ] ;
    group->facesNormalIndices[ index + i+2 ] = normals[ c+1 ] ;
    
    // REplicated for ease
    group->facesMaterialIndices[ index + i ] =
    group->facesMaterialIndices[ index + i+1 ] =
    group->facesMaterialIndices[ index + i+2 ] = currentMaterial ;
    //info( "%s - wound face %d %d %d", group->getName(), verts[0], verts[c], verts[c+1] ) ;
    // Advance by 3 faces, we just added 3 coords
    i += 3 ;
  }
}
#pragma endregion 

void OBJParser::loadMtlFile( char *mtllibName )
{
  if( !mtllibName )
  {
    error( "Mtllibname was null" ) ;
    return ;
  }

  FILE *mtlFile = fopen( mtllibName, "r" ) ;
  if( !mtlFile )
  {
    error( "Couldn't open mtllib %s", mtllibName ) ;
    return ;
  }
  else
    info( "Opened mtllib %s", mtllibName ) ;

  char buf[ 300 ] ;
  currentMaterial = getMaterial( DEFAULT_MATERIAL ) ;

  while( !feof( mtlFile ) )
  {
    fgets( buf, 300, mtlFile ) ;
    cstrnulllastnl( buf ) ;

    if( buf[0] == '#' ) // its a comment
    {
      #if OBJ_PARSER_VERBOSE
      info( buf ) ;
      #endif
      // otherwise empty
    }

    // if it starts with newmtl,
    // its a new material
    else if( !strncmp( buf, MTL_NEW, MTL_NEW_LEN ) )
    {
      // It means we're starting a new
      // material..
      string mtlName = buf+(MTL_NEW_LEN+1) ;

      // Base it on the default base material that Parser uses
      currentMaterial = new Material( *getMaterial( DEFAULT_MATERIAL ) ) ;

      materials.insert( make_pair( mtlName, currentMaterial ) ) ;
      //info( "Loaded material `%s`", mtlName.c_str() ) ;
    }

    // The rest of these just affect the
    // "currentMaterial", whatever that is.
    else
    {
      // For this to RUN, there MUST have been
      // a CREATE MATERIAL stmt previously
      // This'll only happen if there are BLANK LINES
      // above the first newmtl stmt
      // in the worst case you'll see MATERIAL STATEMENTS here
      // and so you'll know something is wrong
      //if( !currentMaterial )
      //  warning( "material parse: no material yet (a line had to be ignored, `%s`)", buf ) ;

      // a material color is being spec
      if( buf[0] == 'K' || buf[0] == 'T' )  setK( currentMaterial, buf ) ;

      // The illum term is being specified
      else if( !strncmp( buf, MTL_ILLUM, MTL_ILLUM_LEN ) )
      {
        // illum term probably corresponds with ke
        //currentMaterial->setIllum( buf ) ;
      }

      // The Ns term is being specified
      else if( !strncmp( buf, MTL_NSPEC, MTL_NSPEC_LEN ) )
      { 
        // just do it inline here,
        int res = sscanf( buf, OBJ_NS, &currentMaterial->Ns ) ;
        argCheck( "setNs", buf, res, 1 ) ;
      }

      else if( !strncmp( buf, MTL_D, MTL_D_LEN ) )
      {
        info( "Not using D term" ) ;
        //currentMaterial->setD( buf ) ;
      }

      else if( !strncmp( buf, MTL_TEXTURE_NAME, MTL_TEXTURE_NAME_LEN ) )
      {
        warning( "Not loading the texture" ) ;
        // We found a texture filename
        // We need to assign it the next
        // available texture id.
        /////char *spriteFile = buf+(MTL_TEXTURE_NAME_LEN+1) ;
        /////int spriteId = window->getNextSpriteId() ;
        /////
        /////window->loadSprite( spriteId, spriteFile ) ;
        /////
        /////// Save the sprite id
        /////currentMaterial->setSpriteId( spriteId ) ;
        /////currentMaterial->setTextureFilename( spriteFile ) ;
      }
    }
  }

  fclose( mtlFile ) ;

  #if OBJ_PARSER_VERBOSE
  info( "%d materials loaded", materials.size() ) ;

  for( pair<string, Material*> matPair : materials )
  {
    Material *mat = matPair.second ;
    info( "  %s:\n    - kd=(%f %f %f)\n    - ke=(%f %f %f)\n    - ks=(%f %f %f) Ns=%f\n    - kt=(%f %f %f)\n", matPair.first.c_str(),
      mat->kd.x, mat->kd.y, mat->kd.z,
      mat->ke.x, mat->ke.y, mat->ke.z,
      mat->ks.x, mat->ks.y, mat->ks.z, mat->Ns,
      mat->kt.x, mat->kt.y, mat->kt.z ) ;
  }
  #endif
}

Material* OBJParser::getMaterial( string materialName )
{
  // get material as defined in .mtl file.
  map<string,Material*>::iterator matEntry = materials.find( materialName ) ;
  
  if( matEntry == materials.end() )
  {
    // Lightwave 2.1 OBJ exporter bug:    
    //  - if usemtl is attempted but that mtl doesn't exist, create it
    //    and see if there is an image with the same filename as the mtl.
    //    If there is, load it and use it for that mtl.
      
    Material* mat = new Material() ;
    materials.insert( make_pair( materialName, mat ) ) ;
    info( "Created material %s", materialName.c_str() ) ;
    return mat ;
  }
  else
    return matEntry->second ;
}

OBJParser::OBJGroup* OBJParser::getGroup( string groupName )
{
  map<string,OBJGroup*>::iterator groupEntry = groups.find( groupName ) ;
  if( groupEntry == groups.end() )  // It doesn't exist, so create it
  {
    OBJGroup *newGroup = new OBJGroup( groupName ) ;
    //info( "Group %s didn't exist, so created it", groupName.c_str() ) ;
    groups.insert( make_pair( groupName, newGroup ) ) ;
    return newGroup ;
  }
  else
    return groupEntry->second ;
}

void OBJParser::pass1()
{
  // Get the material library filename,
  // open and load all materials

  // numFacesStock is just the number of "f 3 4 5" statements
  // in the file,
  int numFacesStock = 0 ;
    
  // numTriangleFacesTOTAL is total # faces, after triangulation
  numVerts=numNormals=numTexCoords=numTriangleFacesTOTAL = 0 ;   
    
  // we start in the default group and material
  currentMaterial = getMaterial( DEFAULT_MATERIAL ) ;
  OBJGroup *currentGroup = getGroup( DEFAULT_GROUP ) ;

  char buf[ 300 ] ;

  #pragma region parse file pass_1
  while( !feof( file ) )
  {
    // Count normals, texture coords, vertices, faces

    // For each line, we have to
    // read and determine if it is
    // a v, vn, vt, f
      
    fgets( buf, 300, file ) ;

    // If the newline gets left on at the end,
    // we actually want to cut it off.
    cstrnulllastnl( buf ) ;

    if( buf[0] == '#' )     // Its a comment, pass it thru
      ; //info( buf ) ;
    else if( buf[0] == 'v' && buf[1] == 'n' )  // vertex normal
      numNormals++ ;
    else if( buf[0] == 'v' && buf[1] == 't' )  // texture coordinate
      numTexCoords++ ;
    else if( buf[0] == 'v' && buf[1] == 'p' )  // parameter space vertices
      warning( "Sorry, parameter space vertices not supported, ignoring" ) ;
    else if( buf[0] == 'v' && isspace(buf[1]) )
      numVerts++ ;  // The only other thing that starts 
      // with 'v' is a vertex itself. the buf[1] character will be either a SPACE or a TAB
    else if( buf[0] == 'f' && isspace(buf[1]) )// a face
    {
      // ALWAYS detect faceSpecMode to see if there are any anomolies.
      FaceSpecMode fsm = (FaceSpecMode)getFaceSpecMode( buf ) ;

      // Inform the user of the change for this group.
      if( currentGroup->faceSpecMode == NOFACE )
      {
        #if OBJ_PARSER_VERBOSE
        info( "FaceSpecMode is `%s`", FaceSpecModeName[ fsm ] ) ;
        #endif
      }
        
      // A bit defensive
      else if( fsm != currentGroup->faceSpecMode )
        warning( "Anomaly detected, facespecmode changed from %s to %s while in group\n%s",
            FaceSpecModeName[ currentGroup->faceSpecMode ],
            FaceSpecModeName[ fsm ], buf ) ;

      currentGroup->faceSpecMode = fsm ; // assign it.

      numFacesStock++ ;

      // We save ONLY TRIANGLES, not other types of polygons.  This means the NUMBER OF FACES
      // that results from a line
      // f 3 6 9 1
      // is __2__, not just one.

      // if the faces are specified as QUADS, or pentagons, then we need to break this
      // into (VERTS-2) faces.
      int verticesInFace = countVerts( buf ) ;
      int facesFromVerts = verticesInFace - 2 ;
      int numVertsAdded = facesFromVerts*3 ;

      numTriangleFacesTOTAL += facesFromVerts ;

      // Increase the numVerts count
      // FOR THE CURRENT GROUP

      // Increment the total number
      // that should be in group->facesVertexIndices
      currentGroup->indexCountPass1 += verticesInFace ;

      // Increment the total number
      // that should be in group->combinedVerticesV
      currentGroup->vertCountPass1 += numVertsAdded ;
    }
    else if( buf[0] == 'g' && isspace(buf[1]) ) //GROUP
    {
      // A group.  Create it now, unless it already exists
      char *groupName = buf+2 ; // pass "g ", name goes to end of line

      // If there's a long name with spaces just cut off the extra names
      // this puts it in the same group as the first name.
      /////////////cstrnullnextsp( groupName ) ;  // Don't do this.
      //Xwhen a file has "more than one name" in the group name string.
      //XI'm not sure what this means either, but it
      //Xcreates a "group" that has a unique name
      //XFIXED by cutoff.

      // the spaces represent hierarchy and parenting.
      //g windshield planePieces
      // would be CHILDGROUP PARENTGROUP
      // windshield is the name of the mesh,
      // and planePieces was a "group" organization in maya

      // if the group doesn't exist, by "currentMaterial" name,
      // then it will be created here.
      currentGroup = getGroup( groupName ) ;
    }
    else if( !strncmp( buf, OBJ_CHANGE_MATERIAL, OBJ_CHANGE_MATERIAL_LEN ) )//usemtl
    {
      string mtlName = buf+(OBJ_CHANGE_MATERIAL_LEN+1) ;
      currentMaterial = getMaterial( mtlName ) ;
    }
    else if( !strncmp( buf, MTL_LIBRARY, MTL_LIBRARY_LEN ) )
    {
      // Its the material library filename!  grab it!
      
      // Now we may as well parse this up here,
      // on the first pass.
      loadMtlFile( buf+(MTL_LIBRARY_LEN+1) ) ;
    }
  } // END OF FILE parse
  #pragma endregion

  info( "verts=%d, normals=%d, texcoords=%d, "
        "stock faces=%d triangulated faces=%d, "
        "groups=%d",
          numVerts, numNormals, numTexCoords,
          numFacesStock, numTriangleFacesTOTAL,
          groups.size() ) ;
  //info( "First pass on %s complete", origFile ) ;

  // Allocate space in the vertices, normals, texcoords arrays
  // +1 because index 0 IS NOT USED in the OBJ file format.
  verts.resize( numVerts+1 ) ;
  normals.resize( numNormals+1 ) ;
  texcoords.resize( numTexCoords+1 ) ;

  // faces are divided up into the different groups,
  // so we can't really say right now faces.resize()..
  for( map<string,OBJGroup*>::iterator groupIter = groups.begin() ;
       groupIter!=groups.end(); ++groupIter )
  {
    OBJGroup *g = groupIter->second ;

    // Allocate enough space in each group object
    // to hold this many vertex indexes
    // g->getNumFaces() WILL READ 0 HERE

    // g->vertCountPass1 is used because
    // it is a count of the total # vertices
    // in the model AFTER triangulation.

    g->facesVertexIndices.resize( g->vertCountPass1 ) ;
    g->facesNormalIndices.resize( g->vertCountPass1 ) ;
    g->facesTextureIndices.resize( g->vertCountPass1 ) ;
    g->facesMaterialIndices.resize( g->vertCountPass1 ) ; // thsi can be divided by 3, but I left it so you don't
    // have to divide the vertex index by 3 in parse
    
    #if OBJ_PARSER_VERBOSE
    info( "Group `%s` has %d vertices, %d faces", g->name.c_str(), g->vertCountPass1, g->facesVertexIndices.size()/3 ) ;

    // Determine what vertex declaration to use
    // based on face mode
    if( g->isFaceSpecMode(Vertices|Texcoords|Normals) ) // v/t/n
      info( "Group %s using PositionTextureNormal vertex", g->name.c_str() ) ;
    else if( g->isFaceSpecMode(Vertices|Texcoords) ) // v/t
      info( "Group %s using PositionTexture vertex", g->name.c_str() ) ;
    else if( g->isFaceSpecMode(Vertices|Normals) )   // v//n
      info( "Group %s using PositionNormal vertex", g->name.c_str() ) ;
    else   // v
      info( "Group %s using Position vertex", g->name.c_str() ) ;
    #endif
  }
}

void OBJParser::pass2()
{
  // In the second pass we actually
  // read up the vertices and save them.

  // A lot of this code is similar
  // to the first pass, only we
  // are actually snapping out the data here.

  int numFacesStock=0 ;

  // DIFFERS from numTriangleFacesTOTAL, this
  // variable is just an OCD count of
  // total faces in second pass, to make sure
  // its the same as numTriangleFacesTOTAL
  int numFacesTOTALSecondPass=0 ;
  numVerts=numNormals=numTexCoords=0 ; // these are very important
    
  string currentGroupName = ""; // we must know what
  // group we're on at all times.

  char buf[ 300 ] ;

  #pragma region parse file pass_2
  
  // Start in the default group with the default material.
  currentMaterial = getMaterial( DEFAULT_MATERIAL ) ;
  OBJGroup *currentGroup = getGroup( DEFAULT_GROUP ) ; 
  
  // 'g' statements change the group to ( CURRENT_GROUP, CURRENT_MATERIAL ) ;
  // 'usemtl' statements change the group to ( CURRENT_GROUP, NEW_MATERIAL ) ;

  rewind( file ) ;
  while( !feof( file ) )
  {
    fgets( buf, 300, file ) ;
    cstrnulllastnl( buf ) ;

    if( buf[0] == '#' )
    {
      // Its a comment, pass it thru
      //info( buf ) ;
    }
    else if( buf[0] == 'v' && buf[1] == 'n' )
    {
      // vertex normal

      // Increment the counter so we know which normal we're on
        
      // This goes BEFORE use of the variable
      // because index 0 is not used.
      numNormals++ ;

      // Actually save it.
      parseNormal( buf, &normals[ numNormals ] ) ;
    }
    else if( buf[0] == 'v' && buf[1] == 't' )
    {
      //texture coordinate
      numTexCoords++ ;  // index 0 not used
      parseTexcoord( buf, &texcoords[ numTexCoords ] ) ;
    }
    else if( buf[0] == 'v' && buf[1] == 'p' )
    {
      // parameter space vertices
      warning( "Sorry, parameter space vertices not supported, ignoring" ) ;
    }
    else if( buf[0] == 'v' && isspace(buf[1]) )
    {
      numVerts++ ; // idx 0 unused

      // The only other thing that starts 
      // with 'v' is a vertex itself.
      // the buf[1] character will be
      // either a SPACE or a TAB
      parseVertex( buf, &verts[ numVerts ] ) ;
    }
    else if( buf[0] == 'f' && isspace(buf[1]) ) // pass 2 face parse
    {
      #pragma region face parse
      numFacesStock++ ;
        
      int verticesInFace = countVerts( buf ) ;
      int facesFromVerts = verticesInFace - 2 ;
      int numVertsAdded = facesFromVerts*3 ; // this is all very
      // twisted looking, but here's what's going
      // on in the 3 lines above.
      // 1. verticesInFace is just a count
      //    of the number of vertices "VISITED"
      //    by this face specification.
      //    f 1 3 2    visits 3 vertices
      //    f 4 5 9 1  visits 4 vertices.

      // We need to know this because we're going
      // to triangulate.  That is, f 4 5 9 1 will
      // be broken into TWO triangles, effectively
      // TWO face specifications
      // f 4 5 9
      // f 4 9 1
      // So that's what FACESFROMVERTS is.
      // in the case of f 4 5 9 1,
      // facesFromVerts = 4 - 2   // meaning
      // we'll extract __2__ faces from
      // f 4 5 9 1.
      // From f 9 5 1 2 8 we'd extract.. 3 faces.
      // f 9 5 1
      // f 9 1 2
      // f 9 2 8
      // This is "fanning" out or whatever, it
      // sometimes produces splintery geometry
      // but it works simply.

      // SO NOW, the NUMBERVERTSADDED variable
      // is the ACTUAL count of vertices we're adding
      // based on the number of faces we've decdied to
      // add after triangulation.  for f 1 3 2 we'd
      // only add 3 vertices, but for f 4 5 9 1 
      // we'd add 3*(NUM_FACES)=3*2=6 vertices.
        
      // keep track of the number of faces
      // we're creating here, as a parity check:
      numFacesTOTALSecondPass += facesFromVerts ;

      // Save the face in the current group
      if( !currentGroup )
      {
        // Create
        warning( "Faces added before any groups were defined, "
          "adding faces to the \"default\" group" ) ;
          
        currentGroup = getGroup( DEFAULT_GROUP ) ;
      }

      // currentGroup will be defined here.
        
      // Now, parse off the face values.
      extractFaces( buf, verticesInFace, currentGroup ) ;

      // HERE WE UPDATE "WHAT FACE WE ARE ON"
      // This IS NOT a count of the number of faces
      // added.. rather its a count of the total # VERTICES added.
      currentGroup->vertCountPass2 += numVertsAdded ;
      // BECAUSE FACES CAN BE SPECIFIED IN ANY ORDER,
      // (like, file can weave in and out of different groups)
      // You really have to be careful about 
      // keeping this number up to date.
      #pragma endregion
    }
    else if( buf[0] == 'g' && isspace(buf[1]) )
    {
      char *groupName = buf+2 ; // pass "g ", name goes to end of line
      ///////////cstrnullnextsp( groupName ) ;
      currentGroupName = groupName ;
      currentGroup = getGroup( groupName ) ;
      if( currentGroup )
      {
        // The group exists
        // We are switching to entry into this group.
        //info( "Switching to group `%s`", groupName ) ;
      }
      else
      {
        // This shouldn't happen, it means we missed
        // creating a group in the first pass
        error( "Obj file parse pass #2:  group `%s` doesn't exist!", groupName ) ;
      }
    }
    else if( !strncmp( buf, OBJ_CHANGE_MATERIAL, OBJ_CHANGE_MATERIAL_LEN ) )//usemtl
    {
      string mtlName  = buf+(OBJ_CHANGE_MATERIAL_LEN+1) ;
      currentMaterial = getMaterial( mtlName ) ;
      //info( "Pass 2: Material changed to %s, kd=%f %f %f",
      //  mtlName.c_str(), currentMaterial->kd.x,currentMaterial->kd.y,currentMaterial->kd.z ) ;
    }
    else if( !strncmp( buf, MTL_LIBRARY, MTL_LIBRARY_LEN ) )
    {
      // Pass #1 loads the material lib file
      //info( "Pass #2:  pass 1 loads the material lib file, skipping" ) ;
    }
  }
  #pragma endregion

  // PARITY CHECKS
  if( numFacesTOTALSecondPass != numTriangleFacesTOTAL )
  {
    warning( "numFacesTOTALSecondPass=%d, numTriangleFacesTOTAL=%d, "
      "but these numbers should have been the same.",
      numFacesTOTALSecondPass, numTriangleFacesTOTAL ) ;
  }
  else
  {
    #if OBJ_PARSER_VERBOSE
    info( "OK! numFacesTOTALSecondPass=%d, numTriangleFacesTOTAL=%d",
      numFacesTOTALSecondPass, numTriangleFacesTOTAL ) ;
    #endif
  }

  setupMeshes() ;
}

void OBJParser::setupMeshes()
{
  // We need to set up vertex arrays
  // Combine all the groups..
  //info( "There are %d groups, setting up meshes, meshType=%s", groups.size(), MeshTypeName[ meshType ] ) ;
  
  for( map<string,OBJGroup*>::iterator groupIter = groups.begin();
        groupIter != groups.end() ; ++groupIter )
  {
    OBJGroup* g = groupIter->second ;
    
    if( !g->vertCountPass2 )  continue ; // empty group, skip.
    
    // I use a different mesh for each group here.
    // This isn't NECESSARY, but it's a logical division to have.
    Mesh *mesh = new Mesh( model, meshType, vertexType ) ;
    ////Material rmat = Material::randomSolid() ;
    for( int i = 0 ; i < g->vertCountPass2 ; i+=3 )
    {
      if( every(i,100) )
        printf( "Vertex %d/%d   \r", i, g->vertCountPass2 ) ;
      int vIndex1 = g->facesVertexIndices[ i ] ;
      int vIndex2 = g->facesVertexIndices[ i+1 ] ;
      int vIndex3 = g->facesVertexIndices[ i+2 ] ;

      int nIndex1 = g->facesNormalIndices[ i ] ;
      int nIndex2 = g->facesNormalIndices[ i + 1 ] ;
      int nIndex3 = g->facesNormalIndices[ i + 2 ] ;

      int tIndex1 = g->facesTextureIndices[ i ] ;
      int tIndex2 = g->facesTextureIndices[ i + 1 ] ;
      int tIndex3 = g->facesTextureIndices[ i + 2 ] ;

      // Because material can only vary
      // between faces, mat is going to be the same
      // across the tri.
      Material *mat = g->facesMaterialIndices[ i ] ;
      
      //!!TEST SHOW THE DRAGON CUTUP
      /////AllVertex v1( verts[ vIndex1 ], normals[ nIndex1 ], rmat ) ;
      /////AllVertex v2( verts[ vIndex2 ], normals[ nIndex2 ], rmat ) ;
      /////AllVertex v3( verts[ vIndex3 ], normals[ nIndex3 ], rmat ) ;

      AllVertex v1( verts[ vIndex1 ], normals[ nIndex1 ], *mat ) ;
      AllVertex v2( verts[ vIndex2 ], normals[ nIndex2 ], *mat ) ;
      AllVertex v3( verts[ vIndex3 ], normals[ nIndex3 ], *mat ) ;


      // If the normal was 0 ( not specified ) then use the one from the triangle
      if( v1.norm.all(0) || v2.norm.all(0) || v3.norm.all(0) )
      {
        Plane pT( v1.pos, v2.pos, v3.pos ) ;
        // Write the same normal into all
        v1.norm = v2.norm = v3.norm = pT.normal ;
      }
  
      // Drop in the texcoord
      v1.texcoord[ TexcoordIndex::Decal ] = texcoords[ tIndex1 ] ;
      v2.texcoord[ TexcoordIndex::Decal ] = texcoords[ tIndex2 ] ;
      v3.texcoord[ TexcoordIndex::Decal ] = texcoords[ tIndex3 ] ;

      // NONINDEXED: Just add a UNIQUE triangle from vertex 1, 2, 3.
      // INDEXED: This takes increasingly long as you add more vertices,
      // an using an octree for spatial search would speed this up.
      mesh->addTri( v1, v2, v3 ) ;
    } // ADD ALL VERTS TO MESH

    // add the last mesh, if it had any tris.
    model->addMesh( mesh ) ;
  } // END FOREACH GROUP

  ////model->meshGroup->calculateNormals() ; //!! Well you only want to
  // calculate normals if they were not specified by the mesh
  if( model->meshGroup )
    info( "Model %s loaded with %d meshes", model->name.c_str(), model->meshGroup->meshes.size() ) ;
  else
    warning( "Model %s loaded with no meshes", model->name.c_str() ) ;

  model->aabb = new AABB( model ) ;
}

void OBJParser::read()
{
  pass1() ;
  pass2() ;

  fclose( file ) ;
}




PLYParser::PLYParser( Model*iModel, MeshType iMeshType, VertexType iVertexType, Material iMat ):
Parser(iModel,iMeshType,iVertexType,iMat)
{
  error( "PLYParser not implemented" ) ;
}

void PLYParser::read()
{
  fclose( file ) ;
}

























