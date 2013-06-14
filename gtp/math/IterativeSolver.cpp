#include "IterativeSolver.h"

IterativeSolver::IterativeSolver( const Eigen::MatrixXf * A_System,
  const Eigen::VectorXf * b_solution,
  Eigen::VectorXf * xVariable,
  real iTolerance )
{
  A = A_System ;
  N = A->rows() ;

  b = b_solution ;
  xGuess = xVariable ;

  tolerance = iTolerance ;

  solutionState = Indeterminate ;

  iteration = 0 ;
  curRow = 0 ;

  norm2 = lastNorm2 = 0.0 ;
}

real IterativeSolver::getLastNorm() { return norm2 ; }

/// Gives you the estimated
/// solution vector x as computed
/// by the iterative solver
Eigen::VectorXf* IterativeSolver::getSolutionX()
{
  return xGuess ;
}

/// The "residual" is a vector that
/// measures "how far off" our xG
/// currently is from the true solution
/// that should produce A*xG - b = 0
void IterativeSolver::updateResidual()
{
  // if the xGuess IS a solution, then A*xGuess IS b.
  *residual = ((*A)*(*xGuess)) - (*b) ;
  // if this is the zero vector then the approximation
  // xGuess is perfect
}

IterativeSolver::SolutionState IterativeSolver::checkConvergence() // does not return early
{
  // Method 1:  compare every element
  ////return residual.isApproxToConstant( 0.0, tolerance ) ;

  // Method 2:  check the mag^2 of the
  // residual vector.  This is better
  // as it gives us an indication whether
  // the solution is converging or diverging
  // (mag^2 will be consistently larger than
  //  last iteration)
  norm2 = residual->dot( *residual ) ;

  if( IsNaN( norm2 ) )
  {
    // If it diverges something's gonna blow up..
    error( "%s failed to solve on iteration %d with NaN", iteration,
      typeid(*this).name() ) ;

    solutionState = Diverged ;
  }
  else if( IsNear( norm2, 0, tolerance ) )
    solutionState = Converged ;
  else
  {
    // here we can detect if the solution is
    // converging or diverging
    if( norm2 < lastNorm2 )
      solutionState = IsConverging ;
    else if( norm2 == lastNorm2 )
      solutionState = Indeterminate ;
    else
      solutionState = IsDiverging ;
  }

  // finally, update the value of the last
  // norm err.
  lastNorm2 = norm2 ;
  return solutionState ;
}

/// Returns true if a Eigen::MatrixXf is
/// diagonally dominant.  DD is
/// a sufficient condition for
/// convergence with the Gauss-Seidel method.
bool IterativeSolver::isDiagonallyDominant( const Eigen::MatrixXf * mat )
{
  return isDiagonallyDominant( mat, 0 ) ;
}

/// Tells you if the matrix is DD,
/// and returns the first row that
/// was not DD
bool IterativeSolver::isDiagonallyDominant( const Eigen::MatrixXf * mat, int *rowNotDD )
{
  if( mat->rows() != mat->cols() )
    return false ; // non-square

  for( int i = 0 ; i < mat->rows() ; i++ )
  {
    real rowSum = 0.0 ;
    for( int j = 0 ; j < mat->cols() ; j++ )
    {
      if( j == i )
        continue ;
      else
        rowSum += fabs( (*mat)( i, j ) ) ;
    }

    if( fabs( (*mat)(i,i) ) < rowSum )
    {
      // give back the row# that was not DD
      if( rowNotDD )  *rowNotDD = i ; 
      return false ;
    }
  }

  return true ;
}