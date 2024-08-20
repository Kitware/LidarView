import socket
import struct
import threading
import queue

from vtkmodules.vtkFiltersSources import vtkCubeSource

from lidarview.modules.lvIONetwork import vtkUDPPing

UDP_IP = "127.0.0.1"
UDP_PORT = 8888
BUFFER_SIZE = 12
PACKET_START = [0x28, 0x2a]
PACKET_END = [0x2a, 0x2c]

def receive_packet(q):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    sock.settimeout(10)

    data, addr = sock.recvfrom(BUFFER_SIZE)
    q.put(data)

def use_cluster_sender():
    cubeSource = vtkCubeSource()
    cubeSource.Update()

    sender = vtkUDPPing()
    sender.SetInputData(cubeSource.GetOutput())
    sender.SetIPAddress(UDP_IP)
    sender.SetDestinationPort(UDP_PORT)
    sender.SetEnabled(True)
    sender.SetMinimalDelay(0)
    sender.Update()

receiver_queue = queue.Queue()
thread_receiver = threading.Thread(target=receive_packet, args=(receiver_queue,))
thread_sender = threading.Thread(target=use_cluster_sender)

thread_receiver.start()
thread_sender.start()

thread_sender.join()
thread_receiver.join(timeout=15)

result = receiver_queue.get()

assert(list(result[0:2]) == PACKET_START)
assert(list(result[10:12]) == PACKET_END)

pointNb = struct.unpack("<LL", result[2:10])[0]
assert(type(pointNb) == int)
assert(pointNb == 24)
