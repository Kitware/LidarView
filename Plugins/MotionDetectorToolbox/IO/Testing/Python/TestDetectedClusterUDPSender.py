import socket
import struct
import threading
import queue

from vtkmodules.vtkCommonDataModel import (vtkFieldData, vtkPolyData, vtkMultiBlockDataSet)
from vtkmodules.vtkFiltersSources import vtkCubeSource
from vtkmodules.vtkCommonCore import (vtkFloatArray, vtkUnsignedShortArray)
from vtkmodules.vtkCommonExecutionModel import vtkPassInputTypeAlgorithm

from lidarview.modules.MotionDetectorToolboxIO import vtkDetectedClusterUDPSender

UDP_IP = "127.0.0.1"
UDP_PORT = 8888
BUFFER_SIZE = 508
BLOCK_SIZE = 30
PACKET_START = [0x28, 0x2a]
PACKET_END = [0x2a, 0x2c]

DISTANCE = 0.9
SIZE = [2, 0.4, 0.9]
CENTER = [0.4, 22, 0.8]
LABEL = 0

def receive_packet(q):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    sock.settimeout(10)

    data, addr = sock.recvfrom(BUFFER_SIZE)

    clusters = []
    if list(data[0:2]) != PACKET_START:
        q.put("Did not find PACKET_START")
    timestamp = struct.unpack("<d", data[2:10])[0]
    blockNb = struct.unpack("<H", data[10:12])[0]
    idx = 12
    for _ in range(0, blockNb):
        idxEnd = idx + BLOCK_SIZE
        blockInfo = struct.unpack("<iHfffffff", data[idx:idxEnd])
        idx = idxEnd
        clusters.append({
            "id": blockInfo[0],
            "label": blockInfo[1],
            "center": [round(block, 4) for block in blockInfo[2:5]],
            "distance": round(blockInfo[4], 5),
            "size": [round(block, 4) for block in blockInfo[6:9]],
        })

    totalSize = struct.unpack("<H", data[idx:idx+2])[0]
    if list(data[idx+2:idx+4]) != PACKET_END:
        q.put("Did not ended by PACKET_END")
    q.put(clusters)

def use_cluster_sender():
    cubeSource = vtkCubeSource()
    cubeSource.SetXLength(SIZE[0])
    cubeSource.SetYLength(SIZE[1])
    cubeSource.SetZLength(SIZE[2])
    cubeSource.SetCenter(CENTER)
    cubeSource.Update()
    source = vtkPolyData()
    source.ShallowCopy(cubeSource.GetOutput())
    clusters = vtkMultiBlockDataSet()
    clusters.SetBlock(0, source)

    distanceArray = vtkFloatArray()
    distanceArray.SetName("Distance")
    distanceArray.SetNumberOfComponents(1)
    distanceArray.InsertNextTuple1(DISTANCE)
    sizeArray = vtkFloatArray()
    sizeArray.SetName("Size")
    sizeArray.SetNumberOfComponents(3)
    sizeArray.InsertNextTuple(SIZE)
    centerArray = vtkFloatArray()
    centerArray.SetName("Center")
    centerArray.SetNumberOfComponents(3)
    centerArray.InsertNextTuple(CENTER)
    labelArray = vtkUnsignedShortArray()
    labelArray.SetName("Label")
    labelArray.SetNumberOfComponents(1)
    labelArray.InsertNextTuple1(LABEL)
    fieldData = vtkFieldData()
    fieldData.AddArray(distanceArray)
    fieldData.AddArray(sizeArray)
    fieldData.AddArray(centerArray)
    fieldData.AddArray(labelArray)

    clusters.GetBlock(0).SetFieldData(fieldData)

    sender = vtkDetectedClusterUDPSender()

    sender.SetInputData(clusters)
    sender.SetIPAddress(UDP_IP)
    sender.SetDestinationPort(UDP_PORT)
    sender.Update()

receiver_queue = queue.Queue()
thread_receiver = threading.Thread(target=receive_packet, args=(receiver_queue,))
thread_sender = threading.Thread(target=use_cluster_sender)

thread_receiver.start()
thread_sender.start()

thread_sender.join()
thread_receiver.join(timeout=15)

result = receiver_queue.get()

print(f"\nResult:\n\t{result}\n")

assert(len(result) == 1)
cluster = result[0]
assert(cluster["id"] == 0)
assert(cluster["label"] == LABEL)
assert(cluster["center"] == CENTER)
assert(cluster["distance"] == DISTANCE)
assert(cluster["size"] == SIZE)
