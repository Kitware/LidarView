import socket
import struct

UDP_IP = "127.0.0.1"
UDP_PORT = 8888
BUFFER_SIZE = 508
BLOCK_SIZE = 50

PACKET_START = [0x28, 0x2a]
PACKET_END = [0x2a, 0x2c]

LABELS = ["Humans", "Others"]

def parse_packet(data):
    clusters = []
    timestamp = 0
    if list(data[0:2]) != PACKET_START:
        print("Invalid packet")
        return clusters, timestamp
    timestamp = struct.unpack("<d", data[2:10])[0]
    blockNb = struct.unpack("<H", data[10:12])[0]
    idx = 12
    for _ in range(0, blockNb):
        idxEnd = idx + BLOCK_SIZE
        blockInfo = struct.unpack("<iHfffffffffff", data[idx:idxEnd])
        idx = idxEnd
        clusters.append({
            "id": blockInfo[0],
            "label": LABELS[blockInfo[1]] if blockInfo[1] < len(LABELS) else LABELS[1],
            "center": [round(block, 4) for block in blockInfo[2:5]],
            "distance": round(blockInfo[5], 5),
            "size": [round(block, 4) for block in blockInfo[6:9]],
            "orientation": [round(block, 4) for block in blockInfo[9:13]],
        })

    totalSize = struct.unpack("<H", data[idx:idx+2])[0]
    if list(data[idx+2:idx+4]) != PACKET_END:
        print("Packet might not be valid")
    return clusters, timestamp

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

while True:
    data, addr = sock.recvfrom(BUFFER_SIZE)

    clusters, timestamp = parse_packet(data)
    print(clusters, timestamp)
