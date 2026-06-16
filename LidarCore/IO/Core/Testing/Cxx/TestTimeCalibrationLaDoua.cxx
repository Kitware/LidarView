#include <vtkMath.h>
#include <vtkTesting.h>

#include "vtkTemporalTransforms.h"
#include "vtkTemporalTransformsReader.h"
#include "vtkTimeCalibration.h"

typedef CorrelationStrategy S;

int TestTimeCalibrationLaDoua(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  std::string dataRoot = testing->GetDataRoot();

  int errors = 0;
  const double gt = -1302.343; // estimated, could be +- 0.1 s
  const double dt = 0.0667;    // smallest sampling rate in both signals
  std::string referenceFile = dataRoot + "/trajectories/la_doua_dataset/slam_lidar_part1.csv";
  std::string alignedFile = dataRoot + "/trajectories/la_doua_dataset/orbslam2_gopro_part1.csv";
  vtkSmartPointer<vtkTemporalTransforms> r, a;
  r = vtkTemporalTransformsReader::OpenTemporalTransforms(referenceFile);
  a = vtkTemporalTransformsReader::OpenTemporalTransforms(alignedFile);

  errors += (std::abs(ComputeTimeShift(r, a, S::JERK_WINDOW, 6) - gt) > 1 * dt);
  errors += (std::abs(ComputeTimeShift(r, a, S::TRAJECTORY_ANGLE, 10) - gt) > 1 * dt);
  errors += (std::abs(ComputeTimeShift(r, a, S::DROT, 1) - gt) > 1 * dt);

  errors += (std::abs(ComputeTimeShift(r, a, S::ORIENTATION_ANGLE, 1.0) - gt) > 2 * dt);
  errors += (std::abs(ComputeTimeShift(r, a, S::DERIVATED_ORIENTATION_ARC, 1.0) - gt) > 2 * dt);

  referenceFile = dataRoot + "/trajectories/la_doua_dataset/slam_lidar_part2.csv";
  alignedFile = dataRoot + "/trajectories/la_doua_dataset/orbslam2_gopro_part2.csv";
  r = vtkTemporalTransformsReader::OpenTemporalTransforms(referenceFile);
  a = vtkTemporalTransformsReader::OpenTemporalTransforms(alignedFile);

  errors += (std::abs(ComputeTimeShift(r, a, S::ACC_WINDOW, 3) - gt) > 1 * dt);
  errors += (std::abs(ComputeTimeShift(r, a, S::ORIENTATION_ANGLE, 1.0) - gt) > 1 * dt);
  errors += (std::abs(ComputeTimeShift(r, a, S::DERIVATED_ORIENTATION_ARC, 1.0) - gt) > 1 * dt);

  errors += (std::abs(ComputeTimeShift(r, a, S::JERK_WINDOW, 6) - gt) > 2 * dt);
  errors += (std::abs(ComputeTimeShift(r, a, S::DROT, 1) - gt) > 2 * dt);
  errors += (std::abs(ComputeTimeShift(r, a, S::DPOS, 1) - gt) > 2 * dt);
  errors += (std::abs(ComputeTimeShift(r, a, S::SPEED_WINDOW, 1) - gt) > 2 * dt);
  errors += (std::abs(ComputeTimeShift(r, a, S::DERIVATED_LENGTH, 1) - gt) > 2 * dt);

  return (errors == 0) ? 0 : 1;
}
