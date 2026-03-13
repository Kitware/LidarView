#include <vtkMath.h>
#include <vtkTesting.h>

#include "vtkTemporalTransforms.h"
#include "vtkTemporalTransformsReader.h"
#include "vtkTimeCalibration.h"

typedef CorrelationStrategy S;

int TestTimeCalibrationMM(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  std::string dataRoot = testing->GetDataRoot();

  int errors = 0;
  const double mm03_gt = -399618.0; // ground truth quality (+- 1e-4)
  const double mm04_gt = -252018.0; // ground truth quality (+- 1e-4)
  const double dt = 0.1;            // biggest sampling rate in both signals
  std::string referenceFile = dataRoot + "/trajectories/mm03/imu.csv";
  std::string alignedFile = dataRoot + "/trajectories/mm03/lidar-slam.csv";
  vtkSmartPointer<vtkTemporalTransforms> complete_r, complete_a, to_sample_r, r, a;
  complete_r = vtkTemporalTransformsReader::OpenTemporalTransforms(referenceFile);
  to_sample_r = complete_r->ExtractTimes(400274, 400274 + 180);
  r = to_sample_r->Subsample(10);
  complete_a = vtkTemporalTransformsReader::OpenTemporalTransforms(alignedFile);
  a = complete_a->ExtractTimes(610, 610 + 180);

  double gt = mm03_gt;
  errors += (std::abs(ComputeTimeShift(r, a, S::ORIENTATION_ANGLE, 1.0) - gt) > 1 * dt);
  errors += (std::abs(ComputeTimeShift(r, a, S::DERIVATED_ORIENTATION_ARC, 1.0) - gt) > 1 * dt);
  errors += (std::abs(ComputeTimeShift(r, a, S::DROT, 1.0) - gt) > 2 * dt);
  errors += (std::abs(ComputeTimeShift(r, a, S::JERK_WINDOW, 6.0) - gt) > 1 * dt);

  errors += (std::abs(ComputeTimeShift(r, a, S::ACC_WINDOW, 3) - gt) > 2 * dt);

  referenceFile = dataRoot + "/trajectories/mm04/imu.csv";
  alignedFile = dataRoot + "/trajectories/mm04/lidar-slam.csv";
  to_sample_r = vtkTemporalTransformsReader::OpenTemporalTransforms(referenceFile)
                  ->ExtractTimes(255064, 255064 + 180);
  r = to_sample_r->Subsample(10);
  a = vtkTemporalTransformsReader::OpenTemporalTransforms(alignedFile)
        ->ExtractTimes(3090, 3090 + 180);

  gt = mm04_gt;
  errors += (std::abs(ComputeTimeShift(r, a, S::ORIENTATION_ANGLE, 1.0) - gt) > 1 * dt);
  errors += (std::abs(ComputeTimeShift(r, a, S::DERIVATED_ORIENTATION_ARC, 1.0) - gt) > 1 * dt);
  errors += (std::abs(ComputeTimeShift(r, a, S::DROT, 1.0) - gt) > 1 * dt);

  return (errors == 0) ? 0 : 1;
}
