#pragma once
#include "globals.h"
#include "classes.h"
#include <opencv2/tracking.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <deque>
#include <numeric>


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
    //int update_frames = 0; //////////
    //int updc = 0;
    bool ok = false;
    int fps;
    int total_frames;

    //tracker pointers
    cv::Ptr<cv::Tracker> tracker;

    FarnebackParams fbParams;

    //oft specific
    cv::Mat prev_frame;
    cv::Mat prevGray; //////////////////
    cv::Mat prev_raw;
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
        prev_raw = processframe.clone();
        cv::cvtColor(processframe, prev_frame, cv::COLOR_BGR2GRAY);
        //cv::goodFeaturesToTrack(prev_frame, corners, 100, 0.3, 7); //<configurable>
        termcrit = cv::TermCriteria(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, settings.maxits, settings.epsilon); //<configurable>

    } 
    else if (settings.trackerType == 2) {}
    else if (settings.trackerType == 3) {}

    else if (settings.trackerType == 6) {
        /* Initialize Farneback parameters
        fbParams.pyrScale   = 0.75;   // typical 0.5 - 0.8
        fbParams.levels     = 3;     // more levels for large motions
        fbParams.winSize    = 15;    // increase for smoother flow
        fbParams.iterations = 3;     
        fbParams.polyN      = 5;     
        fbParams.polySigma  = 1.2;   
        fbParams.flags      = 0;     // or OPTFLOW_FARNEBACK_GAUSSIAN, etc.
        */
    }
    else {
        std::cerr << "Invalid tracker type selected" << std::endl;
        finish_error = true;
    }
    track_intf.poi = cv::Point(cap_intf.frameSize.width/2, cap_intf.frameSize.height/2);
    track_intf.roi.width = track_intf.boxsize;
    track_intf.roi.height = track_intf.boxsize;

    track_intf.roi = cv::Rect(
        track_intf.poi.x - (track_intf.boxsize / 2), 
        track_intf.poi.y - (track_intf.boxsize / 2), 
        track_intf.boxsize, 
        track_intf.boxsize);

    int track_lost_counter = 0;
    int max_lost_search;
    if (track_intf.fps > 0) {
        max_lost_search = track_intf.fps;}
    else {
        max_lost_search = settings.capFPS;}
    int border_tolerance = 10;

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<cv::Point2f> new_points;
    
    while (global_running and !finish_error) {
        frame_count++;
        total_frames++;
        auto startTime = std::chrono::steady_clock::now();
        bool passthrough = false;

        std::unique_lock<std::mutex> lock(sharedData.frameMutex);
        sharedData.frameCondVar.wait(lock, [&sharedData] { return sharedData.hasNewFrame.load(); });
        cv::Mat frame = sharedData.trackFrame.clone();
        lock.unlock();

        cv::Mat resized;
        processframe = cv::Mat();
        if (track_intf.image_scale!=1.0) {cv::resize(frame, resized, process_size);}
        else {resized = frame.clone();}
        // Convert to grayscale & optional blur
        cv::cvtColor(resized, processframe, cv::COLOR_BGR2GRAY);

        if (settings.blur_size > 0) {
            cv::GaussianBlur(processframe, processframe,
                            cv::Size(settings.blur_size, settings.blur_size), 0);
        }

        // Tracking algorithms below
        
        if (settings.trackerType == 0 || cap_intf.no_feed) { 
           passthrough = true;
        }
        else if (settings.trackerType >= 1) { // OFT (only this for now)

            if (track_intf.track) { // & !anomaly

                // Initialize points on first frame
                if (track_intf.oftdata.old_points.empty()) {
                    track_intf.oftdata.old_points = track_intf.roiPoints(); 
                }

                // Compute optical flow
                std::vector<cv::Point2f> new_points;
                std::vector<uchar>      status;
                std::vector<float>      errors;
                cv::calcOpticalFlowPyrLK(
                    prev_frame,
                    processframe,
                    track_intf.oftdata.old_points,
                    new_points,
                    status,
                    errors,
                    cv::Size(track_intf.oft_winsize, track_intf.oft_winsize),
                    track_intf.oft_pyrlevels,
                    termcrit
                );

                // Filter valid points
                std::vector<cv::Point2f> valid_points;
                for (size_t i = 0; i < new_points.size(); ++i) {
                    if (status[i] &&
                        track_intf.isPointInROI(new_points[i], track_intf.point_tolerance)) {
                        valid_points.push_back(new_points[i]);
                    }
                }

                if (!valid_points.empty()) {
                    
                    // compute median-based new POI as before
                    std::nth_element(valid_points.begin(),
                                    valid_points.begin() + valid_points.size()/2,
                                    valid_points.end(),
                                    [](auto&a,auto&b){ return a.x < b.x; });
                    float medianX = valid_points[valid_points.size()/2].x;
                    std::nth_element(valid_points.begin(),
                                    valid_points.begin() + valid_points.size()/2,
                                    valid_points.end(),
                                    [](auto&a,auto&b){ return a.y < b.y; });
                    float medianY = valid_points[valid_points.size()/2].y;
                    cv::Point2f new_poi(medianX, medianY);

                    track_intf.poi = new_poi;
                    track_intf.defineRoi(new_poi);
                    track_intf.oftdata.old_points = valid_points;
                    track_intf.locked   = true;
                    track_intf.locking  = false;
                    track_lost_counter  = 0;
                }

                else {
                    // No valid points: count loss, maybe break lock or reseed
                    track_lost_counter++;
                    if (settings.debug_print) {
                        std::cout << "OFT lost points: "
                                << track_lost_counter << std::endl;
                    }
                    else if (max_lost_search > -1 && track_lost_counter >= max_lost_search) {
                        track_intf.breaklock();
                        std::cout << "OFT lost track after " << max_lost_search
                                << " frames" << std::endl;
                        track_lost_counter = 0;
                        if (settings.inputType == 4) {
                            track_intf.breaklock();
                        }
                    }

                }

                if (!track_intf.track) {
                    track_intf.locked = false;
                }
                prev_frame = processframe.clone();
            }
        }


        if (track_intf.locked) {
            //std::cout << track_intf.poi << " " << cap_intf.frameSize << std::endl;
            cv::Point scaled = track_intf.scaledPoi();
            track_intf.angle.x = (((double)scaled.x / track_intf.framesize.width) - 0.5) * 2;
            track_intf.angle.y = (((double)scaled.y / track_intf.framesize.height) - 0.5) * 2;
            if( track_intf.poi.x < border_tolerance || 
                track_intf.poi.x > (cap_intf.frameSize.width * track_intf.image_scale)-border_tolerance || 
                track_intf.poi.y < border_tolerance || 
                track_intf.poi.y > (cap_intf.frameSize.height * track_intf.image_scale)-border_tolerance) {
                    std::cout << "Track left bounds" << std::endl;
                    track_intf.lost_lock = true;
                    track_intf.locked = false;
                    track_intf.first_lock = true;
                    track_intf.clearlock();
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
        if (settings.trackingFPS>0 && processtime > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleepTime);
        }
    }

    if (global_debug_print) {std::cout << "tracking thread finished" << std::endl;}
    global_running.store(false);
    if (!finish_error) {return 0;}
    else {return 1;}
}