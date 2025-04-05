#pragma once

#include <opencv2/opencv.hpp>
#include <condition_variable>
#include <vector>
#include <mutex>
#include <atomic>
#include "structs.h"

// Globals

extern bool global_debug_print;
extern std::atomic<bool> global_running;
extern std::atomic<int> latestKeyCode;
extern std::atomic<int> framecounter;

struct SharedData {

    std::atomic<bool> searchMode{false}; //???????????
    cv::Mat searchFrame;
    cv::Mat trackFrame;
    cv::Mat displayFrame;
    std::atomic<bool> hasNewFrame{false};
    std::atomic<bool> global_running{true};
    std::atomic<bool> global_debug_print = true;
    std::atomic<int> framecounter;
    std::atomic<int> latestKeyCode{0}; // Use 0 to indicate no key press

    std::mutex frameMutex;
    std::condition_variable frameCondVar;
};