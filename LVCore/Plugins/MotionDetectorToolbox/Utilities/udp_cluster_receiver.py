import socket
import struct

UDP_IP = "127.0.0.1"
UDP_PORT = 8888
BUFFER_SIZE = 508
BLOCK_SIZE = 30

PACKET_START = [0x28, 0x2a]
PACKET_END = [0x2a, 0x2c]

LABELS = ["Humans", "Others"]

def parse_packet(data):
    clusters = []
    if list(data[0:2]) != PACKET_START:
        print("Invalid packet")
        return clusters
    blockNb = struct.unpack("<H", data[2:4])[0]
    idx = 4
    for blockId in range(0, blockNb):
        idxEnd = idx + BLOCK_SIZE
        blockInfo = struct.unpack("<Hfffffff", data[idx:idxEnd])
        idx = idxEnd
        clusters.append({
            "id": blockId,
            "label": LABELS[blockInfo[0]] if blockInfo[0] < len(LABELS) else LABELS[1],
            "center": [round(block, 4) for block in blockInfo[1:4]],
            "distance": round(blockInfo[4], 4),
            "size": [round(block, 4) for block in blockInfo[5:8]],
        })

    totalSize = struct.unpack("<H", data[idx:idx+2])[0]
    if list(data[idx+2:idx+4]) != PACKET_END:
        print("Packet might not be valid")
    return clusters

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

while True:
    data, addr = sock.recvfrom(BUFFER_SIZE)

    clusters = parse_packet(data)
    print(clusters)

