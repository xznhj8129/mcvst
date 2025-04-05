import socket
import time
import json

run = True
connected = False
while run:
    try:
        server_address = ('localhost', 8100)  # Adjust port number as needed
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as serv:
            serv.settimeout(5)
            serv.connect(server_address)
            connected = True
            print('Connected to OpenCV')
            while connected:
                try:
                    serv.send(b"x")
                    data = serv.recv(1024)
                    if not data:
                        break
                    jsd = json.loads(data)
                    print(jsd)
                    time.sleep(0.01)
                except TimeoutError:
                    print('Connection timeout')
                    connected = False
                except ConnectionResetError:
                    print('Connection closed')
                    connected = False


    except ConnectionRefusedError:
        print("Connection refused. Make sure the server is running.")
        time.sleep(1)
        #run = False