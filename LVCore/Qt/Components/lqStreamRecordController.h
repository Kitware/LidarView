/*=========================================================================

  Program: LidarView
  Module:  lqStreamRecordController.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqStreamRecordController_h
#define lqStreamRecordController_h

#include "lqComponentsModule.h"

#include <vtkSmartPointer.h>

#include <QObject>
#include <QString>

class vtkSMProxy;
class pqPipelineSource;

class QTimer;

/**
 * Record all current stream data received in a single pcap file.
 */
class LQCOMPONENTS_EXPORT lqStreamRecordController : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  lqStreamRecordController(QObject* parent = nullptr);
  ~lqStreamRecordController() = default;

public Q_SLOTS:
  /**
   * When called start/stop recoding of stream.
   */
  void onRecordStream(bool status);

Q_SIGNALS:
  /**
   * Emitted to update the check state of the recording.
   */
  void recording(bool isRecording);

  /**
   * Emitted when a recording file is saved.
   */
  void recordingFileFinished(QString filepath);

private Q_SLOTS:
  void onSplitRecording();

private:
  Q_DISABLE_COPY(lqStreamRecordController)

  /**
   * Open pop up to choose file path where to save stream and start stream recording with vtkStream.
   * Return true if recording was started.
   */
  bool startRecording();

  ///@{
  /**
   * Start / stop writer proxy with the right configuration
   */
  void startWriterProxy();
  void stopWriterProxy();
  ///@}

  /**
   * Stop running stream recording and open a pop up to warn user.
   */
  void stopRecording();

  /**
   * Monitor the deleted source to stop stream if necessary
   */
  void onSourceRemoved(pqPipelineSource* src);

private:
  QTimer* SplitRecordTimer;
  vtkSmartPointer<vtkSMProxy> RecorderProxy;
  QString RecordingFilePath;
  QString BaseRecordingFileName;
  QString LastRecordingFileName;
  bool IsRecording = false;
};

#endif // lqStreamRecordController_H
