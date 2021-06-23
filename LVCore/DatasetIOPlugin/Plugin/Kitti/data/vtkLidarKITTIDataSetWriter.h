#ifndef VTKLIDARKITTIDATASETWRITER_H
#define VTKLIDARKITTIDATASETWRITER_H


#include <vtkPolyDataAlgorithm.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>

#include <string>

/*
 * @brief vtkLidarKITTIDataSetWriter writes point clouds in KITTI Format
 * The kitti format is:
 * a binary file for each frame, containing a Nx4 float matrix (with N the number of points)
 * the data is stored row-aligned (the first 4 values correspond to the first point)
 * each point is represented by: x, y, z, reflectance
 *
 * The array to use as reflectance can be set in the "advanced" properties
 * or by using "SetInputArrayToProcess"
 *
 * @warning In the kitti setup, x is pointing forward, and z upwards, make sure
 * your data is oriented the same way before using this writer
 **/
class vtkLidarKITTIDataSetWriter : public vtkPolyDataAlgorithm
{
public:
  static vtkLidarKITTIDataSetWriter* New();
  vtkTypeMacro(vtkLidarKITTIDataSetWriter, vtkPolyDataAlgorithm);
  // void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkGetMacro(FolderName, const std::string&)
  void SetFolderName(const std::string&);

  vtkGetMacro(NumberOfFileNameDigits, unsigned int)
  vtkSetMacro(NumberOfFileNameDigits, unsigned int)

  vtkGetMacro(NormalizeIntensity, bool)
  vtkSetMacro(NormalizeIntensity, bool)

  vtkGetMacro(InputIntensityMaxValue, float)
  vtkSetMacro(InputIntensityMaxValue, float)

protected:
  vtkLidarKITTIDataSetWriter();
  ~vtkLidarKITTIDataSetWriter() = default;

  /*
   * @brief UpdatePipelineIndex updates tHe pipeline index when requesting a new frame
   * (used in RequestUpdateExtent)
   **/
  void UpdatePipelineIndex(vtkInformation *);

  /*
   * @brief ParseCloudData parses a point cloud vtkPolyData to retrieve information
   * to export as a vector of floats containing (x, y, z, intensity) for each
   * point.
   * */
  std::vector<float> ParseCloudData(vtkSmartPointer<vtkPolyData> cloud, vtkDataArray* intensity);

  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector*) override;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  // Name of the folder to write the frames to
  std::string FolderName;

  // Internal variables for pipeline time and index (used to create a filename for each frame)
  double PipelineTime;
  unsigned int PipelineIndex;

  // Desired number of digits in the output file names
  unsigned int NumberOfFileNameDigits = 6;

  // Do normalize intensity to 0-1. By default, data is normalized from 0-255 to 0-1
  bool NormalizeIntensity = 1;
  float InputIntensityMaxValue = 255.;


private:
  vtkLidarKITTIDataSetWriter(const vtkLidarKITTIDataSetWriter&) = delete;
  void operator=(const vtkLidarKITTIDataSetWriter&) = delete;
};


#endif // VTKLIDARKITTIDATASETWRITER_H
