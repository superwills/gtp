#include "IterativeSolverSouthwell.h"

IterativeSolverSouthwell::IterativeSolverSouthwell(
  const Eigen::MatrixXf *A_System,
  const Eigen::VectorXf *b_solution,
  Eigen::VectorXf *xVariable,
  real iTolerance
  ) : IterativeSolver( A_System, b_solution, xVariable, iTolerance )
{
    
}

void IterativeSolverSouthwell::solve() // override
{
  xGuess->setConstant( 0.0 ) ; // set the initial guess for x to 0

  residual = new Eigen::VectorXf( N ) ;

  updateResidual() ; // find the starting error
  norm2 = lastNorm2 = residual->dot( *residual ) ;

  iteration = 0 ;

  while( solutionState != Converged &&
          solutionState != Diverged )
  {
    IterativeSolverSouthwell::preIterate() ;
      
    // Different than Gauss-Seidel iteration..
    IterativeSolverSouthwell::iterate() ;

    IterativeSolverSouthwell::postIterate() ;
  }

  info( "%d iterations, Southwell element method", iteration );
  //DESTROY( residual );
}

void IterativeSolverSouthwell::preIterate()
{
  // major iteration increment
  iteration++ ;

  // Update the residual.  you have to update the
  // whole residual since xGuess(i) is "dirty".
  updateResidual() ;
}

void IterativeSolverSouthwell::iterate() // override  "'override' cannot be used with 'inline'
{
  // Search for the largest residue value.
  // That is the element to relax.
  // use ABSOLUTE VALUE of residue values
  
  // Southwell has a bit more
  // recomputing of the residual vector,
  // because it HAS TO look for the largest
  // present residual on each iteration.
  curRow = 0 ; // index of row with max err
  double maxVal = 0.0 ;
  for( int i = 0 ; i < N ; i++ )
  {
    if( fabs( (*residual)( i ) ) > maxVal )
    {
      maxVal = fabs( (*residual)(i) ) ;
      curRow = i ; // max index occurs @k
    }
  }

  double sum = 0.0 ;

  // for j!=i:
  for( int j = 0 ; j < curRow ; j++ )
    sum += (*A)(curRow,j)*(*xGuess)(j) ;
    
  for( int j = curRow+1 ; j < N ; j++ )
    sum += (*A)(curRow,j)*(*xGuess)(j) ;

  // update xGuess(i).  Each call to this function
  // only does ONE (i) UPDATE though, because (i)
  // is found each time as the row with the largest error.
  (*xGuess)(curRow) = ( (*b)(curRow) - sum ) / (*A)( curRow, curRow ) ;

#if ITERATIVE_SOLVERS_SHOW_PROGRESS_CONSOLE
  if( ! (iteration % 10) )
    printf( "Southwell iteration %lld, norm2=%f\r",
      iteration, getLastNorm() ) ;
#endif
}

void IterativeSolverSouthwell::postIterate()
{
  checkConvergence() ;
  if( solutionState == IsDiverging )
    warning( "Solution actually diverging, iteration=%d, norm2=%f", iteration, norm2 ) ;
}
