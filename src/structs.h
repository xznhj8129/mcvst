#pragma once
#include <vector>
#include <opencv2/opencv.hpp>

struct OFTData {
    std::vector<cv::Point2f> old_points;
};

struct Vec2D {
    float x;
    float y;
};

typedef std::vector<Vec2D> VecArray;

struct SearchDetection {
    int class_id;
    float confidence;
    cv::Rect box;
};

typedef std::vector<SearchDetection> SearchResults;

struct TrackInputs {
    bool valid;
    int lock;
    int unlock;
    float updown;
    float leftright;
    float boxsize;
    int exit;
};

struct FarnebackParams {
    double pyrScale = 0.75;   // Scale between pyramid levels (smaller -> more robust, but slower)
    int levels = 3;          // Number of pyramid layers
    int winSize = 15;        // Window size
    int iterations = 3;      // Number of iterations at each pyramid level
    int polyN = 5;           // Size of the pixel neighborhood for polynomial expansion
    double polySigma = 1.2;  // Std dev of the Gaussian used for polynomial expansion
    int flags = 0;           // Usually set to 0 or OPTFLOW_FARNEBACK_GAUSSIAN
};