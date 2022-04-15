from serial import Serial
import argparse

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Swarm module communication")
    parser.add_argument('--port', type= str, nargs=1, help='the usb port for serial')
    parser.add_argument('--baud', type=int, nargs=1, help='the usb baud rate', default=115200)
    args = parser.parse_args()
    ser = Serial(args.port, args.baud)


def poll_gps(serial: Serial):
    pass


def send_message(line: str) -> bool:
    cmd = line.split(" ")[0]
    ret = ser.readline()
