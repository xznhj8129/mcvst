#pragma once
#include "globals.h"
#include "structs.h"
#include "class_settings.h"

class TrackData {
    private:
        int init_boxsize = 50;
        int guide_mem = settings.capFPS;

    public:
        bool setup = false;
        int movestep = 20;
        cv::Size framesize;
        double image_scale;
        cv::Rect roi;
        cv::Rect lastroi;
        cv::Point poi;
        int boxsize = init_boxsize;
        bool target_lock = false;
        bool locked = false;
        bool first_lock = true;
        bool lock_change = false;
        bool lost_lock = false;
        bool guiding = false;
        GuidanceVector guidance;
        VecArray angmem;
        VecArray velmem;
        TrackData();
        void Init(cv::Size cap_image_size, const double scale);
        cv::Point scaledPoi();
        cv::Rect scaledRoi();
        OFTData oftdata;
        void moveUp();
        void moveDown();
        void moveLeft();
        void moveRight();
        void biggerBox();
        void smallerBox();
        void update(cv::Point newtgt);
        void lock(const int x, const int y);
        void breaklock();
        void changeROI(int keyCode);
        void guide();

};
extern TrackData trackdata;