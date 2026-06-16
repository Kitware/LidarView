/*=========================================================================

  Program: LidarView
  Module:  vtkUDPPointSender.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkUDPPointSender_h
#define vtkUDPPointSender_h

#include "lvIONetworkModule.h" // For export macro
#include <vtkDataArraySelection.h>
#include <vtkSmartPointer.h> // for ivar
#include <vtkUDPSenderAlgorithm.h>

#include <memory>
#include <string>

class vtkDataSet;
class vtkFieldData;

/**
 * @brief vtkUDPPointSender sends point and point data information over UDP.
 *
 * The data sent will follow this format:
 *
 *   ===== PAYLOAD HEADER =====
 *     header_size: uint16_t
 *     is_big_endian: bool
 *     data_type_size: uint8_t
 *     number_of_arrays: uint8_t
 *     |   array_name_size: uint8_t
 *     |   array_name: string
 *     |   array_number_of_components: uint8_t
 *     number_of_points: uint16_t
 *     end_of_header_bytes: 0x28 0x2a
 *
 *   ===== PAYLOAD DATA =====
 *     # For each point (number_of_points):
 *     |   point (x, y, z): 3 * float
 *     |   # For each array (number_of_arrays):
 *     |   |    # For each array component (array_number_of_components):
 *     |   |    |    point_data_array: float
 *
 *   ===== PAYLOAD FOOTER =====
 *     packet_size: uint16_t
 *     end_of_payload: 0x2a 0x2c
 *     null_bytes
 *
 * Notes:
 *  - Point data array header information does not have an entry for points themselves.
 *  - The payload data can consist of multiple points (indicated in the header with
 *    `number_of_points`).
 *  - To parse `point_data_array`, arrays are defined in the header in the same order with their
 * names and number of components.
 *  - After the end of the footer, the packet will be filled with null bytes to reach the maximum
 *    packet size.
 */
class LVIONETWORK_EXPORT vtkUDPPointSender : public vtkUDPSenderAlgorithm
{
public:
  static vtkUDPPointSender* New();
  vtkTypeMacro(vtkUDPPointSender, vtkUDPSenderAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Can be used to select a subset of data point arrays to send.
   */
  vtkGetObjectMacro(ArraySelections, vtkDataArraySelection);

  ///@{
  /**
   * Enable this option to send data only when the value of TimeArrayName exceeds all previous
   * values. This feature is useful for aggregated data with available time information.
   */
  vtkSetMacro(OnlySendNewData, bool);
  vtkGetMacro(OnlySendNewData, bool);
  vtkBooleanMacro(OnlySendNewData, bool);
  ///@}

  ///@{
  /**
   * Get/Set the name of the time array used as the basis for the OnlySendNewData condition.
   *
   * Refer to OnlySendNewData for further information.
   */
  vtkGetMacro(TimeArrayName, std::string);
  vtkSetMacro(TimeArrayName, std::string);
  ///@}

protected:
  vtkUDPPointSender();
  ~vtkUDPPointSender() override;

  int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkUDPPointSender(const vtkUDPPointSender&) = delete;
  void operator=(const vtkUDPPointSender&) = delete;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  void SendData(vtkDataSet* dataset);

  vtkSmartPointer<vtkDataArraySelection> ArraySelections;
  bool OnlySendNewData = false;
  std::string TimeArrayName;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
