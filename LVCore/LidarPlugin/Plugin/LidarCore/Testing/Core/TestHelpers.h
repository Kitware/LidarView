// Copyright 2013 Velodyne Acoustics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __testHelpers_h
#define __testHelpers_h

#include <sstream>
#include <vector>

class vtkLidarReader;
class vtkLidarStream;
class vtkLidarPacketInterpreter;
class vtkPolyData;


// Helper functions
bool compare(const double* const a, const double* const b, const size_t N, double epsilon);

template <size_t N>
bool compare(double const (&a)[N], double const (&b)[N], double epsilon)
{
  return compare(a, b, N, epsilon);
}

std::string toString(const double* const d, const size_t N);

template <size_t N>
std::string toString(double const (&d)[N])
{
  return toString(d, N);
}

vtkPolyData* GetCurrentFrame(vtkLidarReader* HDLreader, int index);

vtkPolyData* GetCurrentFrame(vtkLidarStream* HDLsource, int index);

int GetNumberOfTimesteps(vtkLidarStream* HDLSource);

std::vector<std::string> GenerateFileList(const std::string& metaFileName);

vtkPolyData* GetCurrentReference(const std::vector<std::string>& referenceFilesList, int index);

// Test functions
/**
 * @brief TestFrameCount Checks the number of frame on the actual dataset
 * @param frameCount Number of frame in the current dataset
 * @param referenceCount Number of frame in the reference dataset
 * @return 0 on succes, 1 on failure
 */
int TestFrameCount(unsigned int frameCount, unsigned int referenceCount);

/**
 * @brief TestPointCount Checks the number of points on the actual dataset
 * @param currentFrame Current frame
 * @param currentReference Reference for the current frame
 * @return 0 on success, 1 on failure
 */
int TestPointCount(vtkPolyData* currentFrame, vtkPolyData* currentReference);

/**
 * @brief TestPointDataStructure Checks the number of pointdata arrays & their
 * structure on the actual dataset
 * @param currentFrame Current frame
 * @param currentReference Reference for the current frame
 * @return 0 on success, 1 on failure
 */
int TestPointDataStructure(vtkPolyData* currentFrame, vtkPolyData* currentReference);

/**
 * @brief TestPointDataValues Checks each value on each point data array on the
 *  actual dataset
 * @param currentFrame Current frame
 * @param currentReference Reference for the current frame
 * @return 0 on success, 1 on failure
 */
int TestPointDataValues(vtkPolyData* currentFrame, vtkPolyData* currentReference);

/**
 * @brief TestPointPositions Checks the position of each point on the actual
 * dataset
 * @param currentFrame Current frame
 * @param currentReference Reference for the current frame
 * @return 0 on success, 1 on failure
 */
int TestPointPositions(vtkPolyData* currentFrame, vtkPolyData* currentReference);

int TestNetworkTimeToLidarTime(vtkLidarReader* HDLReader,
                               double referenceNetworkTimeToLidarTime);

/**
 * @brief CheckCurrentFrameAgainst Compare 2 frames
 * @param currentFrame Frame to compare
 * @param currentReference Reference against compare the currentFrame
 * @return nb of error
 */
int CheckCurrentFrame(vtkPolyData* currentFrame, vtkPolyData* currentReference);

/**
 * @brief testLidarReader compare the reader output to prerecorded frames
 * @param reader
 * @param referenceFileName the file containing the vtp to compare with
 * @param FrequencyReference_Hz the reference speed of the sensor
 * @return nb of error
 */
int testLidarReader(vtkLidarReader* reader,
                    double referenceNetworkTimeToDataTime,
                    const std::string& referenceFileName,
                    const double FrequencyReference_Hz = 10);


/**
 * @brief SendAndTestAllFrames send packets of pcapFileName on (destinationIp, dataPort)
 *        and compare the frames output by interpreter to the referenceFilesLists
 * @param stream the stream
 * @param interpreter the interpreter
 * @param pcapFileName file to replay
 * @param referenceFilesList the list of the reference frames information
 * @param destinationIp Ip on which the packets should be sent
 * @param dataPort port on which the packets should be sent
 * @return nb of error
 */
int SendAndTestAllFrames(vtkLidarStream *stream, vtkLidarPacketInterpreter* interpreter,
                         std::vector<std::string> referenceFilesList,
                         const std::string& pcapFileName,
                         std::string destinationIp, int dataPort);


/**
 * @brief testLidarStream comapre the the stream output to prerecorded frames
 * @param stream
 * @param preSend should all packets be sent in a pre-test pass first
 * @param preSendSpeed number of packets to send per second in pre-test pass
 * @param speed number of packets to send per second
 * @param pcapFileName file to replay
 * @param referenceFileName the file containing the vtp to compare with
 * @return nb of error
 */
[[deprecated("Use the simplier signature instead.")]]
int testLidarStream(vtkLidarStream* stream,
                    bool preSend,
                    double preSendSpeed,
                    double speed,
                    const std::string& pcapFileName,
                    const std::string& referenceFileName,
                    bool testLastFrame = false);

/**
 * @brief testLidarStream comapre the the stream output to prerecorded frames
 * @param stream
 * @param pcapFileName file to replay
 * @param referenceFileName the file containing the vtp to compare with
 * @param preSend should all packets be sent in a pre-test pass first
 * @return nb of error
 */
int testLidarStream(vtkLidarStream* stream,
                    const std::string& pcapFileName,
                    const std::string& referenceFileName,
                    bool preSend);

/**
 * @brief TestLidarMulticast compare the stream output to prerecorded frames using the multicast option
 * @param interpreter
 * @param pcapFileName file to replay
 * @param referenceFileName the file containing the vtp to compare with
 * @param preSend should all packets be sent in a pre-test pass first
 * @param dataPort port where data should be received
 * @param correctionFileName the correction file
 * @return nb of error
 */
int TestLidarMulticast(vtkLidarPacketInterpreter* interpreter,
                       const std::string& pcapFileName,
                       const std::string& referenceFileName,
                       bool shouldPreSend, int dataPort,
                       const std::string correctionFileName = "");

/**
 * @brief TestLidarForwarding compare the stream output to prerecorded frames using the forwarding option
 * @param interpreter
 * @param pcapFileName file to replay
 * @param referenceFileName the file containing the vtp to compare with
 * @param preSend should all packets be sent in a pre-test pass first
 * @param dataPort port where data should be received
 * @param correctionFileName the correction file
 * @return nb of error
 */
int TestLidarForwarding(vtkLidarPacketInterpreter* interpreter1,
                        vtkLidarPacketInterpreter* interpreter2,
                        const std::string& pcapFileName,
                        const std::string& referenceFileName,
                        bool shouldPreSend, int dataPort,
                        const std::string correctionFileName);


/**
 * @brief TestLidarRecording compare the stream output to prerecorded frames using the recording option
 * @param interpreter1 interpreter for the stream that forwards the packet
 * @param interpreter2 interpreter for the stream that tests the recorded pcap
 * @param pcapFileName file to replay
 * @param referenceFileName the file containing the vtp to compare with
 * @param preSend should all packets be sent in a pre-test pass first
 * @param dataPort port where data should be received
 * @param correctionFileName the correction file
 * @return nb of error
 */
int TestLidarRecording(vtkLidarPacketInterpreter* interpreter1,
                       vtkLidarPacketInterpreter* interpreter2,
                       const std::string& pcapFileName,
                       const std::string& referenceFileName,
                       bool shouldPreSend, int dataPort,
                       const std::string correctionFileName = "");


bool GetFrameTimeRange(vtkPolyData* frame, double& firstTime, double& lastTime);

#endif
