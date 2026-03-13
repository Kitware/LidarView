import socket
import struct
import threading
import queue

UDP_IP = "127.0.0.1"
UDP_PORT = 8888
BUFFER_SIZE = 508
PACKET_START = [0x28, 0x2a]
PACKET_END = [0x2a, 0x2c]

packet_queue = queue.Queue()

def udp_listener():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))

    while True:
        data, addr = sock.recvfrom(BUFFER_SIZE)
        packet_queue.put((data, addr))


# Start the UDP listener in a separate thread
listener_thread = threading.Thread(target=udp_listener)
listener_thread.daemon = True  # Daemonize thread to exit when main program exits
listener_thread.start()

while True:
    result, _ = packet_queue.get()
    # print("\n=========== New Packet ===========\n")

    #   ===== PAYLOAD HEADER =====
    #     payload_start: 0x28 0x2a
    #     data_type_size: uint8_t
    #     timestamp: double
    #     cluster_id: int32_t
    #     number_of_points: uint16_t
    #
    #   ===== PAYLOAD DATA =====
    #     # For each point (number_of_points):
    #     |   point (x, y, z): 3 * float
    #
    #   ===== PAYLOAD FOOTER =====
    #     packet_size: uint16_t
    #     end_of_payload: 0x2a 0x2c
    #     null_bytes

    payload_start = struct.unpack_from('2s', result, 0)[0]
    data_type_size = struct.unpack_from('B', result, 2)[0]
    timestamp = struct.unpack_from('d', result, 3)[0]
    cluster_id = struct.unpack_from('i', result, 11)[0]
    number_of_points = struct.unpack_from('H', result, 15)[0]

    assert(list(payload_start) == PACKET_START)
    # print(timestamp)
    # print(cluster_id)
    # print(number_of_points)

    offset = 17

    for idx in range(0, number_of_points):
        point = struct.unpack_from("fff", result, offset)
        offset += 3 * data_type_size

    footer_format = 'H2s'  # uint16_t, 2 bytes

    # Unpack the footer
    packet_size, end_of_payload = struct.unpack_from(footer_format, result, offset)

    # print(packet_size)
    assert(list(end_of_payload) == PACKET_END)
    packet_queue.task_done()
