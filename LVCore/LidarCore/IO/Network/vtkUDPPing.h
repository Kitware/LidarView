/*=========================================================================

  Program: LidarView
  Module:  vtkUDPPing.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkUDPPing_h
#define vtkUDPPing_h

#include "lvIONetworkModule.h" // For export macro
#include <vtkSmartPointer.h>   // for ivar
#include <vtkUDPSenderAlgorithm.h>

#include <memory>
#include <string>

class vtkDataSet;

/**
 * vtkUDPPing send a small packet with the current number of point
 * at each pipeline update.
 * To avoid spam a minimal delay between two consecutive packet can
 * be set (default to 1000ms).
 *
 * The data sent will follow this format:
 *
 *   ===== PAYLOAD DATA =====
 *     start_of_payload: 0x28 0x2a
 *     number_of_points: uint64_t
 *     end_of_payload: 0x2a 0x2c
 */
class LVIONETWORK_EXPORT vtkUDPPing : public vtkUDPSenderAlgorithm
{
public:
  static vtkUDPPing* New();
  vtkTypeMacro(vtkUDPPing, vtkUDPSenderAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Minimal delay between two consecutive ping in milliseconds.
   * Default to 1000ms.
   */
  vtkGetMacro(MinimalDelay, int);
  vtkSetMacro(MinimalDelay, int);
  ///@}

protected:
  vtkUDPPing();
  ~vtkUDPPing() override;

  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkUDPPing(const vtkUDPPing&) = delete;
  void operator=(const vtkUDPPing&) = delete;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int MinimalDelay = 1000;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
