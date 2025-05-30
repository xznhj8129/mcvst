#pragma once
#include <string>
#include <termios.h>    // For file operations
#include <fcntl.h>    // For file operations
#include <linux/fb.h> // For framebuffer
#include <sys/ioctl.h> // For ioctl
#include <sys/mman.h> // For memory mapping
#include <unistd.h>  // For close()
#include "globals.h"
#include "class_track.h"
#include "class_settings.h"
#include "class_search.h"

class DisplayInterface {

    private:
        uint16_t convertBGRtoBGR565(const cv::Vec3b& pixel);
        std::vector<uint32_t> convertBGRtoRGBA(const cv::Mat& bgrImage);

    public:
        cv::Mat displayFrame;
        std::string displayType;
        cv::Scalar osdcolor;
        int linesize;

        DisplayInterface();
        void Init(int displaytype);

        // move this somewhere else
        void draw_track(cv::Mat& frame);
        void draw_externbox(cv::Mat& frame, cv::Point poi, int boxsize, std::string text);
        void draw_search_detections(cv::Mat& frame);
        void draw_cornerbox(cv::Mat& frame, cv::Point poi, int boxsize);
        void draw_cornerrect(cv::Mat& frame, const cv::Rect& roi);

        void clearFramebuffer();
        void writeImageToFramebuffer(const cv::Mat& inputImage);
        void setTerminalMode(bool enable);
};

extern DisplayInterface display_intf;