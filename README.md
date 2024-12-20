# MCVST: Multimode Computer Vision Search and Track
Simple homebrewed object recognition and tracking for AI-enabled robotics using the Raspberry Pi or Nvidia Jetson, my second serious C++ project. 

## Requirements:
libopencv-dev

## Usage:
### OpenCV Window
Click: Lock
Spacebar: Lock
A: Track box bigger
S: Track box smaller
C: Reset lock
Arrows: Move track
 : Search Toggle
 : Auto-lock Toggle

## Arguments:
--config=settings.cfg (get program settings from file instead of long terminal command)\
--capture= [file, v4l2, gstreamer]\
--capturepath= [video.mp4, dev/videoX, "gstreamer pipeline", "udp://rtspstream:9000"]\
--capfps= int\
--capsize= auto or 640x480, 1280x720, etc\
--display= [none, window, framebuffer]\
--displayfps= int\
--tracker= [none, kcf, csrt, oft...]\
--trackmarker = [box (default), crosshair, corners, maverick, walleye, lancet]\
--pipper\
--scale float (1 = same size as capture, 2 = 50% size)\
--search= [none, yolo...]\
--autolock\
--input= [default, socket, serial, fifo]\
--inputpath= [socket port, /file/path, /dev/ttySX]\
--output= [none, socket, serial, fifo]\
--outputpath= [/dev/ttySX, socketport, /file/path]\
--record=[filename]\
--showfps
--dnn_model=
--dnn_model_classes=
--dnn_model_tim
--target_class=
--target_conf=
--auto-lock
--score_threshold

## Features TODO:
--zoombox= [topleft, topright, bottomleft, bottomright] (show zoomed target lock box)\
- Search and autolock toggle inputs
- remove hardcoded fifo paths
- Tracking slew rate limiter
- Zoom + OFT zoom track
- Inset corner zoom picture
- Image stabilization
- Box display from external input (ie: from externally computed direction from GPS coordinates)
- Capture: libcamera, rtsp, raspivid
- Display: rtsp, gstreamer, udp
- Input: socket, serial
- Output: serial
- Tracker: mosse, canny, etc
- Tracker params: https://docs.opencv.org/4.9.0/db/dd1/structcv_1_1TrackerKCF_1_1Params.html
- Search: differentiate yolov5, yolov8; CUDA; more algorithms
- finish config file setup

## Bugs TODO:
- Window/Display size set
- FIX THE ENTIRE SCALING FOR INPUTS
- Fix "bad lock"/far lock problem
- fix frame losses using libcamerify
- fix no video screen flashing on lost frames (soft drop vs timeout hard drop)
- yolo:
    fix frame exchange handling
- fix segfaults on exit
- fix error: (-215:Assertion failed) !_src.empty() in function 'cvtColor' (to do with novideo.png)
- fix tracking thread not exiting on program end
- fix kcf/csrt (if at all necessary)
- test/fix frame jitter on high load
- cap size settings (auto/set/dynamic)

## Improvements TODO:
- Use cxxopts
- Finish reworking keyboard input (+ocv window), remove changeROI
- frame size and scale mess, how functions get true capture frame size
- test latency/framerate
- synchronize thread workflow
- check threads for lag
- change globals (lazy) to pointers
- reduce compile times (interlinked)
- match terminal and config file arguments

