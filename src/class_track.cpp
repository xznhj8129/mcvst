#include "class_track.h"

TrackInterface track_intf;

TrackInterface::TrackInterface() {}

void TrackInterface::Init(cv::Size cap_image_size, double scale) {
    //std::cout << "init imagesize " << cap_image_size << std::endl;

    if ((cap_image_size.height != 0) && (cap_image_size.width != 0)) {
        setup = true;
    }
    framesize = cap_image_size;
    image_scale = scale;
    boxsize = settings.init_boxsize * scale;
    movestep = settings.movestep;
    oft_pyrlevels = settings.oft_pyrlevels;
    oft_winsize = settings.oft_winsize;
    fps = settings.trackingFPS;
    maxits = settings.maxits;
    epsilon = settings.maxits;
    track = false;
    
    points_offset = 5.0f;
    point_tolerance = 0.5f;

}

cv::Rect TrackInterface::scaledRoi() {
    return cv::Rect(roi.x / image_scale, roi.y / image_scale, roi.width / image_scale, roi.height / image_scale);
}

cv::Point TrackInterface::scaledPoi() {
    return cv::Point(poi.x / image_scale, poi.y / image_scale);
}

void TrackInterface::moveVertical(float v) {
    int mv;
    if (locked) {mv = movestep / 2;}
    else {mv = movestep;}
    int move = v * mv;
    poi.y =  std::min(std::max(0.f,static_cast<float>(poi.y - move)), static_cast<float>(framesize.height - 1)); 
    roi = cv::Rect(poi.x - (roi.width / 2), poi.y - (roi.height / 2), roi.width, roi.height);
    lock_change = true;
    update(poi);
}

void TrackInterface::moveHorizontal(float h) {
    int mv;
    if (locked) {mv = movestep / 2;}
    else {mv = movestep;}
    int move = h * mv;
    poi.x = std::min(std::max(0.f, static_cast<float>(poi.x + move)), static_cast<float>(framesize.width - 1)); 
    roi = cv::Rect(poi.x - (roi.width / 2), poi.y - (roi.height / 2), roi.width, roi.height);
    lock_change = true;
    update(poi);
}

void TrackInterface::moveUp() {
    int mv;
    if (locked) {mv = movestep / 2;}
    else {mv = movestep;}
    poi.y = std::max(0.f, static_cast<float>(poi.y - mv)); 
    roi = cv::Rect(poi.x - (roi.width / 2), poi.y - (roi.height / 2), roi.width, roi.height);
    lock_change = true;
    update(poi);
}

void TrackInterface::moveDown() {
    int mv;
    if (locked) {mv = movestep / 2;}
    else {mv = movestep;}
    poi.y = std::min(static_cast<float>(poi.y + mv), static_cast<float>(framesize.height - 1)); 
    roi = cv::Rect(poi.x - (roi.width / 2), poi.y - (roi.height / 2), roi.width, roi.height);
    lock_change = true;
    update(poi);
}

void TrackInterface::moveLeft() {
    int mv;
    if (locked) {mv = movestep / 2;}
    else {mv = movestep;}
    poi.x = std::max(0.f, static_cast<float>(poi.x - mv)); 
    roi = cv::Rect(poi.x - (roi.width / 2), poi.y - (roi.height / 2), roi.width, roi.height);
    lock_change = true;
    update(poi);
}

void TrackInterface::moveRight() {
    int mv;
    if (locked) {mv = movestep / 2;}
    else {mv = movestep;}
    poi.x = std::min(static_cast<float>(poi.x + mv), static_cast<float>(framesize.width - 1)); 
    roi = cv::Rect(poi.x - (roi.width / 2), poi.y - (roi.height / 2), roi.width, roi.height);
    lock_change = true;
    update(poi);
}

void TrackInterface::biggerBox() {
    boxsize = std::max(20, boxsize - 10);
    roi.width = boxsize;
    roi.height = boxsize;
    lock_change = true;
    update(poi);
}

void TrackInterface::smallerBox() {
    boxsize = std::min(200, boxsize += 10);
    roi.width = boxsize;
    roi.height = boxsize;
    lock_change = true;
    update(poi);
}

std::vector<cv::Point2f> TrackInterface::roiPoints() {
    float offset = getPointsOffset();
    if (settings.oftpoints<=1) { return { poi }; }
    else {
        cv::Point2f c1 = cv::Point2f(poi.x - offset, poi.y - offset); 
        cv::Point2f c2 = cv::Point2f(poi.x + offset, poi.y - offset); 
        cv::Point2f c3 = cv::Point2f(poi.x - offset, poi.y + offset); 
        cv::Point2f c4 = cv::Point2f(poi.x + offset, poi.y + offset); 
        cv::Point2f c5 = cv::Point2f(poi.x , poi.y - offset); 
        cv::Point2f c6 = cv::Point2f(poi.x , poi.y + offset); 
        cv::Point2f c7 = cv::Point2f(poi.x - offset, poi.y ); 
        cv::Point2f c8 = cv::Point2f(poi.x + offset, poi.y ); 
        if (settings.oftpoints == 4) { return { c1, c2, c3, c4 }; }
        else if (settings.oftpoints == 5) { return { poi, c5, c6, c7, c8 }; }
        else if (settings.oftpoints == 9) { return { poi, c1, c2, c3, c4, c5, c6, c7, c8 }; }
    }
}

float TrackInterface::getPointsOffset() {
    return std::round(boxsize / 10.0f); // 2.0f
}

void TrackInterface::defineRoi(cv::Point2f newpoi) {
    lastroi = roi;
    cv::Rect newroi;
    newroi.x = newpoi.x - (boxsize / 2);
    newroi.y = newpoi.y - (boxsize / 2);
    newroi.width = boxsize;
    newroi.height = boxsize;
    newroi.x = std::clamp(newroi.x, boxsize, framesize.width - boxsize);
    newroi.y = std::clamp(newroi.y, boxsize, framesize.height - boxsize);
    roi = newroi;
}

bool TrackInterface::isPointInROI(const cv::Point2f& point, float tolerance) {
    return (point.x >= (track_intf.poi.x - (boxsize * tolerance)) && 
            point.x < (track_intf.poi.x + (boxsize * tolerance)) &&
            point.y >= (track_intf.poi.y - (boxsize * tolerance)) &&
            point.y < (track_intf.poi.y + (boxsize * tolerance)));
}

void TrackInterface::update(cv::Point newtgt) {
    poi = newtgt;
    if (!settings.oftfeatures) {
        oftdata.old_points.clear();
        oftdata.old_points = roiPoints();
    }
    lastroi = roi;
    roi = cv::Rect(poi.x - (roi.width / 2), poi.y - (roi.height / 2), roi.width, roi.height);
}

void TrackInterface::lock(const int x, const int y) {

    poi = cv::Point(x, y) * image_scale;
    if ((roi.width > 0) && (roi.height > 0)) {
        roi = cv::Rect(
            (x * image_scale) - (roi.width / 2), 
            (y * image_scale) - (roi.height / 2), 
            roi.width, 
            roi.height);
    } else {
        roi = cv::Rect(
            (x * image_scale) - (boxsize / 2),
            (y * image_scale) - (boxsize / 2), 
            boxsize, 
            boxsize); 
    }


    std::cout << 
    "lock at frame " <<
    framecounter <<  
    " poi " << poi << 
    " image scale: " << image_scale << 
    " boxsize: " << boxsize <<
    std::endl;

    lastroi = roi;
    track = true;
    locking = true;
    locked = false;
    lost_lock = false;
    oftdata.old_points.clear();
    if (!settings.oftfeatures) {
        oftdata.old_points = roiPoints();
    }
}

void TrackInterface::breaklock() {
    if (settings.debug_print) {
        std::cout << "Break lock" << std::endl;
    }
    track = false;
    locking = false;
    locked = false;
    first_lock = true;
    oftdata.old_points.clear();
}

void TrackInterface::clearlock() {
    if (settings.debug_print) {
        std::cout << "Clear lock" << std::endl;
    }
    poi = cv::Point(track_intf.framesize.width/2, track_intf.framesize.height/2);
    roi = cv::Rect(poi.x - (roi.width / 2), poi.y - (roi.height / 2), roi.width, roi.height);
    track = false;
    locking = false;
    locked = false;
    first_lock = true;
    oftdata.old_points.clear();
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
            locking = false;
            locked = false;
            break;
    }
    if (change) {
        update(poi);
        locked = false;
    }
}


// Helper to decompose the 2×3 affine matrix into rotation (degrees), scale, and translation
void TrackInterface::decomposeAffine(const cv::Mat& affine, 
                            double& rotationDeg, 
                            double& scale, 
                            cv::Point2f& translation)
{
    // Affine is 2x3: [a11 a12 a13]
    //               [a21 a22 a23]
    // Where [a13, a23] is translation
    // The linear part is [a11 a12; a21 a22]
    
    double a11 = affine.at<double>(0,0);
    double a12 = affine.at<double>(0,1);
    double a21 = affine.at<double>(1,0);
    double a22 = affine.at<double>(1,1);

    // Translation
    translation.x = static_cast<float>(affine.at<double>(0,2));
    translation.y = static_cast<float>(affine.at<double>(1,2));

    // Scale is average of the two scale factors from the 2x2 submatrix
    double scaleX = std::sqrt(a11*a11 + a21*a21);
    double scaleY = std::sqrt(a12*a12 + a22*a22);
    scale = (scaleX + scaleY) * 0.5;

    // Rotation in radians from the first column of the linear part
    double angleRad = std::atan2(a21, a11);
    rotationDeg = angleRad * 180.0 / CV_PI;
}

// Perform dense optical flow, then compute a global affine transform, and update a tracked point
void TrackInterface::denseFlowGlobalMotion(
    const cv::Mat& prevGray,
    const cv::Mat& curGray,
    const FarnebackParams& fbParams,
    cv::Point2f& trackedPoint,  // in/out: point we want to track via global motion
    double& outRotationDeg,
    double& outScale,
    cv::Point2f& outTranslation
) {
    CV_Assert(!prevGray.empty() && !curGray.empty() && 
              prevGray.size() == curGray.size() &&
              prevGray.type() == CV_8UC1 &&
              curGray.type() == CV_8UC1);

    // 1. Compute dense optical flow (Farneback)
    // flow will be a 2-channel image (CV_32FC2): flow(x,y) = (flow_x, flow_y)
    cv::Mat flow;
    cv::calcOpticalFlowFarneback(
        prevGray, 
        curGray,
        flow,
        fbParams.pyrScale,
        fbParams.levels,
        fbParams.winSize,
        fbParams.iterations,
        fbParams.polyN,
        fbParams.polySigma,
        fbParams.flags
    );

    // 2. Collect point matches (src -> dst) for global motion estimation.
    //    We’ll sample flow at a grid step to reduce overhead.
    std::vector<cv::Point2f> srcPts;
    std::vector<cv::Point2f> dstPts;

    const int step = 8; // sampling stride (tune for speed vs. accuracy)
    for (int y = 0; y < flow.rows; y += step) {
        for (int x = 0; x < flow.cols; x += step) {
            cv::Vec2f vec = flow.at<cv::Vec2f>(y, x); // (flow_x, flow_y)
            float flowX = vec[0];
            float flowY = vec[1];

            cv::Point2f srcPt(x, y);
            cv::Point2f dstPt(x + flowX, y + flowY);

            // (Optional) Filter out huge flows or invalid flows if needed
            // e.g., if (std::abs(flowX) > 100 || std::abs(flowY) > 100) continue;

            srcPts.push_back(srcPt);
            dstPts.push_back(dstPt);
        }
    }

    if (srcPts.size() < 10) {
        // Not enough points for a robust transform
        std::cerr << "Not enough flow points to estimate global motion!\n";
        return;
    }

    // 3. Estimate an affine transform (no shear) from srcPts -> dstPts
    //    estimateAffinePartial2D => translation, rotation, uniform scale (and optionally reflection)
    cv::Mat affine = cv::estimateAffinePartial2D(srcPts, dstPts);
    if (affine.empty()) {
        std::cerr << "Failed to estimate global affine transform.\n";
        return;
    }

    // 4. Decompose the affine to get rotation (deg), scale, translation
    decomposeAffine(affine, outRotationDeg, outScale, outTranslation);

    // 5. Update the trackedPoint by applying the affine transform
    //    [x']   [a11  a12  a13] [x]
    //    [y'] = [a21  a22  a23] [y]
    // So x' = a11*x + a12*y + a13
    //    y' = a21*x + a22*y + a23
    //
    // We'll convert the Mat to double to be safe.
    double a11 = affine.at<double>(0,0);
    double a12 = affine.at<double>(0,1);
    double a13 = affine.at<double>(0,2);
    double a21 = affine.at<double>(1,0);
    double a22 = affine.at<double>(1,1);
    double a23 = affine.at<double>(1,2);

    double newX = a11 * trackedPoint.x + a12 * trackedPoint.y + a13;
    double newY = a21 * trackedPoint.x + a22 * trackedPoint.y + a23;
    trackedPoint = cv::Point2f(static_cast<float>(newX), static_cast<float>(newY));
}