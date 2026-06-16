/*=========================================================================

  Program: LidarView
  Module:  vtkTemporalTransformsWriter.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTKTEMPORALTRANSFORMSWRITER_H
#define VTKTEMPORALTRANSFORMSWRITER_H

// #include <vtkPolyDataAlgorithm.h>
#include <vtkPolyDataWriter.h>

#include "lvIOCoreModule.h"

// Inspired by vtkObjWriter
class LVIOCORE_EXPORT vtkTemporalTransformsWriter : public vtkPolyDataWriter
{
public:
  static vtkTemporalTransformsWriter* New();
  vtkTypeMacro(vtkTemporalTransformsWriter, vtkPolyDataWriter)

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  vtkSetMacro(WriteInDegrees, bool);
  vtkGetMacro(WriteInDegrees, bool);

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkTemporalTransformsWriter() = default;
  ~vtkTemporalTransformsWriter();

private:
  vtkTemporalTransformsWriter(const vtkTemporalTransformsWriter&) = delete;
  void operator=(const vtkTemporalTransformsWriter&) = delete;

  bool WriteInDegrees = false;

  char* FileName = nullptr;
};

#endif
