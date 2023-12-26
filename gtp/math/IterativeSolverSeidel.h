#ifndef ITERATIVE_SOLVER_SEIDEL_H
#define ITERATIVE_SOLVER_SEIDEL_H

#include "IterativeSolver.h"

class IterativeSolver ;

class IterativeSolverSeidel : public IterativeSolver
{
  Eigen::VectorXf *xNext ;

public:
  IterativeSolverSeidel(
    const Eigen::MatrixXf *A_System,
    const Eigen::VectorXf *b_solution,
    Eigen::VectorXf * xVariable,
    real iTolerance
    ) ;
  ~IterativeSolverSeidel() ;

  void solve() ;

  void preIterate() ;

  void iterate() ;

  void postIterate() ;


} ;

#endif