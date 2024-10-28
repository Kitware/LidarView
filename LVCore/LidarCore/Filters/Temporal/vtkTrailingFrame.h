/*=========================================================================

  Program:   LidarView
  Module:    vtkTrailingFrame.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkTrailingFrame_h
#define vtkTrailingFrame_h

#include <queue>

#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkPolyDataAlgorithm.h>

#include "lvFiltersTemporalModule.h"

/**
 * @brief The vtkTrailingFrame class is a filter that combine consecutive timestep
 * of its input to produce a multiblock.
 * The input of this filter must produce only consecutive interger timestep.
 */
class LVFILTERSTEMPORAL_EXPORT vtkTrailingFrame : public vtkPolyDataAlgorithm
{
public:
  static vtkTrailingFrame* New();
  vtkTypeMacro(vtkTrailingFrame, vtkPolyDataAlgorithm)

  //! @{
  //! @copydoc NumberOfTrailingFrames
  vtkGetMacro(NumberOfTrailingFrames, unsigned int);
  void SetNumberOfTrailingFrames(const unsigned int value);
  //! @}

  //! @{
  //! @copydoc UseStreamMode
  vtkGetMacro(UseStreamMode, bool);
  vtkSetMacro(UseStreamMode, bool);
  //! @}

  //! @{
  //! @copydoc UseCache
  vtkGetMacro(UseCache, bool);
  vtkSetMacro(UseCache, bool);
  //! @}

protected:
  vtkTrailingFrame() = default;

  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  //! Number of previous timestep to display
  unsigned int NumberOfTrailingFrames = 0;
  //! Should the internal cache be used for speed
  bool UseCache = true;
  //! Use TrailingFrame filter in stream mode
  bool UseStreamMode = false;

  //! Original pipeline time which must be restored after modifying the input filter time
  double PipelineTime = 0;
  //! Index of time step corresponding to PipelineTime
  int PipelineIndex = 0;
  //! Time index range that should be in the cache, it's e right half-open interval : [Tstart, Tend[
  int CacheTimeRange[2] = { -1, -1 };
  //! Last Time index required from the filter to its input filter
  int LastTimeProcessedIndex = -1;
  //! Indicate if the next time to process is after or before the last processed
  int Direction = 1;
  //! In stream keep track of current number of trailing frames
  unsigned int CurrentTrailingFramesNumber = 0;
  //! Cache to save ouput previously produced by the filter
  vtkNew<vtkMultiBlockDataSet> Cache;
  //! List of available time steps from the source
  std::vector<double> TimeSteps;

  //! Help variable
  bool FirstFilterIteration = true;

  // Workaround to handle that multiple RequestUpdateExtent can be call
  // The filter made the assumption that each RequestUpdateExtent is follow by a RequestData
  bool LastCallWasRequestUpdateExtentCall = false;

  int ProcessReadingMode(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  int ProcessStreamingMode(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  vtkTrailingFrame(const vtkTrailingFrame&); // not implemented
  void operator=(const vtkTrailingFrame&);   // not implemented
};

#endif // vtkTrailingFrame_h
