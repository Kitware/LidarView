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
#include <vtkLiveSourceAlgorithm.h>
#include <vtkPassInputTypeAlgorithm.h>
#include <vtkSmartPointer.h> // for ivar

#include <memory>
#include <string>

class vtkCompositeDataSet;
class vtkFieldData;

#ifndef __VTK_WRAP__
#define vtkPassInputTypeAlgorithm vtkLiveSourceAlgorithm<vtkPassInputTypeAlgorithm>
#endif

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
 *     |   label: unsigned short
 *     |   center: 3 * float
 *     |   distance: float
 *     |   size: 3 * float
 *
 *   ===== PAYLOAD FOOTER =====
 *     packet_size: uint16_t
 *     end_of_payload: 0x2a 0x2c
 *     null_bytes
 *
 *
 * @sa vtkMotionDetector
 */
class MOTIONDETECTORTOOLBOXIO_EXPORT vtkDetectedClusterUDPSender : public vtkPassInputTypeAlgorithm
{
public:
  static vtkDetectedClusterUDPSender* New();
  vtkTypeMacro(vtkDetectedClusterUDPSender, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Enable/disable this filter. When disabled, this filter passes data object
   * doing nothing more.
   */
  vtkSetMacro(Enabled, bool);
  vtkGetMacro(Enabled, bool);
  vtkBooleanMacro(Enabled, bool);
  ///@}

  ///@{
  /**
   * IP address to send data over. Can either be a IPv4 address string in dotted decimal form,
   * or an IPv6 address in hexadecimal notation.
   */
  vtkGetMacro(IPAddress, std::string);
  vtkSetMacro(IPAddress, std::string);
  ///@}

  ///@{
  /**
   * Port on which to send data.
   */
  vtkGetMacro(DestinationPort, int);
  vtkSetMacro(DestinationPort, int);
  ///@}

protected:
  vtkDetectedClusterUDPSender();
  ~vtkDetectedClusterUDPSender() override;

  int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkDetectedClusterUDPSender(const vtkDetectedClusterUDPSender&) = delete;
  void operator=(const vtkDetectedClusterUDPSender&) = delete;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  void SendData(vtkCompositeDataSet* dataset);

  bool Enabled = true;
  int DestinationPort;
  std::string IPAddress;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#ifndef __VTK_WRAP__
#undef vtkPassInputTypeAlgorithm
#endif

#endif
