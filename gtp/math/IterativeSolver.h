#ifndef ITERATIVE_SOLVER_H
#define ITERATIVE_SOLVER_H

#ifdef real
#undef real
#endif
#include <Eigen/Eigen>
#define real double

#include "../Globals.h"
#include "../util/StdWilUtil.h"

class IterativeSolver
{
public:
  enum SolutionState
  {
    /// Solver hasn't even
    /// started yet
    Indeterminate,

    /// The solution is in progress
    /// and has neither converged nor
    /// diverged yet, but it looks
    /// like it is converging as the
    /// error is decreasing
    IsConverging,

    /// The solution has completed
    /// and we have a good "x" solution
    Converged,

    /// The solution is going backwards
    /// and error is increasing with each
    /// iteration
    IsDiverging,

    /// Diverged.  Got an infinity somewhere.
    /// The iterative solver 
    /// selected cannot actually solve
    /// the system specified
    Diverged
  } ;

  SolutionState solutionState ;

protected:
  /// The main system to solve.
  /// A copy of the system A is
  /// NOT made, because that's a
  /// huge waste of memory.
  /// The IterativeSolver
  /// series of classes will never
  /// modify the system A
  const Eigen::MatrixXf * A ;
  
  /// The solution vector that we
  /// want A*x = b.
  const Eigen::VectorXf * b ;

public:
  /// A has N rows, the solution vector
  /// has N columns.  Read in ctor, all base.
  int N ;

  /// 'curRow' is an important state variable
  /// its the row we are currently relaxing
  long long curRow ; // this is public so
  // the external solver can access it

  /// 'iteration' is different from I.
  /// It is the MAJOR outer loop iteration.
  /// For Gauss-Seidel and Jacobi, one iteration
  /// constitutes a FULL PASS through the
  /// all the rows of the radiosity matrix,
  /// but for the Southwell algorithm, its
  /// relaxation of a single row of the
  /// matrix.  At each "iteration", the
  /// Southwell needs to find the maximal error
  /// row, so, its concept of an "iteration" is
  /// different.
  long long iteration ; // long long.. support a huge number of iterations!

  /// how far each element may be from 0.0
  /// in order for xGuess to be close enough
  /// to the true value of "x" for us to call
  /// it quits on the iterative process
  /// Solving to an iterative solver
  /// necessarily is accompanied by
  /// "to what precision"?
  real tolerance ;
  
  /// Every iterative technique has
  /// a residual that needs to be
  /// updated after each _major_ iteration
  Eigen::VectorXf* residual ;

  /// xGuess -- our progressively
  /// (iteratively) refined estimate
  /// of what "x" should be
  Eigen::VectorXf* xGuess ;

  /// Maintain copies of the error norms
  real norm2, lastNorm2 ;

public:
  // xVariable is the memory space to put the unknown 'x'
  // in Ax=b.
  IterativeSolver( const Eigen::MatrixXf * A_System,
    const Eigen::VectorXf * b_solution,
    Eigen::VectorXf * xVariable,
    real iTolerance ) ;

  virtual ~IterativeSolver() {}

  real getLastNorm() ;

  /// Gives you the estimated
  /// solution vector x as computed
  /// by the iterative solver
  Eigen::VectorXf* getSolutionX() ;

  /// The "residual" is a vector that
  /// measures "how far off" our xG
  /// currently is from the true solution
  /// that should produce A*xG - b = 0
  void updateResidual() ;

  SolutionState checkConvergence() ;

  /// Returns true if a Eigen::MatrixXf is
  /// diagonally dominant.  DD is
  /// a sufficient condition for
  /// convergence with the Gauss-Seidel method.
  bool isDiagonallyDominant( const Eigen::MatrixXf * mat ) ;

  /// Tells you if the matrix is DD,
  /// and returns the first row that
  /// was not DD
  bool isDiagonallyDominant( const Eigen::MatrixXf * mat, int *rowNotDD ) ;

  /// Completely solves the system to
  /// the tolerance precision you have
  /// specified in the ctor call
  virtual void solve() = 0 ;

  /// Things to be done prior to an
  /// iterate() call
  virtual void preIterate() = 0 ;

  /// Performs an actual iteration
  virtual void iterate() = 0 ;

  /// Things to be done following an
  /// iterate call()
  virtual void postIterate() = 0 ;

} ;

#endif