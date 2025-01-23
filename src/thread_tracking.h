#pragma once
#include "globals.h"
#include "classes.h"
#include <opencv2/tracking.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>


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
    int update_frames = 0; //////////
    int updc = 0;
    bool ok = false;
    int fps;
    int total_frames;

    //tracker pointers
    cv::Ptr<cv::Tracker> tracker;

    FarnebackParams fbParams;

    //oft specific
    cv::Mat frame_gray_init;
    cv::Mat prevGray; //////////////////
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

    else if (settings.trackerType == 6) {
        // Initialize Farneback parameters
        fbParams.pyrScale   = 0.75;   // typical 0.5 - 0.8
        fbParams.levels     = 3;     // more levels for large motions
        fbParams.winSize    = 15;    // increase for smoother flow
        fbParams.iterations = 3;     
        fbParams.polyN      = 5;     
        fbParams.polySigma  = 1.2;   
        fbParams.flags      = 0;     // or OPTFLOW_FARNEBACK_GAUSSIAN, etc.
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

        processframe = cv::Mat();
        if (track_intf.image_scale!=1.0) {cv::resize(frame, processframe, process_size);}
        else {processframe = frame.clone();}
        
        if (settings.trackerType == 0 || cap_intf.no_feed) { // 
           passthrough = true;
        }
        else if (settings.trackerType == 1) { // OFT
            //std::cout << "track loop " << updc << std::endl;
            cv::Mat frame_gray;
            cv::cvtColor(processframe, frame_gray, cv::COLOR_BGR2GRAY);

            /*cv::Mat mask = cv::Mat::zeros(frame_gray.size(), CV_8UC1);
            mask(track_intf.roi) = 255;
            // 	image, maxCorners, qualityLevel, minDistance, mask, blockSize, gradientSize[, corners[, useHarrisDetector[, k]]]
            cv::goodFeaturesToTrack(frame_gray, track_intf.oftdata.old_points, 
                100, //maxcorners
                0.2, //qualitylevel
                7,   //mindistance
                mask
            ); // <configurable> oft variables
            */
            
            if (settings.oftfeatures) {

                if (track_intf.target_lock) {      
                    
                    cv::Mat mask = cv::Mat::zeros(frame_gray.size(), CV_8UC1);
                    mask(track_intf.roi) = 255;
                    std::vector<cv::Point2f> points;
                    cv::goodFeaturesToTrack(frame_gray, points, 
                    100, //maxcorners
                    0.3, //qualitylevel
                    3,   //mindistance
                    mask
                    ); // <configurable> oft variables
                    if (points.size() > 0) {track_intf.oftdata.old_points = points;}

                    std::vector<cv::Point2f> new_points;
                    std::vector<uchar> status;
                    std::vector<float> errors;

                    float tolerance = 1.0;
                    int pyrlevel = 2;
                    cv::calcOpticalFlowPyrLK(
                        frame_gray_init,
                        frame_gray, 
                        track_intf.oftdata.old_points, 
                        new_points, 
                        status, 
                        errors, 
                        cv::Size(15,15), //track_intf.boxsize, track_intf.boxsize), //winsize
                        pyrlevel, //maximal pyramid level number
                        termcrit); 


                    // Filter out corners that have status=1
                    std::vector<cv::Point2f> validPoints;
                    for (int i=0; i<new_points.size(); i++) {
                        std::cout << new_points[i] << "-> " << track_intf.roi << std::endl;
                        if (status[i]  && track_intf.isPointInROI(new_points[i], tolerance)) {
                            validPoints.push_back(new_points[i]);
                        }
                        else {std::cout << "invalid" << std::endl;}
                    }
                
                    // Only update if the track is good
                    std::cout << track_intf.target_lock << " points:" << validPoints.size() << "/" << new_points.size() << " locked:" << track_intf.locked << " poi " << track_intf.poi << " roi " << track_intf.roi << std::endl;
                    if (validPoints.size() > 0) { //if (track_intf.isPointInROI(new_points[0], tolerance)) {

                        std::sort(validPoints.begin(), validPoints.end(), 
                                [](auto &a, auto &b){return a.x < b.x;});
                        float medianX = validPoints[validPoints.size()/2].x;

                        std::sort(validPoints.begin(), validPoints.end(), 
                                [](auto &a, auto &b){return a.y < b.y;});
                        float medianY = validPoints[validPoints.size()/2].y;
                        cv::Point2f newCenter(medianX, medianY);
        
                        track_intf.poi = newCenter;
                        track_intf.defineRoi(newCenter);
                        if (settings.oftfeatures) {
                            track_intf.oftdata.old_points = validPoints; 
                            }
                        else {
                            track_intf.oftdata.old_points = track_intf.roiPoints(); 
                        }

                        if (track_intf.target_lock) {track_intf.locked = true;}
                    }
                    else {
                        std::cout << "bad lock " << new_points[0] << " validpoints:" << validPoints.size() << " " << track_intf.roi <<std::endl;
                        //track_intf.breaklock();
                        track_intf.target_lock = false;
                        track_intf.locked = false;
                    }

                    frame_gray_init = frame_gray.clone(); 
                }
            }
            else {  // normal 


                if (track_intf.target_lock) {      
                    std::vector<cv::Point2f> new_points;
                    std::vector<uchar> status;
                    std::vector<float> errors;
                    float tolerance = 1.5;
                    
                    cv::calcOpticalFlowPyrLK(frame_gray_init, frame_gray, track_intf.oftdata.old_points, new_points, status, errors, cv::Size(15, 15), 2, termcrit); //<configurable> oft variables
                    std::cout << track_intf.target_lock << " " << track_intf.locked << " points:" << new_points.size() << "poi" << track_intf.poi << " roi " << track_intf.roi << std::endl;
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
                        std::cout << "bad lock" << std::endl;
                        track_intf.target_lock = false;
                        track_intf.locked = false;
                    }
                }
            }
                
        }
        else if (settings.trackerType == 2 || settings.trackerType == 3) { // CRST/KCF unfinished, implementation sucks
            //std::cout << "target_lock " << track_intf.target_lock << std::endl;
            //std::cout << "locked " << track_intf.locked << std::endl;
            //std::cout << "first_lock " << track_intf.first_lock << std::endl;
            
            if ((track_intf.target_lock & (!track_intf.locked || track_intf.first_lock) || 
                (track_intf.locked && track_intf.lock_change))) {
                //std::cout << "kcf1" << std::endl;

                //integrate parameter customization: https://docs.opencv.org/4.9.0/db/dd1/structcv_1_1TrackerKCF_1_1Params.html
                if (settings.trackerType == 2) {
                    cv::TrackerKCF::Params x;
                    tracker = cv::TrackerKCF::create();
                    }
                else if (settings.trackerType == 3) {
                    tracker = cv::TrackerCSRT::create();
                    }

                tracker->init(processframe, track_intf.roi);
                //ok = tracker->update(processframe, track_intf.roi); //True means that target was located and false means that tracker cannot locate target in current frame. Note, that latter does not imply that tracker has failed, maybe target is indeed missing from the frame (say, out of sight)
                //if (ok) {
                    track_intf.locked = true;
                    track_intf.target_lock = false;
                    track_intf.first_lock = false;
                    track_intf.lock_change = false;
                //}
            }

            if (track_intf.locked && track_intf.roi.width!=0 && track_intf.roi.height!=0) {
                std::cout << "kcf2" << " " << track_intf.poi << track_intf.roi << std::endl;
                track_intf.lastroi = track_intf.roi;
                track_intf.poi.x = track_intf.roi.x + (track_intf.boxsize/2);
                track_intf.poi.y = track_intf.roi.y + (track_intf.boxsize/2);
                ok = tracker->update(processframe, track_intf.roi);

                if (!ok) {
                    std::cout << "Lost track" << std::endl;
                    track_intf.lost_lock = true;
                    track_intf.locked = false;
                    track_intf.target_lock = true; //re-lock
                    }
            }
        }

        else if (settings.trackerType == 6) { // dense flow (dnu)
                if (track_intf.target_lock) {      
                    //std::cout << "lock loop " << updc << std::endl;  
                    cv::Mat curGray;
                    cv::cvtColor(processframe, curGray, cv::COLOR_BGR2GRAY);
                    if (prevGray.empty()) {
                        prevGray = curGray.clone();
                    }
                    
                    updc += 1;
                    // this isn't actually used?
                    /*if (updc > update_frames) {
                        cv::goodFeaturesToTrack(frame_gray, corners, 100, 0.3, 7); // <configurable> oft variables
                        updc = 0;
                    }*/
                    // Suppose you have two consecutive grayscale frames
                    //cv::Mat prevGray = cv::imread("frame0.png", cv::IMREAD_GRAYSCALE);
                    if (prevGray.empty() || curGray.empty()) {
                        std::cerr << "Could not load test images.\n";
                    }

                    // A tracked point we want to follow. 
                    // In a real app, you might set this from a mouse click or ROI center:
                    cv::Point2f trackedPoint;
                    trackedPoint.x = track_intf.poi.x;
                    trackedPoint.y = track_intf.poi.y;

                    // Outputs: global rotation, scale, translation
                    double rotationDeg  = 0.0;
                    double scale        = 1.0;
                    cv::Point2f transl(0.f, 0.f);

                    // Run the dense-flow-based global motion estimation
                    track_intf.denseFlowGlobalMotion(
                        prevGray,
                        curGray,
                        fbParams,
                        trackedPoint,  // in/out
                        rotationDeg,
                        scale,
                        transl
                    );

                    // Now 'trackedPoint' is updated. Also you have 'rotationDeg', 'scale', 'transl'
                    std::cout << "Global Rotation (deg): " << rotationDeg << "\n";
                    std::cout << "Global Scale:          " << scale       << "\n";
                    std::cout << "Global Translation:    (" << transl.x << ", " << transl.y << ")\n";
                    std::cout << "Updated Tracked Point: (" << trackedPoint.x << ", " << trackedPoint.y << ")\n";
                    track_intf.poi.x = trackedPoint.x;
                    track_intf.poi.y = trackedPoint.y;
                    cv::imshow("test1",curGray);
                    cv::imshow("test2",prevGray);
                    prevGray = curGray.clone();
                }
        }

        if (track_intf.locked) {
            //std::cout << track_intf.poi << " " << cap_intf.frameSize << std::endl;
            cv::Point scaled = track_intf.scaledPoi();
            track_intf.angle.x = (((double)scaled.x / track_intf.framesize.width) - 0.5) * 2;
            track_intf.angle.y = (((double)scaled.y / track_intf.framesize.height) - 0.5) * 2;
            if( track_intf.poi.x < 0 || 
                track_intf.poi.x > (cap_intf.frameSize.width * track_intf.image_scale) || 
                track_intf.poi.y < 0 || 
                track_intf.poi.y > (cap_intf.frameSize.height * track_intf.image_scale)) {
                    std::cout << "Track left bounds" << std::endl;
                    track_intf.lost_lock = true;
                    track_intf.locked = false;
                    track_intf.first_lock = true;
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