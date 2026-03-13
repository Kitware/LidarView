#!/usr/bin/env python3

# This script extracts the transferred data from each packet of a .pcap or .pcapng file
# and generates a new .pcap file with dummy Ethernet, IPv4, and UDP PDUs.
#
# It requires tshark and text2pcap Wireshark utilities to work properly.
# Equivalent to: 
# ```bash
#   tshark -r input.pcapng -T fields -e frame.time_epoch -e data \
#   | while IFS=$'\t' read -r num hex; do
#       echo "\n$num"
#       echo "$hex" | xxd -r -p | hexdump -C
#   done \
#   | text2pcap -t "%s.%f" -l 1 -e ether -u 1234,8888 -4 192.168.0.1,192.168.0.2 - output.pcap
# ```

import subprocess
import argparse
import sys

def extract_and_dump_pcap(input_pcap, output_pcap, src_ip, dst_ip, src_port, dst_port):
    tshark_cmd = [
        "tshark",
        "-r", input_pcap,
        "-T", "fields",
        "-e", "frame.time_epoch",
        "-e", "data"
    ]

    process = subprocess.Popen(tshark_cmd, stdout=subprocess.PIPE, stderr=sys.stderr, text=True)
    dump_lines = []

    if not process.stdout:
        return 1

    for line in process.stdout:
        if not line.strip():
            continue
        num, hex_data = line.strip().split("\t")
        dump_lines.append(f"\n{num}")
        # Convert hex to binary and dump in hex format
        binary_data = bytes.fromhex(hex_data)
        hex_lines = []
        for i in range(0, len(binary_data), 16):
            chunk = binary_data[i:i+16]
            hex_line = " ".join(f"{b:02x}" for b in chunk)
            hex_lines.append(f"{i:08x}:  {hex_line}")
        dump_lines.append("\n".join(hex_lines))
    dump_content = "\n".join(dump_lines)
    print(f"Converted with tshark {int(len(dump_lines) / 2)} packets.")
    print("-------------------------\n")

    text2pcap_cmd = [
        "text2pcap",
        "-t", "%s.%f",
        "-l", "1",
        "-e", "ether",
        "-u", f"{src_port},{dst_port}",
        "-4", f"{src_ip},{dst_ip}",
        "-",  # Read from stdin
        output_pcap
    ]
    text2pcap_process = subprocess.Popen(text2pcap_cmd, stdin=subprocess.PIPE, stdout=sys.stdout, stderr=sys.stderr, text=True)
    text2pcap_process.communicate(input=dump_content)
    return 0

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Extract data from a pcap file and generate a new pcap with dummy Ethernet, IPv4, and UDP headers.")
    parser.add_argument("input_pcap", help="Input pcap or pcapng file")
    parser.add_argument("output_pcap", help="Output pcap file")
    parser.add_argument("--src_ip", default="192.168.0.1", help="Source IP address (default: 192.168.0.1)")
    parser.add_argument("--dst_ip", default="192.168.0.2", help="Destination IP address (default: 192.168.0.2)")
    parser.add_argument("--src_port", type=int, default=1234, help="Source port (default: 1234)")
    parser.add_argument("--dst_port", type=int, default=8888, help="Destination port (default: 8888)")
    args = parser.parse_args()

    ret = extract_and_dump_pcap(args.input_pcap, args.output_pcap, args.src_ip, args.dst_ip, args.src_port, args.dst_port)
    exit(ret)
