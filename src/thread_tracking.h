#pragma once
#include "globals.h"
#include "classes.h"
#include <opencv2/tracking.hpp>

int tracking_thread(SharedData& sharedData) {

    if (!track_intf.setup) {
        if (global_debug_print) {std::cout << "tracking not set up" << std::endl;}
        global_running.store(false);
        return 1;
    }
    std::chrono::milliseconds processtime(1000 / settings.trackingFPS);
    cv::Mat frame;
    cv::Mat processframe;
    int frame_count = 0;
    bool finish_error = false;
    cv::Size process_size((double)cap_intf.frameSize.width * track_intf.image_scale, (double)cap_intf.frameSize.height * track_intf.image_scale);
    int update_frames = 20;
    int updc = 0;
    bool ok = false;
    int fps;
    int total_frames;

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

        if (track_intf.image_scale!=1.0) {cv::resize(frame, processframe, process_size);}
        else {processframe = frame.clone();}
        cv::cvtColor(processframe, frame_gray_init, cv::COLOR_BGR2GRAY);
        cv::goodFeaturesToTrack(frame_gray_init, corners, 100, 0.3, 7); //<configurable>
        termcrit = cv::TermCriteria(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 10, 0.03); //<configurable>
    } 
    else if (settings.trackerType == 2) {}
    else {
        std::cerr << "Invalid tracker type selected" << std::endl;
        finish_error = true;
    }
    track_intf.poi = cv::Point(cap_intf.frameSize.width/2, cap_intf.frameSize.height/2);
    
    auto start = std::chrono::high_resolution_clock::now();
    while (global_running and !finish_error) {
        frame_count++;
        total_frames++;
        auto startTime = std::chrono::steady_clock::now();
        bool passthrough = false;

        std::unique_lock<std::mutex> lock(sharedData.frameMutex);
        sharedData.frameCondVar.wait(lock, [&sharedData] { return sharedData.hasNewFrame.load(); });
        cv::Mat frame = sharedData.trackFrame.clone();
        lock.unlock();
        float tolerance = 10.0;
        processframe = cv::Mat();
        if (track_intf.image_scale!=1.0) {cv::resize(frame, processframe, process_size);}
        else {processframe = frame.clone();}
        
        if (settings.trackerType == 0 || cap_intf.no_feed) { // 
           passthrough = true;
        }
        else if (settings.trackerType == 1) { // OFT
            //std::cout << "track loop " << updc << std::endl;
            if (track_intf.target_lock) {      
                //std::cout << "lock loop " << updc << std::endl;  
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
                
                cv::calcOpticalFlowPyrLK(frame_gray_init, frame_gray, track_intf.oftdata.old_points, new_points, status, errors, cv::Size(15, 15), 2, termcrit); //<configurable> oft variables
                //std::cout << track_intf.target_lock << " " << track_intf.locked << " poi " << track_intf.poi << " roi " << track_intf.roi << std::endl;
                if (track_intf.isPointInROI(new_points[0], tolerance)) {
                    //std::cout << "good lock " << updc << std::endl;
                    frame_gray_init = frame_gray.clone();
                    track_intf.oftdata.old_points = new_points;
                    track_intf.poi = new_points[0];

                    track_intf.lastroi = track_intf.roi;
                    track_intf.roi.x = track_intf.poi.x - (track_intf.boxsize / 2);
                    track_intf.roi.y = track_intf.poi.y - (track_intf.boxsize / 2);
                    track_intf.roi.width = track_intf.boxsize;
                    track_intf.roi.height = track_intf.boxsize;
                    if (track_intf.target_lock) {track_intf.locked = true;}
                }
                else {
                    //std::cout << "fucker " << new_points[0] << " " << track_intf.roi <<std::endl;
                    //std::cout << "bad lock" << std::endl;
                    track_intf.target_lock = false;
                    track_intf.locked = false;
                }
            }
                
        }
        else if (settings.trackerType == 2 or settings.trackerType == 3) { // CRST/KCF unfinished, implementation sucks
            //std::cout << "target_lock " << track_intf.target_lock << std::endl;
            //std::cout << "locked " << track_intf.locked << std::endl;
            //std::cout << "first_lock " << track_intf.first_lock << std::endl;

            if (track_intf.target_lock & (!track_intf.locked || (track_intf.first_lock && !ok))) {
                std::cout << "kcf1" << std::endl;

                //integrate parameter customization: https://docs.opencv.org/4.9.0/db/dd1/structcv_1_1TrackerKCF_1_1Params.html
                if (settings.trackerType == 2) {
                    cv::TrackerKCF::Params x;
                    tracker = cv::TrackerKCF::create();
                    }
                else if (settings.trackerType == 3) {
                    tracker = cv::TrackerCSRT::create();
                    }

                tracker->init(processframe, track_intf.roi);
                ok = tracker->update(processframe, track_intf.roi); //True means that target was located and false means that tracker cannot locate target in current frame. Note, that latter does not imply that tracker has failed, maybe target is indeed missing from the frame (say, out of sight)
            }

            if (ok && track_intf.roi.width!=0 && track_intf.roi.height!=0) {
                std::cout << "kcf2" << std::endl;
                track_intf.lastroi = track_intf.roi;
                track_intf.poi.x = track_intf.roi.x + (track_intf.boxsize/2);
                track_intf.poi.y = track_intf.roi.y + (track_intf.boxsize/2);
                ok = tracker->update(processframe, track_intf.roi);

                if (!ok) {
                    track_intf.lost_lock = true;
                    track_intf.locked = false;
                    }
            }
        }

        if (track_intf.locked) {
            track_intf.guide();
            if( track_intf.poi.x < 0 || 
                track_intf.poi.x > (cap_intf.frameSize.width * track_intf.image_scale) || 
                track_intf.poi.y < 0 || 
                track_intf.poi.y > (cap_intf.frameSize.height * track_intf.image_scale)) {
                std::cout << "Lost track" << std::endl;
                track_intf.lost_lock = true;
                track_intf.breaklock();
            }
        }

        if (frame_count >= 30) {
            auto end = std::chrono::high_resolution_clock::now();
            fps = (int)(frame_count * 1000.0 / std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
            frame_count = 0;
            start = std::chrono::high_resolution_clock::now();
        }
        if (fps > 0) {
            track_intf.track_fps = fps;
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