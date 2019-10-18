#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import socket
from client_auth import encryptText
import threading
import serial
from statistics import mean
import struct


def predict(readings):
    return "logout"

def compute_circuit_info(readings):
    vol = [readings[i][1] for i in range(len(readings))]
    vol = mean(vol)
    cur = [readings[i][0] for i in range(len(readings))]
    cur = mean(cur)
    power = [readings[i][2] for i in range(len(readings))]
    power = mean(power)
    cumPow = readings[-1][3]
    return round(vol, 2), round(cur, 2), round(power, 2), round(cumPow, 2)

def compute_checksum(databytes, checksum):
    print("Length of data is {0} bytes".format(len(databytes)))
    check = 0
    for i in range(len(databytes)):
        check ^= databytes[i]
    print(check, checksum)
    if check == checksum:
        return True
    else:
        return False

def bytes_to_float(four_bytes):
    return struct.unpack('>f', four_bytes)[0]

def bytes_to_int(two_bytes):
    return struct.unpack('>h', two_bytes)[0]

def extract_data(databytes):
    # ignore packet id and packet length
    sensor_bytes = databytes[:48]
    circuit_bytes = databytes[48:]
    sensor_reading = []
    circuit_reading = []

    for i in range(len(sensor_bytes)//2):
        sensor_reading.append(bytes_to_int(sensor_bytes[i*2:(i+1)*2]))

    for i in range(len(circuit_bytes)//4):
        circuit_reading.append(bytes_to_float(circuit_bytes[(i*4):(i+1)*4]))
    return sensor_reading, circuit_reading

class Client(threading.Thread):
    def __init__(self, ip_addr, port_num):
        threading.Thread.__init__(self)
        self.shutdown = threading.Event()
        self.clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.clientSocket.connect((ip_addr, port_num))

    def run(self):
        ### Create serial port
        port = serial.Serial("/dev/ttyS0", baudrate=115200, timeout=3)

        ### Handshake ###
        handshake = 0

        while (handshake == 0):
            port.write(b'1')
            rcv = port.read()
            if (rcv == b'1'):
                handshake = 1
                print("ACK received")

        while (handshake == 1):
            port.write(b'2')
            handshake = 2
            print("Handshake completed!")
        while not self.shutdown.is_set():
            sensor_readings = []
            circuit_readings = []
            count = 0
            while (handshake == 2) and count < frame:
                count += 1
                print("Receiving data...")
                bytedata = port.read(66)
                bytedata = bytedata[bytedata.find(b'#') + 1:]
                print(bytedata)

                if len(bytedata) == 65:
                    if compute_checksum(bytedata[:-1], bytedata[-1]):
                        port.write(ACK)
                        sensor_data, circuit_data = extract_data(bytedata[:-1])
                        print(sensor_data)
                        print(circuit_data)
                        sensor_readings.append(sensor_data)
                        circuit_readings.append(circuit_data)
                    else:
                        port.write(NACK)
                else:
                    port.write(NACK)

            print('Predicting the action: ')
            prediction = predict(sensor_readings)
            vol, cur, power, cumPow = compute_circuit_info(circuit_readings)
            raw_message = "#{0}|{1}|{2}|{3}|{4}".format(prediction, vol, cur, power, cumPow)
            print(raw_message)
            encodedmsg = encryptText(raw_message, secret_key)
            self.clientSocket.sendall(encodedmsg)

            if prediction == 'logout':
                print("Closing socket...")
                self.clientSocket.close()
                self.shutdown.set()


PORT = 6788
HOST = "172.31.27.114"
secret_key = 'secretkeysixteen'
ACK = b'A'
NACK = b'N'

print("Enter frame size: ")
frame = input()
my_client = Client(HOST, PORT)
my_client.start()
