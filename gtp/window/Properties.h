#ifndef PROPERTIES_H
#define PROPERTIES_H

#include "../util/StdWilUtil.h"
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>

struct Properties
{
  rapidjson::Document jsonDoc ;

  Properties( char* filename )
  {
    FILE* f = fopen( filename, "r" ) ;
    if( !f )
    {
      error( "Couldn't open file '%s'", filename ) ;
      return ;
    }

    int bufSize = cfilesize( f );
    // `readBuffer` shouldn't be needed
    // http://rapidjson.org/md_doc_stream.html
    string readBuffer( bufSize, 0 );
    // readBuffer should be allowed to be passed as readBuffer.c_str()
    // but FileReadStream needs char* not const char*
    rapidjson::FileReadStream fs( f, &readBuffer[0], readBuffer.size()) ;
    rapidjson::Reader jsonReader ;

    jsonDoc.ParseStream<0>( fs ) ;

    if( jsonDoc.HasParseError() )
    {
      error( "rapidjson: Couldn't parse json document, pos=%d, error=%s",
        jsonDoc.GetErrorOffset(), jsonDoc.GetParseError() ) ;
    }
    else if( !jsonDoc.IsObject() )
    {
      error( "JsonDoc not an object!" ) ;
    }

    fclose( f ) ;
  }

  rapidjson::Value* getValue(const char* propertyName)
  {
    // sh::bands looks for
    // "sh" : { "bands" : 36 }
    // break the string up based on ::
    char *str = strdup( propertyName ) ;
    char *p = strtok( str, ":" ) ;

    // check top level document
    if( !jsonDoc.HasMember( p ) )
    {
      error( "json doc has no property '%s'", propertyName ) ;
      free(str);
      return 0;
    }
    
    rapidjson::Value* v = &jsonDoc[p];

    while( p = strtok( 0, ":" ) )
    {
      if( !v->HasMember( p ) )
      {
        error( "Error while digging: %s not found, property %s", p, propertyName ) ;
        free(str);
        return 0;
      }

      // otherwise,
      v = &(*v)[ p ] ;  // advance deeper into the tree
    }

    return v;
  }

  // You must know what type it is
  int getInt( const char* propertyName )
  {
    int val = getValue(propertyName)->GetInt();
    //info( Cyan, "json got value %s=%d", propertyName, val ) ;
    return val;
  }

  double getDouble( const char* propertyName )
  {
    double val = getValue(propertyName)->GetDouble();
    //info( Cyan, "json got value %s=%f", propertyName, val ) ;
    return val ;
  }

  string getString( const char* propertyName )
  {
    string val = getValue(propertyName)->GetString() ;
    //info( Cyan, "json got value %s=%s", propertyName, val.c_str() ) ;
    return val ;
  }

} ;

#endif
