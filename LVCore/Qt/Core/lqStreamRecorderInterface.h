/*=========================================================================

  Program: LidarView
  Module:  lqStreamRecorderInterface.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqStreamRecorderInterface_h
#define lqStreamRecorderInterface_h

#include <QObject>

#include "lqCoreModule.h" // needed for export macro
#include <QtPlugin>       // needed for Q_DECLARE_INTERFACE

class pqPipelineSource;

/**
 * Interface that can be use to implement another stream / live source recorder (e.g PCAP, MCAP...)
 */
class LQCORE_EXPORT lqStreamRecorderInterface
{
public:
  virtual ~lqStreamRecorderInterface();

  /**
   * Return label to use for this recorder.
   */
  virtual QString label() = 0;

  /**
   * Return true if the interface support recording the given proxy.
   */
  virtual bool canRecord(pqPipelineSource* src) = 0;

  /**
   * Return true if the recorder support record multiple sources in the same
   * file.
   */
  virtual bool supportMultipleSources() = 0;

  ///@{
  /**
   * Methods to start / stop a recording.
   */
  virtual bool startRecording(QString recordingPath, QList<pqPipelineSource*> src) = 0;
  virtual void stopRecording() = 0;
  ///@}

  /**
   * Split a recording in multiple separate files, usually call start and then stop internal methods.
   */
  virtual void splitRecording() = 0;

  /**
   * Return the last filename that was use to save a file
   */
  virtual QString recordingFilename() = 0;

protected:
  lqStreamRecorderInterface();

private:
  Q_DISABLE_COPY(lqStreamRecorderInterface)
};

Q_DECLARE_INTERFACE(lqStreamRecorderInterface, "com.kitware/lidarview/streamrecorder")

#endif
