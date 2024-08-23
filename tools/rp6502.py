#!/usr/bin/env python3
#
# Copyright (c) 2023 Rumbledethumps
#
# SPDX-License-Identifier: BSD-3-Clause
# SPDX-License-Identifier: Unlicense

# Control RP6502 RIA via UART

import os
import re
import time
import serial
import binascii
import argparse
import configparser
import platform
from typing import Union


class Monitor:
    """Manages the monitor application on the serial console."""

    DEFAULT_TIMEOUT = 0.5
    UART_BAUDRATE = 115200

    def __init__(self, name, timeout=DEFAULT_TIMEOUT):
        self.serial = serial.Serial()
        self.serial.setPort(name)
        self.serial.timeout = timeout
        self.serial.baudrate = self.UART_BAUDRATE
        self.serial.open()

    def send_break(self, duration=0.01, retries=1):
        """Stop the 6502 and return to monitor."""
        self.serial.read_all()
        if platform.system() == "Darwin":
            self.serial.baudrate = 1200
            self.serial.write(b"\0")
            self.serial.baudrate = self.UART_BAUDRATE
        else:
            self.serial.send_break(duration)
        try:
            self.wait_for_prompt("]")
            return
        except TimeoutError as te:
            if retries <= 0:
                raise te
        self.send_break(duration, retries - 1)

    def command(self, str, timeout=DEFAULT_TIMEOUT):
        """Send one command and wait for next monitor prompt"""
        self.serial.write(bytes(str, "ascii"))
        self.serial.write(b"\r")
        self.wait_for_prompt("]", timeout)

    def reset(self):
        """Start the 6502."""
        self.serial.write(b"RESET\r")
        self.serial.read_until()

    def binary(self, addr: int, data):
        """Send data to memory using BINARY command."""
        command = f"BINARY ${addr:04X} ${len(data):03X} ${binascii.crc32(data):08X}\r"
        self.serial.write(bytes(command, "utf-8"))
        self.serial.write(data)
        self.wait_for_prompt("]")

    def upload(self, file, name):
        """Upload readable file to remote file "name" """
        self.serial.write(bytes(f"UPLOAD {name}\r", "ascii"))
        self.wait_for_prompt("}")
        file.seek(0)
        while True:
            chunk = file.read(1024)
            if len(chunk) == 0:
                break
            command = f"${len(chunk):03X} ${binascii.crc32(chunk):08X}\r"
            self.serial.write(bytes(command, "ascii"))
            self.serial.write(chunk)
            self.wait_for_prompt("}")
        self.serial.write(b"END\r")
        self.wait_for_prompt("]")

    def send_rom(self, rom):
        """Send rom."""
        addr, data = rom.next_rom_data(0)
        while data != None:
            self.binary(addr, data)
            addr += len(data)
            addr, data = rom.next_rom_data(addr)

    def wait_for_prompt(self, prompt, timeout=DEFAULT_TIMEOUT):
        """Wait for prompt."""
        prompt = bytes(prompt, "ascii")
        start = time.monotonic()
        while True:
            if len(prompt) == 1:
                data = self.serial.read()
            else:
                data = self.serial.read_until()
            if data[0:1] == b"?":
                monitor_result = data.decode("ascii")
                monitor_result += self.serial.read_until().decode("ascii").strip()
                raise RuntimeError(monitor_result)
            if data == prompt:
                break
            if len(data) == 0:
                if time.monotonic() - start > timeout:
                    raise TimeoutError()


class ROM:
    """Virtual ROM aka The RP6502 ROM."""

    def __init__(self):
        """ROMs begin with up to a screen of help text"""
        """ followed by a sparse array of virtual ROM. """
        self.help = []
        self.data = [0 for i in range(0x20000)]
        self.alloc = [0 for i in range(0x20000)]

    def add_help(self, string):
        """Add help string."""
        if len(string) > 80:
            raise RuntimeError("Help line too long")
        self.help.append(string)
        if len(self.help) > 24:
            raise RuntimeError("Help lines > 24")

    def add_binary_data(self, data, addr: int):
        """Add binary data."""
        offset = 0
        length = len(data)
        self.allocate_rom(addr, length)
        for i in range(length):
            self.data[addr + i] = data[offset + i]

    def add_irq_vector(self, addr: int):
        """Set IRQ vector in $FFFE and $FFFF."""
        if addr < 0 or addr > 0xFFFF:
            raise RuntimeError(f"Invalid IRQ vector: ${addr:04X}")
        self.allocate_rom(0xFFFE, 2)
        self.data[0xFFFE] = addr & 0xFF
        self.data[0xFFFF] = addr >> 8

    def add_nmi_vector(self, addr: int):
        """Set NMI vector in $FFFA and $FFFB."""
        if addr < 0 or addr > 0xFFFF:
            raise RuntimeError(f"Invalid NMI vector: ${addr:04X}")
        self.allocate_rom(0xFFFA, 2)
        self.data[0xFFFA] = addr & 0xFF
        self.data[0xFFFB] = addr >> 8

    def add_reset_vector(self, addr: int):
        """Set reset vector in $FFFC and $FFFD."""
        if addr < 0 or addr > 0xFFFF:
            raise RuntimeError(f"Invalid reset vector: ${addr:04X}")
        self.allocate_rom(0xFFFC, 2)
        self.data[0xFFFC] = addr & 0xFF
        self.data[0xFFFD] = addr >> 8

    def add_binary_file(self, file, addr: Union[int, None] = None):
        """Add binary memory data from file. addr=None uses"""
        """first two bytes as address and second two bytes as reset."""
        with open(file, "rb") as f:
            data = f.read()
        if addr == None:
            if len(data) < 4:
                raise RuntimeError("No addresses found.")
            addr = data[0] + data[1] * 256
            self.add_reset_vector(data[2] + data[3] * 256)
            data = data[4:]
        self.add_binary_data(data, addr)

    def add_rp6502_file(self, file):
        """Add RP6502 ROM data from file."""
        with open(file, "rb") as f:
            # Decode first line as cp850 because binary garbage can
            # raise here before our better message gets to the user.
            command = f.readline().decode("cp850")
            if not re.match("^#![Rr][Pp]6502(\r|)\n$", command):
                raise RuntimeError(f"Invalid RP6502 ROM file: {file}")
            while True:
                command = f.readline().decode("ascii").rstrip()
                if len(command) == 0:
                    break
                se = re.search("^ *(# )", command)
                if se:
                    self.add_help(command[se.start(1) + 2 :])
                    continue
                if re.search("^ *#$", command):
                    self.add_help("")
                    continue
                se = re.search("^ *([^ ]+) *([^ ]+) *([^ ]+) *$", command)
                if se:

                    def str_to_address(str):
                        """Supports $FFFF number format."""
                        if str:
                            str = re.sub("^\\$", "0x", str)
                        if re.match("^(0x|)[0-9A-Fa-f]*$", str):
                            return eval(str)
                        else:
                            raise RuntimeError(f"Invalid address: {str}")

                    addr = str_to_address(se.group(1))
                    length = str_to_address(se.group(2))
                    crc = str_to_address(se.group(3))
                    self.allocate_rom(addr, length)
                    data = f.read(length)
                    if len(data) != length or crc != binascii.crc32(data):
                        raise RuntimeError(f"Invalid CRC in block address: ${addr:04X}")
                    for i in range(length):
                        self.data[addr + i] = data[i]
                    continue
                raise RuntimeError(f"Corrupt RP6502 ROM file: {file}")

    def allocate_rom(self, addr, length):
        """Marks a range of memory as used. Raises on error."""
        if (
            (addr < 0x10000 and addr + length > 0x10000)
            or addr + length > 0x20000
            or addr < 0
            or length < 0
        ):
            raise IndexError(
                f"RP6502 invalid address ${addr+i:04X} or length ${length+i:03X}"
            )
        for i in range(length):
            if self.alloc[addr + i]:
                raise MemoryError(f"RP6502 ROM data already exists at ${addr+i:04X}")
            self.alloc[addr + i] = 1

    def has_reset_vector(self):
        """Returns true if $FFFC and $FFFD have been set."""
        return self.alloc[0xFFFC] and self.alloc[0xFFFD]

    def next_rom_data(self, addr: int):
        """Find next up-to-1k chunk starting at addr."""
        for addr in range(addr, 0x20000):
            if self.alloc[addr]:
                length = 0
                while self.alloc[addr + length]:
                    length += 1
                    if length == 1024 or addr + length == 0x10000:
                        break
                return addr, bytearray(self.data[addr : addr + length])
        return None, None


def exec_args():
    # Give a hint at where the USB CDC mounts on various OSs
    if platform.system() == "Windows":
        default_device = "COM1"
    elif platform.system() == "Darwin":
        default_device = "/dev/tty.usbmodem"
    elif platform.system() == "Linux":
        default_device = "/dev/ttyACM0"
    else:
        default_device = "/dev/tty"

    # Standard library argument parser
    parser = argparse.ArgumentParser(
        description="Interface with RP6502 RIA console via UART. Manage RP6502 ROM asset packaging."
    )
    parser.add_argument(
        "command",
        choices=["run", "upload", "create"],
        help="Run local RP6502 ROM file by sending to RP6502 RAM. "
        "Upload any local files to RP6502 USB MSC drive. "
        "Create RP6502 ROM file from a local binary file and additional local ROM files. ",
    )
    parser.add_argument("filename", nargs="*", help="Local filename(s).")
    parser.add_argument("-o", dest="out", metavar="name", help="Output path/filename.")
    parser.add_argument(
        "-c",
        "--config",
        dest="config",
        metavar="name",
        help=f"Configuration file for serial device.",
    )
    parser.add_argument(
        "-D",
        "--device",
        dest="device",
        metavar="dev",
        default=default_device,
        help=f"Serial device name. Default={default_device}",
    )
    parser.add_argument(
        "-a",
        "--address",
        dest="address",
        metavar="addr",
        help="Starting address of file. If not provided, "
        "the first two bytes of the file are the start address and "
        "the second two bytes of the file are the reset vector.",
    )
    parser.add_argument("-i", "--irq", dest="irq", metavar="addr", help="IRQ vector.")
    parser.add_argument("-n", "--nmi", dest="nmi", metavar="addr", help="NMI vector.")
    parser.add_argument(
        "-r", "--reset", dest="reset", metavar="addr", help="Reset vector."
    )
    args = parser.parse_args()

    # Standard library configuration parser
    if args.config:
        config = configparser.ConfigParser()
        if not os.path.exists(args.config):
            config["RP6502"] = {"device": args.device}
            config.write(open(args.config, "w"))
        else:
            config.read(args.config)
        if config.has_section("RP6502"):
            args.device = config["RP6502"].get("device", args.device)

    # Additional validation and conversion
    def str_to_address(parser, str, errmsg):
        """Supports $FFFF number format."""
        if str:
            str = re.sub("^\\$", "0x", str)
            if re.match("^(0x|)[0-9A-Fa-f]*$", str):
                return eval(str)
            else:
                parser.error(f"argument {errmsg}: invalid address: '{str}'")

    args.address = str_to_address(parser, args.address, "-a/--address")
    args.irq = str_to_address(parser, args.irq, "-i/--irq")
    args.nmi = str_to_address(parser, args.nmi, "-n/--nmi")
    args.reset = str_to_address(parser, args.reset, "-r/--reset")

    # python3 tools/rp6502.py run
    if args.command == "run":
        print(f"[{os.path.basename(__file__)}] Loading ROM {args.filename[0]}")
        rom = ROM()
        rom.add_rp6502_file(args.filename[0])
        if args.reset != None:
            rom.add_reset_vector(args.reset)
        print(f"[{os.path.basename(__file__)}] Opening device {args.device}")
        mon = Monitor(args.device)
        print(f"[{os.path.basename(__file__)}] Sending BREAK")
        mon.send_break()
        print(f"[{os.path.basename(__file__)}] Sending ROM data")
        mon.send_rom(rom)
        if rom.has_reset_vector():
            print(f"[{os.path.basename(__file__)}] Resetting")
            mon.reset()
        else:
            print("No reset vector. Not resetting.")

    # python3 tools/rp6502.py upload
    if args.command == "upload":
        print(f"[{os.path.basename(__file__)}] Opening device {args.device}")
        mon = Monitor(args.device)
        if len(args.filename) > 0:
            mon.send_break()
        for file in args.filename:
            print(f"[{os.path.basename(__file__)}] Uploading {file}")
            with open(file, "rb") as f:
                if len(args.filename) == 1 and args.out != None:
                    dest = args.out
                else:
                    dest = os.path.basename(file)
                mon.upload(f, dest)

    # python3 tools/rp6502.py create
    if args.command == "create":
        print(f"[{os.path.basename(__file__)}] Creating {args.out}")
        rom = ROM()
        if args.irq != None:
            rom.add_irq_vector(args.irq)
        if args.nmi != None:
            rom.add_nmi_vector(args.nmi)
        if args.reset != None:
            rom.add_reset_vector(args.reset)
        print(f"[{os.path.basename(__file__)}] Adding Binary Asset {args.filename[0]}")
        rom.add_binary_file(args.filename[0], args.address)
        for file in args.filename[1:]:
            print(f"[{os.path.basename(__file__)}] Adding ROM Asset {file}")
            rom.add_rp6502_file(file)
        with open(args.out, "wb+") as file:
            file.write(b"#!RP6502\n")
            for help in rom.help:
                file.write(bytes(f"# {help}\n", "ascii"))
            addr, data = rom.next_rom_data(0)
            while data != None:
                file.write(
                    bytes(
                        f"${addr:04X} ${len(data):03X} ${binascii.crc32(data):08X}\n",
                        "ascii",
                    )
                )
                file.write(data)
                addr += len(data)
                addr, data = rom.next_rom_data(addr)


# This file may be included or run like a program. e.g.
#   import importlib
#   rp6502 = importlib.import_module("tools.rp6502")
if __name__ == "__main__":
    exec_args()
