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
#include <numeric>

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
object_t ParseObject(const std::string &line){
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


vtkSmartPointer<vtkPolyData> ApplyEigenIsometryToPolyData(const Eigen::Isometry3d &p, vtkSmartPointer<vtkPolyData> pd)
{
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
  transformFilter->SetInputData(0, pd);
  transformFilter->SetTransform(tm);
  transformFilter->Update();

  return transformFilter->GetOutput();
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
  cubeSource->SetXLength(object.length);
  cubeSource->SetYLength(object.height);
  cubeSource->SetZLength(object.width);
  cubeSource->Update();
  vtkSmartPointer<vtkPolyData> bb = cubeSource->GetOutput();


  // Construct the transform from object_t to move the bbox to its correct
  // position and orientation
  // In the ground truth, it looks like the vertical position is the one of the bottom of the object,
  // differently from the other dimensions
  Eigen::Translation3d ts(object.position[0], object.position[1] - object.height / 2, object.position[2]);
  Eigen::AngleAxisd r(object.rotY, Eigen::Vector3d::UnitY());  // in rad
  Eigen::Isometry3d p(ts * r);

  bb = ApplyEigenIsometryToPolyData(p, bb);

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

/*
 * @brief ParseCalibFile parses a calibration file and returns the corresponding lidar to camera transformation
 * as an Eigen isometry
 **/
Eigen::Isometry3d ParseCalibFile(const std::string &filename)
{
  std::fstream f;
  f.open(filename, std::ios::in);
  // In order to project the detections from the rectified camera coordinate system to the lidar coordinate system, only R0_rect and Tr_velo_to_cam are required
  // need to be extracted
  std::vector<double> P2, R0_rect, Tr_velo_to_cam;

  if (f.is_open()){
    std::string line;
    while(std::getline(f, line)){
      std::stringstream ss(line);
      std::string key;
      std::getline(ss, key, ' ');
      if (key == "R0_rect:")
      {
        while (std::getline(ss, key, ' '))
        {
          R0_rect.push_back(std::stod(key));
        }
      }
      else if (key == "Tr_velo_to_cam:")
      {
        while (std::getline(ss, key, ' '))
        {
          Tr_velo_to_cam.push_back(std::stod(key));
        }
      }
    }
  }
  else
  {
    std::cerr << "Could not open calibration file." << std::endl;
    return Eigen::Isometry3d::Identity();
  }
  f.close();

  // Transform Tr_velo_to_cam into a 4x4 matrix using by adding a 1 as the bottom-right
  // element and 0's elsewhere.
  assert(R0_rect.size() == 9 && "R0_rect has an incorrect size, it should contain 3x3 elements");
  std::vector<double>::iterator it = R0_rect.begin();
  R0_rect.insert(it+3, 0);
  it = R0_rect.begin();
  R0_rect.insert(it+7, 0);
  it = R0_rect.begin();
  R0_rect.insert(it+11, 0);
  R0_rect.insert(R0_rect.end(), { 0, 0, 0, 1});
  Eigen::Matrix<double, 4, 4, Eigen::RowMajor> rotation(R0_rect.data());

  // Transform Tr_velo_to_cam into a 4x4 matrix using by adding a 1 as the bottom-right
  // element and 0's elsewhere.
  assert(Tr_velo_to_cam.size() == 12 && "Tr_velo_to_cam has an incorrect size, it should contain 3x4 elements");
  Tr_velo_to_cam.insert(Tr_velo_to_cam.end(), { 0, 0, 0, 1});

  Eigen::Matrix<double, 4, 4, Eigen::RowMajor> lidar2cam(Tr_velo_to_cam.data());

  return Eigen::Isometry3d(rotation * lidar2cam);
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
void vtkKITTIObjectLabelsReader::SetFolderName(const std::string &path)
{
  if (path == this->FolderName)
  {
    return;
  }

  if (!boost::filesystem::exists(path))
  {
    vtkErrorMacro("Folder not be found! Contrary to what the name of this function implies,"
                    " the input must be the folder containing all \".txt\" label  files for a given sequence/dataset");
    return;
  }

  // count number of frames inside the folder
  this->NumberOfFrames = 0;
  boost::filesystem::path folder(path);
  boost::filesystem::directory_iterator it(folder);
  while (it != boost::filesystem::directory_iterator())
  {
    this->NumberOfFrames++;
    *it++;
  }
  this->FolderName = path + "/";
  this->Modified();
}


//-----------------------------------------------------------------------------
void vtkKITTIObjectLabelsReader::SetCalibFolderName(const std::string &path)
{
  if (path == this->CalibFolderName)
  {
    return;
  }

  if (!boost::filesystem::exists(path))
  {
    vtkErrorMacro("Folder not be found! Please provide a valid folder path to the calibration files for each frame");
    return;
  }

  this->CalibFolderName = path + "/";
  this->Modified();
}


//-----------------------------------------------------------------------------
void vtkKITTIObjectLabelsReader::GetLabelData(int frameIndex, vtkMultiBlockDataSet* output)
{
  // produce path to the required .txt file
  std::stringstream ss;
  ss << std::setw(this->NumberOfFileNameDigits) << std::setfill('0') << frameIndex;
  std::string filename = this->GetFolderName() + ss.str() + ".txt";

  std::vector<object_t> objects;

  // parse label file (1 line per object)
  std::fstream f;
  f.open(filename, std::ios::in);
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

  Eigen::Isometry3d Lidar2Camera = Eigen::Isometry3d::Identity();
  if (this->UseCalibration)
  {
    // produce path to the calibration .txt file
    std::stringstream ss;
    ss << std::setw(this->NumberOfFileNameDigits) << std::setfill('0') << frameIndex;
    std::string calibFilename = this->GetCalibFolderName() + ss.str() + ".txt";
    Lidar2Camera = ParseCalibFile(calibFilename);
  }


  // create a polydata for each object with a 3d bounding box and field values
  unsigned int nbObjects = objects.size();
  output->SetNumberOfBlocks(nbObjects);
  for (unsigned int i=0; i < nbObjects; i++)
  {
    auto bb = ConvertLabelToPolyData(objects[i]);
    if (this->UseCalibration)
    {
      Eigen::Isometry3d inv = Lidar2Camera.inverse();
      bb = ApplyEigenIsometryToPolyData(inv, bb);
    }
    output->SetBlock(i, bb);
  }
}

//-----------------------------------------------------------------------------
int vtkKITTIObjectLabelsReader::RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *outputVector)
{
  if (this->FolderName.empty())
  {
    vtkErrorMacro("Please specify a folder name");
    return VTK_ERROR;
  }
  if (this->UseCalibration && this->CalibFolderName.empty())
  {
    vtkErrorMacro("Please specify a calibration folder path or untick 'Use Calibration'");
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
  std::vector<double> timesteps(numberOfTimesteps);
  std::iota(timesteps.begin(), timesteps.end(), 0.);

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
