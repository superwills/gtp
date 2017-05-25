#include "IterativeSolverJacobi.h"

IterativeSolverJacobi::IterativeSolverJacobi(
  const Eigen::MatrixXf *A_System,
  const Eigen::VectorXf *b_solution,
  Eigen::VectorXf * xVariable,
  real iTolerance
  ) : IterativeSolver( A_System, b_solution, xVariable, iTolerance )
{

}

IterativeSolverJacobi::~IterativeSolverJacobi()
{
  // DO NOT delete xGuess. It should be maintained by
  // the client code
}

void IterativeSolverJacobi::solve() // override
{
  // only Jacobi and Gauss-Seidel have xNext.
  xNext = new Eigen::VectorXf( N ) ;
  xNext->setConstant( 0.0 ) ;
  xGuess->setConstant( 0.0 ) ;

  residual = new Eigen::VectorXf( N ) ;

  updateResidual() ;
  norm2 = lastNorm2 = residual->dot( *residual ) ;

  // check first for diagonal dominance
  if( !isDiagonallyDominant( A ) )
    warning( "Your matrix A is not diagonally dominant.  A solution is not guaranteed" ) ; //!! This warning should be removed

  iteration = 0 ;

  while( solutionState != Converged &&
          solutionState != Diverged )
  {
    IterativeSolverJacobi::preIterate() ;

    // step the solver.  for jacobi, this involves
    // a step through all the rows of A
    for( curRow = 0 ; curRow < N ; curRow++ )
      IterativeSolverJacobi::iterate() ; // encourage inlining

    IterativeSolverJacobi::postIterate() ;
  }

  info( "%d iterations, Jacobi method", iteration ) ;
  DESTROY( xNext ) ;
  DESTROY( residual ) ;
  // don't delete xGuess, that is the solution now
}

/// Operations to be performed
/// prior to an iteration
void IterativeSolverJacobi::preIterate()
{
  // major iteration increment
  iteration++ ;

  // The residual is updated
  updateResidual() ;
}

void IterativeSolverJacobi::iterate() // override
{
  // the definition for a single iteration
  // of the Jacobi method:
  real sum = 0.0 ;

  // for all j EXCEPT j!=i
  for( int j = 0 ; j < curRow ; j++ )
    sum += (*A)(curRow,j)*(*xGuess)(j) ;
  for( int j = curRow+1 ; j < N ; j++ )
    sum += (*A)(curRow,j)*(*xGuess)(j) ;

  (*xNext)(curRow) = ( (*b)(curRow) - sum ) /
                  (*A)( curRow, curRow ) ;

#if ITERATIVE_SOLVERS_SHOW_PROGRESS_CONSOLE
  printf( "Jacobi iteration %lld, norm2=%f\r",
    iteration, getLastNorm() ) ;
#endif
}

/// Ops to be performed after
/// an iteration
void IterativeSolverJacobi::postIterate()
{
  // now, our current guess is updated to be
  // what the iterating over rows above produced
  *xGuess = *xNext ;

  // Check if the solution
  // is converging or diverging
  checkConvergence() ;

  if( solutionState == IsDiverging )
    warning( "Solution actually diverging, iteration=%d, norm2=%f", iteration, norm2 ) ;
}