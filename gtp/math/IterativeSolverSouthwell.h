#ifndef ITERATIVE_SOLVER_SOUTHWELL_H
#define ITERATIVE_SOLVER_SOUTHWELL_H

#include "IterativeSolver.h"

class IterativeSolver ;

class IterativeSolverSouthwell : public IterativeSolver
{
public:
  IterativeSolverSouthwell(
    const Eigen::MatrixXf *A_System,
    const Eigen::VectorXf *b_solution,
    Eigen::VectorXf *xVariable,
    real iTolerance
    ) ;

  void solve() ;

  void preIterate() ;

  void iterate() ;

  void postIterate() ;

  
} ;

#endif