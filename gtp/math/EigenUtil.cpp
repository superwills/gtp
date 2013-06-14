#include "EigenUtil.h"



void printMat( const Eigen::MatrixXf & mat )
{
  warning( "Printing %dx%d matrix to file, this could take a while...", mat.rows(), mat.cols() ) ;

  plainFile( "\n\ntrix = [\n" ) ;
  for( int row = 0; row < mat.rows() ; row++ )
  {
    for( int col = 0 ; col < mat.cols() ; col++ )
      plainFile( "%12.8f ", mat(row,col) ) ;

    plainFile( " ;\n" ) ; //EOL
  }
  plainFile( "] ;\n\n" ) ;
}

