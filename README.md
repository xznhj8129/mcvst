# MCVST: Multimode Computer Vision Search and Track
Simple homebrewed object recognition and tracking for AI-enabled robotics using the Raspberry Pi or Nvidia Jetson, my second serious C++ project. \
Far from ready yet, lots of basic stuff to fix. Use settings file for now.

## Requirements:
libopencv-dev
sudo apt-get install nlohmann-json3-dev

## Usage:
Use cfg file: ./tracker --config=settings.cfg\
(command arguments coming back)\

### Capture:
* v4l2 
* gstreamer
* opencv (using opencv cap)
* udp

### Display:
* OpenCV window (click to track)
* Framebuffer for no-GUI Raspberry Pi 2
* gstreamer

### Input:
* Opencv window mouse/keyboard
* Keyboard in terminal
* FIFO
* socket
* serial

#### Input format: JSON
{"lock": 0, "reset": 0, "lr": 0.0, "ud": 0.0, "boxsize": 0, "shutdown": 0}\
{
    "lock": 0, 
    "reset": 0, 
    "lr": 0.0, 
    "ud": 0.0, 
    "boxsize": 0, 
    "shutdown": 0, 
    "extbox": [
        [markertype_int, "box text", [664, 335, 44, 55]],
        [markertype_int, "box text 2", [433, 946, 33, 22]]
    ]
}\

### Output:
* FIFO
* socket
* serial

#### Output format: JSON
{'locked': 0, 'tracking': [0.436, -0.1132], 'detections': [[3, 0.757324, [508, 946, 71, 78]], [3, 0.552246, [1770, 940, 76, 47]]}\


### Tracker types:
* Lucas-Kanade Sparse Optical Flow
* Kernelized Correlation Filter
* CSRT
* MOSSE

### Search types:
* YOLOv5
* YOLOv9

### OpenCV Window
Click: Lock\
Spacebar: Lock\
A: Track box bigger\
S: Track box smaller\
C: Reset lock\
Arrows: Move track\
 : Search Toggle\
 : Auto-lock Toggle\

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

## Testing:


## Parameters:
maxits – Maximum Iterations
This is the maximum number of iterations the algorithm will perform. In this case, the algorithm will run up to 10 iterations for each point or element it’s processing. If it hasn’t met the accuracy condition by then, it stops anyway.

epsilon – Epsilon (Accuracy Threshold)
This is the epsilon value, representing the desired accuracy. It’s a small number (0.03) that defines the minimum change required between iterations. If the change (e.g., in error or position) drops below 0.03, the algorithm considers it converged and stops, even if it hasn’t reached 10 iterations. In the context of optical flow, this might mean the tracked point’s position changes by less than 0.03 pixels (or another unit, depending on the algorithm’s setup).



## Features TODO:
* --zoombox= [topleft, topright, bottomleft, bottomright] (show zoomed target lock box)
- Search and autolock toggle inputs
- Tracking slew rate limiter
- Zoom + OFT zoom track
- Inset corner zoom picture
- Image stabilization
- Box display from external input (ie: from externally computed direction from GPS coordinates)
- Text display from external input 
- Capture: libcamera, rtsp, raspivid
- Display: rtsp, gstreamer, udp
- Input: serial
- Output: serial
- Tracker: mosse, canny, etc (or not)
- Search: differentiate yolov5, yolov8; CUDA; OpenCL; more algorithms
- return command line arguments

## Bugs TODO:
- fix [ WARN:0@8.460] global memory.hpp:67 operator() Device memory deallocation failed in deleter.
OpenCV(4.8.0-dev) /home/frog/Dev/opencv/modules/dnn/src/cuda4dnn/csl/memory.hpp:61: error: (-217:Gpu API call) driver shutting down in function 'operator()'
Exception will be ignored.
- Window/Display size set
- FIX THE ENTIRE SCALING FOR INPUTS
- Fix "bad lock"/far lock problem
- fix frame losses using libcamerify
- yolo:
    fix frame exchange handling
- fix segfaults on exit
- fix tracking threads not exiting on program end
- fix kcf/csrt (if at all necessary)
- test/fix frame jitter on high load
- cap size settings (auto/set/dynamic)
terminate called after throwing an instance of 'cv::Exception'
  what():  OpenCV(4.8.0-dev) /home/anon/Dev/opencv/modules/video/src/lkpyramid.cpp:1260: error: (-215:Assertion failed) (npoints = prevPtsMat.checkVector(2, CV_32F, true)) >= 0 in function 'calc'


## Improvements TODO:
- Use cxxopts
- frame size and scale mess, how functions get true capture frame size
- test latency/framerate
- synchronize thread workflow
- check threads for lag
- change globals (lazy) to pointers
- reduce compile times (interlinked)
- match terminal and config file arguments

