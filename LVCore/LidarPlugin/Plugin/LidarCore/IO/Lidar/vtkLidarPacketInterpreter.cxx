#include "vtkLidarPacketInterpreter.h"

#include <ctime>
#include <sstream>

namespace {
//-----------------------------------------------------------------------------
vtkSmartPointer<vtkCellArray> NewVertexCells(vtkIdType numberOfVerts)
{
  vtkNew<vtkIdTypeArray> cells;
  cells->SetNumberOfValues(numberOfVerts * 2);
  vtkIdType* ids = cells->GetPointer(0);
  for (vtkIdType i = 0; i < numberOfVerts; ++i)
  {
    ids[i * 2] = 1;
    ids[i * 2 + 1] = i;
  }

  vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();
  cellArray->SetCells(numberOfVerts, cells.GetPointer());
  return cellArray;
}

//-----------------------------------------------------------------------------
// Returns the value that is equal to x modulo mod, and that is inside to (0, mod(
// mod must be > 0.0
double place_in_interval(double x, double mod)
{
  if (x < 0.0)
  {
    return x + std::ceil(- x / mod) * mod; // not equal to std::fmod (always >= 0)
  } else {
    return std::fmod(x, mod);
  }
}

//-----------------------------------------------------------------------------
// Returns true if x is "inside" (a, b) modulo mod
bool inside_interval_mod(double x, double a, double b, double mod)
{
  // first step: place everything in [0.0, mod]
  x = place_in_interval(x, mod);
  a = place_in_interval(a, mod);
  b = place_in_interval(b, mod);
  if (a >= b)
  {
    // [ ...in...|b|...out...|a|...in...]
    return x >= a || x <= b;
  }
  else
  {
    // [ ...out...|a|...in...|b|...out...]
    return x >= a && x <= b;
  }
}
}

//-----------------------------------------------------------------------------
bool vtkLidarPacketInterpreter::SplitFrame(bool force, FramingMethod_t framingMethodAskingForSplitFrame)
{
  if ((force || this->FramingMethod == framingMethodAskingForSplitFrame) && this->CurrentFrame)
  {
    const vtkIdType nPtsOfCurrentDataset= this->CurrentFrame->GetNumberOfPoints();
    if (this->IgnoreEmptyFrames && (nPtsOfCurrentDataset == 0) && !force)
    {
      return false;
    }

    // add vertex to the polydata
    this->CurrentFrame->SetVerts(NewVertexCells(this->CurrentFrame->GetNumberOfPoints()));
    // split the frame
    this->Frames.push_back(this->CurrentFrame);
    // create a new frame
    this->CurrentFrame = this->CreateNewEmptyFrame(0, nPtsOfCurrentDataset);

    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void vtkLidarPacketInterpreter::SetLaserSelection(int index, int value)
{
  if ((index < 0) ||
      (index >= this->LaserSelection->GetNumberOfTuples()))
  {
    vtkErrorMacro(<< "Bad mode index: " << index);
  }

  this->LaserSelection->SetTuple1(index, value);
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkIntArray* vtkLidarPacketInterpreter::GetLaserSelection()
{
  return this->LaserSelection.GetPointer();
}

//-----------------------------------------------------------------------------
bool vtkLidarPacketInterpreter::shouldBeCroppedOut(double pos[3])
{
  bool pointInside = true;
  switch (this->CropMode)
  {
    case CROP_MODE::Cartesian:
    {
      pointInside = pos[0] >= this->CropRegion[0] && pos[0] <= this->CropRegion[1];
      pointInside &= pos[1] >= this->CropRegion[2] && pos[1] <= this->CropRegion[3];
      pointInside &= pos[2] >= this->CropRegion[4] && pos[2] <= this->CropRegion[5];
      break;
    }
    case CROP_MODE::Spherical:
    {
      double R = std::sqrt(pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]);
      // azimuth in [-180°, 180°]
      // The choosen convention is clockwise from +y, hence -atan2(-x, y)
      double azimuth = (180.0 / vtkMath::Pi()) * std::atan2(pos[0], pos[1]);
      // vertAngle in [-90°, 90°], increasing with z - 0 is horizontal
      double vertAngle = 90.0 - (180.0 / vtkMath::Pi()) * std::acos(pos[2] / R);

      pointInside = inside_interval_mod(azimuth, this->CropRegion[0], this->CropRegion[1], 360.0);
      pointInside &= vertAngle >= this->CropRegion[2] && vertAngle <= this->CropRegion[3];
      pointInside &= R >= this->CropRegion[4] && R <= this->CropRegion[5];
      break;
    }
    case CROP_MODE::Cylindric:
    {
      // space holder for future implementation
    }
    case CROP_MODE::None:
      return false;
  }
  return !((pointInside && !this->CropOutside) || (!pointInside && this->CropOutside));
}

//-----------------------------------------------------------------------------
bool vtkLidarPacketInterpreter::IsNewData()
{
  return this->IsNewFrameReady();
}

//-----------------------------------------------------------------------------
bool vtkLidarPacketInterpreter::IsValidPacket(unsigned char const * data, unsigned int dataLength)
{
  return this->IsLidarPacket(data, dataLength);
}

//-----------------------------------------------------------------------------
void vtkLidarPacketInterpreter::ResetCurrentData()
{
  this->ResetCurrentFrame();
}

//-----------------------------------------------------------------------------
void vtkLidarPacketInterpreter::ProcessPacketWrapped(unsigned char const * data,
                                                  unsigned int dataLength,
                                                  double PacketNetworkTime_s)
{
  // if the framing Method is the NetworkPacketTime one
  // We check if the frame has te be split.
  if(this->IsLidarPacket(data, dataLength) && this->FramingMethod == FramingMethod_t::NETWORK_PACKET_TIME_FRAMING)
  {
    auto currentFrameNumber = static_cast<unsigned long long>(PacketNetworkTime_s / this->FrameDuration_s);
    if (this->LastNetworkTimeFrameNumber != 0 // do not frame on first call of this function
        && currentFrameNumber != this->LastNetworkTimeFrameNumber) // new frame found
    {
      this->SplitFrame(false, FramingMethod_t::NETWORK_PACKET_TIME_FRAMING);
    }
    this->LastNetworkTimeFrameNumber = currentFrameNumber;
  }

  // Interpreter the packet
  this->ProcessPacket(data, dataLength);
}

//-----------------------------------------------------------------------------
bool vtkLidarPacketInterpreter::PreProcessPacketWrapped(unsigned char const * data,
                              unsigned int dataLength, fpos_t filePosition, double packetNetworkTime_s,
                              std::vector<FrameInformation>* frameCatalog)
{
  // If the framing method is the interpreter one
  // frameCatalog is filled with the PreProcessPacket function
  if(this->FramingMethod == FramingMethod_t::INTERPRETER_FRAMING)
  {
    return this->PreProcessPacket(data, dataLength, filePosition, packetNetworkTime_s, frameCatalog);
  }

  // If the framing method is the network time packet one,
  // frameCatalog is filled every frameDuration time.
  if(this->FramingMethod == FramingMethod_t::NETWORK_PACKET_TIME_FRAMING)
  {
    unsigned long long currentFrameNumber = static_cast<unsigned long long>(packetNetworkTime_s / this->FrameDuration_s);
    if (currentFrameNumber != this->previousFrameNumber)
    {
      FrameInformation information;
      information.FilePosition = filePosition;
      information.FirstPacketNetworkTime = packetNetworkTime_s;
      // FirstPacketDataTime is not well-defined, because we do not parse
      // the data and thus cannot get data time, however providing a
      // plausible value can help prevent breaking some algorithms.
      // Possible improvement: first pass using INTERPRETER_FRAMING to
      // compute the timeshift, then apply it to FirstPacketNetworkTime
      information.FirstPacketDataTime = packetNetworkTime_s;
      frameCatalog->push_back(information);
      this->previousFrameNumber = currentFrameNumber;
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
std::string vtkLidarPacketInterpreter::GetDefaultRecordFileName()
{
  std::stringstream defaultFileName;

  // Add time string YYYY-mm-dd-HH-MM-SS
  std::time_t t = std::time(nullptr);
  std::tm tm = *std::localtime(&t);
  defaultFileName << std::put_time(&tm, "%Y-%m-%d-%H-%M-%S");

  return defaultFileName.str();
}
