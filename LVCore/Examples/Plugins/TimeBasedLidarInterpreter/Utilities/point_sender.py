import socket
import random
import struct
import time

# Configuration for UDP transmission
UDP_IP = "127.0.0.1"
UDP_PORT = 2360
RADIUS = 50

# Initialize UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def random_point_in_sphere():
    """Generates a random point within a unit sphere."""
    while True:
        x = random.uniform(-RADIUS, RADIUS)
        y = random.uniform(-RADIUS, RADIUS)
        z = random.uniform(-RADIUS, RADIUS)
        if x**2 + y**2 + z**2 <= RADIUS:
            return x, y, z

while True:
    points_data = b""
    # Get PC time in nanosecond and convert to microsecond
    timestamp = int(time.time_ns() * 1e-3)
    for _ in range(100):

        x, y, z = random_point_in_sphere()

        reflectance = random.uniform(0, 2550) / 10.

        timestamp += 10 # 10us

        packed_data = struct.pack('ffffq', x, y, z, reflectance, timestamp)
        points_data += packed_data

    sock.sendto(points_data, (UDP_IP, UDP_PORT))
    time.sleep(0.001)  # 1ms
