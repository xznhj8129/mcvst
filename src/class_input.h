#pragma once
#include <string>
#include <thread>
#include <opencv2/opencv.hpp>
#include "globals.h"
#include "class_settings.h"
#include "class_track.h"
#include "structs.h"
#include "globals.h"

class InputInterface {
    public:
        InputInterface();
        void Init();
        void input_command(int input);
        void input_vec(TrackInputs inputs, int last_btn1);
};

extern InputInterface input_intf;
