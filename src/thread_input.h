#pragma once
#include "globals.h"
#include "classes.h"

int input_thread(SharedData& sharedData) {
    bool finish_error = false;

    //insert thread sleeper here

    if (settings.displayType == 1) {} //do nothing, handled by opencv

    else if (settings.displayType == 2) { //framebuffer
        display_intf.setTerminalMode(true);
        char c;
        while (global_running) {
            c = getchar();
            int keyCode = 0; 
            if (c == ' ') {
                if (!trackdata.target_lock) {
                    trackdata.lock(trackdata.framesize.width/2, trackdata.framesize.height/2);
                }
                else {
                    trackdata.target_lock = false;
                    trackdata.locked = false;
                }
            } else if (c == 27) { // ESC sequence for arrow keys
                getchar(); // Skip the '['
                c = getchar();
                switch(c) {
                    case 'A': 
                        trackdata.changeROI(82);
                        break; // Up arrow
                    case 'B': 
                        trackdata.changeROI(84);
                        break; // Down arrow
                    case 'C': 
                        trackdata.changeROI(83);
                        break; // Right arrow
                    case 'D': 
                        trackdata.changeROI(81);
                        break; // Left arrow
                    default: 
                        keyCode = 27; 
                        global_running.store(false);
                        break; // ESC pressed
                }
            } else if (c == 'c' || c =='a' || c == 's') {
                trackdata.changeROI(c);
            }
            if (keyCode != 0) { // Reset after processing
                latestKeyCode.store(0); }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        display_intf.setTerminalMode(false); // Restore terminal settings
    }
    else if (settings.inputType==1) //socket 
    {}
    else if (settings.inputType==2) //serial
    {}

    if (global_debug_print) {std::cout << "input thread finished" << std::endl;}
    if (!finish_error) {return 0;}
    else {return 1;}
}