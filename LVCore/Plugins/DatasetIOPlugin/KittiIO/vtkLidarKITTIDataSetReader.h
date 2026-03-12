#ifndef VTKLIDARKITTIDATASETREADER_H
#define VTKLIDARKITTIDATASETREADER_H

#include <vtkPolyDataAlgorithm.h>

#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include "vtkKittiIOModule.h" // for export macro

/**
 * @brief The vtkLidarKITTIDataSetReader class
 * @warning Only a small subset of method are implemented
 */
class VTKKITTIIO_EXPORT vtkLidarKITTIDataSetReader : public vtkPolyDataAlgorithm
{
public:
  static vtkLidarKITTIDataSetReader* New();
  vtkTypeMacro(vtkLidarKITTIDataSetReader, vtkPolyDataAlgorithm)

  vtkSmartPointer<vtkPolyData> GetFrame(int frameNumber);

  vtkGetMacro(FileName, std::string);
  void SetFileName(const std::string& filename);

  vtkGetMacro(NumberOfFrames, int);

  vtkSetMacro(NumberOfFileNameDigits, int);

  // return the number of channels
  vtkGetMacro(NbrLaser, int);

private:
  vtkLidarKITTIDataSetReader();
  ~vtkLidarKITTIDataSetReader();

  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  //! folder containing all the .bin file for a sequence
  //! this should be named FolderName but to keep the same API we will keep FileName
  std::string FileName = "";

  //! Number of previous frames to display with the current frame (concatenation of frames)
  int NumberOfTrailingFrames;

  //! Number of frame in this sequence
  int NumberOfFrames = 0;

  int NbrLaser = 64;

  //! Number of digits expected in the filenames to read
  int NumberOfFileNameDigits = 10;

  vtkLidarKITTIDataSetReader(const vtkLidarKITTIDataSetReader&) = delete;
  void operator=(const vtkLidarKITTIDataSetReader&) = delete;
};

#endif // VTKLIDARKITTIDATASETREADER_H
