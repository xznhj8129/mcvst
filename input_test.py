import socket
import time
import json
import pygame

def get_joystick(joystick):
    # roll, pitch, throt, yaw, SE, SB, SF, SE; Down/Left= -1
    #Far LT     6
    #Near LT    4
    #Far RT     7
    #Near RT    5
    #A1         0
    #B          1
    #C          2
    #D          3
    # SELECT    8
    # START     9
    #JSCLICK L 10
    #JSCLICK R  11
    # Down = positive direction
    #L JOY DOWN 2
    #L JOY RIGHT 1
    #R JOY DOWN  3
    #R JOY RIGHT 4
    axisdata = [[0]*12, [0]*12]
    axes = [0] * 12
    buttons = [0] * 12
    for i in range(joystick.get_numaxes()):
        axis = round(joystick.get_axis(i),3)
        #if axis!=0:
        #    print('axis',i, axis)
        axes[i] = axis
    for i in range(joystick.get_numbuttons()):
        btt = joystick.get_button(i)
        #if btt!=0:
        #    print('button',i, btt)
        buttons[i] = btt
    return axes, buttons

def gen_command(joystick):
    
    #Inputs:
    #- Lock (bool, momentary)
    #- Reset (bool, momentary)
    #- Updown (-1 to 1)
    #- Leftright (-1 to 1)
    #- Boxsize (-1, 0, 1) (smaller or bigger)
    #- Shutdown
    pygame.event.pump()
    axis, buttons = get_joystick(joystick)
    cmd = {}
    cmd["lock"] = buttons[7]
    cmd["reset"] = buttons[3]
    cmd["lr"] = axis[2]
    cmd["ud"] = axis[3] * -1
    cmd["boxsize"] = int(axis[0])
    cmd["shutdown"] = 0
    j = json.dumps(cmd)+'\n'
    return j
    

# Initialize pygame and the joystick
pygame.init()
pygame.joystick.init()

# Check if there is at least one joystick connected
if pygame.joystick.get_count() == 0:
    print("No joystick connected.")
    pygame.quit()
    exit()

# Use the first joystick
joystick = pygame.joystick.Joystick(0)
joystick.init()

print("Joystick initialized.")

run = True
connected = False
while run:
    try:
        server_address = ('localhost', 8101)  # Adjust port number as needed
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as serv:
            serv.settimeout(5)
            serv.connect(server_address)
            connected = True
            print('Connected to MCVST')
            while connected:
                try:
                    
                    c = gen_command(joystick)
                    serv.send(bytes(c,encoding="utf-8"))

                    r = serv.recv(1)

                    time.sleep(0.1)
                except TimeoutError:
                    print('Connection timeout')
                    connected = False
                except ConnectionResetError:
                    print('Connection closed')
                    connected = False
                except BrokenPipeError:
                    print('Connection broken')
                    connected = False


    except ConnectionRefusedError:
        print("Connection refused. Make sure the server is running.")
        time.sleep(1)
        #run = False"""