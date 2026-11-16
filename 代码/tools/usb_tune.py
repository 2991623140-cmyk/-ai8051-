"""Read AI8051U USB CDC telemetry and send bounded tuning commands."""

import argparse
import time

import serial
from serial.tools import list_ports


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--list', action='store_true')
    parser.add_argument('--port')
    parser.add_argument('--seconds', type=float, default=10)
    parser.add_argument('--command', action='append', default=[])
    args = parser.parse_args()
    ports = list(list_ports.comports())
    if args.list:
        for port in ports:
            print(f'{port.device}: {port.description} [{port.hwid}]')
        if not ports:
            print('No serial ports detected.')
        return
    if not args.port:
        parser.error('--port is required; use --list to find the USB CDC port')
    if args.seconds <= 0 or args.seconds > 60:
        parser.error('--seconds must be greater than zero and at most 60')
    for command in args.command:
        if '\r' in command or '\n' in command or len(command) > 23:
            parser.error('each --command must be one line with at most 23 characters')
    with serial.Serial(args.port, 115200, timeout=0.1, write_timeout=2) as connection:
        for command in args.command or ['?']:
            connection.write((command + '\n').encode('ascii'))
        deadline = time.monotonic() + args.seconds
        pending = bytearray()
        while time.monotonic() < deadline:
            pending.extend(connection.read(connection.in_waiting or 1))
            while b'\n' in pending:
                line, _, rest = pending.partition(b'\n')
                pending = bytearray(rest)
                print(line.decode('ascii', errors='replace').rstrip('\r'), flush=True)
            if len(pending) > 4096:
                raise RuntimeError('Received an oversized telemetry line')


if __name__ == '__main__':
    main()
