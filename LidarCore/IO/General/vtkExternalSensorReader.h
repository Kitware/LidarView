#ifndef VTKEXTERNALSENSORREADER_H
#define VTKEXTERNALSENSORREADER_H

#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTableAlgorithm.h>

#include <string>

#include "lvIOGeneralModule.h"

class vtkNormalizeExternalSensorData;

/**
 * @class   vtkExternalSensorReader
 * @brief   Reads external sensor data (IMU / GNSS / Odometry) from CSV files
 *
 * This reader loads external sensor data (IMU, INS/GNSS, Odometry) from a CSV file.
 * It produces a vtkTable with scalar arrays corresponding to the selected sensors.
 */
class LVIOGENERAL_EXPORT vtkExternalSensorReader : public vtkTableAlgorithm
{
public:
  static vtkExternalSensorReader* New();
  vtkTypeMacro(vtkExternalSensorReader, vtkTableAlgorithm);

  ///@{
  /**
   * Set / Get the filename.
   */
  vtkSetMacro(FileName, std::string);
  vtkGetMacro(FileName, std::string);
  ///@}

  ///@{
  /**
   * Get the normalization filter attached to this reader.
   */
  vtkGetObjectMacro(Normalizer, vtkNormalizeExternalSensorData);
  ///@}

  /**
   * Provide the list of column names parsed from the CSV header
   */
  vtkStringArray* GetHeaderColumns();

  /**
   * Attach the normalization filter that post-processes the raw table output.
   */
  virtual void SetNormalizer(vtkNormalizeExternalSensorData*);

protected:
  vtkExternalSensorReader();
  ~vtkExternalSensorReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  vtkMTimeType GetMTime() override;

private:
  vtkExternalSensorReader(const vtkExternalSensorReader&) = delete;
  void operator=(const vtkExternalSensorReader&) = delete;

  std::string FileName;

  vtkNormalizeExternalSensorData* Normalizer = nullptr;
  std::vector<std::string> HeaderColumns;
  vtkSmartPointer<vtkStringArray> HeaderColumnsCacheArray;

  void ParseHeaderIfNeeded();
  void LoadHeaderFromFile();
};

#endif // VTKEXTERNALSENSORREADER_H
