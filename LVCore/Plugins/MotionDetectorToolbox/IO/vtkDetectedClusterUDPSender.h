/*=========================================================================

  Program: LidarView
  Module:  vtkDetectedClusterUDPSender.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkDetectedClusterUDPSender_h
#define vtkDetectedClusterUDPSender_h

#include "MotionDetectorToolboxIOModule.h" // For export macro
#include <vtkSmartPointer.h>               // for ivar
#include <vtkUDPSenderAlgorithm.h>

#include <memory>
#include <string>

class vtkCompositeDataSet;
class vtkFieldData;

/**
 * vtkDetectedClusterUDPSender sends cluster information (vtkMotionDetector output)
 *
 * The data sent will follow this format:
 *
 *   ===== PAYLOAD HEADER =====
 *     payload_start: 0x28 0x2a
 *     timestamp: double
 *     number_of_cluster: uint16_t
 *
 *   ===== PAYLOAD DATA =====
 *     # For each cluster (number_of_cluster):
 *     |   cluster_id: int
 *     |   label: unsigned short
 *     |   center: 3 * float
 *     |   distance: float
 *     |   size: 3 * float
 *     |   orientation: 4 * float
 *
 *   ===== PAYLOAD FOOTER =====
 *     packet_size: uint16_t
 *     end_of_payload: 0x2a 0x2c
 *     null_bytes
 *
 *
 * @sa vtkMotionDetector
 */
class MOTIONDETECTORTOOLBOXIO_EXPORT vtkDetectedClusterUDPSender : public vtkUDPSenderAlgorithm
{
public:
  static vtkDetectedClusterUDPSender* New();
  vtkTypeMacro(vtkDetectedClusterUDPSender, vtkUDPSenderAlgorithm);

protected:
  vtkDetectedClusterUDPSender();
  ~vtkDetectedClusterUDPSender() override;

  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkDetectedClusterUDPSender(const vtkDetectedClusterUDPSender&) = delete;
  void operator=(const vtkDetectedClusterUDPSender&) = delete;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  void SendPacket(uint16_t nbOfBlocks, uint16_t currentIdx);
  void SendData(vtkCompositeDataSet* blocks, double timestamp);

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
