#include "globals.h"
#include "structs.h"
#include "classes.h"
#include "threads.h"

//std::unique_ptr<SettingsClass> args;
//std::unique_ptr<TrackInterface> global_track_intf;
//std::unique_ptr<CaptureInterface> global_cap;
//std::unique_ptr<DisplayInterface> global_display;

// Main

int main(int argc, char** argv) {
    //std::shared_ptr<SettingsClass> args = std::make_shared<SettingsClass>(argc, argv);
    //std::shared_ptr<CaptureInterface> global_cap = std::make_shared<CaptureInterface>(args->displayType, args->capturePath, args->capSize);
    //std::shared_ptr<TrackInterface> global_track_intf = std::make_shared<TrackInterface>(args->capSize, args->processScale);

    SharedData sharedData;

    settings.Init(argc, argv);
    cap_intf.Init(settings.captureType, settings.capturePath, settings.capSize);
    track_intf.Init(cap_intf.frameSize, settings.processScale);
    display_intf.Init(settings.displayType);
    search_intf.Init();
    input_intf.Init();

    std::thread captureThread(capture_thread, std::ref(sharedData));
    std::thread displayThread(display_thread, std::ref(sharedData));
    std::thread searchThread(search_thread, std::ref(sharedData));
    std::thread trackThread(tracking_thread, std::ref(sharedData));
    std::thread inputThread(input_thread, std::ref(sharedData));
    std::thread outputThread(output_thread, std::ref(sharedData));

    captureThread.join();
    displayThread.join();
    searchThread.join();
    trackThread.join();
    inputThread.join();
    outputThread.join();
    
    search_intf.shutdown();
    cv::cuda::resetDevice();
    return 0;
}
