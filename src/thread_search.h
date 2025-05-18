#pragma once
#include "globals.h"
#include "classes.h"


int search_thread(SharedData& sharedData) {
    bool finish_error = false;
    cv::Mat frame;
    cv::Mat processframe;
    cv::Size process_size((double)cap_intf.frameSize.width * track_intf.image_scale, (double)cap_intf.frameSize.height * track_intf.image_scale);
    if (!search_intf.setup || settings.searchType==0) {
        if (global_debug_print) {std::cout << "search thread finished" << std::endl;}
        return 1;
    }

    std::chrono::milliseconds processtime(1000 / settings.searchFPS);
    int frame_count = 0;
    int fps;
    int total_frames;

    // insert thread sleeper here
    if (settings.searchType==1) {
        std::cout << "Search thread started" << std::endl;
    }
    else {
        std::cerr << "Invalid search type selected" << std::endl;
        finish_error = true;
    }


    cv::Mat mask;
    cv::Rect searchbox;
    if (settings.search_limit_zone) {
        int x = int(process_size.width  * settings.search_zone[0]);
        int y = int(process_size.height * settings.search_zone[1]);
        int w = int(process_size.width  * (settings.search_zone[2] - settings.search_zone[0]));
        int h = int(process_size.height * (settings.search_zone[3] - settings.search_zone[1]));
        searchbox = cv::Rect{x,y,w,h};

        mask = cv::Mat(process_size, CV_8UC1, cv::Scalar(0));
        mask(searchbox).setTo(255);
        cv::bitwise_not(mask, mask);
    }

    auto start = std::chrono::high_resolution_clock::now();
    while (global_running & !finish_error) {

        if (settings.searchType == 0 || cap_intf.no_feed) { // 
           break;
        }
        else {        
            frame_count++;
            total_frames++;
            auto startTime = std::chrono::steady_clock::now();

            std::unique_lock<std::mutex> lock(sharedData.frameMutex);
            sharedData.frameCondVar.wait(lock, [&sharedData] { return sharedData.hasNewFrame.load(); });
            cv::Mat frame = sharedData.searchFrame.clone();
            lock.unlock();
            
            if (track_intf.image_scale!=1.0) {cv::resize(frame, processframe, process_size);}
            else {processframe = frame.clone();}

            if (settings.search_limit_zone) {
                processframe.setTo(cv::Scalar(0,0,0), mask);
            }

            if (settings.searchType==1) {
                SearchResults output;
                search_intf.detect(processframe, output); /////
                int detections = output.size();

                for (int i = 0; i < detections; ++i)
                {
                    SearchDetection detection = output[i];
                    int classId = detection.class_id;
                    if (settings.debug_print) {
                        std::cout << search_intf.class_list[classId] << " " << classId << " " << detection.confidence << " "<< detection.box.x+(detection.box.width/2) << " "<< detection.box.y+(detection.box.height/2) << std::endl;
                        }
                }
                search_intf.output = output;
                frame_count++;
                
            }
            if (frame_count >= 30) {
                auto end = std::chrono::high_resolution_clock::now();
                fps = (int)(frame_count * 1000.0 / std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
                frame_count = 0;
                start = std::chrono::high_resolution_clock::now();
            }
            if (fps > 0) {
                search_intf.search_fps = fps;
            }

            auto endTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            auto sleepTime = processtime - elapsedTime;
            if (settings.searchFPS>0 && processtime > std::chrono::milliseconds(0)) {
                std::this_thread::sleep_for(sleepTime);
            }

        }
    }
    //cv::cuda::resetDevice();

    if (global_debug_print) {std::cout << "Search thread finished" << std::endl;}
    if (!finish_error) {return 0;}
    else {
        global_running.store(false);
        return 1;
    }
}