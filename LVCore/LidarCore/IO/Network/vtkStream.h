/*=========================================================================

  Program: LidarView
  Module:  vtkStream.h

  Copyright 2013 Velodyne Acoustics, Inc.
  Copyright 2018 (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkStream_h
#define vtkStream_h

#include <memory>
#include <mutex>
#include <string>

#include <vtkDataObjectAlgorithm.h>
#include <vtkSmartPointer.h>

#include "vtkInterpreter.h"
#include "vtkLiveSourceAlgorithm.h"
#include "vtkStreamPacketHandler.h"

// To deprecate
#include "vtkLidarViewDeprecation.h"
#include "vtkUDPPacketReceiver.h"

#include "lvIONetworkModule.h"

class vtkInterpreter;
class vtkPacketRecorder;

#ifndef __VTK_WRAP__
#define vtkDataObjectAlgorithm vtkLiveSourceAlgorithm<vtkDataObjectAlgorithm>
#endif

class LVIONETWORK_EXPORT vtkStream : public vtkDataObjectAlgorithm
{
public:
  vtkTypeMacro(vtkStream, vtkDataObjectAlgorithm)

  vtkMTimeType GetMTime() override;

  virtual void Start();
  virtual void Stop();

  vtkGetMacro(ListeningPort, unsigned int);
  vtkSetMacro(ListeningPort, unsigned int);

  vtkGetMacro(IsCrashAnalysing, bool);
  vtkSetMacro(IsCrashAnalysing, bool);

  vtkGetObjectMacro(PacketHandler, vtkStreamPacketHandler);
  void SetPacketHandler(vtkStreamPacketHandler* handler);

  /**
   * @brief GetNeedsUpdate
   * @return true if a new data is ready
   */
  bool GetNeedsUpdate();

  /**
   * @brief CheckForNewData Check if there is new data available
   * @warning Not thread safe! Be sure the DataMutex is locked before calling it!
   *
   */
  virtual int CheckForNewData() = 0;

  /**
   * @brief mutex to protect the data store in the Stream as they are filled
   * in one thread by the ConsumerThread, and accesed in another thread by the
   * RequestData.
   *
   * @todo it look like the interpreter is another shared ressource.
   * However unless making the Interpreter thread-safe, it seams impossible
   * to lock it before accesing it, as it will be accessed by some generated
   * code, due to paraview.
   */
  std::mutex DataMutex;

protected:
  vtkStream();
  virtual ~vtkStream();

  /**
   * Used to start the stream automatically.
   */
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Can be used by subclasses to start the stream with specifics parameters
   */
  void Start(const std::vector<unsigned int>& ports);

  /**
   * This function is called for each packet received.
   * Should be implemented by subclasses.
   */
  virtual void ConsumePacket(const std::vector<uint8_t>& pkt, double timestamp) = 0;

private:
  vtkStream(const vtkStream&) = delete;
  void operator=(const vtkStream&) = delete;

  /*!< The port to receive information*/
  unsigned int ListeningPort = 2368;

  bool IsCrashAnalysing = false;

  vtkSmartPointer<vtkStreamPacketHandler> PacketHandler;
};

#ifndef __VTK_WRAP__
#undef vtkDataObjectAlgorithm
#endif

#endif // vtkStream_h
