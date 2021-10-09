import serial
import time
import socket
from time import sleep

HOST = '3.129.44.87'  # The server's hostname or IP address
PORT = 8080            # The port used by the server
cycle_1 = '1\n'
cycle_2 = '2\n'
idle = '0\n'

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect((HOST, PORT))

ser = serial.Serial(port='COM8', baudrate=9600, timeout=.1)
ser.flushInput()


while True:
    
    #try:
    client.send('123,'.encode('utf-8'))
    ser_bytes = ser.readline().decode('utf-8').rstrip()
    client.send(ser_bytes.encode('utf-8'))
    '''
    command = client.recv(1024)
    decoded_command = str(command[0:len(command)].decode("utf-8"))
    print('Received from server: ', decoded_command)
    #ser_bytes = ser.readline().decode('utf-8').rstrip()
    print('Read from Arduino', ser_bytes)


    #decoded_bytes = float(ser_bytes[0:len(ser_bytes)-2].decode("utf-8"))   <- might be an useful function to understand how decoding works
    #ser.write(str(chr(counter)))
    #ser.write(bytes(counter, 'utf-8')) #WORKS, BUT PRINTS THE NUMBER TWICE IF \N IS NOT USED
    #ser.write(counter.encode('utf-8')) #WORKS, BUT PRINTS THE DATA TWICE IF \N IS NOT USED
    # ^USED TO SEND WHATEVER THE SERVER SAYS TO THE ARDUINO
    #ser.write('hello'.encode('utf-8')) #WORKS, BUT PRINTS THE DATA TWICE IF \N IS NOT USED
    #ser.writelines('hi'.encode('utf-8')) #BAD
    #client.send(ser_bytes.encode('utf-8'))
    

    client.send('Sent to server: '.encode('utf-8'))
    client.send(ser_bytes.encode('utf-8'))# '\n'.encode('utf-8'))
    client.send('\n'.encode('utf-8'))'''
    sleep(1)
    
    '''Support for Energy state machines'''
    
    '''
    if decoded_command == ARCHONTIS_0 :
        ser.write(idle.encode('utf-8'))
    elif decoded_command == ARCHONTIS_1 :
        ser.write(cycle_1.encode('utf-8'))
    else:89
        ser.write(cycle_2.encode('utf-8'))
    '''
    
    
    ''' (leftovers)
        with open("test_data.csv","a") as f:
            writer = csv.writer(f,delimiter=",")
            writer.writerow([time.time(),decoded_bytes])
        '''    
    '''except:
        print("Keyboard interrupt")
        break'''