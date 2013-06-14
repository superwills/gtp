#ifndef EIGENUTIL_H
#define EIGENUTIL_H

#include <stdio.h>

// unfortunately I clashed with
// std::real in #defining real
#ifdef real
#undef real
#endif
#include <Eigen/Eigen>
#define real double

#include "../util/StdWilUtil.h"

#include "IterativeSolver.h"
#include "IterativeSolverJacobi.h"
#include "IterativeSolverSeidel.h"
#include "IterativeSolverSouthwell.h"

void printMat( const Eigen::MatrixXf & mat ) ;



#endif