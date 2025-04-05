
#pragma once
#include "globals.h"
#include "classes.h"

int capture_thread(SharedData& sharedData) {
    bool finish_error = false;
    
    // Only calculate duration if needed
    std::chrono::milliseconds frameDuration = (settings.capFPS != 0) 
        ? std::chrono::milliseconds(1000 / settings.capFPS)
        : std::chrono::milliseconds(0);

    while (global_running) {
        // ONLY time if we need to limit FPS
        auto startTime = (settings.capFPS != 0)
            ? std::chrono::steady_clock::now()
            : std::chrono::steady_clock::time_point{};

        bool gotframe = false;
        cv::Mat frame;

        cap_intf.video.read(frame);

        if (!frame.empty()) {
            framecounter += 1;
            //std::lock_guard<std::mutex> lock(sharedData.frameMutex);
            frame.copyTo(sharedData.searchFrame);
            frame.copyTo(sharedData.trackFrame);
            frame.copyTo(sharedData.displayFrame);
            sharedData.hasNewFrame.store(true);
            sharedData.frameCondVar.notify_all();
            gotframe = true;
            cap_intf.no_feed = false;
            if (settings.capSize.height == 0 && settings.capSize.width==0) {
                settings.capSize.width = frame.cols;
                settings.capSize.height = frame.rows;
                std::cout << "Set size to " << settings.capSize.width << "x" << settings.capSize.height << std::endl;
            }
        } else {
            gotframe = false;
            cap_intf.no_feed = true;
            sharedData.hasNewFrame.store(false);
            sharedData.frameCondVar.notify_all();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // ONLY sleep if we're limiting FPS
        if (settings.capFPS != 0) {
            auto endTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            auto sleepTime = frameDuration - elapsedTime;
            
            if (sleepTime > std::chrono::milliseconds(0)) {
                std::this_thread::sleep_for(sleepTime);
            }
        }
    }

    global_running.store(false);
    if (global_debug_print) std::cout << "capture thread finished" << std::endl;
    return finish_error ? 1 : 0;
}
