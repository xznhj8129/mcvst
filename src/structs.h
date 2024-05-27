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

struct GuidanceVector {
    Vec2D angle;
    Vec2D angvel;
    Vec2D angaccel;
};

struct SearchDetection {
    int class_id;
    float confidence;
    cv::Rect box;
};

typedef std::vector<SearchDetection> SearchResults;