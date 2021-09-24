// Copyright 2013 Velodyne Acoustics, Inc.
// Copyright 2018 Kitware SAS.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef VTKINTERPRETER_H
#define VTKINTERPRETER_H

#include <vtkAlgorithm.h>

#include "LidarCoreModule.h"

class vtkTransform;

class LIDARCORE_EXPORT vtkInterpreter : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkInterpreter, vtkAlgorithm)

  /**
   * @brief WrapProcessPacket process the data packet to get the current informations
   * @param data raw data packet
   * @param dataLength size of the data packet
   * @param PacketReceptionTime
   */
  virtual void ProcessPacketWrapped(unsigned char const * data, unsigned int dataLength, double vtkNotUsed(PacketNetworkTime))
  {
    this->ProcessPacket(data, dataLength);
  }

  /**
   * @brief ProcessPacket process the data packet to get the current informations
   * @param data raw data packet
   * @param dataLength size of the data packet
   * @param PacketReceptionTime
   */
  virtual void ProcessPacket(unsigned char const * data, unsigned int dataLength) = 0;

  /**
   * @brief IsValidPacket check if the packet is valid
   * @param data raw data packet
   * @param dataLength size of the data packet
   */
  virtual bool IsValidPacket(unsigned char const * data, unsigned int dataLength) = 0;

  /**
   * @brief IsNewData check if new data is available
   */
  virtual bool IsNewData() = 0;

  /**
   * @brief ResetCurrentData reset the current data
   */
  virtual void ResetCurrentData() = 0;

  virtual int64_t GetManufacturerMACAddress() { return 0xffffffffffff;}

  vtkGetMacro(ApplyTransform, bool)
  vtkSetMacro(ApplyTransform, bool)

  vtkGetObjectMacro(SensorTransform, vtkTransform)
  virtual void SetSensorTransform(vtkTransform *);

  vtkMTimeType GetMTime() override;

protected:
  vtkInterpreter() = default;
  virtual ~vtkInterpreter() = default;

  //! Indicate if the vtkInterpreter::SensorTransform is apply
  bool ApplyTransform = false;

  //! Fixed transform to apply to the Sensor output points.
  vtkTransform* SensorTransform = nullptr;

private:
  vtkInterpreter(const vtkInterpreter&) = delete;
  void operator=(const vtkInterpreter&) = delete;

};

#endif // VTKINTERPRETER_H
