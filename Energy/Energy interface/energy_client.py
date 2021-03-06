import serial
import time
import socket
from time import sleep

HOST = '3.129.44.87'  # The server's hostname or IP address
PORT = 8080            # The port used by the server
cycle_1 = '1\n'
cycle_2 = '2\n'
idle = '0\n'
marker = 123456
comma = ","
client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect((HOST, PORT))

ser = serial.Serial(port='COM8', baudrate=9600, timeout=.1)
ser.flushInput()


while True:
    
    #try:
    ser_bytes = ser.readline().decode('utf-8').rstrip()
    to_server = int(ser_bytes)
    data = str(marker) + str(comma) + str(to_server)
    #client.send(marker.encode('utf-8'),to_server)
    #client.send(marker, to_server)
    client.send(data.encode('utf-8'))
    
    print('Read from Arduino', ser_bytes)
    print('Sent to server: ', data)

    sleep(1)
    
    '''Support for Energy state machines'''
    
    '''
    if decoded_command == ARCHONTIS_0 :
        ser.write(idle.encode('utf-8'))
    elif decoded_command == ARCHONTIS_1 :
        ser.write(cycle_1.encode('utf-8'))
    else:
        ser.write(cycle_2.encode('utf-8'))
    '''
    