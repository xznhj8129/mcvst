#pragma once
#include "globals.h"
#include "classes.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unistd.h>

#define FIFO_PATH "/tmp/input_fifo"

TrackInputs read_from_fifo() {
    std::ifstream fifo(FIFO_PATH);
    std::string line;
    TrackInputs inputs;

    if (fifo) {
        std::getline(fifo, line);
        std::stringstream ss(line);
        std::string token;

        if (std::getline(ss, token, ',')) {
            inputs.lock = std::stoi(token);
        }
        if (std::getline(ss, token, ',')) {
            inputs.unlock = std::stoi(token);
        }
        if (std::getline(ss, token, ',')) {
            inputs.updown = std::stof(token);
        }
        if (std::getline(ss, token, ',')) {
            inputs.leftright = std::stof(token);
        }
        if (std::getline(ss, token, ',')) {
            inputs.boxsize = std::stoi(token);
        }
        if (std::getline(ss, token, ',')) {
            inputs.exit = std::stoi(token);
        }
        inputs.valid = true;
    }

    return inputs;
}


void input_command(int input) {
    /*
    Inputs:
        Lock 1
        Unlock 2
        float UpDown 3
        float LeftRight 4
        BoxSize 5
        Exit 6
    
    */
    switch (input) {
        case 1: 
            if (!trackdata.target_lock) {
                trackdata.lock(trackdata.poi.x, trackdata.poi.y);
            }
            else {
                trackdata.target_lock = false;
                trackdata.locked = false;
            }
            break;

        case 2: 
            trackdata.breaklock();
            break;

        case 3: 
            trackdata.moveUp();
            break;

        case 4: 
            trackdata.moveDown();
            break;

        case 5: 
            trackdata.moveLeft();
            break;

        case 6: 
            trackdata.moveRight();
            break;

        case 7: 
            trackdata.biggerBox();
            break;

        case 8: 
            trackdata.smallerBox();
            break;

        case 9:
            global_running.store(false);
            break;
    }
}

void input_vec(TrackInputs inputs, int last_btn1) {
    if (inputs.valid) {

        if (inputs.lock && last_btn1 == 0) {input_command(1);}
        else if (inputs.unlock) {input_command(2);}
        if (inputs.updown >0.1 || inputs.updown < -0.1) {trackdata.moveVertical(inputs.updown);}  // UP_ARROW
        if (inputs.leftright >0.1 || inputs.leftright < -0.1) {trackdata.moveHorizontal(inputs.leftright);}  // UP_ARROW
        if (inputs.boxsize== -1) {input_command(7);} // s
        else if (inputs.boxsize == 1) {input_command(8);} // a
    }
}


int input_thread(SharedData& sharedData) {
    bool finish_error = false;

    //insert thread sleeper here

    if (settings.inputType==0) {
        if (settings.displayType == 1) {} //do nothing, handled by opencv

        else if (settings.displayType == 2) { //framebuffer
            display_intf.setTerminalMode(true);
            char c;
            while (global_running) {
                c = getchar();
                int keyCode = 0; 
                if (c == ' ') {
                    input_command(1);} 
                else if (c == 27) { // ESC sequence for arrow keys
                    getchar(); // Skip the '['
                    c = getchar();
                    switch(c) {
                        case 'A': 
                            input_command(3);
                            break; // Up arrow
                        case 'B': 
                            input_command(4);
                            break; // Down arrow
                        case 'C': 
                            input_command(6);
                            break; // Right arrow
                        case 'D': 
                            input_command(5);
                            break; // Left arrow
                        default: 
                            keyCode = 27; 
                            global_running.store(false);
                            break; // ESC pressed
                    }
                } else if (c == 'c') {input_command(2);}
                else if (c=='a') {input_command(7);}
                else if (c=='s') {input_command(8);}

                if (keyCode != 0) { // Reset after processing
                    latestKeyCode.store(0); }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            display_intf.setTerminalMode(false); // Restore terminal settings
        }
    }
    else if (settings.inputType==1) //socket 
    {}
    else if (settings.inputType==2) //serial
    {}
    else if (settings.inputType==3) //fifo
    {
        int last_btn1 = 0;
        while (global_running) {
            try {
                TrackInputs inputs = read_from_fifo(); // use settings.inputPath
                if (inputs.valid) {
                    input_vec(inputs, last_btn1);
                    last_btn1 = inputs.lock;
                }
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    if (global_debug_print) {std::cout << "input thread finished" << std::endl;}
    if (!finish_error) {return 0;}
    else {return 1;}
}