/*=========================================================================

  Program:   LidarView
  Module:    HungarianAlgorithm.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "HungarianAlgorithm.h"
//------------------------------------------------------------------------------
void HungarianAlgorithm::Solve(const Eigen::MatrixXd& costMatrix,
  std::vector<int>& assignmentRows,
  std::vector<int>& assignmentCols)
{
  int nRows = costMatrix.rows();
  int nCols = costMatrix.cols();
  int n = std::max(nRows, nCols);

  Eigen::MatrixXd cost = Eigen::MatrixXd::Zero(n, n);
  cost.block(0, 0, nRows, nCols) = costMatrix;

  std::vector<double> u(n + 1), v(n + 1);
  std::vector<int> p(n + 1), way(n + 1);

  for (int i = 1; i <= n; ++i)
  {
    p[0] = i;
    int j0 = 0;
    std::vector<double> minv(n + 1, std::numeric_limits<double>::infinity());
    std::vector<char> used(n + 1, false);

    do
    {
      used[j0] = true;
      int i0 = p[j0], j1 = 0;
      double delta = std::numeric_limits<double>::infinity();

      for (int j = 1; j <= n; ++j)
      {
        if (!used[j])
        {
          double cur = cost(i0 - 1, j - 1) - u[i0] - v[j];
          if (cur < minv[j])
          {
            minv[j] = cur;
            way[j] = j0;
          }
          if (minv[j] < delta)
          {
            delta = minv[j];
            j1 = j;
          }
        }
      }

      for (int j = 0; j <= n; ++j)
      {
        if (used[j])
        {
          u[p[j]] += delta;
          v[j] -= delta;
        }
        else
        {
          minv[j] -= delta;
        }
      }

      j0 = j1;
    } while (p[j0] != 0);

    do
    {
      int j1 = way[j0];
      p[j0] = p[j1];
      j0 = j1;
    } while (j0);
  }

  assignmentRows.clear();
  assignmentCols.clear();

  for (int j = 1; j <= nCols; ++j)
  {
    if (p[j] <= nRows)
    {
      assignmentRows.push_back(p[j] - 1);
      assignmentCols.push_back(j - 1);
    }
  }
}