#!/usr/bin/env python3

import serial
import csv
import struct
import time

def compute_checksum(databytes, checksum):
#    print("Length of data is {0} bytes".format(len(databytes)))
    check = 0
    for i in range(len(databytes)):
        check ^= databytes[i]
#    print(check, checksum)
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
    sensor = databytes[:48]
    circuit = databytes[48:]
    my_dict = {}
    fieldnames = ['wrist_Gx', 'wrist_Gy', 'wrist_Gz', 'wrist_Ax', 'wrist_Ay', 'wrist_Az',
                'biceps_Gx', 'biceps_Gy', 'biceps_Gz', 'biceps_Ax', 'biceps_Ay', 'biceps_Az',
                'palm_Gx', 'palm_Gy', 'palm_Gz', 'palm_Ax', 'palm_Ay', 'palm_Az',
                   'thigh_Gx', 'thigh_Gy', 'thigh_Gz', 'thigh_Ax', 'thigh_Ay', 'thigh_Az',
                    'current', 'voltage', 'power', 'energy']
    for i in range(len(fieldnames)):
        if i < 24:
            my_dict[fieldnames[i]] = bytes_to_int(sensor[i*2:(i+1)*2])
        else:
            my_dict[fieldnames[i]] = bytes_to_float(circuit[(i-24)*4:(i-23)*4])
    return my_dict

port=serial.Serial("/dev/ttyS0", baudrate=115200, timeout=3)
ACK = b'A'
NACK = b'N'

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

### Main code ###
# input dance move label from user
print('Enter dance move to train')
label = input()

start = time.time()
prevcount = count = 0
timer15 = start + 5  # 15 seconds
# 15 seconds countdown before start
while True:
    if(time.time() > prevcount):
        print('{:0.0f}'.format(time.time()-start))
        count = count + 1
        prevcount = start + count

    if time.time() > timer15:
        print('START')
        break
start = time.time()
prevcount = count = 0
timer60 = start + 60  # 1 min
readings = []
while(handshake == 2):
    if (time.time() > prevcount):
        print('{:0.0f}'.format(time.time() - start))
        count += 1
        prevcount = start + count

    if time.time() > timer60:
        print('STOP')
        break
    print("Receiving data...")
    bytedata=port.read(66)
    bytedata = bytedata[bytedata.find(b'#')+1:]
    #print(bytedata)

    #seq = bytedata.decode('ascii').strip()
    if len(bytedata) == 65:
        if compute_checksum(bytedata[:-1], bytedata[-1]):
            port.write(ACK)

            data_dict = extract_data(bytedata[:-1])
            #print(data_dict)
            readings.append(data_dict)
        else:
            port.write(NACK)

    else:
        port.write(NACK)

with open('{0}_data.csv'.format(label), mode='w') as csv_file:
    fieldnames = ['wrist_Gx', 'wrist_Gy', 'wrist_Gz', 'wrist_Ax', 'wrist_Ay', 'wrist_Az',
               'biceps_Gx', 'biceps_Gy', 'biceps_Gz', 'biceps_Ax', 'biceps_Ay', 'biceps_Az',
               'palm_Gx', 'palm_Gy', 'palm_Gz', 'palm_Ax', 'palm_Ay', 'palm_Az',
                'thigh_Gx', 'thigh_Gy', 'thigh_Gz', 'thigh_Ax', 'thigh_Ay', 'thigh_Az',
                'current', 'voltage', 'power', 'energy']
    writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
    writer.writeheader()
    for i in range (len(readings)):
        writer.writerow(readings[i])
port.close()
