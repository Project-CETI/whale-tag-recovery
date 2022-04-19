from serial import Serial
import argparse


def nmea_checksum(msg: str):
    """
    Make a NMEA 0183 checksum on a string. Skips leading ! or $ and stops
    at *
    It ignores anything before the first $ or ! in the string
    """

    # Find the start of the NMEA sentence
    startchars = "!$"
    for c in startchars:
        i = msg.find(c)
        if i >= 0:
            break
    else:
        return (False, None, None)

    # Calculate the checksum on the message
    sum1 = 0
    for c in msg[i + 1 :]:
        if c == "*":
            break
        sum1 = sum1 ^ ord(c)
    sum1 = sum1 & 0xFF

    return msg + f"*{sum1:x}"


def send_with_responce(msg: str, ser: Serial):
    ser.write(nmea_checksum(msg).encode("ascii"))
    return ser.readline()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Swarm module debug terminal with integrated NMEA phrase checksum calculation"
    )
    parser.add_argument(
        "--port",
        type=str,
        nargs=1,
        required=True,
        help="the serial port to talk to swarm on",
    )
    args = parser.parse_args()
    ser = Serial((args.port[0]), 115200)
    ser.write("$CS*10\n".encode("ascii"))
    while True:
        command = input("Command: ")
        if command == "exit":
            break
        print(send_with_responce(command, ser))
    ser.close()
