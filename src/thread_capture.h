#pragma once
#include "globals.h"
#include "classes.h"

int capture_thread(SharedData& sharedData) {
    bool finish_error = false;
    std::chrono::milliseconds frameDuration(1000 / settings.capFPS);
    while (global_running) {
    //while (sharedData.global_running.load()) {
        auto startTime = std::chrono::steady_clock::now();
        bool gotframe = false;
        cv::Mat frame;

        while (!gotframe and global_running) {

        auto startTime2 = std::chrono::steady_clock::now();
        // sharedData.hasNewFrame.store(false); //debug: causes tracking failure for some reason
        cap_intf.video.read(frame);
        auto endTime2 = std::chrono::steady_clock::now();
        auto elapsedTime2 = std::chrono::duration_cast<std::chrono::milliseconds>(endTime2 - startTime2);
        //std::cout << "cap time: " << elapsedTime2.count() << "ms" << std::endl;
            //video.read(frame);

            if (!frame.empty()) {
                //std::lock_guard<std::mutex> lock(sharedData.frameMutex);
                frame.copyTo(sharedData.searchFrame);
                frame.copyTo(sharedData.trackFrame);
                frame.copyTo(sharedData.displayFrame);
                sharedData.hasNewFrame.store(true);
                sharedData.frameCondVar.notify_all();
                gotframe = true;
            } else {
                //std::lock_guard<std::mutex> lock(sharedData.frameMutex);
                //cap_intf.novideo.copyTo(sharedData.searchFrame); //debug: causes tracking loss when using real camera? 
                //cap_intf.novideo.copyTo(sharedData.trackFrame);
                //cap_intf.novideo.copyTo(sharedData.displayFrame);
                sharedData.hasNewFrame.store(false);
                sharedData.frameCondVar.notify_all();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        auto endTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        auto sleepTime = frameDuration - elapsedTime;

        if (sleepTime > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleepTime);
        }
    }
    global_running.store(false);
    if (global_debug_print) {std::cout << "capture thread finished" << std::endl;}

    if (!finish_error) {return 0;}
    else {return 1;}
}