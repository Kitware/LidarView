/*=========================================================================

  Program: LidarView
  Module:  vtkClustersPointsUDPSender.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkClustersPointsUDPSender_h
#define vtkClustersPointsUDPSender_h

#include "MotionDetectorToolboxIOModule.h" // For export macro
#include <vtkSmartPointer.h>               // for ivar
#include <vtkUDPSenderAlgorithm.h>

#include <memory>
#include <string>

class vtkDataSet;

/**
 * vtkClustersPointsUDPSender sends all points of each cluster independently
 * should be used with vtkMotionDetector.
 *
 * The data sent will follow this format:
 *
 *   ===== PAYLOAD HEADER =====
 *     payload_start: 0x28 0x2a
 *     data_type_size: uint8_t
 *     timestamp: double
 *     cluster_id: int32_t
 *     number_of_points: uint16_t
 *
 *   ===== PAYLOAD DATA =====
 *     # For each point (number_of_points):
 *     |   point (x, y, z): 3 * float
 *
 *   ===== PAYLOAD FOOTER =====
 *     packet_size: uint16_t
 *     end_of_payload: 0x2a 0x2c
 *     null_bytes
 *
 * Notes:
 *  - The payload data can consist of multiple points (indicated in the header with
 *    `number_of_points`).
 *  - After the end of the footer, the packet will be filled with null bytes to reach the maximum
 *    packet size.
 */
class MOTIONDETECTORTOOLBOXIO_EXPORT vtkClustersPointsUDPSender : public vtkUDPSenderAlgorithm
{
public:
  static vtkClustersPointsUDPSender* New();
  vtkTypeMacro(vtkClustersPointsUDPSender, vtkUDPSenderAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Control port used to enabled / disable this filter by TCP connection.
   */
  vtkGetMacro(ControlPort, int);
  vtkSetMacro(ControlPort, int);
  ///@}

protected:
  vtkClustersPointsUDPSender();
  ~vtkClustersPointsUDPSender() override;

  int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkClustersPointsUDPSender(const vtkClustersPointsUDPSender&) = delete;
  void operator=(const vtkClustersPointsUDPSender&) = delete;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  void SendData(vtkDataSet* dataset, double timestamp, int clusterId);

  int ControlPort = 8887;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
