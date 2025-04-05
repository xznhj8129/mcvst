#pragma once
#include <string>
#include <opencv2/opencv.hpp>
#include <cstdlib>
#include <libconfig.h++>
#include "globals.h"

class SettingsClass {
    public:
        bool debug_print;
        std::string cfgFile;
        int captureType;
        std::string capturePath;
        std::string capFormat;
        cv::Size capSize;
        cv::Size displaySize;
        int inputType;
        std::string inputPath;
        double processScale;
        int displayType;
        std::string displayPath;
        int searchType;
        int outputType;
        std::string outputPath;
        int socketport;
        bool use_cuda;
        bool use_eis;
        bool record_output;
        std::string recordPath;

        int trackerType;
        int trackMarker;
        int init_boxsize;
        int oftpoints;
        bool oftfeatures;
        int oft_winsize;
        int oft_pyrlevels;

        int trackingFPS;
        int displayFPS;
        int searchFPS;
        int capFPS;
        bool showFPS;

        std::string osdColor;
        int osdLinesize;
        bool showPipper;

        int capWB;
        int capBrightness;
        int capContrast;
        int capSat;

        bool search_dnn_setup = false;
        bool search_auto_lock;
        int search_target_class;
        double search_target_conf;
        double search_score_threshold;
        double search_nms_threshold;
        double search_confidence_threshold;
        double search_yolo_width;
        double search_yolo_height;
        std::string search_dnn_model;
        std::string search_dnn_model_classes;
        int search_dnn_model_dim;
        int search_dnn_model_rows;
        bool search_limit_zone;
        double search_zone[4];

        int movestep;
        int test_frame;
        int test_boxsize;
        int test_x;
        int test_y;

        SettingsClass();
        void Init(int argc, char** argv);
};

extern SettingsClass settings;