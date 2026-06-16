/*=========================================================================

  Program:   LidarView
  Module:    HungarianAlgorithm.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef HUNGARIAN_ALGORITHM_H
#define HUNGARIAN_ALGORITHM_H

#include <Eigen/Dense>
#include <limits>
#include <vector>

#include "lvCommonCoreModule.h"

namespace HungarianAlgorithm
{
/**
 * A utility function to solve hungarian association problem.
 * https://en.wikipedia.org/wiki/Hungarian_algorithm
 */
void LVCOMMONCORE_EXPORT Solve(const Eigen::MatrixXd& costMatrix,
  std::vector<int>& assignmentRows,
  std::vector<int>& assignmentCols);
}

#endif