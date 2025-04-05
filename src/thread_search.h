#pragma once
#include "globals.h"
#include "classes.h"


int search_thread(SharedData& sharedData) {
    bool finish_error = false;
    cv::Mat frame;
    cv::Mat processframe;
    cv::Size process_size((double)cap_intf.frameSize.width * track_intf.image_scale, (double)cap_intf.frameSize.height * track_intf.image_scale);
    int frame_count;
    if (!search_intf.setup || settings.searchType==0) {
        if (global_debug_print) {std::cout << "search thread finished" << std::endl;}
        return 1;
    }

    // insert thread sleeper here
    if (settings.searchType==1) {
        std::cout << "Search thread started" << std::endl;
    }
    else {
        std::cerr << "Invalid search type selected" << std::endl;
        finish_error = true;
    }

    while (global_running & !finish_error) {

        if (settings.searchType == 0 || cap_intf.no_feed) { // 
           break;
        }
        else {
            std::unique_lock<std::mutex> lock(sharedData.frameMutex);
            sharedData.frameCondVar.wait(lock, [&sharedData] { return sharedData.hasNewFrame.load(); });
            cv::Mat frame = sharedData.searchFrame.clone();
            lock.unlock();
            processframe = frame.clone();
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
        }
    }
    cv::cuda::resetDevice();

    if (global_debug_print) {std::cout << "Search thread finished" << std::endl;}
    if (!finish_error) {return 0;}
    else {
        global_running.store(false);
        return 1;
    }
}