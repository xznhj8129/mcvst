#include "class_capture.h"

CaptureInterface cap_intf;

CaptureInterface::CaptureInterface() {};

CaptureInterface::~CaptureInterface() {
    video.release(); //???
};

void CaptureInterface::Init(int captureType, std::string capturePath, cv::Size capSize) {
    captype = captureType;
    got_video = false;
    no_feed = false;
    bool valid = false;
    setup = false;
    cv::Mat novideoimage = cv::imread("novideo.png", cv::IMREAD_COLOR);
    cv::cvtColor(novideoimage, novideoimage, cv::COLOR_BGRA2BGR);

    if (captype == 1) { //v4l2
        cv::VideoCapture cap(capturePath, cv::CAP_V4L2);
            
        if (!cap.isOpened()) {
            std::cerr << "V4L2 Error: Couldn't open capture." << std::endl;
            global_running.store(false);
        } else {
            //https://docs.opencv.org/4.9.0/d4/d15/group__videoio__flags__base.html
            cap.set(cv::CAP_PROP_FRAME_WIDTH, capSize.width);
            cap.set(cv::CAP_PROP_FRAME_HEIGHT, capSize.height);
            cap.set(cv::CAP_PROP_FPS, settings.capFPS);
            cap.set(cv::CAP_PROP_AUTO_WB, settings.capWB); //white balance
            cap.set(cv::CAP_PROP_BRIGHTNESS, settings.capBrightness); //night: 90
            cap.set(cv::CAP_PROP_CONTRAST, settings.capContrast); //night: 80
            cap.set(cv::CAP_PROP_SATURATION, settings.capSat); //night: 20
            //cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('B', 'G', 'R', '3'));
            cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc(settings.capFormat[0],settings.capFormat[1],settings.capFormat[2],settings.capFormat[3]));
            std::cout << "Cap size: " << capSize.width << "x" << capSize.height << std::endl;
            std::cout<< "Cap FPS: " << settings.capFPS << std::endl;
            std::cout << "Cap format: " << settings.capFormat[0] << settings.capFormat[1] << settings.capFormat[2] << settings.capFormat[3] << std::endl;
            video = cap;
            valid = true;
        }
    } 
    else if (captype == 2) { //gstreamer
        cv::VideoCapture cap(capturePath, cv::CAP_GSTREAMER);
        if (!cap.isOpened()) {
            std::cerr << "Gstreamer cap Error: Couldn't open capture." << std::endl;
            global_running.store(false);
        } else {
            video = cap;
            valid = true;
        }
    }
    else if (captype == 3 || captype == 4 ) { //file or rtsp (opencv)
        cv::VideoCapture cap(capturePath);
        if (!cap.isOpened()) {
            std::cerr << "OpenCV Cap Error: Couldn't open capture." << std::endl;
            global_running.store(false);
        } else {
            video = cap;
            valid = true;
        }
    } 
    else {
        std::cerr << "Invalid capture type selected" <<  std::endl;
        global_running.store(false);
    }


    if (valid) {
        frameSize.width = video.get(cv::CAP_PROP_FRAME_WIDTH);
        frameSize.height = video.get(cv::CAP_PROP_FRAME_HEIGHT);
        std::cout << "Capture open " << frameSize << std::endl;
        cv::resize(novideoimage, novideo, cv::Size(frameSize.width, frameSize.height), 0, 0, cv::INTER_LINEAR);
        setup = true;
    } else {
        std::cerr << "Unable to setup Capture Interface" << std::endl;
        cv::resize(novideoimage, novideo, cv::Size(640, 480), 0, 0, cv::INTER_LINEAR); //fallback
        global_running.store(false);
    }
}

void CaptureInterface::capFrame(SharedData& sharedData) { // capture frame
    //deprecated
}

/*cv::Mat CaptureInterface::getFrame() { // return frame from read
    bool gotframe = false;
    cv::Mat frame;
    while (!gotframe and global_running) {
        if (!cap_frame.empty()) {
            //std::lock_guard<std::mutex> lock(capFrameMutex);
            frame = cap_frame.clone();
            //std::swap(cap_frame, frame);
        }
        if (frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else {
            gotframe = true;
            return frame;
        }
    }
    return frame;

}*/