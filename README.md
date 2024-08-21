# MCVST: Multimode Computer Vision Search and Track
Simple homebrewed object recognition and tracking for AI-enabled robotics using the Raspberry Pi or Nvidia Jetson, my second serious C++ project. 

## Requirements:
libopencv-dev

## Usage:
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


## To do:
- Fix "bad lock" problem
- Finish reworking keyboard input (+ocv window)
- Tracking slew rate limiter
- Zoom + OFT zoom track
- Image stabilization
- Box display from external input (ie: from externally computed direction from GPS coordinates)
- Capture: libcamera, rtsp, raspivid
- Display: rtsp, gstreamer, socket?
- Input: socket, serial
- Output: serial
- Tracker: mosse, canny, etc
- Tracker params: https://docs.opencv.org/4.9.0/db/dd1/structcv_1_1TrackerKCF_1_1Params.html
- Search: differentiate yolov5, yolov8; CUDA; more algorithms
- frame size and scale mess, how functions get true capture frame size
- finish config file setup
- test latency/framerate
- synchronize thread workflow
- test/fix frame jitter on high load
- check threads for lag
- change globals (lazy) to pointers
- reduce compile times (interlinked)
- fix kcf/csrt
- match terminal and config file arguments

## Bugs:
- fix frame losses using libcamerify
- fix no video screen flashing on lost frames (soft drop vs timeout hard drop)
- yolo:
    fix frame exchange handling
- fix segfaults on exit
- fix error: (-215:Assertion failed) !_src.empty() in function 'cvtColor' (to do with novideo.png)
- fix tracking thread not exiting on program end