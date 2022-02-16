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

#ifndef VTKLIDARPROVIDERINTERNAL_H
#define VTKLIDARPROVIDERINTERNAL_H

#include <vtkSystemIncludes.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkPolyData.h>

#include "IO/vtkInterpreter.h"
#include "IO/FrameInformation.h"

#include "LidarCoreModule.h"

enum FramingMethod_t {
        INTERPRETER_FRAMING = 0, // the interpreter in charge of the framing
        NETWORK_PACKET_TIME_FRAMING = 1 // interpreter not in charge
};

class LIDARCORE_EXPORT  vtkLidarPacketInterpreter : public vtkInterpreter
{
public:
  vtkTypeMacro(vtkLidarPacketInterpreter, vtkInterpreter)
  void PrintSelf(ostream& vtkNotUsed(os), vtkIndent vtkNotUsed(indent)) override {} //TODO

  /**
   * @brief The CropModeEnum enum to select the cropping mode
   */
  enum CROP_MODE
  {
    None = 0,       /*!< 0 */
    Cartesian = 1,  /*!< 1 */
    Spherical = 2,  /*!< 2 */
    Cylindric = 3,  /*!< 3 */
  };

  /**
   * @brief LoadCalibration read a provided calibration file to initialize the sensor's
   * calibration parameters (angles corrections, distances corrections, ...) which will be
   * used later on while processing the packet.
   * @param filename should be garanty to exist, as no check will be perform.
   */
  virtual void LoadCalibration(const std::string& filename) = 0;

  /**
   * @brief GetCalibrationTable return a table conttaining all information related to the sensor
   * calibration.
   */
  virtual vtkSmartPointer<vtkTable> GetCalibrationTable() { return this->CalibrationData.Get(); }

  /**
   * @brief SplitFrame
   *        Take the current frame under construction and place it in another buffer
   *        if the current framing method is the same as framingMethodAskingForSplitFrame
   * @param force force the split even if the frame is empty regardless of the Framing method
   * @param framingMethodAskingForSplitFrame Framing method which ask for the splitFrame
   *        by default it's called with the interpreter framing method to avoid modification of all specific interpreters
   */
  virtual bool SplitFrame(bool force = false, FramingMethod_t framingMethodAskingForSplitFrame = FramingMethod_t::INTERPRETER_FRAMING);

  /**
   * @brief PreProcessPacket is use to construct the frame index and get some corretion
   * or live calibration comming from the data. A warning should be raise in case the calibration
   * information does not match the data (ex: factory field, number of laser, ...)
   * @param data raw data packet
   * @param dataLength size of the data packet
   * @param packetInfo[out] Miscellaneous information about the packet
   */
  virtual bool PreProcessPacket(unsigned char const * data, unsigned int dataLength,
                                fpos_t filePosition = fpos_t(), double packetNetworkTime = 0,
                                std::vector<FrameInformation>* frameCatalog = nullptr) = 0;

  /**
   * @brief PreProcessPacketWrapped is used to construct the frame index
   *        It calls PreProcessPacket if the Framing method is the interpreter one
   *        It calls FillFrameCatalogIfOutsideFramingMethod otherwise
   * @param data raw data packet
   * @param dataLength size of the data packet
   * @param packetInfo[out] Miscellaneous information about the packet
   */
  virtual bool PreProcessPacketWrapped(unsigned char const * data, unsigned int dataLength,
                                       fpos_t filePosition = fpos_t(), double packetNetworkTime = 0,
                                       std::vector<FrameInformation>* frameCatalog = nullptr);

  /**
   * @brief IsLidarPacket check if the given packet is really a lidar packet
   * @param data raw data packet
   * @param dataLength size of the data packet
   */
  virtual bool IsLidarPacket(unsigned char const * data, unsigned int dataLength) = 0;

  /**
   * @brief ResetCurrentFrame reset all information to handle some new frame. This reset the
   * frame container, some information about the current frame, guesses about the sensor type, etc
   */
  virtual void ResetCurrentFrame() { this->CurrentFrame = this->CreateNewEmptyFrame(0); }

  /**
   * @brief isNewFrameReady check if a new frame is ready
   */
  virtual bool IsNewFrameReady() { return this->Frames.size(); }

  /**
   * @brief GetLastFrameAvailable return the last frame that have been process completely
   * @return
   */
  virtual vtkSmartPointer<vtkPolyData> GetLastFrameAvailable() { return this->Frames.back(); }

  /**
   * @brief ClearAllFramesAvailable delete all frames that have been process
   */
  virtual void ClearAllFramesAvailable() { this->Frames.clear(); }

  /**
   * @brief GetDefaultRecordFileName return the default file name when recording a stream
   * @return
   */
  virtual std::string GetDefaultRecordFileName();

  /**
   * @brief GetSensorInformation return information to display to the user
   * @param shortVersion True to have a succinct version of the sensor information
   * @return
   */
  virtual std::string GetSensorInformation(bool shortVersion = false) = 0;

  /**
   * @brief CreateNewFrameInformation create a new frame information
   * @return
   */
  virtual FrameInformation GetParserMetaData() { return this->ParserMetaData; }

  /**
   * @brief ResetParserMetaData reset the metadata used by the
   *        interpreter during the parsing of the udp packets
   *        within the function ProcessPacket
   * @return
   */
  virtual void ResetParserMetaData() { this->ParserMetaData.Reset(); }

  /**
   * @brief SetParserMetaData set the metadata
   *        used by the interpreter during the parsing
   *        of the udp packets within the function
   *        ProcessPacket
   * @return
   */
  virtual void SetParserMetaData(const FrameInformation& metaData) { this->ParserMetaData = metaData; }

  virtual int GetNumberOfChannels() { return this->CalibrationReportedNumLasers; }

  bool IsNewData() override;

  bool IsValidPacket(unsigned char const * data, unsigned int dataLength) override;

  void ResetCurrentData() override;

  void ProcessPacketWrapped(unsigned char const * data, unsigned int dataLength, double PacketNetworkTime) override;

  vtkGetMacro(CalibrationFileName, std::string)
  vtkSetMacro(CalibrationFileName, std::string)

  vtkGetMacro(IsCalibrated, bool)

  vtkGetMacro(TimeOffset, double)
  vtkSetMacro(TimeOffset, double)

  /**
   * @copydoc LidarPacketInterpreter::LaserSelection
   */
  virtual vtkIntArray* GetLaserSelection();
  virtual void SetLaserSelection(int index, int value);

  vtkGetMacro(DistanceResolutionM, double)

  vtkGetMacro(Frequency, double)
  vtkGetMacro(Rpm, double)

  vtkGetMacro(IgnoreZeroDistances, bool)
  vtkSetMacro(IgnoreZeroDistances, bool)

  vtkGetMacro(IgnoreEmptyFrames, bool)
  vtkSetMacro(IgnoreEmptyFrames, bool)

  vtkGetMacro(CropMode, int)
  vtkSetMacro(CropMode, int)

  vtkGetMacro(CropOutside, bool)
  vtkSetMacro(CropOutside, bool)

  vtkSetVector6Macro(CropRegion, double)

  vtkGetMacro(EnableAdvancedArrays, bool);
  vtkSetMacro(EnableAdvancedArrays, bool);

  vtkSetMacro(FramingMethod, int)
  vtkGetMacro(FramingMethod, int)

  vtkSetMacro(FrameDuration_s, double)
  vtkGetMacro(FrameDuration_s, double)

protected:
  /**
   * @brief CreateNewEmptyFrame construct a empty polyData with the right DataArray and allocate some
   * space. No CellArray should be created as it can be create once the frame is ready.
   * @param numberOfPoints indicate the space to allocate @todo change the meaning
   */
  virtual vtkSmartPointer<vtkPolyData> CreateNewEmptyFrame(vtkIdType numberOfPoints, vtkIdType prereservedNumberOfPoints = 0) = 0;

  /**
   * @brief shouldBeCroppedOut Returns true if a point should be removed,
   * i.e. if it lays *outside* the cropping volume.
   *
   * This cropping as the same definition as in Gimp: when cropping an image
   * by drawing a selection, only the selection is kept.
   * If this->CropOutside == false (the default), the cropping volume is
   * the 3D volume inside the two boundaries defined using either cartesian
   * coordinates or spherical coordinates.
   * if this->CropOutside == true the cropping volume is the 3D volume
   * outside the two boundaries (i.e. complement set of the previous case)
   * @param pos cartesian coordinates of the point to be check
   */
  bool shouldBeCroppedOut(double pos[3]);

  //! Buffer to store the frame once they are ready
  std::vector<vtkSmartPointer<vtkPolyData> > Frames;

  //! Frame under construction
  vtkSmartPointer<vtkPolyData> CurrentFrame;

  //! File containing all calibration information
  std::string CalibrationFileName = "";

  ///! Calibration data store in a table
  vtkNew<vtkTable> CalibrationData;

  //! Number of laser which can be shoot at the same time or at least in dt < epsilon
  int CalibrationReportedNumLasers = -1;

  //! Indicate if the sensor is calibrated or has been succesfully calibrated
  bool IsCalibrated = false;

  //! TimeOffset in seconds relative to the system clock
  double TimeOffset = 0.;

  //! Indicate for each laser if the points obtained by this specific laser
  //! should process/display (true) or ignore (false)
  vtkNew<vtkIntArray> LaserSelection;

  //! Laser distance resolution (quantum) which also correspond to the points precision
  double DistanceResolutionM = 0;

  //! Frequency in Hz at which a new frame is obtained.
  double Frequency = 0;

  //! Sensor Rotation Per Minute (Rotational Lidar only).
  double Rpm = 0;

  //! Process/skip points with a zero value distance.
  //! These points correspond to a missing return or a too close return.
  bool IgnoreZeroDistances = true;

  //! Proccess/skip frame with 0 points
  bool IgnoreEmptyFrames = false;

  //! Indicate which cropping mode should be used.
  int CropMode = CROP_MODE::None;

  //! If true, the region outside of the area defined by CropRegion is cropped/removed.
  //! If false, the region within the area is cropped/removed.
  bool CropOutside = false;

  //! Meta data required to correctly parse the
  //! data contained within the udp packets
  FrameInformation ParserMetaData;

  //! Depending on the CroppingMode selected this can have different meaning:
  //! - Cartesian -> [x_min, x_max, y_min, y_max, z_min, z_max]
  //! - Spherical -> [azimuth_min, azimuth_max, vertAngle_min, vertAngle_max, r_min, r_max]
  //! The choosen convention is clockwise from y+
  //! (-180° <= azimuth <= 180, -90° <= vertAngle <= 90°, r >= 0.0,
  //! vertAngle increasing with z and vertAngle = 0 in plane z = 0)
  //! any interval is valid for the azimuthal but [ a, b ] is different from
  //! [ b, a ]
  //! - Cylindric -> Not implemented yet
  //! all distances are in meters and all angles are in degrees
  double CropRegion[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

  bool EnableAdvancedArrays = false;

  //! Framing method
  int FramingMethod = FramingMethod_t::INTERPRETER_FRAMING;
  double FrameDuration_s = 0;
  unsigned long long LastNetworkTimeFrameNumber = 0;
  unsigned long long previousFrameNumber = 0;

  vtkLidarPacketInterpreter() = default;
  virtual ~vtkLidarPacketInterpreter() = default;

private:
  vtkLidarPacketInterpreter(const vtkLidarPacketInterpreter&) = delete;
  void operator=(const vtkLidarPacketInterpreter&) = delete;

};

#endif // VTKLIDARPROVIDERINTERNAL_H
