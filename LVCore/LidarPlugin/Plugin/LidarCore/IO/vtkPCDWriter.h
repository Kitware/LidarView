#ifndef VTKPCDWRITER_H
#define VTKPCDWRITER_H

#include <vtkWriter.h>

#include "LidarCoreModule.h"

class vtkPolyData;

/**
 * @brief The vtkPCDWriter class write Point Cloud that are stored in a polydata in a pcd file.
 *
 * PCD file format specification: https://pcl-tutorials.readthedocs.io/en/latest/pcd_file_format.html
 *
 * @TODO: - update to timeseries in PV 5.9
 *        - update to passarray in PV 5.X
 */
class LIDARCORE_EXPORT vtkPCDWriter : public vtkWriter
{
public:
  static vtkPCDWriter* New();
  vtkTypeMacro(vtkPCDWriter, vtkWriter)

  vtkSetStringMacro(FileName)
  vtkGetStringMacro(FileName)

  vtkGetMacro(Binary, bool)
  vtkSetMacro(Binary, bool)

  vtkGetMacro(FloatPointPrecision, int)
  vtkSetMacro(FloatPointPrecision, int)

protected:
  vtkPCDWriter() = default;
  ~vtkPCDWriter();

  void WriteData() override;
  int FillInputPortInformation(int, vtkInformation* info) override;

private:
  vtkPCDWriter(const vtkPCDWriter&) = delete;
  void operator =(const vtkPCDWriter&) = delete;

  void WriteHeader(vtkPolyData*, std::ofstream&);
  void WriteBody(vtkPolyData*, std::ofstream&);

  char* FileName = nullptr;
  bool Binary = false;
  int FloatPointPrecision = 8;
};
#endif // VTKPCDWRITER_H
