#ifndef PROPERTIES_H
#define PROPERTIES_H

#include "../util/StdWilUtil.h"
#include <rapidjson/document.h>
#include <rapidjson/filestream.h>

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

    rapidjson::FileStream fs( f ) ;
    rapidjson::Reader jsonReader ;

    jsonDoc.ParseStream<0>( fs ) ;

    if( jsonDoc.HasParseError() )
    {
      error( "rapidjson: Couldn't parse json document, "
        "pos=%d, error=%s",
        jsonDoc.GetErrorOffset(),
        jsonDoc.GetParseError() ) ;
    }
    else if( !jsonDoc.IsObject() )
    {
      error( "JsonDoc not an object!" ) ;
    }

    fclose( f ) ;
  }

  /////rapidjson::Value* dig( char* propertyName )
  /////{
  /////  // you can't return a GenericValue because copy construction is forbidden
  /////  rapidjson::Value* v ;
  /////  return v ;
  /////}

  // You must know what type it is
  int getInt( const char* propertyName )
  {
    // prepare to dig, sh::props::bands looks for
    // "sh" : { "props" : { "bands" : 36 } }
    char *str = strdup( propertyName ) ;
    char *p = strtok( str, ":" ) ;

    // so, break the string up based on ::.
    // sh:bands = 63 is { "sh":{ "bands":63 } }
    rapidjson::GenericValue< rapidjson::UTF8<> > v ;
    
    // check top level document
    if( !jsonDoc.HasMember( p ) )
    {
      error( "json doc has no property '%s'", propertyName ) ;
      return 0 ;
    }
    
    v = jsonDoc[ p ] ;

    while( p = strtok( NULL, ":" ) )
    {
      if( !v.HasMember( p ) )
      {
        error( "Error while digging: %s not found, property %s", p, propertyName ) ;
        return 0 ;
      }

      // otherwise,
      v = v[ p ] ;  // advance deeper into the tree
    } ;

    int val = v.GetInt();
    //info( Cyan, "json got value %s=%d", propertyName, val ) ;
    free( str ) ;
    return val ;
  }

  double getDouble( const char* propertyName )
  {
    //if( !hasProperty( propertyName ) )  return 0.0 ;
    //else return jsonDoc[ propertyName ].GetDouble() ;
    // prepare to dig, sh::props::bands looks for
    // "sh" : { "props" : { "bands" : 36 } }
    char *str = strdup( propertyName ) ;
    char *p = strtok( str, ":" ) ;

    // so, break the string up based on ::.
    // sh:bands = 63 is { "sh":{ "bands":63 } }
    rapidjson::GenericValue< rapidjson::UTF8<> > v ;
    
    // check top level document
    if( !jsonDoc.HasMember( p ) )
    {
      error( "json doc has no property '%s'", propertyName ) ;
      return 0 ;
    }
    
    v = jsonDoc[ p ] ;

    while( p = strtok( NULL, ":" ) )
    {
      if( !v.HasMember( p ) )
      {
        error( "Error while digging: %s not found, property %s", p, propertyName ) ;
        return 0 ;
      }

      // otherwise,
      v = v[ p ] ;  // advance deeper into the tree
    } ;

    double val = v.GetDouble();
    info( Cyan, "json got value %s=%f", propertyName, val ) ;
    free( str ) ;
    return val ;
  }

  string getString( const char* propertyName )
  {
    //if( !hasProperty( propertyName ) )  return 0.0 ;
    //else return jsonDoc[ propertyName ].GetDouble() ;
    // prepare to dig, sh::props::bands looks for
    // "sh" : { "props" : { "bands" : 36 } }
    char *str = strdup( propertyName ) ;
    char *p = strtok( str, ":" ) ;

    // so, break the string up based on ::.
    // sh:bands = 63 is { "sh":{ "bands":63 } }
    rapidjson::GenericValue< rapidjson::UTF8<> > v ;
    
    // check top level document
    if( !jsonDoc.HasMember( p ) )
    {
      error( "json doc has no property '%s'", propertyName ) ;
      return 0 ;
    }
    
    v = jsonDoc[ p ] ;

    while( p = strtok( NULL, ":" ) )
    {
      if( !v.HasMember( p ) )
      {
        error( "Error while digging: %s not found, property %s", p, propertyName ) ;
        return 0 ;
      }

      // otherwise,
      v = v[ p ] ;  // advance deeper into the tree
    } ;

    string val = v.GetString() ;
    info( Cyan, "json got value %s=%s", propertyName, val.c_str() ) ;
    free( str ) ; // str was the duplicate.
    return val ;
  }

} ;

#endif
