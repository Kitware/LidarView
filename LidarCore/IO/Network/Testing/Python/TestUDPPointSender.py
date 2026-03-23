import socket
import struct
import threading
import queue

from vtkmodules.vtkCommonDataModel import vtkDataObject
from vtkmodules.vtkCommonCore import vtkFloatArray
from vtkmodules.vtkFiltersSources import vtkCubeSource

from lidarview.modules.lvIONetwork import vtkUDPPointSender

UDP_IP = "127.0.0.1"
UDP_PORT = 8888
BUFFER_SIZE = 508
PACKET_START = [0x28, 0x2a]
PACKET_END = [0x2a, 0x2c]

cubeSource = vtkCubeSource()
cubeSource.Update()
data = cubeSource.GetOutput()

dummyArray = vtkFloatArray()
dummyArray.SetName("Dummy")
dummyArray.SetNumberOfTuples(data.GetNumberOfPoints())
for i in range(0, data.GetNumberOfPoints()):
    dummyArray.InsertTuple1(i, i)

data.GetAttributesAsFieldData(vtkDataObject.POINT).AddArray(dummyArray)

def receive_packet(q):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    sock.settimeout(10)

    data, addr = sock.recvfrom(BUFFER_SIZE)
    q.put(data)

def use_cluster_sender():
    sender = vtkUDPPointSender()
    sender.SetInputData(data)
    sender.SetIPAddress(UDP_IP)
    sender.SetDestinationPort(UDP_PORT)
    sender.SetEnabled(True)
    sender.GetArraySelections().EnableArray("Dummy")
    sender.Update()

receiver_queue = queue.Queue()
thread_receiver = threading.Thread(target=receive_packet, args=(receiver_queue,))
thread_sender = threading.Thread(target=use_cluster_sender)

thread_receiver.start()
thread_sender.start()

thread_sender.join()
thread_receiver.join(timeout=15)

result = receiver_queue.get()

# ===== PAYLOAD HEADER =====
#   header_size: uint16_t
#   is_big_endian: bool
#   data_type_size: uint8_t
#   number_of_arrays: uint8_t
#   |   array_name_size: uint8_t
#   |   array_name: string
#   |   array_number_of_components: uint8_t
#   number_of_points: uint16_t
#   end_of_header_bytes: 0x28 0x2a
#
# ===== PAYLOAD DATA =====
#   # For each point (number_of_points):
#   |   point (x, y, z): 3 * float
#   |   # For each array (number_of_arrays):
#   |   |    # For each array component (array_number_of_components):
#   |   |    |    point_data_array: float
#
# ===== PAYLOAD FOOTER =====
#   packet_size: uint16_t
#   end_of_payload: 0x2a 0x2c
#   null_bytes

header_format = 'H?BB'  # uint16_t, bool, uint8_t, uint8_t
header_size, is_big_endian, data_type_size, number_of_arrays = struct.unpack(header_format, result[:5])

assert(header_size == 16)
assert(is_big_endian == False)
assert(data_type_size == 4)
assert(number_of_arrays == 1)

offset = 6
array_name_size = struct.unpack_from('B', result, 5)[0]
array_name = struct.unpack_from(f'{array_name_size}s', result, 6)[0].decode("utf-8")
offset += array_name_size
array_number_of_components = struct.unpack_from('B', result, offset)[0]
offset += 1

assert(array_name == "Dummy")
assert(array_number_of_components == 1)

number_of_points = struct.unpack_from('H', result, offset)[0]
offset += 2
assert(number_of_points == 24)

assert(list(result[offset:offset+2]) == PACKET_START)
offset += 2

for idx in range(0, number_of_points):
    point = struct.unpack_from("fff", result, offset)
    offset += 3 * data_type_size
    component = struct.unpack_from('f', result, offset)[0]
    assert(component == idx)
    offset += data_type_size

footer_format = 'H2s'  # uint16_t, 2 bytes

# Unpack the footer
packet_size, end_of_payload = struct.unpack_from(footer_format, result, offset)

assert(packet_size == offset + 4)
assert(list(end_of_payload) == PACKET_END)
