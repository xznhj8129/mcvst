#pragma once
#include "globals.h"
#include "structs.h"
#include "class_settings.h"

class TrackInterface {
        int guide_mem = settings.capFPS;

    public:
        bool setup = false;
        int movestep;
        cv::Size framesize;
        double image_scale;
        cv::Rect roi;
        cv::Rect lastroi;
        cv::Point lastpoi;
        cv::Point poi;
        int boxsize;

        bool track = false;
        bool locking = false;
        bool locked = false;

        bool first_lock = true;
        bool lock_change = false;
        bool lost_lock = false;
        int fps;
        float track_fps;

        Vec2D angle;
        VecArray angmem;
        VecArray velmem;

        int oft_winsize;
        int oft_pyrlevels;
        int maxits;
        double epsilon;
        double points_offset;
        double point_tolerance = 0.5f;
        double track_srl = 10.0f;

        TrackInterface();
        void Init(cv::Size cap_image_size, const double scale);

        cv::Point scaledPoi();
        cv::Rect scaledRoi();
        OFTData oftdata;
        void moveVertical(float v);
        void moveHorizontal(float h);
        void moveUp();
        void moveDown();
        void moveLeft();
        void moveRight();
        void biggerBox();
        void smallerBox();
        void resizeBox(int newsize);
        void update(cv::Point newtgt);
        void lock(const int x, const int y);
        void breaklock();
        void clearlock();
        void defineRoi(cv::Point2f poi);
        void changeROI(int keyCode);
        bool isPointInROI(const cv::Point2f& point, float tolerance);
        std::vector<cv::Point2f> roiPoints();
        void decomposeAffine(const cv::Mat& affine, 
                            double& rotationDeg, 
                            double& scale, 
                            cv::Point2f& translation);
        void denseFlowGlobalMotion(
            const cv::Mat& prevGray,
            const cv::Mat& curGray,
            const FarnebackParams& fbParams,
            cv::Point2f& trackedPoint,  // in/out: point we want to track via global motion
            double& outRotationDeg,
            double& outScale,
            cv::Point2f& outTranslation
        );
        float getPointsOffset();

};
extern TrackInterface track_intf;