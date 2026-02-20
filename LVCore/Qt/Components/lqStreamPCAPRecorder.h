/*=========================================================================

  Program: LidarView
  Module:  lqStreamPCAPRecorder.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqStreamPCAPRecorder_h
#define lqStreamPCAPRecorder_h

#include "lqStreamRecorderInterface.h"

#include "lqComponentsModule.h" // needed for export macros

#include <vtkSmartPointer.h>

class vtkSMProxy;

/**
 * Recorder for PCAP files.
 */
class LQCOMPONENTS_EXPORT lqStreamPCAPRecorder
  : public QObject
  , public lqStreamRecorderInterface
{
  Q_OBJECT
  Q_INTERFACES(lqStreamRecorderInterface)

  typedef QObject Superclass;

public:
  lqStreamPCAPRecorder(QObject* parent = nullptr);
  ~lqStreamPCAPRecorder() override;

  /**
   * Return label to use for this recorder.
   */
  QString label() override { return "PCAP"; }

  /**
   * Return true if the interface support recording the given proxy.
   */
  bool canRecord(pqPipelineSource* src) override;

  /**
   * Return true if the recorder support record multiple sources in the same
   * file.
   */
  bool supportMultipleSources() override { return true; }

  ///@{
  /**
   * Methods to start / stop a recording.
   */
  bool startRecording(QString recordingPath, QList<pqPipelineSource*> src) override;
  void stopRecording() override;
  ///@}

  /**
   * Split a recording in multiple separate files.
   */
  void splitRecording() override;

  /**
   * Return the last filename that was use to save a file
   */
  QString recordingFilename() override;

private:
  Q_DISABLE_COPY(lqStreamPCAPRecorder)

  void startWriterProxy();
  void stopWriterProxy();

  vtkSmartPointer<vtkSMProxy> RecorderProxy;
  QString RecordingFilePath;
  QString BaseRecordingFileName;
  QString LastRecordingFileName;
};

#endif
