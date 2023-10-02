/*=========================================================================

  Program: LidarView
  Module:  vtkLidarPacketInterpreter.cxx

  Copyright 2013 Velodyne Acoustics, Inc.
  Copyright 2018 (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTKLIDARPROVIDERINTERNAL_H
#define VTKLIDARPROVIDERINTERNAL_H

// Compliance with vtk's fpos_t policy, needs to be included before any libc header
#include <vtkSystemIncludes.h>

#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>

#include "FrameInformation.h"
#include "vtkInterpreter.h"

#include "lvIOLidarModule.h"

enum FramingMethod_t
{
  INTERPRETER_FRAMING = 0,        // the interpreter in charge of the framing
  NETWORK_PACKET_TIME_FRAMING = 1 // interpreter not in charge
};

class LVIOLIDAR_EXPORT vtkLidarPacketInterpreter : public vtkInterpreter
{
public:
  vtkTypeMacro(vtkLidarPacketInterpreter, vtkInterpreter)
  void PrintSelf(ostream& vtkNotUsed(os), vtkIndent vtkNotUsed(indent)) override {} // TODO

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
   *        by default it's called with the interpreter framing method to avoid modification of all
   *        specific interpreters
   */
  virtual bool SplitFrame(bool force = false,
    FramingMethod_t framingMethodAskingForSplitFrame = FramingMethod_t::INTERPRETER_FRAMING);

  /**
   * @brief PreProcessPacket is use to construct the frame index and get some corretion
   * or live calibration comming from the data. A warning should be raise in case the calibration
   * information does not match the data (ex: factory field, number of laser, ...)
   * @param data raw data packet
   * @param dataLength size of the data packet
   * @param packetInfo[out] Miscellaneous information about the packet
   */
  virtual bool PreProcessPacket(unsigned char const* data,
    unsigned int dataLength,
    fpos_t filePosition = fpos_t(),
    double packetNetworkTime = 0,
    std::vector<FrameInformation>* frameCatalog = nullptr) = 0;

  /**
   * @brief PreProcessPacketWrapped is used to construct the frame index
   *        It calls PreProcessPacket if the Framing method is the interpreter one
   *        It calls FillFrameCatalogIfOutsideFramingMethod otherwise
   * @param data raw data packet
   * @param dataLength size of the data packet
   * @param packetInfo[out] Miscellaneous information about the packet
   */
  virtual bool PreProcessPacketWrapped(unsigned char const* data,
    unsigned int dataLength,
    fpos_t filePosition = fpos_t(),
    double packetNetworkTime = 0,
    std::vector<FrameInformation>* frameCatalog = nullptr);

  /**
   * @brief IsLidarPacket check if the given packet is really a lidar packet
   * @param data raw data packet
   * @param dataLength size of the data packet
   */
  virtual bool IsLidarPacket(unsigned char const* data, unsigned int dataLength) = 0;

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
  virtual void SetParserMetaData(const FrameInformation& metaData)
  {
    this->ParserMetaData = metaData;
  }

  virtual int GetNumberOfChannels() { return this->CalibrationReportedNumLasers; }

  bool IsNewData() override;

  bool IsValidPacket(unsigned char const* data, unsigned int dataLength) override;

  void ResetCurrentData() override;

  void ProcessPacketWrapped(unsigned char const* data,
    unsigned int dataLength,
    double PacketNetworkTime) override;

  vtkGetMacro(CalibrationFileName, std::string);
  vtkSetMacro(CalibrationFileName, std::string);

  vtkGetMacro(IsCalibrated, bool);

  vtkGetMacro(TimeOffset, double);
  vtkSetMacro(TimeOffset, double);

  /**
   * @copydoc LidarPacketInterpreter::LaserSelection
   */
  virtual vtkIntArray* GetLaserSelection();
  virtual void SetLaserSelection(int index, int value);

  vtkGetMacro(DistanceResolutionM, double);

  vtkGetMacro(Frequency, double);
  vtkGetMacro(Rpm, double);

  vtkGetMacro(IgnoreZeroDistances, bool);
  vtkSetMacro(IgnoreZeroDistances, bool);

  vtkGetMacro(IgnoreEmptyFrames, bool);
  vtkSetMacro(IgnoreEmptyFrames, bool);

  vtkGetMacro(EnableAdvancedArrays, bool);
  vtkSetMacro(EnableAdvancedArrays, bool);

  vtkSetMacro(FramingMethod, int);
  vtkGetMacro(FramingMethod, int);

  vtkSetMacro(FrameDuration_s, double);
  vtkGetMacro(FrameDuration_s, double);

protected:
  /**
   * @brief CreateNewEmptyFrame construct a empty polyData with the right DataArray and allocate
   * some space. No CellArray should be created as it can be create once the frame is ready.
   * @param numberOfPoints indicate the space to allocate @todo change the meaning
   */
  virtual vtkSmartPointer<vtkPolyData> CreateNewEmptyFrame(vtkIdType numberOfPoints,
    vtkIdType prereservedNumberOfPoints = 0) = 0;

  //! Buffer to store the frame once they are ready
  std::vector<vtkSmartPointer<vtkPolyData>> Frames;

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

  //! Meta data required to correctly parse the
  //! data contained within the udp packets
  FrameInformation ParserMetaData;

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
