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
    else if (settings.trackerType == 3) {}

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

        // Tracking algorithms below
        
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
            
            if (settings.oftfeatures) { // Multiple OFT features to track (drifts)

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

            else {  // single-pixel OFT tracking

                if (track_intf.target_lock) {      
                    std::vector<cv::Point2f> new_points;
                    std::vector<uchar> status;
                    std::vector<float> errors;
                    float tolerance = 1.5;
                    
                    cv::calcOpticalFlowPyrLK(frame_gray_init, frame_gray, track_intf.oftdata.old_points, new_points, status, errors, cv::Size(15, 15), 2, termcrit); //<configurable> oft variables

                    std::cout << 
                    track_intf.target_lock << " " << 
                    track_intf.locked << 
                    " points:" << new_points.size() << 
                    " poi: " << new_points[0] << 
                    " roi: " << track_intf.roi << 
                    " old_points " << track_intf.oftdata.old_points <<
                    std::endl;

                    if (!track_intf.isPointInROI(new_points[0], tolerance)) {
                        std::cout << "bad lock" << std::endl;
                        //track_intf.target_lock = false;
                        //track_intf.locked = false;
                        new_points.clear();
                        new_points.push_back(track_intf.poi);
                    }
                    track_intf.oftdata.old_points = new_points;
                    track_intf.poi = new_points[0];

                    track_intf.lastroi = track_intf.roi;
                    track_intf.roi.x = track_intf.poi.x - (track_intf.boxsize / 2);
                    track_intf.roi.y = track_intf.poi.y - (track_intf.boxsize / 2);
                    track_intf.roi.width = track_intf.boxsize;
                    track_intf.roi.height = track_intf.boxsize;
                    if (track_intf.target_lock) {track_intf.locked = true;}
                    frame_gray_init = frame_gray.clone();
                }
            }
                
        }
        else if (settings.trackerType == 2 || settings.trackerType == 3) { // KCF/CSRT branch
            // Reinitialize tracker if a re-lock is requested or if we’re not already tracking.
            if ((track_intf.target_lock & (!track_intf.locked || track_intf.first_lock) || 
                (track_intf.locked && track_intf.lock_change))) {
                if (settings.trackerType == 2) {
                    tracker = cv::TrackerKCF::create();
                } else if (settings.trackerType == 3) {
                    tracker = cv::TrackerCSRT::create();
                }
                // Create a named ROI variable from our existing roi (an lvalue)
                cv::Rect initBox = track_intf.roi;
                tracker->init(processframe, initBox);
                track_intf.locked = true;
                track_intf.target_lock = false;
                track_intf.first_lock = false;
                track_intf.lock_change = false;
            }

            if (track_intf.locked && track_intf.roi.width!=0 && track_intf.roi.height!=0) {
                // Save the last ROI before updating.
                track_intf.lastroi = track_intf.roi;

                // Declare a named variable for the ROI update.
                cv::Rect trackingBox = track_intf.roi;
                ok = tracker->update(processframe, trackingBox);
                if (ok) {
                    // Update the internal ROI from the updated trackingBox.
                    track_intf.roi = trackingBox;
                    // Update the tracked point (poi) as the center of the ROI.
                    track_intf.poi.x = track_intf.roi.x + (track_intf.roi.width / 2);
                    track_intf.poi.y = track_intf.roi.y + (track_intf.roi.height / 2);
                } else {
                    std::cout << "Lost track" << std::endl;
                    track_intf.lost_lock = true;
                    track_intf.locked = false;
                    track_intf.target_lock = true; // Request re-lock next frame.
                }
            }
        }

        else if (settings.trackerType == 6) { // dense optical flow using ROI
            cv::Mat curGray;
            cv::cvtColor(processframe, curGray, cv::COLOR_BGR2GRAY);
            if (prevGray.empty()) {
                prevGray = curGray.clone();
            }

            // Ensure the ROI is within image bounds.
            cv::Rect roiRect = track_intf.roi & cv::Rect(0, 0, curGray.cols, curGray.rows);
            if (roiRect.width <= 0 || roiRect.height <= 0) {
                std::cerr << "Invalid ROI\n";
                prevGray = curGray.clone();
                break; // or continue, depending on your logic
            }

            // Create a mask that highlights the ROI.
            cv::Mat mask = cv::Mat::zeros(curGray.size(), CV_8UC1);
            mask(roiRect) = 255;

            // Compute dense optical flow over the full image.
            cv::Mat flow;
            cv::calcOpticalFlowFarneback(
                prevGray,
                curGray,
                flow,
                fbParams.pyrScale,
                fbParams.levels,
                fbParams.winSize,
                fbParams.iterations,
                fbParams.polyN,
                fbParams.polySigma,
                fbParams.flags
            );

            // Collect point matches only from inside the ROI.
            std::vector<cv::Point2f> srcPts;
            std::vector<cv::Point2f> dstPts;
            const int step = 8; // sampling stride
            for (int y = roiRect.y; y < roiRect.y + roiRect.height; y += step) {
                for (int x = roiRect.x; x < roiRect.x + roiRect.width; x += step) {
                    cv::Vec2f vec = flow.at<cv::Vec2f>(y, x);
                    float flowX = vec[0];
                    float flowY = vec[1];
                    cv::Point2f srcPt(static_cast<float>(x), static_cast<float>(y));
                    cv::Point2f dstPt = srcPt + cv::Point2f(flowX, flowY);
                    srcPts.push_back(srcPt);
                    dstPts.push_back(dstPt);
                }
            }

            if (srcPts.size() < 10) {
                std::cerr << "Not enough flow points in ROI to estimate motion!\n";
                // You might choose to mark a failure to lock or simply update prevGray and try again.
                prevGray = curGray.clone();
            }
            else {
                // Estimate an affine transform from the matched points.
                cv::Mat affine = cv::estimateAffinePartial2D(srcPts, dstPts);
                if (affine.empty()) {
                    std::cerr << "Failed to estimate global affine transform.\n";
                }
                else {
                    // Decompose the affine to get rotation, scale, and translation.
                    double rotationDeg = 0.0, scale = 1.0;
                    cv::Point2f translation(0.f, 0.f);
                    track_intf.decomposeAffine(affine, rotationDeg, scale, translation);

                    // Update the tracked point using the affine transform.
                    cv::Point2f trackedPoint = track_intf.poi;
                    double a11 = affine.at<double>(0,0);
                    double a12 = affine.at<double>(0,1);
                    double a13 = affine.at<double>(0,2);
                    double a21 = affine.at<double>(1,0);
                    double a22 = affine.at<double>(1,1);
                    double a23 = affine.at<double>(1,2);
                    double newX = a11 * trackedPoint.x + a12 * trackedPoint.y + a13;
                    double newY = a21 * trackedPoint.x + a22 * trackedPoint.y + a23;
                    trackedPoint = cv::Point2f(static_cast<float>(newX), static_cast<float>(newY));

                    std::cout << "Global Rotation (deg): " << rotationDeg << "\n";
                    std::cout << "Global Scale:          " << scale       << "\n";
                    std::cout << "Global Translation:    (" << translation.x << ", " << translation.y << ")\n";
                    std::cout << "Updated Tracked Point: (" << trackedPoint.x << ", " << trackedPoint.y << ")\n";

                    track_intf.poi = trackedPoint;
                }
            }
            cv::imshow("denseFlow: curGray", curGray);
            cv::imshow("denseFlow: ROI mask", mask);
            prevGray = curGray.clone();
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