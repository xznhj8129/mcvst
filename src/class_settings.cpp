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

    // First pass: Check arguments (stops if --config= found and loaded)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // Help request?
        if ((arg.find("--help") == 0) || (arg.find("--h") == 0)) {
            global_running.store(false);
            exit(0);
        }

        // Config file argument
        if (arg.find("--config=") == 0) {
            cfgFile = arg.substr(9);
            libconfig::Config config;
            try {
                config.readFile(cfgFile.c_str());
            } catch (libconfig::FileIOException &e) {
                std::cerr << "FileIOException occurred. Could not read" << cfgFile.c_str() << "\n";
                exit(EXIT_FAILURE);
            } catch (libconfig::ParseException &e) {
                std::cerr << "Parse error at " << e.getFile() << ":" << e.getLine()
                          << " - " << e.getError() << std::endl;
                exit(EXIT_FAILURE);
            }

            std::string line;
            try {
                line = "debugprint";       debug_print = config.lookup(line);
                line = "socketport";       socketport = config.lookup(line);
                line = "movestep";         movestep = config.lookup(line);

                line = "input";            intype = config.lookup(line).c_str();
                line = "inputpath";        
                if (intype != "socket" && intype !="test") {inputPath = config.lookup(line).c_str();}

                line = "trackingfps";      trackingFPS = config.lookup(line);
                line = "searchfps";        searchFPS = config.lookup(line);
                line = "displayfps";       displayFPS = config.lookup(line);
                line = "capfps";           capFPS = config.lookup(line);

                line = "capture";          captype = config.lookup(line).c_str();
                line = "capturepath";      capturePath = config.lookup(line).c_str();
                line = "capformat";        capFormat = config.lookup(line).c_str();
                line = "capsize";

                const libconfig::Setting& capsizesetting = config.lookup(line);
                if (capsizesetting.getLength() == 2) {
                    capSize.width  = capsizesetting[0]; 
                    capSize.height = capsizesetting[1]; 
                }
                else if (capsizesetting.getLength() == 1 || capsizesetting.getLength() > 2 ) {
                    throw std::runtime_error("Invalid 'capsize' setting in config file.");
                }

                line = "cap_wb";          capWB = config.lookup(line);
                line = "cap_br";          capBrightness = config.lookup(line);
                line = "cap_contrast";    capContrast = config.lookup(line);
                line = "cap_saturation";  capSat = config.lookup(line);
                line = "showfps";         showFPS = config.lookup(line);

                line = "osd_color";       color = config.lookup(line).c_str();
                line = "osd_linesize";    osdLinesize = config.lookup(line);

                line = "display";         dtype = config.lookup(line).c_str();
                line = "displaypath";     displayPath = config.lookup(line).c_str();
                line = "displaysize";
                displaySize.width  = config.lookup(line)[0];
                displaySize.height = config.lookup(line)[1];

                line = "scale";               processScale = config.lookup(line);
                line = "tracker";             tracktype = config.lookup(line).c_str();
                line = "default_trackbox_size"; init_boxsize = config.lookup(line);
                line = "oft_points";          oftpoints = config.lookup(line);
                line = "oft_trackfeatures";   oftfeatures = config.lookup(line);
                line = "epsilon";           epsilon = config.lookup(line);
                line = "maxits";            maxits = config.lookup(line);
                line = "markertype";          markertype = config.lookup(line).c_str();
                line = "pipper";              showPipper = config.lookup(line);
                line = "record";              record_output = config.lookup(line);
                line = "recordpath";          recordPath = config.lookup(line).c_str();
                line = "use_cuda";            use_cuda = config.lookup(line);
                line = "use_eis";             use_eis = config.lookup(line);

                line = "output";              outtype = config.lookup(line).c_str();
                line = "outputpath";
                if (outtype != "socket" && outtype!="none") {outputPath = config.lookup(line).c_str();}

                line = "search";                 searchtype = config.lookup(line).c_str();
                line = "dnn_model";              search_dnn_model = config.lookup(line).c_str();
                line = "dnn_model_classes";      search_dnn_model_classes = config.lookup(line).c_str();
                line = "dnn_model_dim";          search_dnn_model_dim = config.lookup(line);
                line = "target_class";           search_target_class = config.lookup(line);
                line = "yolo_input_width";       search_yolo_width = config.lookup(line);
                line = "yolo_input_height";      search_yolo_height = config.lookup(line);
                line = "target_conf";            search_target_conf = config.lookup(line);
                line = "auto_lock";             search_auto_lock = config.lookup(line);
                line = "score_threshold";        search_score_threshold = config.lookup(line);
                line = "nms_threshold";          search_nms_threshold = config.lookup(line);
                line = "confidence_threshold";   search_confidence_threshold = config.lookup(line);
                line = "rows";                   search_dnn_model_rows = config.lookup(line);
                line = "search_limit_zone";      search_limit_zone = config.lookup(line);
                line = "test_frame";            test_frame = config.lookup(line);
                line = "test_x";            test_x = config.lookup(line);
                line = "test_y";            test_y = config.lookup(line);
                line = "test_boxsize";            test_boxsize = config.lookup(line);

                line = "search_zone";
                search_zone[0] = config.lookup(line)[0];
                search_zone[1] = config.lookup(line)[1];
                search_zone[2] = config.lookup(line)[2];
                search_zone[3] = config.lookup(line)[3];

                search_dnn_setup = true;

            } catch (libconfig::SettingNotFoundException &e) {
                std::cerr << "Error in configuration file at section: " << line << std::endl;
                global_running = false;
                exit(EXIT_FAILURE);
            }
            // According to original code, once we have a config, we break. 
            // This means any further command-line overrides are NOT processed.
            break;
        }

        //
        // No config file: parse other command-line arguments to fill in the blanks
        //
        else {std::cout << "Use the config file" << std::endl;}/*
            if (arg.find("--input=") == 0) {
                intype = arg.substr(8);
            }
            else if (arg.find("--inputpath=") == 0) {
                inputPath = arg.substr(12);
            }
            else if (arg.find("--capture=") == 0) {
                captype = arg.substr(10);
            }
            else if (arg.find("--capturepath=") == 0) {
                capturePath = arg.substr(14);
            }
            else if (arg.find("--capformat=") == 0) {
                capFormat = arg.substr(12);
            }
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
            else if (arg.find("--capwb=") == 0) {
                capWB = stoi(arg.substr(7));
            }
            else if (arg.find("--capbr=") == 0) {
                capBrightness = stoi(arg.substr(7));
            }
            else if (arg.find("--capcontrast=") == 0) {
                capContrast = stoi(arg.substr(13));
            }
            else if (arg.find("--capsat=") == 0) {
                capSat = stoi(arg.substr(9));
            }
            else if (arg.find("--showfps") == 0) {
                showFPS = true;
            }
            else if (arg.find("--osdcolor=") == 0) {
                color = arg.substr(10);
            }
            else if (arg.find("--osdlinesize=") == 0) {
                osdLinesize = stoi(arg.substr(13));
            }
            else if (arg.find("--display=") == 0) {
                dtype = arg.substr(10);
            }
            else if (arg.find("--displayfps=") == 0) {
                displayFPS = stoi(arg.substr(12));
            }
            else if (arg.find("--scale=") == 0) {
                processScale = stod(arg.substr(8));
            }
            else if (arg.find("--tracker=") == 0) {
                tracktype = arg.substr(10);
            }
            else if (arg.find("--markertype=") == 0) {
                markertype = arg.substr(13);
            }
            else if (arg.find("--pipper") == 0) {
                showPipper = true;
            }
            else if (arg.find("--record") == 0) {
                record_output = true;
            }
            else if (arg.find("--recordpath=") == 0) {
                recordPath = arg.substr(12);
            }
            else if (arg.find("--use_cuda") == 0) {
                use_cuda = true;
            }
            else if (arg.find("--use_eis") == 0) {
                use_eis = true;
            }
            else if (arg.find("--output=") == 0) {
                outtype = arg.substr(9);
            }
            else if (arg.find("--outputpath=") == 0) {
                if (outtype == "socket") {
                    socketport = stoi(arg.substr(13));
                } else {
                    outputPath = arg.substr(12);
                }
            }
            else if (arg.find("--search=") == 0) {
                searchtype = arg.substr(9);
            }
            else if (arg.find("--trackingfps=") == 0) {
                trackingFPS = stoi(arg.substr(14));
            }
            else if (arg.find("--searchfps=") == 0) {
                searchFPS = stoi(arg.substr(12));
            }
            else if (arg.find("--default_trackbox_size=") == 0) {
                init_boxsize = stoi(arg.substr(23));
            }
            else if (arg.find("--oftpoints=") == 0) {
                oftpoints = stoi(arg.substr(11));
            }
            else if (arg.find("--oftfeatures=") == 0) {
                oftfeatures = stoi(arg.substr(13));
            }
            else if (arg.find("--dnn_model=") == 0) {
                search_dnn_model = arg.substr(11);
            }
            else if (arg.find("--dnn_model_classes=") == 0) {
                search_dnn_model_classes = arg.substr(19);
            }
            else if (arg.find("--dnn_model_dim=") == 0) {
                search_dnn_model_dim = stoi(arg.substr(16));
            }
            else if (arg.find("--yolo_input_width=") == 0) {
                search_yolo_width = stoi(arg.substr(19));
            }
            else if (arg.find("--yolo_input_height=") == 0) {
                search_yolo_height = stoi(arg.substr(20));
            }
            else if (arg.find("--rows=") == 0) {
                search_dnn_model_rows = stoi(arg.substr(7));
            }
            else if (arg.find("--target_class=") == 0) {
                search_target_class = stoi(arg.substr(14));
            }
            else if (arg.find("--target_conf=") == 0) {
                search_target_conf = stod(arg.substr(13));
            }
            else if (arg.find("--auto_lock") == 0) {
                search_auto_lock = true;
            }
            else if (arg.find("--score_threshold=") == 0) {
                search_score_threshold = stod(arg.substr(17));
            }
            else if (arg.find("--nms_threshold=") == 0) {
                search_nms_threshold = stod(arg.substr(15));
            }
            else if (arg.find("--confidence_threshold=") == 0) {
                search_confidence_threshold = stod(arg.substr(22));
            }
            else if (arg.find("--search_limit_zone=") == 0) {
                search_limit_zone = stoi(arg.substr(19));
            }
            else if (arg.find("--search_zone=") == 0) {
                // Example format: "0,0,640,480"
                std::istringstream iss(arg.substr(13));
                int z0, z1, z2, z3;
                char delim;
                if (!(iss >> z0 >> delim >> z1 >> delim >> z2 >> delim >> z3)) {
                    std::cerr << "Invalid --search_zone= format. Expected 4 integers separated by commas.\n";
                    exit(EXIT_FAILURE);
                }
                search_zone[0] = z0; search_zone[1] = z1;
                search_zone[2] = z2; search_zone[3] = z3;
            }
            else {
                std::cerr << "Unknown or incomplete argument: " << arg << std::endl;
                exit(1);
            }
        }*/
    }

    // Check for missing capture path
    if (capturePath.empty()) {
        std::cerr << "No capture path provided" << std::endl;
        global_running.store(false);
    }

    // Convert strings to enumerations for internal usage
    if (dtype == "none")                 { displayType = 0; }
    else if (dtype == "" || dtype=="window")    { displayType = 1; }
    else if (dtype == "framebuffer")     { displayType = 2; }
    else if (dtype == "gstreamer")       { displayType = 3; }
    else if (dtype == "rtsp")            { displayType = 4; }
    else if (dtype == "udp")             { displayType = 5; }

    if (captype == "" || captype=="v4l2") { captureType = 1; }
    else if (captype == "gstreamer")      { captureType = 2; }
    else if (captype == "file")         { captureType = 3; }
    else if (captype == "rtsp")           { captureType = 4; }

    if (markertype == "" || markertype=="box")    { trackMarker = 1; }
    else if (markertype == "crosshair")           { trackMarker = 2; }
    else if (markertype == "corners")             { trackMarker = 3; }
    else if (markertype == "walleye")             { trackMarker = 4; }
    else if (markertype == "maverick")            { trackMarker = 5; }
    else if (markertype == "lancet")              { trackMarker = 6; }

    if (tracktype == "" || tracktype=="none") { trackerType = 0; }
    else if (tracktype == "oft")              { trackerType = 1; }
    else if (tracktype == "kcf")              { trackerType = 2; }
    else if (tracktype == "csrt")             { trackerType = 3; }
    else if (tracktype == "mosse")            { trackerType = 4; }
    else if (tracktype == "denseoft")         { trackerType = 6; }

    if (intype == "" || intype == "none")          { inputType = 0; }
    else if (intype == "socket")  { inputType = 1; }
    else if (intype == "serial")  { inputType = 2; }
    else if (intype == "fifo")    { inputType = 3; }
    else if (intype == "test")    { inputType = 4; }

    if (outtype == "" || outtype == "none")         { outputType = 0; }
    else if (outtype == "socket")  { outputType = 1; }
    else if (outtype == "serial")  { outputType = 2; }
    else if (outtype == "fifo")    { outputType = 3; }

    if (searchtype == "" || searchtype == "none")       { searchType = 0; }
    else if (searchtype == "yolo") { searchType = 1; }

    // Provide defaults for anything still unset
    if (capturePath.empty())   { capturePath = "/dev/video0"; }
    if (processScale == 0)     { processScale = 1; }
    if (movestep == 0)         { movestep = 20; }
    if (maxits == 0)         { maxits = 10; }
    if (epsilon == 0.03)         { epsilon =  0.03; }
    if (init_boxsize == 0)     { init_boxsize = 50; }
    if (oftpoints == 0 || (trackerType != 1)) { oftpoints = 1; }
    if (capBrightness == 0)    { capBrightness = 50; }
    if (osdLinesize == 0)      { osdLinesize = 1; }
    if (color.empty())         { osdColor = "white"; }
    else                       { osdColor = color; }
    if (capWB == 0)            { capWB = 50; }
    if (capContrast == 0)      { capContrast = 50; }
    if (oft_winsize == 0)      { oft_winsize = 15; }
    if (oft_pyrlevels == 0)     { oft_pyrlevels = 2; }
    if (capSat == 0)           { capSat = 50; }
    if (capFormat.empty())     { capFormat = "BGR3"; }
    //if (capFPS == 0)           { capFPS = 30; }
    if (trackingFPS == 0)      { trackingFPS = 100; }
    if (searchFPS == 0)        { searchFPS = 30; }
    if (displayFPS == 0)       { displayFPS = 60; }

    // Basic consistency checks (example—add your own as needed)
    if ((searchType == 1) && (search_dnn_model.empty())) {
        std::cerr << "Warning: YOLO search requested but no dnn_model set.\n";
    }
    if ((outputType == 2) && (socketport == 0)) {
        std::cerr << "Warning: Output set to socket but no socket port specified.\n";
    }
}
