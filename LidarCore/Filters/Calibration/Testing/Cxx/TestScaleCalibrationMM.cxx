#include "vtkEigenTools.h"
#include "vtkGeometricCalibration.h"
#include "vtkTemporalTransformsReader.h"
#include "vtkTimeCalibration.h"

#include <vtkMath.h>
#include <vtkTesting.h>

int TestScaleCalibrationMM(int argc, char* argv[])
{
  // number of error in the test
  int errors = 0;
  const double mm03_gt = -399618.0; // ground truth quality (+- 1e-4)
  const double mm04_gt = -252018.0; // ground truth quality (+- 1e-4)

  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  std::string dataRoot = testing->GetDataRoot();

  // get command line filenames parameters
  std::string referenceFile;
  std::string alignedFile;
  vtkSmartPointer<vtkTemporalTransforms> complete_r, complete_a, r, a;

  referenceFile = dataRoot + "/trajectories/mm03/imu.csv";
  alignedFile = dataRoot + "/trajectories/mm03/lidar-slam.csv";
  complete_r = vtkTemporalTransformsReader::OpenTemporalTransforms(referenceFile);
  r = complete_r->ExtractTimes(400274, 400274 + 180)->Subsample(10);
  complete_a = vtkTemporalTransformsReader::OpenTemporalTransforms(alignedFile);
  a = complete_a->ExtractTimes(610, 610 + 180)->ApplyTimeshift(-mm03_gt);
  ;

  errors += std::abs(1.0 - ComputeScale(r, a, CorrelationStrategy::SPEED_WINDOW, 1.0)) > 0.01;
  errors += std::abs(1.0 - ComputeScale(r, a, CorrelationStrategy::JERK_WINDOW, 6.0)) > 0.05;
  errors += std::abs(1.0 - ComputeScale(r, a, CorrelationStrategy::DPOS, 1.0)) > 0.01;
  errors += std::abs(1.0 - ComputeScale(r, a, CorrelationStrategy::ACC_WINDOW, 3.0)) > 0.05;
  errors += std::abs(1.0 - ComputeScale(r, a, CorrelationStrategy::DERIVATED_LENGTH, 1.0)) > 0.01;

  referenceFile = dataRoot + "/trajectories/mm04/imu.csv";
  alignedFile = dataRoot + "/trajectories/mm04/lidar-slam.csv";
  r = vtkTemporalTransformsReader::OpenTemporalTransforms(referenceFile)
        ->ExtractTimes(255064, 255064 + 180)
        ->Subsample(10);
  a = vtkTemporalTransformsReader::OpenTemporalTransforms(alignedFile)
        ->ExtractTimes(3090, 3090 + 180)
        ->ApplyTimeshift(-mm04_gt);

  errors += std::abs(1.0 - ComputeScale(r, a, CorrelationStrategy::SPEED_WINDOW, 1.0)) > 0.01;
  std::cout << errors << std::endl;

  return errors;
}
