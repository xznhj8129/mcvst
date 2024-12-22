#pragma once
#include <fstream>
#include <opencv2/opencv.hpp>
#include "structs.h"
#include "globals.h"
#include "class_track.h"
#include "class_capture.h"
#include "class_settings.h"
#include <cxxopts.hpp>

class SearchInterface {
    private:

    public:
        std::vector<cv::Scalar> colors;
        float yolo_INPUT_WIDTH;
        float yolo_INPUT_HEIGHT;
        float yolo_SCORE_THRESHOLD;
        float yolo_NMS_THRESHOLD;
        float yolo_CONFIDENCE_THRESHOLD;
        std::string searchType;
        cv::Size procSize;
        cv::dnn::Net net;
        std::vector<std::string> class_list;
        bool is_cuda;
        bool setup;
        std::vector<SearchDetection> output;
        float search_fps;

        SearchInterface();
        void Init();
        //bool InitYOLO(bool cuda, std::string net, std::string classfile);
        std::vector<std::string> load_class_list();
        void load_net(cv::dnn::Net &net, bool is_cuda);
        cv::Mat format_yolov5(const cv::Mat &source);
        void detect(cv::Mat &image, SearchResults &output);

};
extern SearchInterface search_intf;