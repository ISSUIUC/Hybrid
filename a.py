import serial

port = serial.Serial("COM3", 460800,timeout=0.5)
port.write(b'a' * 1000)
for i in range(100):
    print(port.read())
