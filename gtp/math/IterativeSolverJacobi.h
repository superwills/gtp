#ifndef ITERATIVE_SOLVER_JACOBI_H
#define ITERATIVE_SOLVER_JACOBI_H

#include "IterativeSolver.h"

class IterativeSolver ;

class IterativeSolverJacobi : public IterativeSolver
{
  Eigen::VectorXf *xNext ;

public:
  IterativeSolverJacobi(
    const Eigen::MatrixXf *A_System,
    const Eigen::VectorXf *b_solution,
    Eigen::VectorXf * xVariable,
    real iTolerance
    ) ;
  virtual ~IterativeSolverJacobi() ;

  void solve() ;

  /// Operations to be performed
  /// prior to an iteration
  void preIterate() ;

  void iterate() ;

  /// Ops to be performed after
  /// an iteration
  void postIterate() ;

} ;

#endif