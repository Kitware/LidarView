/*=========================================================================

  Program: LidarView
  Module:  vtkPacketRecorder.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkPacketRecorder_h
#define vtkPacketRecorder_h

#include "lvIONetworkModule.h"
#include <vtkObject.h>
#include <vtkSetGet.h>

#include <memory>
#include <thread>

namespace Tins
{
class Packet;
};

/**
 * vtkPacketRecorder will write every packet it receives in the writing queue.
 */
class LVIONETWORK_EXPORT vtkPacketRecorder : public vtkObject
{
public:
  static vtkPacketRecorder* New();
  vtkTypeMacro(vtkPacketRecorder, vtkObject);

  ///@{
  /**
   * Set the output file name, it must be set to be able to start recording.
   */
  vtkSetMacro(RecordingFileName, std::string);
  vtkGetMacro(RecordingFileName, std::string);
  ///@}

  ///@{
  /**
   * Start or stop recording the UDP stream.
   * Calling StartRecording creates a new pcap file with the specified filename.
   */
  void StartRecording();
  void StopRecording();
  vtkGetMacro(IsRecording, bool);
  ///@}

  /**
   * Add a tins packet to the packet queue to write.
   * Note: the recorder is able to drop packets if the writer is not fast enough.
   */
  void AddPacketToWritingQueue(const Tins::Packet* pkt);

protected:
  vtkPacketRecorder();
  ~vtkPacketRecorder() override;

private:
  vtkPacketRecorder(const vtkPacketRecorder&) = delete;
  void operator=(const vtkPacketRecorder&) = delete;

  std::string RecordingFileName;
  std::unique_ptr<std::thread> WritingThread;
  std::atomic<bool> IsRecording = false;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
