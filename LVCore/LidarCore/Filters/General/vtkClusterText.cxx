/*=========================================================================

  Program: LidarView
  Module:  vtkClusterText.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClusterText.h"

#include <vtkDataArray.h>
#include <vtkFieldData.h>
#include <vtkInformation.h>
#include <vtkIntArray.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkVectorText.h>

#include <array>
#include <iomanip>
#include <sstream>
#include <vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkClusterText);

//----------------------------------------------------------------------------
int vtkClusterText::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get IO
  auto input = vtkMultiBlockDataSet::GetData(inputVector[0]);
  auto output = vtkMultiBlockDataSet::GetData(outputVector, 0);

  unsigned int numBlocks = input->GetNumberOfBlocks();
  output->SetNumberOfBlocks(numBlocks);
  for (unsigned int blockId = 0; blockId < numBlocks; ++blockId)
  {
    // Get block
    auto block = vtkDataObject::SafeDownCast(input->GetBlock(blockId));
    if (!block || !block->GetFieldData())
    {
      continue;
    }
    // Get field data
    vtkFieldData* fd = block->GetFieldData();
    if (!fd || !this->FieldDataArrayName)
    {
      continue;
    }
    // Get cluster information from field data
    vtkDataArray* centerArr = vtkDataArray::SafeDownCast(fd->GetArray("Center"));
    vtkDataArray* heightArr = vtkDataArray::SafeDownCast(fd->GetArray("Height"));
    vtkIntArray* clusterIdArr = vtkIntArray::SafeDownCast(fd->GetArray("ClusterId"));
    vtkAbstractArray* valueArray = fd->GetAbstractArray(this->FieldDataArrayName);
    if (!centerArr || !heightArr || !valueArray || !clusterIdArr)
    {
      vtkWarningMacro(<< "Missing required arrays to display cluster information");
      continue;
    }
    double center[3];
    centerArr->GetTuple(0, center);
    double height = heightArr->GetTuple1(0);

    // Build text
    std::ostringstream txt;
    txt << this->FieldDataArrayName << ": " << std::fixed << std::setprecision(2);
    if (valueArray->GetNumberOfComponents() == 1)
    {
      txt << valueArray->GetVariantValue(0);
    }
    else
    {
      txt << "(";
      for (int c = 0; c < valueArray->GetNumberOfComponents(); ++c)
      {
        if (c > 0)
        {
          txt << ", ";
        }
        txt << valueArray->GetVariantValue(c);
      }
      txt << ")";
    }
    vtkNew<vtkVectorText> text3D;
    text3D->SetText(txt.str().c_str());
    text3D->Update();

    vtkNew<vtkTransform> transform;
    transform->Translate(center[0], center[1], center[2] + height * 0.6);
    transform->RotateX(90);
    transform->RotateY(-90);
    transform->Scale(this->TextScale, this->TextScale, this->TextScale);

    vtkNew<vtkTransformPolyDataFilter> tfText;
    tfText->SetInputConnection(text3D->GetOutputPort());
    tfText->SetTransform(transform);
    tfText->Update();

    vtkSmartPointer<vtkPolyData> labelPoly = tfText->GetOutput();
    labelPoly->GetFieldData()->ShallowCopy(fd);
    output->SetBlock(blockId, labelPoly);
    std::string labelblockName("Cluster-" + std::to_string(clusterIdArr->GetValue(0)));
    output->GetMetaData(blockId)->Set(vtkCompositeDataSet::NAME(), labelblockName.c_str());
  }
  return 1;
}
