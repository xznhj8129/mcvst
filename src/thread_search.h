#pragma once
#include "globals.h"
#include "classes.h"


int search_thread(SharedData& sharedData) {
    bool finish_error = false;
    cv::Mat frame;
    cv::Mat processframe;

    if (!search_intf.setup) {
        if (global_debug_print) {std::cout << "search thread finished" << std::endl;}
        return 1;
    }

    // insert thread sleeper here

    if (settings.searchType==0) {} //passthrough
    else if (settings.searchType==1) {}
    else {
        std::cerr << "Invalid search type selected" << std::endl;
        finish_error = true;
    }

    while (global_running & !finish_error) {
        int frame_count;

        //std::unique_lock<std::mutex> lock(sharedData.frameMutex);
        //sharedData.frameCondVar.wait(lock, [&sharedData] { return sharedData.hasNewFrame.load(); });
        cv::Mat frame = sharedData.searchFrame.clone();
        //lock.unlock();

        processframe = cv::Mat();
        if (trackdata.image_scale!=1.0) {cv::resize(frame, processframe, search_intf.procSize);}
        else {processframe = frame.clone();}
        
        if (settings.searchType == 0 || cap_intf.no_feed) { // 
           break;
        }
        else if (settings.searchType==1) {
        
            SearchResults output;
            search_intf.detect(frame, output);
            int detections = output.size();

            for (int i = 0; i < detections; ++i)
            {
                SearchDetection detection = output[i];
                //cv::Rect box = detection.box;
                int classId = detection.class_id;
                std::cout << search_intf.class_list[classId] << " " << classId << " " << detection.confidence << " "<< detection.box.x+(detection.box.width/2) << " "<< detection.box.y+(detection.box.height/2) << std::endl;
            }

            search_intf.output = output;
            //std::swap(search_intf.output, output);
            frame_count++;
        }
    }

    if (global_debug_print) {std::cout << "search thread finished" << std::endl;}
    if (!finish_error) {return 0;}
    else {
        global_running.store(false);
        return 1;
    }
}