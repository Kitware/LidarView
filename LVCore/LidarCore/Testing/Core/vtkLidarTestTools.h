/*=========================================================================

  Program: LidarView
  Module:  vtkLidarTestTools.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkLidarTestTools_h
#define vtkLidarTestTools_h

#include "lvTestingCoreModule.h" // for export macro

#include <string>

class vtkLidarReader;
class vtkLidarStream;

struct LVTESTINGCORE_EXPORT vtkLidarTestTools
{
  /**
   * Compare the reader output to prerecorded frames, indicated in
   * referenceDataFileName.
   */
  static int TestLidarReaderWithBaseline(vtkLidarReader* reader,
    const std::string& referenceDataFileName);

  /**
   * Compare the stream output to prerecorded frames, indicated in
   * referenceDataFileName. ShouldPreSend can be used to configure
   * autocalib LiDARs.
   */
  static int TestLidarStreamWithBaseline(vtkLidarStream* stream,
    const std::string& pcapFileName,
    const std::string& referenceDataFileName,
    bool shouldPreSend = false);

  /**
   * Testing suite for packet interpreter. Check scalars names,
   * point timestamps sanity and transform.
   */
  static int TestPacketInterpreter(vtkLidarReader* reader);

private:
  /**
   * Internal test specific for packet interpreter timestamp coherency.
   * Used in TestPacketInterpreter.
   */
  static int TestPacketInterpreterTimeFrames(vtkLidarReader* reader, int type);
};

VTK_ABI_NAMESPACE_END
#endif // vtkLidarTestTools_h
