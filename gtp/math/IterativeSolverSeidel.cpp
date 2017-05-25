#include "IterativeSolverSeidel.h"

IterativeSolverSeidel::IterativeSolverSeidel(
  const Eigen::MatrixXf *A_System,
  const Eigen::VectorXf *b_solution,
  Eigen::VectorXf * xVariable,
  real iTolerance
  ) : IterativeSolver( A_System, b_solution, xVariable, iTolerance )
{
  
}

IterativeSolverSeidel::~IterativeSolverSeidel()
{
  //don't delete xGuess
}

void IterativeSolverSeidel::solve() // override
{
  // only Jacobi and Gauss-Seidel have xNext.
  xNext = new Eigen::VectorXf( N ) ;
  xNext->setConstant( 0.0 ) ;// ON the first iteration, xNext starts out all 0's,
  // on subsequent iterations, xNext has to start as what
  // xGuess ended at on the previous iteration.
  xGuess->setConstant( 0.0 ) ; // start out with an initial xGuess of 0.

  residual = new Eigen::VectorXf( N ) ;

  updateResidual() ;
  norm2 = lastNorm2 = residual->dot( *residual ) ;

  // check first for diagonal dominance
  if( !isDiagonallyDominant( A ) )
    info( "Your matrix A is not diagonally dominant.  A solution is not guaranteed, but I'll probably manage." ) ;

  iteration = 0 ;
    
  while( solutionState != Converged &&
          solutionState != Diverged )
  {
    IterativeSolverSeidel::preIterate() ;
      
    // Go through the rows of A,
    // purpose:  update each of the rows
    // of xNext(i)
    for( curRow = 0 ; curRow < N ; curRow++ )
      IterativeSolverSeidel::iterate() ;

    IterativeSolverSeidel::postIterate() ;
  }

  info( "%d iterations, Gauss-Seidel method", iteration ) ;
  DESTROY( xNext ) ;
 // DESTROY( residual ) ;
  // don't delete xGuess it is the solution
}

void IterativeSolverSeidel::preIterate()
{
  iteration++ ;

  // on each "outer" iteration of Gauss-Seidel
  // recompute the entire vector, so, the entire
  // residual needs to be updated
  updateResidual() ;
}

void IterativeSolverSeidel::iterate() // override
{
  // now use that residual at each ROW i
  // (divided diagonal entry in Aii for that row)
  // to find the next "guess"
  double sum = 0.0 ;

  // do before i:  go from j=0 to (i-1)..
  // in Gauss-Seidel, this uses the xNext vector
  // (because those values are available)
  // and not the xGuess vector
  for( int j = 0 ; j < curRow ; j++ )
    sum += (*A)(curRow,j)*(*xNext)(j) ; // this line is the
  // only difference from the Jacobi method..
  // This small, small difference can almost
  // HALF the number of iterations required to solve.

  // no check for if( j==i ),
  // do after i:  go from j=(i+1) to (N-1)
  for( int j = curRow+1 ; j < N ; j++ )
    sum += (*A)(curRow,j)*(*xGuess)(j) ;

  (*xNext)(curRow) = ( (*b)(curRow) - sum ) / (*A)( curRow, curRow ) ;

  // you don't need to update the residual
  // every step with Gauss Seidel
  // (you will systematically iterate through)

#if ITERATIVE_SOLVERS_SHOW_PROGRESS_CONSOLE
  printf( "Seidel iteration %lld, norm2=%f\r", //typically does not reach more than 5-10 iterations
    iteration, getLastNorm() ) ;
#endif
}

void IterativeSolverSeidel::postIterate()
{
  // entire xNext vector will have been updated,
  // so use it
  *xGuess = *xNext ;

  checkConvergence() ;

  if( solutionState == IsDiverging )
    warning( "Solution actually diverging, iteration=%d, norm2=%f", iteration, norm2 ) ;
}
