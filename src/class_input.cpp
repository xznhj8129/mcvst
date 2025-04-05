
#include "class_input.h"
InputInterface input_intf;

InputInterface::InputInterface(){};

//"yolomodels/yolov5s-visdrone.onnx"
void InputInterface::Init() {
};


void InputInterface::input_command(int input) {
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
            if (!track_intf.track) {
                track_intf.lock(track_intf.poi.x, track_intf.poi.y);
            }
            else {
                track_intf.breaklock();
            }
            break;

        case 2: 
            track_intf.clearlock();
            break;

        case 3: 
            track_intf.moveUp();
            break;

        case 4: 
            track_intf.moveDown();
            break;

        case 5: 
            track_intf.moveLeft();
            break;

        case 6: 
            track_intf.moveRight();
            break;

        case 7: 
            track_intf.biggerBox();
            break;

        case 8: 
            track_intf.smallerBox();
            break;

        case 9:
            global_running.store(false);
            break;
    }
}

void InputInterface::input_vec(TrackInputs inputs, int last_btn1) {
    if (inputs.valid) {

        if (inputs.lock && last_btn1 == 0) {input_command(1);}
        else if (inputs.unlock) {input_command(2);}

        if (inputs.updown >0.05 || inputs.updown < -0.05) {track_intf.moveVertical(inputs.updown);}  // UP_ARROW

        if (inputs.leftright >0.05 || inputs.leftright < -0.05) {track_intf.moveHorizontal(inputs.leftright);}  // LEFT_ARROW

        if (inputs.boxsize== -1) {input_command(7);} // s
        else if (inputs.boxsize == 1) {input_command(8);} // a
    }
}

