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
    //int update_frames = 0; //////////
    //int updc = 0;
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
        termcrit = cv::TermCriteria(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, settings.maxits, settings.epsilon); //<configurable>

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

    int track_lost_counter;
    int max_lost_search = 3;

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
            
            //std::cout << track_intf.track << " " << track_intf.locking << " " << track_intf.locked << std::endl;
            if (track_intf.track) {    
                cv::Mat frame_gray;
                cv::cvtColor(processframe, frame_gray, cv::COLOR_BGR2GRAY);

                // If old_points is empty (e.g., first frame), initialize with points around POI
                if (track_intf.oftdata.old_points.empty()) {
                    track_intf.oftdata.old_points = track_intf.roiPoints(); 
                }

                std::vector<cv::Point2f> new_points;
                std::vector<uchar> status;
                std::vector<float> errors;

                cv::calcOpticalFlowPyrLK(
                    frame_gray_init, 
                    frame_gray, 
                    track_intf.oftdata.old_points, 
                    new_points, 
                    status, 
                    errors, 
                    cv::Size(track_intf.oft_winsize, track_intf.oft_winsize), //15
                    track_intf.oft_pyrlevels,                // Pyramid levels 2
                    termcrit          // Predefined: (COUNT | EPS, 10, 0.03)
                );

                // Filter valid points within ROI
                std::vector<cv::Point2f> valid_points;
                for (size_t i = 0; i < new_points.size(); ++i) {
                    if (status[i] && track_intf.isPointInROI(new_points[i], track_intf.point_tolerance)) {
                        valid_points.push_back(new_points[i]);
                    }
                }

                if (valid_points.size() > 0) {
                    // Calculate median of valid points for new POI
                    std::sort(valid_points.begin(), valid_points.end(), 
                            [](const cv::Point2f& a, const cv::Point2f& b) { return a.x < b.x; });
                    float medianX = valid_points[valid_points.size() / 2].x;
                    std::sort(valid_points.begin(), valid_points.end(), 
                            [](const cv::Point2f& a, const cv::Point2f& b) { return a.y < b.y; });
                    float medianY = valid_points[valid_points.size() / 2].y;
                    cv::Point2f new_poi(medianX, medianY);

                    // Update POI and ROI
                    track_intf.poi = cv::Point(static_cast<int>(new_poi.x), static_cast<int>(new_poi.y)); // Convert back to cv::Point
                    track_intf.defineRoi(new_poi);
                    track_intf.oftdata.old_points = valid_points; // Update for next iteration
                    track_intf.locked = true;
                    track_intf.locking = false;
                    track_lost_counter = 0;

                } else {
                    track_lost_counter++;
                    if (settings.debug_print) {
                        std::cout << "Single-point OFT lost track: no valid points " << track_lost_counter << std::endl;
                    }

                    if (settings.inputType==4){
                        track_intf.breaklock(); //immediately end if track breaks in testing
                    } else if (max_lost_search> -1 && (track_lost_counter >= max_lost_search)) {
                        track_intf.breaklock();
                        std::cout << "OFT lost track after " << max_lost_search << " frames" << std::endl;
                    } else {
                        new_points.clear();
                        new_points.push_back(track_intf.poi);
                    }
                }
                if (!track_intf.track) { track_intf.locked = false;}
                frame_gray_init = frame_gray.clone();
            }
            else { }
        }
        else if (settings.trackerType == 2 || settings.trackerType == 3) { // KCF/CSRT branch
            if ((track_intf.track & (!track_intf.locked || track_intf.first_lock) || 
                (track_intf.locked && track_intf.lock_change))) {
                if (settings.trackerType == 2) {
                    tracker = cv::TrackerKCF::create();
                } else if (settings.trackerType == 3) {
                    tracker = cv::TrackerCSRT::create();
                }
                cv::Rect initBox = track_intf.roi;
                tracker->init(processframe, initBox);
                track_intf.locked = true;
                track_intf.locking = false;
                track_intf.first_lock = false;
                track_intf.lock_change = false;
            }

            if (track_intf.locked && track_intf.roi.width!=0 && track_intf.roi.height!=0) {
                track_intf.lastroi = track_intf.roi;

                cv::Rect trackingBox = track_intf.roi;
                ok = tracker->update(processframe, trackingBox);
                if (ok) {
                    track_intf.roi = trackingBox;
                    track_intf.poi.x = track_intf.roi.x + (track_intf.roi.width / 2);
                    track_intf.poi.y = track_intf.roi.y + (track_intf.roi.height / 2);
                } else {
                    std::cout << "Lost track" << std::endl;
                    track_intf.lost_lock = true;
                    track_intf.locked = false;
                    track_intf.locking = true; // Request re-lock next frame.
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
            int border_tolerance = 5;
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