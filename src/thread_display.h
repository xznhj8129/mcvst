#pragma once
#include "globals.h"
#include "classes.h"

void onMouse(int event, int x, int y, int, void*) {
    if (event == cv::EVENT_LBUTTONDOWN) {trackdata.lock(x, y);}
}

int display_thread(SharedData& sharedData) {
    bool finish_error = false;
    std::chrono::milliseconds frameDuration(1000 / settings.displayFPS);
    cv::Mat frame;
    int frame_count = 0;
    int fc = 0;
    int total_frames = 0;
    int fps = -1;

    if (settings.displayType==0) {
        if (global_debug_print) {std::cout << "display thread finished" << std::endl;}
        return 0;
    }
    else if (settings.displayType == 1) {
        cv::namedWindow("Tracking", cv::WINDOW_NORMAL);
        cv::setMouseCallback("Tracking", onMouse);
        cv::imshow("Tracking",cap_intf.novideo);
    }
    else if (settings.displayType == 2) {
        display_intf.clearFramebuffer();
    }
    else {
        std::cerr << "Invalid display type selected" << std::endl;
        finish_error = true;
    }

    auto start = std::chrono::high_resolution_clock::now();
    while (global_running and !finish_error) {
        auto startTime = std::chrono::steady_clock::now();

        frame = cv::Mat();
        //std::unique_lock<std::mutex> lock(sharedData.frameMutex);
        //sharedData.frameCondVar.wait(lock, [&sharedData] { return sharedData.hasNewFrame.load(); });
        cv::Mat frame = sharedData.displayFrame.clone();
        //lock.unlock();

        frame_count++;
        total_frames++; 

        if (frame.empty()) {
            fc++;
            std::cout << "empty frame " << fc << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else {
            
            //cv::rectangle(frame, trackdata.scaledRoi(), display_intf.osdcolor, display_intf.linesize);
            if (trackdata.locked) {display_intf.draw_track(frame);}
            else if (settings.showPipper) {display_intf.draw_cornerbox(frame, cv::Point(trackdata.poi.x, trackdata.poi.y), trackdata.boxsize);}

            if (settings.showFPS) {
                if (frame_count >= 30) {
                    auto end = std::chrono::high_resolution_clock::now();
                    fps = frame_count * 1000.0 / std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                    frame_count = 0;
                    start = std::chrono::high_resolution_clock::now();
                }
                if (fps > 0) {
                    std::ostringstream fps_label;
                    fps_label << std::fixed << std::setprecision(2);
                    fps_label << "FPS: " << fps;
                    std::string fps_label_str = fps_label.str();
                    cv::putText(frame, fps_label_str, cv::Point(10, 25), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
                }
            }

            if (settings.displayType == 1) {
                cv::imshow("Tracking", frame);
                int keyCode = cv::waitKey(1) & 0xFF; // Ensure keyCode is correctly masked with 0xFF
                if (keyCode == 27) {break;} // Exit if ESC is pressed
                if (keyCode!=255) {trackdata.changeROI(keyCode);}
                        
            } else if (settings.displayType == 2) {
                display_intf.writeImageToFramebuffer(frame);
            }

            auto endTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            //std::cout << "display frame: " << elapsedTime.count() << " ms" << std::endl;
            auto sleepTime = frameDuration - elapsedTime;
            frame_count++;

            if (sleepTime > std::chrono::milliseconds(0)) {
                std::this_thread::sleep_for(sleepTime);
            }
        }
        

    }
    if (settings.displayType == 1) {cv::destroyAllWindows();}

    if (global_debug_print) {std::cout << "display thread finished" << std::endl;}
    global_running.store(false);
    if (!finish_error) {return 0;}
    else {return 1;}
}
