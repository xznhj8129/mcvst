#include "class_track.h"

TrackData trackdata;

TrackData::TrackData() {}

void TrackData::Init(cv::Size cap_image_size, double scale) {
    //std::cout << "init imagesize " << cap_image_size << std::endl;

    if ((cap_image_size.height != 0) & (cap_image_size.width != 0)) {
        setup = true;
    }
    framesize = cap_image_size;
    image_scale = scale;
    boxsize = init_boxsize * scale;
}

cv::Rect TrackData::scaledRoi() {
    return cv::Rect(roi.x / image_scale, roi.y / image_scale, roi.width / image_scale, roi.height / image_scale);
}

cv::Point TrackData::scaledPoi() {
    return cv::Point(poi.x / image_scale, poi.y / image_scale);
}

void TrackData::moveUp() {
    poi.y = std::max(0.f, static_cast<float>(poi.y - movestep)); 
    lock_change = true;
}

void TrackData::moveDown() {
    poi.y = std::min(static_cast<float>(poi.y + movestep), static_cast<float>(framesize.height - 1)); 
    lock_change = true;
}

void TrackData::moveLeft() {
    poi.x = std::max(0.f, static_cast<float>(poi.x - movestep)); 
    lock_change = true;
}

void TrackData::moveRight() {
    poi.x = std::min(static_cast<float>(poi.x + movestep), static_cast<float>(framesize.width - 1)); 
    lock_change = true;
}

void TrackData::biggerBox() {
    boxsize = std::max(20, boxsize - 10);
    roi.width = boxsize;
    roi.height = boxsize;
}

void TrackData::TrackData::smallerBox() {
    boxsize = std::min(200, boxsize += 10);
    roi.width = boxsize;
    roi.height = boxsize;
}

void TrackData::update(cv::Point newtgt) {
    poi = newtgt;
    oftdata.old_points.clear();
    oftdata.old_points.push_back(poi);
    //target_lock = true;
    lastroi = roi;
    roi = cv::Rect(poi.x - (roi.width / 2), poi.y - (roi.height / 2), roi.width, roi.height);
}

void TrackData::lock(const int x, const int y) {
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

void TrackData::breaklock() {}

void TrackData::changeROI(int keyCode) { //used in window, mmal...
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
        case 32: //space
            lock(framesize.width/2, framesize.height/2);
            break;
        case 99: //c
            target_lock = false;
            break;
    }
    if (change) {
        update(poi);
        locked = false;
    }
}

void TrackData::guide() {
    cv::Point scaled = scaledPoi();
    guidance.angle.x = (((double)scaled.x / framesize.width) - 0.5) * 2;;
    guidance.angle.y = (((double)scaled.y / framesize.height) - 0.5) * 2;;
    //azimuth = (((double)center.x / frame_width) - 0.5) * 2;
    //elevation = (((double)center.y / frame_height) - 0.5) * 2;

    //velocity not implemented yet
    if (!guiding and target_lock) {
        guiding = true;
        guidance.angvel.x = 0;
        guidance.angvel.y = 0;
        guidance.angaccel.x = 0;
        guidance.angaccel.y = 0;
    }
    else if (guiding and !target_lock) {
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