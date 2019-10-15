#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import socket
from client_auth import encryptText
import threading

class Client(threading.Thread):
    def __init__(self, ip_addr, port_num):
        threading.Thread.__init__(self)
        self.shutdown = threading.Event()
        self.clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.clientSocket.connect((ip_addr, port_num))

    def run(self):
        while not self.shutdown.is_set():
            print('Please input the data: ')
            userInput = input()
            encodedmsg = encryptText(userInput, secret_key)
            self.clientSocket.sendall(encodedmsg)
            action = userInput[userInput.find('#') + 1:].split('|')[0]
            if action == 'logout':
                print("Closing socket...")
                self.clientSocket.close()
                self.shutdown.set()


PORT = 6788
HOST = "172.31.27.114"
secret_key = 'secretkeysixteen'

my_client = Client(HOST, PORT)
my_client.start()
