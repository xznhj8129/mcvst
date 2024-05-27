## MCVST: Multimode Computer Vision Search and Track
WIP

Requirements:
libopencv-dev

Usage:\
--capture= [file, v4l2, gstreamer]\
--capturepath= [video.mp4, dev/videoX, "gstreamer pipeline", "udp://rtspstream:9000"]\
--display= [none, window, framebuffer]\
--tracker= [none, kcf, csrt, oft...]\
--trackmarker = [box (default), crosshair, corners, maverick, walleye, lancet]\
--pipper\
--search= [none, yolo...]\
--input= [default, socket, serial, fifo]\
--inputpath= [socket port, /file/path, /dev/ttySX]\
--output= [none, socket, serial, fifo]\
--outputpath= [/dev/ttySX, socketport, /file/path]\
--record=[filename]\
--showfps\

To do:
- Inputs: libcamera, rtsp, raspivid
- Display: rtsp, gstreamer
- Tracker: mosse, canny, etc
- Tracker params: https://docs.opencv.org/4.9.0/db/dd1/structcv_1_1TrackerKCF_1_1Params.html
- frame size and scale mess, how functions get true capture frame size
- fix kcf/csrt
- yolo:
    fix frame exchange handling
- fix segfaults on exit
- fix error: (-215:Assertion failed) !_src.empty() in function 'cvtColor' (to do with novideo.png)
- finish config file setup
- rework keyboard input (+ocv window)
- test latency/framerate
- synchronize thread workflow
- test/fix frame jitter on high load
- check threads for lag
- change globals (lazy) to pointers
- reduce compile times (interlinked)