#ifndef LQSAVELIDARFRAMEREACTION_H
#define LQSAVELIDARFRAMEREACTION_H

#include <pqReaction.h>

#include <vtkSmartPointer.h>

#include "lqapplicationcomponents_export.h"

class pqPipelineSource;
class vtkSMSourceProxy;
class vtkSMProxy;

/**
* @ingroup Reactions
* Save the frame generate by a LidarReader. The writer should be choosen at compile time.
* Basic example are `PCDWriter` or `DataSetCSVWriter`.
*
* The reaction provide some basic UI to let the user select the frame to save:
* - current frame
* - all frame
* - frame interval
*/
class LQAPPLICATIONCOMPONENTS_EXPORT lqSaveLidarFrameReaction : public pqReaction
{
    Q_OBJECT
    typedef pqReaction Superclass;

public:
  lqSaveLidarFrameReaction(QAction* action, const QString& writerName,
                           const QString& extension, bool DisplaySettings = false,
                           bool useDirectory = false, bool keepNameFromPcapFile = false,
                           bool fileNameWithFrameNumber = false);

  /**
   *  Save the frame generate by the lidar "lidar"
   *  Start and Stop correspond to the timestep index. Both are included [start; stop]
   *  In case both are -1, the current timestep is saved
   */
  virtual bool saveFrame(vtkSMProxy * lidar ,int start = -1, int stop = -1);

  pqPipelineSource * getCorrectLidar();

public slots:
  /**
   * Enable/disable the QAction
   */
  void onUpdateUI(pqPipelineSource* src);

  /**
  * Called when the action is triggered.
  */
  virtual void onTriggered() override;

protected:
  /**
   * Create the writer.
   * Overwrite this function to change the writer default settings
   */
  virtual vtkSmartPointer<vtkSMSourceProxy> CreateWriter(vtkSMSourceProxy* lidarProxy);

  /**
   * Generate the filename given the baseName and the timestep
   * Overwrite this function to have a custom naming convention
   */
  virtual QString GenerateFileName(QString baseName, double timestep);

  /**
   * Get the folder and the baseName from the user (with a file dialog).
   */
  virtual bool GetFolderAndBaseNameFromUser(vtkSMProxy * lidar);


  // True if the user only select a directory where all exported files will be saved (with default filename)
  bool UseDirectory;

  // True if the exported filename should have the same name as the origin pcap file ("Frame" otherwise)
  bool KeepNameFromPcapFile;

  // True if the exported fileName should have the number of the frame (instead of the timestamp)
  bool FileNameWithFrameNumber;

  // True if the settings of the writer should be exposed to the user
  bool DisplaySettings;

  QString FolderPath;
  QString BaseName;

  QString WriterName;
  QString Extension;

private:
  Q_DISABLE_COPY(lqSaveLidarFrameReaction)
};

#endif // LQSAVELIDARFRAMEREACTION_H
