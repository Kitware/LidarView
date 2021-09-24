#include "vtkPositionOrientationPacketReader.h"
#include "vtkHelper.h"

#include <vtkCellArray.h>
#include <vtkInformation.h>
#include <vtkLine.h>
#include <vtkNew.h>
#include <vtkPolyLine.h>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPositionOrientationPacketReader)

//-----------------------------------------------------------------------------
vtkPositionOrientationPacketReader::vtkPositionOrientationPacketReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
}

//-----------------------------------------------------------------------------
int vtkPositionOrientationPacketReader::FillOutputPortInformation(int port, vtkInformation* info)
{
  if ( port == 0 )
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData" );
    return 1;
  }
  if ( port == 1 )
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable" );
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
vtkMTimeType vtkPositionOrientationPacketReader::GetMTime()
{
  if (this->Interpreter)
  {
    return std::max(this->Superclass::GetMTime(), this->Interpreter->GetMTime());
  }
  return this->Superclass::GetMTime();
}

//-----------------------------------------------------------------------------
void vtkPositionOrientationPacketReader::SetFileName(const std::string &filename)
{
  if (filename == this->FileName)
  {
    return;
  }

  this->FileName = filename;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPositionOrientationPacketReader::Open()
{
  this->Close();
  this->Reader = new vtkPacketFileReader;

  std::string filterPCAP = "udp";
  if (this->PositionOrientationPort != -1)
  {
    filterPCAP += " port " + std::to_string(this->PositionOrientationPort);
  }

  if (!this->Reader->Open(this->FileName, filterPCAP.c_str()))
  {
    vtkErrorMacro(<< "Failed to open packet file: " << this->FileName << "!\n"
                                                 << this->Reader->GetLastError());
    this->Close();
  }
}

//-----------------------------------------------------------------------------
void vtkPositionOrientationPacketReader::Close()
{
  delete this->Reader;
  this->Reader = nullptr;
}

//-----------------------------------------------------------------------------
void vtkPositionOrientationPacketReader::ReadPositionOrientation(vtkSmartPointer<vtkPolyData> & positionOrientationInfo,
                                                                 vtkSmartPointer<vtkTable> & rawInfo)
{
  if (!this->Reader)
  {
    vtkErrorMacro("ReadPositionOrientation() called but packet file reader is not open.");
    return;
  }

  const unsigned char* data = nullptr;
  unsigned int dataLength = 0;
  double timeSinceStart;

  while (this->Reader->NextPacket(data, dataLength, timeSinceStart))
  {
    // If the current packet is not valid,
    // skip it and update the file position
    if (!this->Interpreter->IsValidPacket(data, dataLength))
    {
      continue;
    }

    // Process the packet
    this->Interpreter->ProcessPacket(data, dataLength);
  }

  // Save positions and orientation information if packets contain them
  if(this->Interpreter->HasPositionOrientationInformation())
  {
    positionOrientationInfo = this->Interpreter->GetPositionOrientation();

    // Set the polyline to the poly data to see the position orientation information
    vtkSmartPointer<vtkPolyLine> polyLine = CreatePolyLineFromPoints(positionOrientationInfo->GetPoints());
    vtkNew<vtkCellArray> cellArray;
    cellArray->InsertNextCell(polyLine);
    positionOrientationInfo->SetLines(cellArray);

    this->Interpreter->FillInterpolatorFromPositionOrientation();
  }

  // Save raw information if packets contain them
  if(this->Interpreter->HasRawInformation())
  {
    rawInfo = this->Interpreter->GetRawInformation();
  }

}

//-----------------------------------------------------------------------------
int vtkPositionOrientationPacketReader::RequestData(vtkInformation *vtkNotUsed(request),
                                                    vtkInformationVector **vtkNotUsed(inputVector),
                                                    vtkInformationVector *outputVector)
{
  vtkPolyData* outputPositionOrientation = vtkPolyData::GetData(outputVector);
  vtkTable* outputRawInformation = vtkTable::GetData(outputVector, 1);

  if (this->FileName.empty())
  {
    vtkErrorMacro("FileName has not been set.");
    return 0;
  }

  if (this->Interpreter)
  {
    // We need to reset the interpreter data before filling it to avoid data stacking.
    // For example, this can happen if the sensor transform is modified after a first instantiation
    // The previous result needs to be erase to avoid having 2 trajectories
    this->Interpreter->ResetCurrentData();

    this->Open();
    vtkSmartPointer<vtkPolyData> polydata;
    vtkSmartPointer<vtkTable> table;
    this->ReadPositionOrientation(polydata, table);
    if(polydata)
    {
      outputPositionOrientation->ShallowCopy(polydata);
    }
    if(table)
    {
      outputRawInformation->ShallowCopy(table);
    }
    this->Close();
  }

  return 1;
}
