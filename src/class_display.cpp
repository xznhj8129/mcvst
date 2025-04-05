#include "class_display.h"

DisplayInterface display_intf;

DisplayInterface::DisplayInterface() {
};

void DisplayInterface::Init(int displaytype) {
    if (settings.osdColor=="white") {osdcolor = cv::Scalar(255, 255, 255);}
    else if (settings.osdColor=="black") {osdcolor = cv::Scalar(0, 0, 0);}
    else if (settings.osdColor=="red") {osdcolor = cv::Scalar(0, 0, 255);}
    else if (settings.osdColor=="green") {osdcolor = cv::Scalar(0, 255, 0);}
    linesize = settings.osdLinesize;
    displayType = displaytype;
};

// Function to convert a single BGR pixel to BGR565
uint16_t DisplayInterface::convertBGRtoBGR565(const cv::Vec3b& pixel) {
    return ((pixel[2] >> 3) << 11) | ((pixel[1] >> 2) << 5) | (pixel[0] >> 3);
}

std::vector<uint32_t> DisplayInterface::convertBGRtoRGBA(const cv::Mat& bgrImage) {
    std::vector<uint32_t> rgbaImage(bgrImage.rows * bgrImage.cols);
    for (int y = 0; y < bgrImage.rows; ++y) {
        for (int x = 0; x < bgrImage.cols; ++x) {
            cv::Vec3b pixel = bgrImage.at<cv::Vec3b>(y, x);

            // Extract BGR components
            uint8_t b = pixel[0];
            uint8_t g = pixel[1];
            uint8_t r = pixel[2];
            uint8_t a = 0xFF; // Fully opaque

            // Combine into a single RGBA pixel
            uint32_t rgba = (a << 24) | (r << 16) | (g << 8) | b;

            rgbaImage[y * bgrImage.cols + x] = rgba;
        }
    }
    return rgbaImage;
}


void DisplayInterface::draw_cornerbox(cv::Mat& frame, cv::Point poi, int boxsize) {
    cv::Point topleft(poi.x - boxsize / 2, poi.y - boxsize / 2);
    cv::Point topright(poi.x + boxsize / 2, poi.y - boxsize / 2);
    cv::Point bottomleft(poi.x - boxsize / 2, poi.y + boxsize / 2);
    cv::Point bottomright(poi.x + boxsize / 2, poi.y + boxsize / 2);

    // Top-left corner
    cv::line(frame, topleft, topleft + cv::Point(boxsize / 4, 0), osdcolor, linesize); // Horizontal
    cv::line(frame, topleft, topleft + cv::Point(0, boxsize / 4), osdcolor, linesize); // Vertical

    // Top-right corner
    cv::line(frame, topright, topright - cv::Point(boxsize / 4, 0), osdcolor, linesize); // Horizontal
    cv::line(frame, topright, topright + cv::Point(0, boxsize / 4), osdcolor, linesize); // Vertical

    // Bottom-left corner
    cv::line(frame, bottomleft, bottomleft + cv::Point(boxsize / 4, 0), osdcolor, linesize); // Horizontal
    cv::line(frame, bottomleft, bottomleft - cv::Point(0, boxsize / 4), osdcolor, linesize); // Vertical

    // Bottom-right corner
    cv::line(frame, bottomright, bottomright - cv::Point(boxsize / 4, 0), osdcolor, linesize); // Horizontal
    cv::line(frame, bottomright, bottomright - cv::Point(0, boxsize / 4), osdcolor, linesize); // Vertical
}

void DisplayInterface::draw_cornerrect(cv::Mat& frame, const cv::Rect& roi) {
    // Calculate corner points based on rectangle dimensions
    const cv::Point tl(roi.x, roi.y);                     // Top-left
    const cv::Point tr(roi.x + roi.width, roi.y);         // Top-right
    const cv::Point bl(roi.x, roi.y + roi.height);        // Bottom-left
    const cv::Point br(roi.x + roi.width, roi.y + roi.height); // Bottom-right

    // Calculate corner line lengths as 25% of respective dimensions
    const int hLength = roi.width / 4;   // Horizontal line length
    const int vLength = roi.height / 4;  // Vertical line length

    // Draw top-left corner
    cv::line(frame, tl, tl + cv::Point(hLength, 0), osdcolor, linesize);  // Right
    cv::line(frame, tl, tl + cv::Point(0, vLength), osdcolor, linesize); // Down

    // Draw top-right corner
    cv::line(frame, tr, tr - cv::Point(hLength, 0), osdcolor, linesize); // Left
    cv::line(frame, tr, tr + cv::Point(0, vLength), osdcolor, linesize);  // Down

    // Draw bottom-left corner
    cv::line(frame, bl, bl + cv::Point(hLength, 0), osdcolor, linesize);  // Right
    cv::line(frame, bl, bl - cv::Point(0, vLength), osdcolor, linesize);  // Up

    // Draw bottom-right corner
    cv::line(frame, br, br - cv::Point(hLength, 0), osdcolor, linesize); // Left
    cv::line(frame, br, br - cv::Point(0, vLength), osdcolor, linesize); // Up
}

void DisplayInterface::draw_track(cv::Mat& frame) {
    //cv::Mat processed = frame.clone();
    cv::Rect scaledroi = track_intf.scaledRoi();
    cv::Point scaledpoi = track_intf.scaledPoi();
    cv::Size displaySize = settings.displaySize;

    if (settings.trackMarker == 1) {
        cv::rectangle(frame, scaledroi, osdcolor, linesize);
    }
    if (settings.trackMarker == 4) { //walleye
        cv::line(frame, cv::Point(scaledroi.x, 0),                      cv::Point(scaledroi.x, frame.rows), osdcolor, linesize); // top
        cv::line(frame, cv::Point(scaledroi.x + scaledroi.width, 0),    cv::Point(scaledroi.x + scaledroi.width, frame.rows), osdcolor, linesize); // top
        cv::line(frame, cv::Point(0, scaledroi.y),                      cv::Point(frame.cols, scaledroi.y), osdcolor, linesize); //left
        cv::line(frame, cv::Point(0, scaledroi.y + scaledroi.height),   cv::Point(frame.cols, scaledroi.y + scaledroi.height), osdcolor, linesize); //left

    }
    if (settings.trackMarker == 5 || settings.trackMarker == 6) { //maverick or lancet
        cv::line(frame, cv::Point(scaledpoi.x, 0),                          cv::Point(scaledpoi.x, scaledpoi.y - (track_intf.boxsize/2)), osdcolor, linesize); // top
        cv::line(frame, cv::Point(scaledpoi.x, track_intf.framesize.height), cv::Point(scaledpoi.x, scaledpoi.y + (track_intf.boxsize/2)), osdcolor, linesize); // down
        cv::line(frame, cv::Point(0, scaledpoi.y),                          cv::Point(scaledpoi.x - (track_intf.boxsize/2), scaledpoi.y), osdcolor, linesize); //left
        cv::line(frame, cv::Point(track_intf.framesize.width, scaledpoi.y),  cv::Point(scaledpoi.x + (track_intf.boxsize/2), scaledpoi.y), osdcolor, linesize); // right
    }   

    if (settings.trackMarker == 6 || settings.trackMarker == 3) { //lancet or corners
        draw_cornerbox(frame, scaledpoi, track_intf.boxsize);
    }

    if (settings.trackMarker == 6 ) { //lancet
        //center cross
        cv::line(frame, cv::Point(scaledpoi.x - (track_intf.boxsize/5) , scaledpoi.y), cv::Point(scaledpoi.x + (track_intf.boxsize / 5), scaledpoi.y), osdcolor, linesize);
        cv::line(frame, cv::Point(scaledpoi.x, scaledpoi.y - (track_intf.boxsize/5)), cv::Point(scaledpoi.x, scaledpoi.y + (track_intf.boxsize/5)), osdcolor, linesize);
    }

    if (settings.trackMarker == 2) { //crosshair
        int sizescale = 4;
        int scalew = track_intf.framesize.width/sizescale;
        int scaleh = track_intf.framesize.height/sizescale;
        cv::drawMarker(zoomed_frame, marker_pos, oscdolor, cv::MARKER_CROSS, scalew, linesize);
        /*
        int x1 = scaledpoi.x;
        int y1 = scaledpoi.y - (track_intf.boxsize/2) - (scalew / sizescale);
        int x2 = scaledpoi.x;
        int y2 = scaledpoi.y - (track_intf.boxsize/2);

        int x3 = scaledpoi.x;
        int y3 = scaledpoi.y + (track_intf.boxsize/2);
        int x4 = scaledpoi.x;
        int y4 = scaledpoi.y + (track_intf.boxsize/2) + (scalew / sizescale);

        int x5 = scaledpoi.x - (track_intf.boxsize/2) - (scalew / sizescale);
        int y5 = scaledpoi.y;
        int x6 = scaledpoi.x - (track_intf.boxsize/2);
        int y6 = scaledpoi.y;

        int x7 = scaledpoi.x + (track_intf.boxsize/2);
        int y7 = scaledpoi.y;
        int x8 = scaledpoi.x + (track_intf.boxsize/2) + (scalew / sizescale);
        int y8 = scaledpoi.y;

        cv::line(frame, cv::Point(x1, y1), cv::Point(x2, y2), osdcolor, linesize); // top
        cv::line(frame, cv::Point(x3, y3), cv::Point(x4, y4), osdcolor, linesize); // down
        cv::line(frame, cv::Point(x5, y5), cv::Point(x6, y6), osdcolor, linesize); //left
        cv::line(frame, cv::Point(x7, y7), cv::Point(x8, y8), osdcolor, linesize); // right*/
    
    }

    //indicators
    cv::putText(frame, "LOCK", cv::Point(10, track_intf.framesize.height - 25), cv::FONT_HERSHEY_SIMPLEX, 1, osdcolor, linesize);
}

void DisplayInterface::draw_externbox(cv::Mat& frame, cv::Point poi, int boxsize, std::string text) {
    //cv::Mat processed = frame.clone();
    cv::Rect boxroi = cv::Rect(poi.x - (boxsize / 2), poi.y - (boxsize / 2), boxsize, boxsize);
    cv::rectangle(frame, boxroi, osdcolor, linesize);
    cv::putText(frame, text, cv::Point(boxroi.x, boxroi.y + boxsize), cv::FONT_HERSHEY_SIMPLEX, 1, osdcolor, linesize);
}

void DisplayInterface::draw_search_detections(cv::Mat& frame, SearchResults results) {


}

void DisplayInterface::writeImageToFramebuffer(const cv::Mat& inputImage) {
    const char* fbDevicePath = "/dev/fb0";
    // Open the framebuffer device
    int fbFd = open(fbDevicePath, O_RDWR);
    if (fbFd == -1) {
        perror("Error: cannot open framebuffer device");
        return;
    }

    // Get variable screen information
    struct fb_var_screeninfo vinfo;
    if (ioctl(fbFd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        close(fbFd);
        return;
    }

    // Get fixed screen information for stride
    struct fb_fix_screeninfo finfo;
    if (ioctl(fbFd, FBIOGET_FSCREENINFO, &finfo)) {
        perror("Error reading fixed information");
        close(fbFd);
        return;
    }

    // Calculate the scaling factor while preserving aspect ratio
    double scaleHeight = vinfo.yres / static_cast<double>(inputImage.rows);
    double scaleWidth = vinfo.xres / static_cast<double>(inputImage.cols);
    double scale = std::min(scaleHeight, scaleWidth);

    // New dimensions
    int newWidth = static_cast<int>(inputImage.cols * scale);
    int newHeight = static_cast<int>(inputImage.rows * scale);

    // Resize image
    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight), 0, 0, cv::INTER_LINEAR);

    // Calculate centering position
    int xoffset = (vinfo.xres - newWidth) / 2;
    int yoffset = (vinfo.yres - newHeight) / 2;

    // Map the framebuffer to user memory
    size_t screensize = vinfo.yres_virtual * finfo.line_length;
    unsigned char* fbp = static_cast<unsigned char*>(mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbFd, 0));
    if (fbp == reinterpret_cast<unsigned char*>(-1)) {
        perror("Error: failed to map framebuffer device to memory");
        close(fbFd);
        return;
    }

    // Clear framebuffer (optional, to remove previous images/text)
    //memset(fbp, 0, screensize); // this causes flickering

    // Write the image centered in the framebuffer
    for (int y = 0; y < newHeight; ++y) {
        for (int x = 0; x < newWidth; ++x) {
            int fbIdx = (x + xoffset) + (y + yoffset) * vinfo.xres;
            //int imgIdx = x + y * newWidth; //unused
            uint16_t bgr565 = convertBGRtoBGR565(resizedImage.at<cv::Vec3b>(y, x));
            *reinterpret_cast<uint16_t*>(fbp + fbIdx * 2) = bgr565;  // 2 bytes per pixel
        }
    }

    // Cleanup
    munmap(fbp, screensize);
    close(fbFd);
}

void DisplayInterface::clearFramebuffer() {
    const char* fbDevicePath = "/dev/fb0";
    // Open the framebuffer device
    int fbFd = open(fbDevicePath, O_RDWR);
    if (fbFd == -1) {
        perror("Error: cannot open framebuffer device");
        return;
    }

    // Get variable screen information
    struct fb_var_screeninfo vinfo;
    if (ioctl(fbFd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        close(fbFd);
        return;
    }

    // Get fixed screen information for stride
    struct fb_fix_screeninfo finfo;
    if (ioctl(fbFd, FBIOGET_FSCREENINFO, &finfo)) {
        perror("Error reading fixed information");
        close(fbFd);
        return;
    }

    // Map the framebuffer to user memory
    size_t screensize = vinfo.yres_virtual * finfo.line_length;
    unsigned char* fbp = static_cast<unsigned char*>(mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbFd, 0));
    if (fbp == reinterpret_cast<unsigned char*>(-1)) {
        perror("Error: failed to map framebuffer device to memory");
        close(fbFd);
        return;
    }

    //clear framebuffer
    memset(fbp, 0, screensize); 

    // Cleanup
    munmap(fbp, screensize);
    close(fbFd);
}

void DisplayInterface::setTerminalMode(bool enable) {
    static struct termios oldt, newt;
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
}