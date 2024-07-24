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

#include "lvIONetworkModule.h"

class NetworkPacket;
class PacketConsumer;
class PacketFileWriter;
class PacketReceiver;
class vtkInterpreter;

#ifndef __VTK_WRAP__
#define vtkDataObjectAlgorithm vtkLiveSourceAlgorithm<vtkDataObjectAlgorithm>
#endif

class LVIONETWORK_EXPORT vtkStream : public vtkDataObjectAlgorithm
{
public:
  vtkTypeMacro(vtkStream, vtkDataObjectAlgorithm)

  virtual void Start();
  virtual void Stop();

  vtkSetMacro(RecordingFilename, std::string);
  void StartRecording();
  void StartRecording(std::shared_ptr<PacketFileWriter> writer);
  void StopRecording();
  bool IsRecording();

  vtkGetMacro(ListeningPort, int);
  void SetListeningPort(int);

  vtkGetMacro(MulticastAddress, std::string);
  virtual void SetMulticastAddress(const std::string&);

  vtkGetMacro(LocalListeningAddress, std::string);
  virtual void SetLocalListeningAddress(const std::string&);

  vtkGetMacro(ForwardedIpAddress, std::string);
  virtual void SetForwardedIpAddress(const std::string& ipAddress);

  vtkGetMacro(ForwardedPort, int);
  void SetForwardedPort(int);

  vtkGetMacro(IsForwarding, bool);
  virtual void SetIsForwarding(bool);

  vtkGetMacro(IsCrashAnalysing, bool);
  virtual void SetIsCrashAnalysing(bool value);

  vtkGetObjectMacro(Interpreter, vtkInterpreter);

  /**
   * @brief GetNeedsUpdate
   * @return true if a new data is ready
   */
  bool GetNeedsUpdate();

  /**
   * @brief AddNewData Add the new data available to the specific buffer of the Stream
   * @warning Not thread safe! Be sure the DataMutex is locked before calling it!
   */
  virtual void AddNewData() = 0;

  /**
   * @brief ClearAllDataAvailable Clear the buffer that contains data
   * @warning Not thread safe! Be sure the DataMutex is locked before calling it!
   */
  virtual void ClearAllDataAvailable() = 0;

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

  vtkMTimeType GetMTime() override;

protected:
  vtkStream();
  // WARNING: inheriting classes destructors must call Stop() on this base class
  // because this base Stream class holds (and starts) the callback thread that
  // call the packet consumer of the derived class.
  // Forgetting to do so might let the callback thread "ConsumerThread" call
  // the consumer function "AddNewData" after actual inherited class destruction,
  // resulting in a pure virtual function call at runtime.
  virtual ~vtkStream() = 0;

  int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  virtual int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override = 0;

  //! Generic Interpreter
  vtkSmartPointer<vtkInterpreter> Interpreter;

private:
  vtkStream(const vtkStream&) = delete;
  void operator=(const vtkStream&) = delete;

  /*!< The port to receive information*/
  int ListeningPort = 2368;
  /*!< The multicast address to receive packets*/
  std::string MulticastAddress;
  /*!< The Listening address in case of multiples interfaces*/
  std::string LocalListeningAddress;

  /*!< The file to record into */
  std::string RecordingFilename;

  /*!< Allowing the forwarding of the packets*/
  bool IsForwarding = false;
  /*!< The port to send forwarded packets*/
  int ForwardedPort = 2369;
  /*!< The ip to send forwarded packets*/
  std::string ForwardedIpAddress = "127.0.0.1";

  bool IsCrashAnalysing = false;

  //! Thread that will listen on the network to get the packets
  std::unique_ptr<PacketReceiver> ReceiverThread;
  //! Thread that will consume the packets
  std::unique_ptr<PacketConsumer> ConsumerThread;

  //! Thread that will write the packets if recording, provided at the time of writing
  std::shared_ptr<PacketFileWriter> WriterThread;

  //! Callback function used by the ReceiverThread once a new NetworkPacket is ready
  //! The given packet will be queue in the Consumer and Writer thread queue.
  void EnqueuePacket(NetworkPacket* packet);

  bool IsRunning();

  //! helper function
  template <class T>
  void SetAttributeAndRestartIfRunning(T& attribute, const T& value);
};

#ifndef __VTK_WRAP__
#undef vtkDataObjectAlgorithm
#endif

#endif // vtkStream_h
