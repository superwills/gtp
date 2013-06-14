#include "IndexMap.h"
#include "../geometry/CubeMap.h"

#define WRAP_CUBE_LEFT(row,col,ROWS,COLS,sp)     row,col-sp
#define WRAP_CUBE_RIGHT(row,col,ROWS,COLS,sp)    row,col+sp
#define WRAP_CUBE_UP(row,col,ROWS,COLS,sp)       row-sp,col
#define WRAP_CUBE_DOWN(row,col,ROWS,COLS,sp)     row+sp,col

#define WRAP_CUBE_LEFT_45(row,col,ROWS,COLS,sp)  row+sp,col-sp
#define WRAP_CUBE_RIGHT_45(row,col,ROWS,COLS,sp) row-sp,col+sp
#define WRAP_CUBE_UP_45(row,col,ROWS,COLS,sp)    row-sp,col-sp
#define WRAP_CUBE_DOWN_45(row,col,ROWS,COLS,sp)  row+sp,col+sp


// for the rail offsets,
// I need to say just how far to offset,
// without trying to find the final row/col.
#define RAIL_LEFT(sp)      0,-sp
#define RAIL_RIGHT(sp)     0,+sp
#define RAIL_UP(sp)       -sp,0
#define RAIL_DOWN(sp)     +sp,0

#define RAIL_LEFT_45(sp)  +sp,-sp
#define RAIL_RIGHT_45(sp) -sp,+sp
#define RAIL_UP_45(sp)    -sp,-sp
#define RAIL_DOWN_45(sp)  +sp,+sp

void IndexMap::generateRailedLattice( CubeMap* cubemap )
{
  info( "Building index map for railed lattice.." ) ;

  int ROWS=cubemap->pixelsPerSide-1, COLS=cubemap->pixelsPerSide-1 ;
  
  ////// WHEN I REDUCED IT TO 1 LIFT LEVEL THE ALGORITHM WORKED.
  ////// THERE MUST BE SOMETHING WRONG WITH SUBSEQUENT LIFT LEVELS.
  int maxLevel = 1 ; // log2( cubemap->pixelsPerSide ) - 1 ;   // IT IS -1 BECAUSE THE LAST LEVEL DOESN'T WORK (there are N-1 elts on the cube faces, not N)
  
  #pragma region test area
  // TEST
  //for( int rail=0 ; rail <= 0; rail++ )
  //{
  //  for( int along = 0 ; along < 4*256 ; along+=2 )
  //  {
  //      visits.push_back(
  //        LatticeVisit( cubemap->railedIndex(RAIL,rail,along), 1,
  //          cubemap->railedIndexMove( RAIL, rail, along, RAIL_UP_45(2) ),
  //          cubemap->railedIndexMove( RAIL, rail, along, RAIL_DOWN_45(2) ),
  //          cubemap->railedIndexMove( RAIL, rail, along, RAIL_LEFT_45(2) ),
  //          cubemap->railedIndexMove( RAIL, rail, along, RAIL_RIGHT_45(2) )
  //          )
  //        ) ;
  //  }
  //}
  //for( int face = PY ; face <= PY ; face++ )
  //{
  //  for( int row = 0 ; row < 1 ; row++ )
  //  {
  //    for( int col = 0 ; col < 255; col++ )
  //    {
  //      visits.push_back(
  //        LatticeVisit( cubemap->railedIndex(face,row,col), 1, 
  //          cubemap->railedIndexMove( face, row,col, RAIL_LEFT(1) ),
  //          0, 0, 0 )
  //        )
  //      ;
  //    }
  //  }
  //}
  //return ;
  #pragma endregion

  vector<LatticeVisit> *cv ; // current visit
  LatticeNormalization *cn ; // current normalization

  for( int level = 1 ; level <= maxLevel ; level++ )
  {
    // for lattice, skip is 2^level
    int skip = 1 << level ;
    int sp = skip / 2 ;

    // the current visit is 
    cv = new vector<LatticeVisit>() ;
    visits.push_back( cv ) ;

    // a lattice normalization is added too
    cn = new LatticeNormalization() ;
    normalizations.push_back( cn ) ;

    for( int FACE= -1 ; FACE <= 5 ; FACE++ )
    {
      #pragma region odd square lattice
      // MUST VISIT THE RAILS
      // HANDLE THE CORNERS DIFFERENTLY?
      // (SUM 3, DIVIDE BY 3 (use a 0 for entry 4) )
      if( FACE == RAIL )
      {
        // TOP,BOT RAIL
        for( int rail=0 ; rail < 2 ; rail++ )
          for( int along = sp ; along < cubemap->railGetRailSize( rail ) ; along+=skip )
          {
            int idx = cubemap->railedIndex(RAIL,rail,along) ;
            cv->push_back(
              LatticeVisit( idx, -.25, 
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_LEFT(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_RIGHT(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_UP(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_DOWN(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;// this value touched by this transform step,
            // must be included in normalization
          }
        
        // SIDE RAILS
        for( int rail=2 ; rail < 6 ; rail++ )
          // SIDE RAILS HAVE AN ODD FOR A FIRST ELT
          for( int along = sp-1 ; along < cubemap->railGetRailSize( rail ) ; along+=skip )
          {
            int idx = cubemap->railedIndex(RAIL,rail,along) ;
            cv->push_back(
              LatticeVisit( idx, -.25, 
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_LEFT(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_RIGHT(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_UP(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_DOWN(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
      }
      else
      {
        // ODD SQUARE:
        //TR box
        for( int row = skip-1 ; row < ROWS ; row+=skip )
          for( int col = sp-1; col < COLS ; col+=skip )
          {
            int idx = cubemap->railedIndex(FACE,row,col) ;
            cv->push_back(
              LatticeVisit( idx, -.25,
                cubemap->railedIndexMove( FACE, row, col, RAIL_LEFT(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_RIGHT(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_UP(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_DOWN(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
        //BL box
        for( int row = sp-1 ; row < ROWS ; row+=skip )
          for( int col = skip-1 ; col < COLS ; col+=skip )
          {
            int idx = cubemap->railedIndex(FACE,row,col) ;
            cv->push_back(
              LatticeVisit( idx, -.25,
                cubemap->railedIndexMove( FACE, row, col, RAIL_LEFT(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_RIGHT(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_UP(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_DOWN(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
      }
      #pragma endregion
    }

    // You need to power correct here.
    // create a NEW group
    cv = new vector<LatticeVisit>() ;
    visits.push_back( cv ) ;
    cn = new LatticeNormalization() ;
    normalizations.push_back( cn ) ;

    for( int FACE= -1 ; FACE <= 5 ; FACE++ )
    {
      #pragma region even square lattice
      if( FACE == RAIL )
      {
        ///////TOP,BOT
        for( int rail=0 ; rail < 2 ; rail++ )
          // walk ALONG the rail.
          for( int along = 0 ; along < cubemap->railGetRailSize( rail ) ; along+=skip )
          {
            int idx = cubemap->railedIndex(RAIL,rail,along);
            cv->push_back( 
              LatticeVisit( idx, .125, 
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_LEFT(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_RIGHT(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_UP(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_DOWN(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
        // SIDE
        for( int rail=2 ; rail < 6 ; rail++ )
          for( int along = skip-1 ; along < cubemap->railGetRailSize( rail ) ; along+=skip )
          {
            int idx = cubemap->railedIndex(RAIL,rail,along) ;
            cv->push_back( 
              LatticeVisit( idx, .125, 
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_LEFT(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_RIGHT(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_UP(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_DOWN(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
      }
      else
      {
        // EVEN square
        // TL box
        for( int row = sp-1 ; row < ROWS ; row+=skip )
          for( int col = sp-1 ; col < COLS ; col+=skip )
          {
            int idx = cubemap->railedIndex(FACE,row,col);
            cv->push_back(
              LatticeVisit( idx, .125,
                cubemap->railedIndexMove( FACE, row, col, RAIL_LEFT(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_RIGHT(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_UP(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_DOWN(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
        // BR box
        for( int row = skip-1 ; row < ROWS ; row+=skip )
          for( int col = skip-1 ; col < COLS ; col+=skip )
          {
            int idx = cubemap->railedIndex(FACE,row,col);
            cv->push_back(
              LatticeVisit( idx, .125,
                cubemap->railedIndexMove( FACE, row, col, RAIL_LEFT(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_RIGHT(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_UP(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_DOWN(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
      }
      #pragma endregion
    }

    cv = new vector<LatticeVisit>() ;
    visits.push_back( cv ) ;
    cn = new LatticeNormalization() ;
    normalizations.push_back( cn ) ;

    for( int FACE= -1 ; FACE <= 5 ; FACE++ )
    {
      #pragma region 45 odd 
      if( FACE == RAIL )
      {
        //////// IN THE 45 LATTICE THE RAIL IS NONE!! of them
      }
      else
      {
        // the blacks and whites are actually grid aligned now.
        // 45 ODD
        for( int row = sp-1 ; row < ROWS ; row+=skip )
          for( int col = sp-1 ; col < COLS ; col+=skip )
          {
            int idx = cubemap->railedIndex(FACE,row,col);
            cv->push_back(
              LatticeVisit( idx, -.25,
                cubemap->railedIndexMove( FACE, row, col, RAIL_LEFT_45(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_RIGHT_45(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_UP_45(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_DOWN_45(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
      }
      #pragma endregion
    }

    cv = new vector<LatticeVisit>() ;
    visits.push_back( cv ) ;
    cn = new LatticeNormalization() ;
    normalizations.push_back( cn ) ;

    for( int FACE= -1 ; FACE <= 5 ; FACE++ )
    {
      #pragma region 45 even
      // 45 EVEN
      // TL
      if( FACE == RAIL )
      {
        // All even on 45 rail, but must skip/offset differently
        // on top/siderails.
        for( int rail=0 ; rail < 2 ; rail++ )
          for( int along = 0 ; along < cubemap->railGetRailSize( rail ) ; along+=skip )
          {
            int idx = cubemap->railedIndex(RAIL,rail,along) ;
            cv->push_back( 
              LatticeVisit( idx, .125, 
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_LEFT_45(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_RIGHT_45(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_UP_45(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_DOWN_45(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }

        for( int rail=2 ; rail < 6 ; rail++ )
          for( int along = skip-1 ; along < cubemap->railGetRailSize( rail ) ; along+=skip )
          {
            int idx = cubemap->railedIndex(RAIL,rail,along);
            cv->push_back( 
              LatticeVisit( idx, .125, 
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_LEFT_45(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_RIGHT_45(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_UP_45(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_DOWN_45(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
      }
      else
      {
        for( int row = skip-1 ; row < ROWS ; row+=skip )
          for( int col = skip-1 ; col < COLS ; col+=skip )
          {
            int idx = cubemap->railedIndex(FACE,row,col) ;
            cv->push_back(
              LatticeVisit( idx, .125,
                cubemap->railedIndexMove( FACE, row, col, RAIL_LEFT_45(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_RIGHT_45(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_UP_45(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_DOWN_45(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
      }
      #pragma endregion
    }
  }









  //               _ _  __ _       
  //              | (_)/ _| |      
  //   _   _ _ __ | |_| |_| |_ ___ 
  //  | | | | '_ \| | |  _| __/ __|
  //  | |_| | | | | | | | | |_\__ \
  //   \__,_|_| |_|_|_|_|  \__|___/
  // 
  // UNLIFTS
  // for lattice, skip is 2^level
  for( int level = maxLevel ; level >= 1 ; level-- )
  {
    int skip = 1 << level ;
    int sp = skip / 2 ;
    
    cv = new vector<LatticeVisit>() ;
    unvisits.push_back( cv ) ;
    cn = new LatticeNormalization() ;
    unnormalizations.push_back( cn ) ;

    for( int FACE=5; FACE >= -1; FACE-- )
    {
      #pragma region 45 even
      // 45 EVEN
      // TL
      if( FACE == RAIL )
      {
        /// ALL EVEN on 45 rail
        for( int rail=0 ; rail < 2 ; rail++ )
          for( int along = 0 ; along < cubemap->railGetRailSize( rail ) ; along+=skip )
          {
            int idx = cubemap->railedIndex(RAIL,rail,along);
            cv->push_back( 
              LatticeVisit( idx, -.125, 
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_LEFT_45(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_RIGHT_45(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_UP_45(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_DOWN_45(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }

        for( int rail=2 ; rail < 6 ; rail++ )
          for( int along = skip-1 ; along < cubemap->railGetRailSize( rail ) ; along+=skip )
          {
            int idx = cubemap->railedIndex(RAIL,rail,along);
            cv->push_back( 
              LatticeVisit( idx, -.125, 
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_LEFT_45(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_RIGHT_45(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_UP_45(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_DOWN_45(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
      }
      else
      {
        for( int row = skip-1 ; row < ROWS ; row+=skip )
          for( int col = skip-1 ; col < COLS ; col+=skip )
          {
            int idx = cubemap->railedIndex(FACE,row,col) ;
            cv->push_back(
              LatticeVisit( idx, -.125,
                cubemap->railedIndexMove( FACE, row, col, RAIL_LEFT_45(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_RIGHT_45(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_UP_45(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_DOWN_45(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
      }
      #pragma endregion
    }

    cv = new vector<LatticeVisit>() ;
    unvisits.push_back( cv ) ;
    cn = new LatticeNormalization() ;
    unnormalizations.push_back( cn ) ;

    for( int FACE=5; FACE >= -1; FACE-- )
    {
      #pragma region 45 odd 
      if( FACE == RAIL )
      {
        
      }
      else
      {
        // the blacks and whites are actually grid aligned now.
        // 45 ODD
        for( int row = sp-1 ; row < ROWS ; row+=skip )
          for( int col = sp-1 ; col < COLS ; col+=skip )
          {
            int idx = cubemap->railedIndex(FACE,row,col) ;
            cv->push_back(
              LatticeVisit( idx, .25,
                cubemap->railedIndexMove( FACE, row, col, RAIL_LEFT_45(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_RIGHT_45(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_UP_45(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_DOWN_45(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
      }
      #pragma endregion
    }

    cv = new vector<LatticeVisit>() ;
    unvisits.push_back( cv ) ;
    cn = new LatticeNormalization() ;
    unnormalizations.push_back( cn ) ;

    for( int FACE=5; FACE >= -1; FACE-- )
    {
      #pragma region even square lattice
      if( FACE == RAIL )
      {
        //TOP,BOT
        for( int rail=0 ; rail < 2 ; rail++ )
          // walk ALONG the rail.
          for( int along = 0 ; along < cubemap->railGetRailSize( rail ) ; along+=skip )
          {
            int idx = cubemap->railedIndex(RAIL,rail,along) ;
            cv->push_back( 
              LatticeVisit( idx, -.125, 
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_LEFT(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_RIGHT(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_UP(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_DOWN(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
        
        // SIDE
        for( int rail=2 ; rail < 6 ; rail++ )
          // walk ALONG the rail.
          for( int along = skip-1 ; along < cubemap->railGetRailSize( rail ) ; along+=skip )
          {
            int idx = cubemap->railedIndex(RAIL,rail,along);
            cv->push_back( 
              LatticeVisit( idx, -.125, 
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_LEFT(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_RIGHT(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_UP(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_DOWN(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
      }
      else
      {
        // EVEN square
        // TL box
        for( int row = sp-1 ; row < ROWS ; row+=skip )
          for( int col = sp-1 ; col < COLS ; col+=skip )
          {
            int idx = cubemap->railedIndex(FACE,row,col);
            cv->push_back(
              LatticeVisit( idx, -.125,
                cubemap->railedIndexMove( FACE, row, col, RAIL_LEFT(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_RIGHT(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_UP(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_DOWN(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
        // BR box
        for( int row = skip-1 ; row < ROWS ; row+=skip )
          for( int col = skip-1 ; col < COLS ; col+=skip )
          {
            int idx = cubemap->railedIndex(FACE,row,col);
            cv->push_back(
              LatticeVisit( idx, -.125,
                cubemap->railedIndexMove( FACE, row, col, RAIL_LEFT(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_RIGHT(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_UP(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_DOWN(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
      }
      #pragma endregion
    }

    cv = new vector<LatticeVisit>() ;
    unvisits.push_back( cv ) ;
    cn = new LatticeNormalization() ;
    unnormalizations.push_back( cn ) ;

    for( int FACE=5; FACE >= -1; FACE-- )
    {
      #pragma region odd square lattice
      // ODD SQUARE:
      //TR box
      if( FACE == RAIL )
      {
        // TOP,BOT RAIL
        for( int rail=0 ; rail < 2 ; rail++ )
          for( int along = sp ; along < cubemap->railGetRailSize( rail ) ; along+=skip )
          {
            int idx = cubemap->railedIndex(RAIL,rail,along);
            cv->push_back(
              LatticeVisit( idx, .25, 
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_LEFT(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_RIGHT(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_UP(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_DOWN(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
        
        // SIDE RAILS
        for( int rail=2 ; rail < 6 ; rail++ )
          for( int along = sp-1 ; along < cubemap->railGetRailSize( rail ) ; along+=skip )
          {
            int idx = cubemap->railedIndex(RAIL,rail,along) ;
            cv->push_back(
              LatticeVisit( idx, .25, 
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_LEFT(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_RIGHT(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_UP(sp) ),
                cubemap->railedIndexMove( RAIL, rail, along, RAIL_DOWN(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
      }
      else
      {
        //ODD SQUARE TR box
        for( int row = skip-1 ; row < ROWS ; row+=skip )
          for( int col = sp-1 ; col < COLS ; col+=skip )
          {
            int idx = cubemap->railedIndex(FACE,row,col);
            cv->push_back(
              LatticeVisit( idx, .25,
                cubemap->railedIndexMove( FACE, row, col, RAIL_LEFT(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_RIGHT(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_UP(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_DOWN(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
        
        //BL box
        for( int row = sp-1 ; row < ROWS ; row+=skip )
          for( int col = skip-1 ; col < COLS ; col+=skip )
          {
            int idx = cubemap->railedIndex(FACE,row,col) ;
            cv->push_back(
              LatticeVisit( idx, .25,
                cubemap->railedIndexMove( FACE, row, col, RAIL_LEFT(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_RIGHT(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_UP(sp) ),
                cubemap->railedIndexMove( FACE, row, col, RAIL_DOWN(sp) )
              ) ) ;
            cn->indices.push_back( idx ) ;
          }
      }
      #pragma endregion
    }
  }// end unlifts
}


#if 0
void IndexMap::generateLattice( CubeMap* cubemap )
{
  info( "Building index map for regular lattice.." ) ;

  int rows=cubemap->pixelsPerSide, cols=cubemap->pixelsPerSide ;
  
  int maxLevel = log2( rows ) ;

  // for lattice, skip is 2^level
  for( int FACE=0; FACE < 6 ; FACE++ )
  {
    for( int level = 1 ; level <= maxLevel ; level++ )
    {
      int skip = 1 << level ;
      int sp = skip / 2 ;

      #pragma region odd square lattice
      // ODD SQUARE:
      //TR box
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          visits.push_back(
            LatticeVisit( cubemap->index(FACE,row,col), -.25,
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_LEFT(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_RIGHT(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_UP(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_DOWN(row,col,rows,cols,sp) )
            ) ) ;

      //BL box
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          visits.push_back(
            LatticeVisit( cubemap->index(FACE,row,col), -.25,
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_LEFT(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_RIGHT(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_UP(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_DOWN(row,col,rows,cols,sp) )
            ) ) ;
      #pragma endregion
    
      #pragma region even square lattice
      // EVEN square
      // TL box
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          visits.push_back(
            LatticeVisit( cubemap->index(FACE,row,col), .125,
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_LEFT(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_RIGHT(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_UP(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_DOWN(row,col,rows,cols,sp) )
            ) ) ;
      // BR box
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          visits.push_back(
            LatticeVisit( cubemap->index(FACE,row,col), .125,
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_LEFT(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_RIGHT(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_UP(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_DOWN(row,col,rows,cols,sp) )
            ) ) ;
      #pragma endregion
    
      #pragma region 45 odd 
      // the blacks and whites are actually grid aligned now.
      // 45 ODD
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          visits.push_back(
            LatticeVisit( cubemap->index(FACE,row,col), -.25,

              cubemap->getWrappedIndex( FACE, WRAP_CUBE_LEFT_45(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_RIGHT_45(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_UP_45(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_DOWN_45(row,col,rows,cols,sp) )
            
            )
          ) ;
          
      #pragma endregion
    
      #pragma region 45 even
      // 45 EVEN
      // TL
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          visits.push_back(
            LatticeVisit( cubemap->index(FACE,row,col), .125,

              cubemap->getWrappedIndex( FACE, WRAP_CUBE_LEFT_45(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_RIGHT_45(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_UP_45(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_DOWN_45(row,col,rows,cols,sp) )
            
            )
          ) ;
      #pragma endregion
    
    }
  }





  // UNLIFTS
  // for lattice, skip is 2^level
  for( int level = maxLevel ; level >= 1 ; level-- )
  {
    for( int FACE=5; FACE >= 0 ; FACE-- )
    {
      int skip = 1 << level ;
      int sp = skip / 2 ;

      #pragma region 45 even
      // 45 EVEN
      // TL
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          unvisits.push_back(
            LatticeVisit( cubemap->index(FACE,row,col), -.125,

              cubemap->getWrappedIndex( FACE, WRAP_CUBE_LEFT_45(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_RIGHT_45(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_UP_45(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_DOWN_45(row,col,rows,cols,sp) )
            
            )
          ) ;
      #pragma endregion

      #pragma region 45 odd 
      // the blacks and whites are actually grid aligned now.
      // 45 ODD
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          unvisits.push_back(
            LatticeVisit( cubemap->index(FACE,row,col), .25,

              cubemap->getWrappedIndex( FACE, WRAP_CUBE_LEFT_45(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_RIGHT_45(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_UP_45(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_DOWN_45(row,col,rows,cols,sp) )
            
            )
          ) ;
          
      #pragma endregion

      #pragma region even square lattice
      // EVEN square
      // TL box
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          unvisits.push_back(
            LatticeVisit( cubemap->index(FACE,row,col), -.125,

              cubemap->getWrappedIndex( FACE, WRAP_CUBE_LEFT(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_RIGHT(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_UP(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_DOWN(row,col,rows,cols,sp) )
            
            )
          ) ;
      // BR box
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          unvisits.push_back(
            LatticeVisit( cubemap->index(FACE,row,col), -.125,

              cubemap->getWrappedIndex( FACE, WRAP_CUBE_LEFT(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_RIGHT(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_UP(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_DOWN(row,col,rows,cols,sp) )
            
            )
          ) ;
      #pragma endregion

      #pragma region odd square lattice
      // ODD SQUARE:
      //TR box
      for( int row = 0 ; row < rows ; row+=skip )
        for( int col = sp ; col < cols ; col+=skip )
          unvisits.push_back(
            LatticeVisit( cubemap->index(FACE,row,col), .25,

              cubemap->getWrappedIndex( FACE, WRAP_CUBE_LEFT(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_RIGHT(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_UP(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_DOWN(row,col,rows,cols,sp) )
            
            )
          ) ;

      //BL box
      for( int row = sp ; row < rows ; row+=skip )
        for( int col = 0 ; col < cols ; col+=skip )
          unvisits.push_back(
            LatticeVisit( cubemap->index(FACE,row,col), .25,

              cubemap->getWrappedIndex( FACE, WRAP_CUBE_LEFT(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_RIGHT(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_UP(row,col,rows,cols,sp) ),
              cubemap->getWrappedIndex( FACE, WRAP_CUBE_DOWN(row,col,rows,cols,sp) )
            
            )
          ) ;
      #pragma endregion
    
      
    }
  }// end unlifts

}

#endif