#include "class_settings.h"

SettingsClass settings;

SettingsClass::SettingsClass() {};

void SettingsClass::Init(int argc, char** argv) {
    std::string dtype;
    std::string captype;
    std::string tracktype;
    std::string searchtype;
    std::string outtype;
    std::string intype;
    std::string markertype;
    std::string color;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if ((arg.find("--help") == 0)|| (arg.find("--h") == 0)) {
            
            global_running.store(false);
            exit(0);
        }

        if (arg.find("--config=") == 0) {
            cfgFile = arg.substr(9);
            libconfig::Config config;
            try{
                const char * c = cfgFile.c_str();
                config.readFile(c);
            } catch (libconfig::FileIOException &e){
                std::cerr << "FileIOException occurred. Could not read cam.cfg!\n";
                exit (EXIT_FAILURE);
            } catch (libconfig::ParseException &e){
                std::cerr << "Parse error at " << e.getFile() << ":" << e.getLine()
                            << " - " << e.getError() << std::endl;
                exit(EXIT_FAILURE);
            }

            std::string line;
            try {
                line = "input"; intype = config.lookup(line).c_str();
                line = "inputpath"; inputPath = config.lookup(line).c_str();

                line = "trackingfps"; trackingFPS = config.lookup(line);
                line = "searchfps"; searchFPS = config.lookup(line);
                line = "displayfps"; displayFPS = config.lookup(line);
                line = "capfps"; capFPS = config.lookup(line);

                line = "capture"; captype = config.lookup(line).c_str();
                line = "capturepath"; capturePath = config.lookup(line).c_str();
                line = "capformat"; capFormat = config.lookup(line).c_str();
                line = "capsize"; 
                //capSize.width = config.lookup(line)[0];
                //capSize.height = config.lookup(line)[1];
                std::string capsizearg = config.lookup(line).c_str();
                if (capsizearg == "auto") { }
                else {
                    capSize.width = config.lookup(line)[0];
                    capSize.height = config.lookup(line)[1];
                }
                line = "cap_wb"; capWB = config.lookup(line);
                line = "cap_br"; capBrightness = config.lookup(line);
                line = "cap_contrast"; capContrast = config.lookup(line);
                line = "cap_saturation"; capSat = config.lookup(line);
                line = "showfps"; showFPS = config.lookup(line);

                line = "osd_color"; color = config.lookup(line).c_str();
                line = "osd_linesize"; osdLinesize = config.lookup(line);
                
                line = "display"; dtype = config.lookup(line).c_str();
                line = "displaypath"; displayPath = config.lookup(line).c_str();
                line = "displaysize"; 
                displaySize.width = config.lookup(line)[0];
                displaySize.height = config.lookup(line)[1];
                line = "scale"; processScale = config.lookup(line);
                line = "tracker"; tracktype = config.lookup(line).c_str();
                line = "default_trackbox_size"; init_boxsize = config.lookup(line);
                line = "oft_points"; oftpoints = config.lookup(line);
                line = "oft_trackfeatures"; oftfeatures = config.lookup(line);
                line = "markertype"; markertype = config.lookup(line).c_str();
                line = "pipper"; showPipper = config.lookup(line);
                line = "record"; record_output = config.lookup(line);
                line = "recordpath"; recordPath = config.lookup(line).c_str();
                line = "use_cuda"; use_cuda = config.lookup(line);
                line = "use_eis"; use_eis = config.lookup(line);

                line = "output"; outtype = config.lookup(line).c_str();
                line = "outputpath";
                if (outtype == "socket") {
                    socketport = config.lookup(line);
                    }
                else {
                    outputPath = config.lookup(line).c_str();
                    }

                line = "search"; searchtype = config.lookup(line).c_str();
                line = "dnn_model"; search_dnn_model = config.lookup(line).c_str();
                line = "dnn_model_classes"; search_dnn_model_classes = config.lookup(line).c_str();
                line = "dnn_model_dim"; search_dnn_model_dim =  config.lookup(line);
                line = "target_class"; search_target_class = config.lookup(line);
                line = "yolo_input_width"; search_yolo_width = config.lookup(line);
                line = "yolo_input_height"; search_yolo_height = config.lookup(line);
                line = "target_conf"; search_target_conf = config.lookup(line);                
                line = "auto_lock"; search_auto_lock = config.lookup(line);
                line = "score_threshold"; search_score_threshold = config.lookup(line);
                line = "nms_threshold"; search_nms_threshold = config.lookup(line);
                line = "confidence_threshold"; search_confidence_threshold = config.lookup(line);
                line = "rows"; search_dnn_model_rows = config.lookup(line);
                line = "search_limit_zone"; search_limit_zone = config.lookup(line);
                line = "search_zone"; 

                search_zone[0] = config.lookup(line)[0];
                search_zone[1] = config.lookup(line)[1];
                search_zone[2] = config.lookup(line)[2];
                search_zone[3] = config.lookup(line)[3];

                search_dnn_setup = true;

                
            } catch(libconfig::SettingNotFoundException &e) {
                std::cerr << "Error in configuration file at section: " << line << std::endl;
                global_running = false;
                exit (EXIT_FAILURE);
            }
            break;
        }
        else {
            if (arg.find("--input=") == 0) {intype = arg.substr(8);}
            else if (arg.find("--inputpath=") == 0) {inputPath = arg.substr(12);} 
            else if (arg.find("--capture=") == 0) {captype = arg.substr(10);} 
            else if (arg.find("--capturepath=") == 0) {capturePath = arg.substr(14);} 
            else if (arg.find("--capformat=") == 0) {capFormat = arg.substr(12);} 
            else if (arg.find("--capfps=") == 0) {
                capFPS = stoi(arg.substr(9));
            } 
            else if (arg.find("--capsize=") == 0) {
                std::istringstream iss(arg.substr(10));
                int width, height;
                char delimiter;

                if (!(iss >> width >> delimiter >> height) || delimiter != 'x') {
                    throw std::runtime_error("Invalid size format");
                }
                capSize = cv::Size(width, height);
            } 
            else if (arg.find("--display=") == 0) {dtype = arg.substr(10);} 
            else if (arg.find("--displayfps=") == 0) {dtype = arg.substr(10);} 
            else if (arg.find("--scale=") == 0) {processScale = stod(arg.substr(8));} 
            else if (arg.find("--tracker=") == 0) {tracktype = arg.substr(10);} 
            else if (arg.find("--trackmarker=") == 0) {markertype = arg.substr(14);} 
            else if (arg.find("--pipper") == 0) {showPipper = true;} 
            else if (arg.find("--search=") == 0) {searchtype = arg.substr(9);} 
            else if (arg.find("--output=") == 0) {outtype = arg.substr(9);} 
            else if (arg.find("--outputpath=") == 0) {
                if (outtype == "socket") {socketport = stoi(arg.substr(13));}
                else {outputPath = arg.substr(12);}
                
                } 
            else if (arg.find("--showfps") == 0) {showFPS = true;} 
            else {
                std::cerr << "Unknown or incomplete argument: " << arg << std::endl;
                exit(1);
            }
        }
    }

    if (capturePath == "") {
        std::cerr << "No capture path provided" << std::endl;
        global_running.store(false);
    }

    if (dtype == "none") {displayType = 0;} 
    else if (dtype == "" or dtype=="window") {displayType = 1;} 
    else if (dtype == "framebuffer") {displayType = 2;}
    else if (dtype == "gstreamer") {displayType = 3;} 
    else if (dtype == "rtsp") {displayType = 4;} 
    else if (dtype == "udp") {displayType = 5;} 

    if (captype == "" or captype=="v4l2") {captureType = 1;} 
    else if (captype == "gstreamer") {captureType = 2;}
    else if (captype == "opencv") {captureType = 3;}
    else if (captype == "rtsp") {captureType = 4;}

    if (markertype == "" or markertype=="box") {trackMarker = 1;} 
    else if (markertype == "crosshair") {trackMarker = 2;}
    else if (markertype == "corners") {trackMarker = 3;}
    else if (markertype == "walleye") {trackMarker = 4;}
    else if (markertype == "maverick") {trackMarker = 5;}
    else if (markertype == "lancet") {trackMarker = 6;}

    if (tracktype == "" or tracktype=="none") {trackerType = 0;}
    else if (tracktype == "oft") {trackerType = 1;}
    else if (tracktype == "kcf") {trackerType = 2;}
    else if (tracktype == "csrt") {trackerType = 3;}
    else if (tracktype == "mosse") {trackerType = 4;}

    if (intype == "") {inputType = 0;}
    if (intype == "serial") {inputType = 1;}
    else if (intype == "socket") {inputType = 2;}
    else if (intype == "fifo") {inputType = 3;}

    if (outtype == "") {outputType = 0;}
    if (outtype == "serial") {outputType = 1;}
    else if (outtype == "socket") {outputType = 2;}
    else if (outtype == "fifo") {outputType = 3;}

    if (searchtype == "") {searchType = 0;}
    else if (searchtype == "yolo") {searchType = 1;}

    if (capturePath == "") {capturePath = "/dev/video0";}
    if (processScale==0) {processScale = 1;}
    if (init_boxsize==0) {init_boxsize = 50;}
    //if (capSize.width==0) {capSize.width = 640;}
    //if (capSize.height==0) {capSize.height = 480;}
    if (oftpoints==0 || (trackerType!=1)) {oftpoints=1;}
    if (capBrightness==0) {capBrightness = 50;}
    if (osdLinesize==0) {osdLinesize = 1;}
    if (osdColor=="") {osdColor = "white";}
    if (capWB==0) {capWB = 50;}
    if (capContrast==0) {capContrast = 50;}
    if (capSat==0) {capSat = 50;}
    if (capFormat=="") {capFormat = "BGR3";}
    if (capFPS==0) {capFPS = 30;}
    if (trackingFPS==0) {trackingFPS = 100;}
    if (searchFPS==0) {searchFPS = 30;}
    if (displayFPS==0) {displayFPS = 60;}
}
