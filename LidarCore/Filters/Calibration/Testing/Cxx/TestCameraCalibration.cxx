#include "CameraCalibration.h"
#include "CameraProjection.h"
#include "vtkEigenTools.h"

#include <vtkTesting.h>

#include <Eigen/Dense>

//----------------------------------------------------------------------------
int TestFisheyeModelCalibration(std::string matchedFilename, std::string groundtruthFilename)
{
  // relatively high epsilon since the data are stored
  // in a .csv file with low digit
  double epsilon = 1e-2;

  // Load the 3D - 2D matches
  std::vector<Eigen::Vector3d> X;
  std::vector<Eigen::Vector2d> x;
  LoadMatchesFromCSV(matchedFilename, X, x);
  if (X.size() == 0)
  {
    return 0;
  }

  // Estimate the fisheye camera model parameters
  Eigen::Matrix<double, 3, 4> P;
  LinearPinholeCalibration(X, x, P);
  Eigen::Matrix3d R, K;
  Eigen::Vector3d T;
  CalibrationMatrixDecomposition(P, K, R, T);
  Eigen::Matrix<double, 11, 1> W;
  GetParametersFromMatrix(K, R, T, W);
  NonLinearPinholeCalibration(X, x, W);
  Eigen::Matrix<double, 15, 1> Wf = Eigen::Matrix<double, 15, 1>::Zero();
  for (int i = 0; i < 11; ++i)
  {
    Wf(i) = W(i);
  }
  NonLinearFisheyeCalibration(X, x, Wf);

  // Load the expected parameters and test
  Eigen::VectorXd Wg;
  LoadCameraParamsFromCSV(groundtruthFilename, Wg);
  for (int i = 0; i < 15; ++i)
  {
    if (std::abs(Wg(i) - Wf(i)) / std::abs(Wg(i)) > epsilon)
    {
      std::cout << "Expected: " << Wg(i) << " got: " << Wf(i) << std::endl;
      return 0;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
int TestCameraCalibration(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  std::string dataRoot = testing->GetDataRoot();

  int errors = 0;

  std::string fisheyeMatchesFilename = dataRoot + "/Camera/fisheye_camera.csv";
  std::string fisheyeGroundtruthFilename = dataRoot + "/Camera/FisheyeParamsExpected.csv";
  errors += TestFisheyeModelCalibration(fisheyeMatchesFilename, fisheyeGroundtruthFilename);

  return errors;
}
