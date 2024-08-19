#pragma once
#include "globals.h"
#include "classes.h"
#include <opencv2/tracking.hpp>

int tracking_thread(SharedData& sharedData) {

    if (!trackdata.setup) {
        if (global_debug_print) {std::cout << "tracking not set up" << std::endl;}
        global_running.store(false);
        return 1;
    }
    std::chrono::milliseconds processtime(1000 / settings.trackingFPS);
    cv::Mat frame;
    cv::Mat processframe;
    bool finish_error = false;
    cv::Size process_size((double)cap_intf.frameSize.width * trackdata.image_scale, (double)cap_intf.frameSize.height * trackdata.image_scale);
    int update_frames = 20;
    int updc = 0;
    bool ok = false;

    //tracker pointers
    cv::Ptr<cv::Tracker> tracker;

    //oft specific
    cv::Mat frame_gray_init;
    std::vector<cv::Point2f> corners;
    cv::TermCriteria termcrit;

    if (settings.trackerType == 0) {
        std::cout << "tracking thread finished" << std::endl;
        return 0;
    } else if (settings.trackerType == 1) {
        
        std::unique_lock<std::mutex> lock(sharedData.frameMutex);
        sharedData.frameCondVar.wait(lock, [&sharedData] { return sharedData.hasNewFrame.load(); });
        cv::Mat frame = sharedData.trackFrame.clone();
        lock.unlock();

        if (trackdata.image_scale!=1.0) {cv::resize(frame, processframe, process_size);}
        else {processframe = frame.clone();}
        cv::cvtColor(processframe, frame_gray_init, cv::COLOR_BGR2GRAY);
        cv::goodFeaturesToTrack(frame_gray_init, corners, 100, 0.3, 7); //<configurable>
        termcrit = cv::TermCriteria(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 10, 0.03); //<configurable>
    } else if (settings.trackerType == 2) {}
    else {
        std::cerr << "Invalid tracker type selected" << std::endl;
        finish_error = true;
    }
    
    while (global_running & !finish_error) {
        auto startTime = std::chrono::steady_clock::now();
        bool passthrough = false;


        std::unique_lock<std::mutex> lock(sharedData.frameMutex);
        sharedData.frameCondVar.wait(lock, [&sharedData] { return sharedData.hasNewFrame.load(); });
        cv::Mat frame = sharedData.trackFrame.clone();
        lock.unlock();

        processframe = cv::Mat();
        if (trackdata.image_scale!=1.0) {cv::resize(frame, processframe, process_size);}
        else {processframe = frame.clone();}
        
        if (settings.trackerType == 0 || cap_intf.no_feed) { // 
           passthrough = true;
        }
        else if (settings.trackerType == 1) { // OFT
            
            if (trackdata.target_lock) {        
                cv::Mat frame_gray;
                cv::cvtColor(processframe, frame_gray, cv::COLOR_BGR2GRAY);
                updc += 1;
                if (updc > update_frames) {
                    cv::goodFeaturesToTrack(frame_gray, corners, 100, 0.3, 7); // <configurable> oft variables
                    updc = 0;
                }
                std::vector<cv::Point2f> new_points;
                std::vector<uchar> status;
                std::vector<float> errors;
                
                cv::calcOpticalFlowPyrLK(frame_gray_init, frame_gray, trackdata.oftdata.old_points, new_points, status, errors, cv::Size(15, 15), 2, termcrit); //<configurable> oft variables
                frame_gray_init = frame_gray.clone();
                trackdata.oftdata.old_points = new_points;
                trackdata.poi = new_points[0];

                trackdata.lastroi = trackdata.roi;
                trackdata.roi.x = trackdata.poi.x - (trackdata.boxsize / 2);
                trackdata.roi.y = trackdata.poi.y - (trackdata.boxsize / 2);
                trackdata.roi.width = trackdata.boxsize;
                trackdata.roi.height = trackdata.boxsize;
            }
                
        }
        else if (settings.trackerType == 2 or settings.trackerType == 3) { // CRST/KCF unfinished, implementation sucks
            std::cout << "target_lock " << trackdata.target_lock << std::endl;
            std::cout << "locked " << trackdata.locked << std::endl;
            std::cout << "first_lock " << trackdata.first_lock << std::endl;

            if (trackdata.target_lock & (!trackdata.locked || (trackdata.first_lock && !ok))) {
                std::cout << "kcf1" << std::endl;

                //integrate parameter customization: https://docs.opencv.org/4.9.0/db/dd1/structcv_1_1TrackerKCF_1_1Params.html
                if (settings.trackerType == 2) {
                    cv::TrackerKCF::Params x;
                    tracker = cv::TrackerKCF::create();
                    }
                else if (settings.trackerType == 3) {
                    tracker = cv::TrackerCSRT::create();
                    }

                tracker->init(processframe, trackdata.roi);
                ok = tracker->update(processframe, trackdata.roi); //True means that target was located and false means that tracker cannot locate target in current frame. Note, that latter does not imply that tracker has failed, maybe target is indeed missing from the frame (say, out of sight)
            }

            if (ok && trackdata.roi.width!=0 && trackdata.roi.height!=0) {
                std::cout << "kcf2" << std::endl;
                trackdata.lastroi = trackdata.roi;
                trackdata.poi.x = trackdata.roi.x + (trackdata.boxsize/2);
                trackdata.poi.y = trackdata.roi.y + (trackdata.boxsize/2);
                ok = tracker->update(processframe, trackdata.roi);

                if (!ok) {trackdata.lost_lock = true;}
            }
        }

        if (trackdata.target_lock) {
            trackdata.guide();
            if( trackdata.poi.x < 0 || 
                trackdata.poi.x > (cap_intf.frameSize.width * trackdata.image_scale) || 
                trackdata.poi.y < 0 || 
                trackdata.poi.y > (cap_intf.frameSize.height * trackdata.image_scale)) {
                std::cout << "Lost track" << std::endl;
                trackdata.lost_lock = true;
                trackdata.target_lock = false;
            }
        }
        
        auto endTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        auto sleepTime = processtime - elapsedTime;
        if (processtime > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleepTime);
        }
    }

    if (global_debug_print) {std::cout << "tracking thread finished" << std::endl;}
    global_running.store(false);
    if (!finish_error) {return 0;}
    else {return 1;}
}