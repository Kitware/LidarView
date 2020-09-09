#include "vtkKITTIObjectLabelsReader.h"

#include <vtkMultiBlockDataSet.h>
#include <vtkObjectFactory.h>
#include <vtkInformationVector.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkInformation.h>
#include <vtkPolyData.h>
#include <vtkFieldData.h>
#include <vtkCubeSource.h>
#include <vtkStringArray.h>
#include <vtkIntArray.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <Eigen/Geometry>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

# include <boost/filesystem.hpp>

#include "vtkHelper.h"


namespace {

typedef struct object {
  std::string type;
  float truncation;
  int occlusion;
  float alpha;  // rad
  std::vector<float> bbox2d;  // xmin, ymin, xmax, ymax
  float height;
  float width;
  float length;
  std::vector<float> position;  // x, y, z
  float rotY;  // rad
  float score = 1;
} object_t;


/**
 * @brief ParseObject parses a label line to create an `object` instance
 */
object_t ParseObject(std::string line){
  std::stringstream ss(line);
  object_t object;
  std::string tmp_str;
  std::getline(ss, object.type, ' ');
  std::getline(ss, tmp_str, ' ');
  object.truncation = std::stof(tmp_str);
  std::getline(ss, tmp_str, ' ');
  object.occlusion = std::stoi(tmp_str);
  std::getline(ss, tmp_str, ' ');
  object.alpha = std::stof(tmp_str);
  for (unsigned int ii=0; ii<4; ii++)
  {
    std::getline(ss, tmp_str, ' ');
    object.bbox2d.push_back(std::stof(tmp_str));
  }
  std::getline(ss, tmp_str, ' ');
  object.height = std::stof(tmp_str);
  std::getline(ss, tmp_str, ' ');
  object.width = std::stof(tmp_str);
  std::getline(ss, tmp_str, ' ');
  object.length = std::stof(tmp_str);
  for (unsigned int ii=0; ii<3; ii++)
  {
    std::getline(ss, tmp_str, ' ');
    object.position.push_back(std::stof(tmp_str));
  }
  std::getline(ss, tmp_str, ' ');
  object.rotY = std::stof(tmp_str);

  if (std::getline(ss, tmp_str, ' '))
  {
    object.score = std::stof(tmp_str);
  }
  return object;
}


/*
 * @brief ConvertLabelToPolyData converts a object_t label (for example parsed 
 * with ParseObject) to a PolyData (a 3D box with field data containins its 
 * attributes)
 **/
vtkSmartPointer<vtkPolyData> ConvertLabelToPolyData(const object_t object)
{

  auto cubeSource = vtkSmartPointer<vtkCubeSource>::New();

  // Generate 3d bbox centered on zero with the correct dimensions in order to
  // be able to rotate the box around its center
  cubeSource->SetCenter(0, 0, 0);
  cubeSource->SetXLength(object.height);
  cubeSource->SetYLength(object.width);
  cubeSource->SetZLength(object.length);
  std::cout << "Object " << object.type <<" with size: " << object.height << ", " << object.width << ", " << object.length << std::endl;
  cubeSource->Update();
  vtkSmartPointer<vtkPolyData> bb = cubeSource->GetOutput();


  // Construct the transform from object_t to move the bbox to its correct
  // position and orientation
  Eigen::Translation3d ts(object.position[0], object.position[1], object.position[2]);
  Eigen::Matrix3d r(Eigen::AngleAxisd(object.rotY, Eigen::Vector3d::UnitY());  // in rad
  Eigen::Isometry3d p(ts * Eigen::Quaterniond(r));
  vtkNew<vtkMatrix4x4> m;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
    {
      m->SetElement(i, j, p(i, j));
    }
  vtkNew<vtkTransform> tm;
  tm->SetMatrix(m);

  // transform the bounding box
  vtkNew<vtkTransformPolyDataFilter> transformFilter;
  transformFilter->SetInputData(0, bb);
  transformFilter->SetTransform(tm);
  transformFilter->Update();

  bb = transformFilter->GetOutput();

  // Add additionnal info as field data
  auto typeData = createArray<vtkStringArray>("type", 1, 1);
  typeData->SetValue(0, object.type);
  bb->GetFieldData()->AddArray(typeData);

  auto truncData = createArray<vtkFloatArray>("truncation", 1, 1);
  truncData->SetValue(0, object.truncation);
  bb->GetFieldData()->AddArray(truncData);

  auto occlData = createArray<vtkIntArray>("occlusion", 1, 1);
  occlData->SetValue(0, object.occlusion);
  bb->GetFieldData()->AddArray(occlData);

  auto scoreData = createArray<vtkFloatArray>("score", 1, 1);
  scoreData->SetValue(0, object.score);
  bb->GetFieldData()->AddArray(scoreData);

  return bb;
}

}


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKITTIObjectLabelsReader)


//-----------------------------------------------------------------------------
vtkKITTIObjectLabelsReader::vtkKITTIObjectLabelsReader()
{
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
void vtkKITTIObjectLabelsReader::SetFileName(const std::string &filename)
{
  if (filename == this->FileName)
  {
    return;
  }

  if (!boost::filesystem::exists(filename))
  {
    vtkErrorMacro("Folder not be found! Contrary to what the name of this function implies,"
                    " the input must be the folder containing all \".txt\" label  files for a given sequence/dataset");
    return;
  }

  // count number of frames inside the folder
  this->NumberOfFrames = 0;
  boost::filesystem::path folder(filename);
  boost::filesystem::directory_iterator it(folder);
  while (it != boost::filesystem::directory_iterator())
  {
    this->NumberOfFrames++;
    *it++;
  }
  this->FileName = filename + "/";
  this->Modified();
}



//-----------------------------------------------------------------------------
void vtkKITTIObjectLabelsReader::GetLabelData(int frameIndex, vtkMultiBlockDataSet* output)
{
  // produce path to the required .txt file
  std::stringstream ss;
  ss << std::setw(this->NumberOfFileNameDigits) << std::setfill('0') << frameIndex;
  std::string filename = this->GetFileName() + ss.str() + ".txt";

  std::vector<object_t> objects;

  // parse label file
  fstream f;
  f.open(filename, ios::in);
  if (f.is_open()){
    std::string line;
    while(std::getline(f, line)){
      auto object = ParseObject(line);
      if (object.type != "DontCare")
      {    
        objects.push_back(object);
      }
    }
    f.close();
  }

  // create a polydata for each object with a 3d bounding box and field values
  unsigned int nbObjects = objects.size();
  output->SetNumberOfBlocks(nbObjects);
  for (unsigned int i=0; i < nbObjects; i++)
  {
    output->SetBlock(i, ConvertLabelToPolyData(objects[i]));
  }
}

//-----------------------------------------------------------------------------
int vtkKITTIObjectLabelsReader::RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *outputVector)
{
  if (this->FileName.empty())
  {
    vtkErrorMacro("Please specify a file name")
    return VTK_ERROR;
  }
  auto *output = vtkMultiBlockDataSet::GetData(outputVector->GetInformationObject(0));

  auto *info = outputVector->GetInformationObject(0);

  int timestep = 0;
  if (info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    timestep = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }
  // Because no timestep are present in the .bin file we consider that timestep = frame number.
  if (timestep < 0 || timestep >= this->GetNumberOfFrames())
  {
    vtkErrorMacro("Cannot fulfill timestep request: " << timestep << ".  There are only "
                                                   << this->GetNumberOfFrames() << " datasets.");
    return 0;
  }
//  output->ShallowCopy(GetLabelData(timestep));
  this->GetLabelData(timestep, output);
  return 1;
}


//----------------------------------------------------------------------------
int vtkKITTIObjectLabelsReader::RequestInformation(vtkInformation* vtkNotUsed(request),
                                                   vtkInformationVector** vtkNotUsed(inputVector),
                                                   vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  int numberOfTimesteps = this->NumberOfFrames;
  std::vector<double> timesteps;
  for (int i = 0; i < numberOfTimesteps; ++i)
  {
    timesteps.push_back(i);
  }

  if (numberOfTimesteps)
  {
    double timeRange[2] = { timesteps.front(), timesteps.back() };
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timesteps.front(), timesteps.size());
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }
  else
  {
    info->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    info->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }
  return 1;
}
