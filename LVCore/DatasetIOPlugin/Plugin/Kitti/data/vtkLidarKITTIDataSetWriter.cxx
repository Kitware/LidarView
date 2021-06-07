#include "vtkLidarKITTIDataSetWriter.h"

#include <vtkInformationVector.h>
#include <vtkInformation.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <vector>
#include <fstream>
#include <sstream>

#include <boost/filesystem.hpp>

namespace
{

/*
 * @brief WriteToBinaryFile writes the information to export in a binary file
 * */
void WriteToBinaryFile(std::vector<float> &inData, const std::string &filename)
{
  std::ofstream fout(filename, std::ios::out | std::ios::binary);
  if (!fout.is_open())
  {
    std::cout << "Error: could not open file " << filename << " for writing." << std::endl;
  }
  else
  {
    fout.write(reinterpret_cast<char*>(&inData[0]), inData.size() * sizeof(float));
    fout.close();
  }
}
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkLidarKITTIDataSetWriter)

//-----------------------------------------------------------------------------
vtkLidarKITTIDataSetWriter::vtkLidarKITTIDataSetWriter()
{
// TODO: set it as a writer
//  this->SetNumberOfOutputPorts(0);
}


void vtkLidarKITTIDataSetWriter::UpdatePipelineIndex(vtkInformation * inInfo)
{
  // Save current pipeline time step
  this->PipelineTime = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  // TODO check best way to get the pipeline index

  double *time_steps = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int nb_time_steps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  std::vector<double> TimeSteps(time_steps, time_steps + nb_time_steps);
  // get the index corresponding to the requested pipeline time
  // find the index of the first time step that is not less than pipeline time
  this->PipelineIndex = std::distance(TimeSteps.begin(),
                                      std::lower_bound(TimeSteps.begin(),
                                                       TimeSteps.end(),
                                                       this->PipelineTime));

  // check if the previous index is closer
  if (this->PipelineIndex > 0)
  {
    if (TimeSteps[this->PipelineIndex] - this->PipelineTime > this->PipelineTime - TimeSteps[this->PipelineIndex - 1])
    {
      this->PipelineIndex -= 1;
    }
  }
}

//-----------------------------------------------------------------------------
int vtkLidarKITTIDataSetWriter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
                                      vtkInformationVector** inputVector,
                                      vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  this->UpdatePipelineIndex(inInfo);

  return 1;
}

//-----------------------------------------------------------------------------
void vtkLidarKITTIDataSetWriter::SetFolderName(const std::string &foldername)
{
  if (foldername.empty())
  {
    this->FolderName = "";
    return;
  }
  if (!boost::filesystem::is_directory(foldername))
  {
     boost::filesystem::create_directory(foldername);
  }
  this->FolderName = foldername;
  return;
}

std::vector<float> vtkLidarKITTIDataSetWriter::ParseCloudData(vtkSmartPointer<vtkPolyData> cloud, vtkDataArray* intensity)
{
  std::vector<float> dataToWrite(4 * cloud->GetNumberOfPoints());

  for (int pointIndex = 0; pointIndex < cloud->GetNumberOfPoints(); ++pointIndex)
  {
    double* pos = cloud->GetPoint(pointIndex);
    float ptIntensity = static_cast<float>(intensity->GetTuple1(pointIndex));
    if (this->NormalizeIntensity)
    {
      ptIntensity /= this->InputIntensityMaxValue;
    }

    dataToWrite[4 * pointIndex] = static_cast<float>(pos[0]);
    dataToWrite[4 * pointIndex + 1] = static_cast<float>(pos[1]);
    dataToWrite[4 * pointIndex + 2] = static_cast<float>(pos[2]);
    dataToWrite[4 * pointIndex + 3] = ptIntensity;
  }

  return dataToWrite;
}

//-----------------------------------------------------------------------------
int vtkLidarKITTIDataSetWriter::RequestData(vtkInformation *, vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  if (this->FolderName.empty())
  {
    vtkErrorMacro("FolderName is empty. Please provide a folder name to write to.");
  }
  // Get IO
  vtkPolyData* inCloud = vtkPolyData::GetData(inputVector[0], 0);
  vtkPolyData* outCloud = vtkPolyData::GetData(outputVector, 0);

  // Copy input over (TODO: remove this part when using as a writer)
  outCloud->ShallowCopy(inCloud);

  std::stringstream ss;
  ss << std::setw(this->NumberOfFileNameDigits) << std::setfill('0') << this->PipelineIndex;

  std::string frameFileName((boost::filesystem::path(this->FolderName) / (ss.str() + ".bin")).string());

  // Get intensity array
  vtkDataArray* intensity = this->GetInputArrayToProcess(0, inputVector);
  if (!intensity)
  {
    vtkErrorMacro(<<"No Intensity array selected.");
    return 1;
  }

  // Data for the .bin file are stored in a std::vector to make it easier to use
  std::vector<float> dataToWrite = this->ParseCloudData(inCloud, intensity);

  WriteToBinaryFile(dataToWrite, frameFileName);

  return VTK_OK;
}
