#pragma once
#include <string>
#include <thread>
#include <opencv2/opencv.hpp>
#include "globals.h"
#include "class_settings.h"

class CaptureInterface {
    private:
        
    public:
        int captype;
        cv::Mat novideo;
        cv::VideoCapture video;
        cv::Mat cap_frame;
        cv::Size frameSize;
        bool got_video;
        bool no_feed;
        bool setup;
        CaptureInterface();
        ~CaptureInterface();
        void Init(int captureType, std::string capturePath, cv::Size capSize);
        void capFrame(SharedData& sharedData);
        //cv::Mat getFrame();
};

extern CaptureInterface cap_intf;