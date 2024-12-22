#include "class_track.h"

TrackInterface track_intf;

TrackInterface::TrackInterface() {}

void TrackInterface::Init(cv::Size cap_image_size, double scale) {
    //std::cout << "init imagesize " << cap_image_size << std::endl;

    if ((cap_image_size.height != 0) & (cap_image_size.width != 0)) {
        setup = true;
    }
    framesize = cap_image_size;
    image_scale = scale;
    boxsize = init_boxsize * scale;
}

cv::Rect TrackInterface::scaledRoi() {
    return cv::Rect(roi.x / image_scale, roi.y / image_scale, roi.width / image_scale, roi.height / image_scale);
}

cv::Point TrackInterface::scaledPoi() {
    return cv::Point(poi.x / image_scale, poi.y / image_scale);
}

void TrackInterface::moveVertical(float v) {
    int move = v * movestep;
    poi.y =  std::min(std::max(0.f,static_cast<float>(poi.y - move)), static_cast<float>(framesize.height - 1)); 
    roi = cv::Rect(poi.x - (roi.width / 2), poi.y - (roi.height / 2), roi.width, roi.height);
    lock_change = true;
    update(poi);
}

void TrackInterface::moveHorizontal(float h) {
    int move = h * movestep;
    poi.x = std::min(std::max(0.f, static_cast<float>(poi.x + move)), static_cast<float>(framesize.width - 1)); 
    roi = cv::Rect(poi.x - (roi.width / 2), poi.y - (roi.height / 2), roi.width, roi.height);
    lock_change = true;
    update(poi);
}

void TrackInterface::moveUp() {
    poi.y = std::max(0.f, static_cast<float>(poi.y - movestep)); 
    roi = cv::Rect(poi.x - (roi.width / 2), poi.y - (roi.height / 2), roi.width, roi.height);
    lock_change = true;
    update(poi);
}

void TrackInterface::moveDown() {
    poi.y = std::min(static_cast<float>(poi.y + movestep), static_cast<float>(framesize.height - 1)); 
    roi = cv::Rect(poi.x - (roi.width / 2), poi.y - (roi.height / 2), roi.width, roi.height);
    lock_change = true;
    update(poi);
}

void TrackInterface::moveLeft() {
    poi.x = std::max(0.f, static_cast<float>(poi.x - movestep)); 
    roi = cv::Rect(poi.x - (roi.width / 2), poi.y - (roi.height / 2), roi.width, roi.height);
    lock_change = true;
    update(poi);
}

void TrackInterface::moveRight() {
    poi.x = std::min(static_cast<float>(poi.x + movestep), static_cast<float>(framesize.width - 1)); 
    roi = cv::Rect(poi.x - (roi.width / 2), poi.y - (roi.height / 2), roi.width, roi.height);
    lock_change = true;
    update(poi);
}

void TrackInterface::biggerBox() {
    boxsize = std::max(20, boxsize - 10);
    roi.width = boxsize;
    roi.height = boxsize;
    update(poi);
}

void TrackInterface::smallerBox() {
    boxsize = std::min(200, boxsize += 10);
    roi.width = boxsize;
    roi.height = boxsize;
    update(poi);
}

void TrackInterface::update(cv::Point newtgt) {
    poi = newtgt;
    oftdata.old_points.clear();
    oftdata.old_points.push_back(poi);
    //target_lock = true;
    lastroi = roi;
    roi = cv::Rect(poi.x - (roi.width / 2), poi.y - (roi.height / 2), roi.width, roi.height);
}

void TrackInterface::lock(const int x, const int y) {
    poi = cv::Point(x, y) * image_scale;
    if ((roi.width > 0) & (roi.height > 0)) {
        roi = cv::Rect((x * image_scale) - (roi.width / 2), (y * image_scale) - (roi.height / 2), roi.width, roi.height);
    } else {
        roi = cv::Rect((x * image_scale) - (boxsize / 2), (y * image_scale) - (boxsize / 2), boxsize, boxsize); 
    }
    lastroi = roi;
    target_lock = true;
    locked = false;
    lost_lock = false;
    oftdata.old_points.clear();
    oftdata.old_points.push_back(poi);
}

void TrackInterface::breaklock() {
    poi = cv::Point(track_intf.framesize.width/2, track_intf.framesize.height/2);
    roi = cv::Rect(poi.x - (roi.width / 2), poi.y - (roi.height / 2), roi.width, roi.height);
    target_lock = false;
    locked = false;
}

void TrackInterface::changeROI(int keyCode) { //used in window, mmal...
    bool change = false;
    switch (keyCode) {
        case 82: //arrow keys
            moveUp();
            change = true;
            break; // Move up
        case 84: 
            moveDown();
            change = true;
            break; // Move down
        case 81:
            moveLeft();
            change = true;
            break; // Move left
        case 83: 
            moveRight();
            change = true;
            break; // Move right
        case 115: // a
            biggerBox();
            change = true;
            break;
        case 97: // s
            smallerBox();
            change = true;
            break;
        case 99: //c
            target_lock = false;
            locked = false;
            break;
    }
    if (change) {
        update(poi);
        locked = false;
    }
}

bool TrackInterface::isPointInROI(const cv::Point2f& point, float tolerance) {
    return (point.x >= (poi.x - (boxsize * tolerance)) && 
            point.x < (poi.x + (boxsize * tolerance)) &&
            point.y >= (poi.y - (boxsize * tolerance)) &&
            point.y < (poi.y + (boxsize * tolerance)));
}

void TrackInterface::guide() {
    cv::Point scaled = scaledPoi();
    guidance.angle.x = (((double)scaled.x / framesize.width) - 0.5) * 2;;
    guidance.angle.y = (((double)scaled.y / framesize.height) - 0.5) * 2;;
    //azimuth = (((double)center.x / frame_width) - 0.5) * 2;
    //elevation = (((double)center.y / frame_height) - 0.5) * 2;

    //velocity not implemented yet
    if (!guiding and locked) {
        guiding = true;
        guidance.angvel.x = 0;
        guidance.angvel.y = 0;
        guidance.angaccel.x = 0;
        guidance.angaccel.y = 0;
    }
    else if (guiding and !locked) {
        guiding = false;
    }
    else {
        guidance.angvel.x = 0;
        guidance.angvel.y = 0;
        guidance.angaccel.x = 0;
        guidance.angaccel.y = 0;
        angmem.clear();
        velmem.clear();
    }
}