#ifndef VTKKITTIOBJECTLABELSREADER_H
#define VTKKITTIOBJECTLABELSREADER_H

#include <string>

#include <vtkMultiBlockDataSetAlgorithm.h>


/**
 * @brief vtkKITTIObjectLabelsReader reads labels in the format adopted in
 * KITTI 3D objects detection benchmark
 * (cf. http://www.cvlibs.net/datasets/kitti/eval_object.php?obj_benchmark=3d)
 *
 * By default, the output boxes are in the camera coordinate system.
 * If a calibration file is provided, the output boxes will be in the lidar coordinate system
 *
 * Implementation details:
 * ----------------------
 *
 * Labels for each frame are stored in a text file "{frame_index}.txt"
 * With 1 line per object.
 *
 * The fields for each objects are:
 * [0] type ('Car', 'Pedestrian', ...)
 * [1] truncation (truncated pixel ratio [0..1])
 * [2] occlusion (0=visible, 1=partly occluded, 2=fully occluded, 3=unknown)
 * [3] alpha (object observation angle [-pi..pi])
 * [4..7] 2d bounding box in 0 based coordinated (left, top, right, bottom)
 * [8] 3d bbox height
 * [9] 3d bbox width
 * [10] 3d bbox length (in meters)
 * [11..13] 3d location (x, y, z) in camera coordinates
 * [14] rotY rotation angle around Y axis in camera coordinates [-pi..pi]
 * [15] score (only for results)
 *
 * (Source: https://github.com/kuixu/kitti_object_vis/blob/master/kitti_util.py)
 *
 * Calibration:
 *
 * The transformation from lidar coordinates to camera coodinates is:
 * x = P2 * R0_rect * Tr_velo_to_cam * y
 * "
 * The Px matrices project a point in the rectified referenced camera coordinate to the camera_x image.
 * camera_0 is the reference camera coordinate. R0_rect is the rectifying rotation for reference coordinate
 * ( rectification makes images of multiple cameras lie on the same plan). Tr_velo_to_cam maps a point in
 * point cloud coordinate to reference co-ordinate.
 * "
 * (source: https://medium.com/test-ttile/kitti-3d-object-detection-dataset-d78a762b5a4)
 *
 * TODO Calibration between camera and lidar from calibration file
*/

class vtkKITTIObjectLabelsReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkKITTIObjectLabelsReader* New();
  vtkTypeMacro(vtkKITTIObjectLabelsReader, vtkMultiBlockDataSetAlgorithm)

  void GetLabelData(int frameIndex, vtkMultiBlockDataSet* output);

  vtkGetMacro(FolderName, std::string)
  void SetFolderName(const std::string& path);

  vtkSetMacro(UseCalibration, bool)
  vtkGetMacro(UseCalibration, bool)

  vtkGetMacro(CalibFolderName, std::string)
  void SetCalibFolderName(const std::string& path);

  vtkGetMacro(NumberOfFrames, int)

  vtkSetMacro(NumberOfFileNameDigits, int)

private:
vtkKITTIObjectLabelsReader();

  int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector) override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  //! folder containing all the .bin files for a sequence
  std::string FolderName = "";

  //! Number of frame in this sequence
  int NumberOfFrames = 0;

  //! Number of digits expected in the filenames to read
  int NumberOfFileNameDigits = 10;

  //! Use calibration files to project the bboxes to the lidar coordinate system?
  bool UseCalibration = false;

  //! folder containing all the calibration files for a sequence
  //! If not provided, the boxes are in the camera coordinate system
  std::string CalibFolderName = "";

  vtkKITTIObjectLabelsReader(const vtkKITTIObjectLabelsReader&) = delete;
  void operator=(const vtkKITTIObjectLabelsReader&) = delete;

};

#endif  // VTKKITTIOBJECTLABELSREADER_H
