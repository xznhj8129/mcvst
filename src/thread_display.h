#pragma once
#include "globals.h"
#include "classes.h"
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <iomanip>
#include <sstream>

void onMouse(int event, int x, int y, int, void*) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        cv::Point2f clicked_point;
        double scale_x = (double)cap_intf.frameSize.width / settings.displaySize.width;
        double scale_y = (double)cap_intf.frameSize.height / settings.displaySize.height;
        clicked_point.x = x * scale_x;
        clicked_point.y = y * scale_y;
        track_intf.lock(clicked_point.x, clicked_point.y);
        
        /*
        cv::Point2f clicked_point;
        if (center.x < 0 || center.y < 0) {
            // No zoom: map directly from display to original frame
            double scale_x = (double)cap_intf.frameSize.width / settings.displaySize.width;
            double scale_y = (double)cap_intf.frameSize.height / settings.displaySize.height;
            clicked_point.x = x * scale_x;
            clicked_point.y = y * scale_y;
        } else {
            // Zoomed in: map click to the cropped area
            double scale_x = crop_rect.width / display_size.width;
            double scale_y = crop_rect.height / display_size.height;

            // Coordinates in the cropped area
            double x_in_crop = x * scale_x;
            double y_in_crop = y * scale_y;

            // Map back to original frame coordinates
            clicked_point.x = x_in_crop + crop_rect.x;
            clicked_point.y = y_in_crop + crop_rect.y;
        }

        // Initialize tracking
        center = clicked_point;
        tracking_points[0].clear();
        tracking_points[1].clear();
        tracking_points[0].push_back(center);
        tracking = true;
        prev_gray.release();  // Reset previous frame
        */
    }
}

int display_thread(SharedData& sharedData) {
    bool finish_error = false;
    std::chrono::milliseconds frameDuration(1000 / settings.displayFPS);
    cv::Mat frame;
    int frame_count = 0;
    int fc = 0;
    int total_frames = 0;
    int fps = -1;

    GstElement *pipeline = nullptr;
    GstElement *appsrc = nullptr;
    bool gst_pipeline_ready = false;

    if (settings.displayType == 0) {
        if (global_debug_print) {std::cout << "display thread finished" << std::endl;}
        return 0;
    }
    else if (settings.displayType == 1) {
        cv::namedWindow("Tracking", cv::WINDOW_NORMAL);
        cv::resizeWindow("Tracking", settings.displaySize.width, settings.displaySize.height);
        cv::setMouseCallback("Tracking", onMouse);
        cv::imshow("Tracking",cap_intf.novideo);
    }
    else if (settings.displayType == 2) {
        display_intf.clearFramebuffer();
    }
    else if (settings.displayType == 3) {
        // GStreamer pipeline (e.g. streaming to local UDP sink)
        // Sending frames as H.264 RTP over UDP:
        // appsrc -> videoconvert -> x264enc -> rtph264pay -> udpsink
        // Adjust host/port as needed
        GError* error = nullptr;
        pipeline = gst_parse_launch(
            "appsrc name=mysrc is-live=true format=time ! videoconvert ! x264enc tune=zerolatency bitrate=500 speed-preset=superfast ! rtph264pay config-interval=1 pt=96 ! udpsink host=127.0.0.1 port=5000 sync=false",
            &error
        );
        if (!pipeline || error) {
            std::cerr << "Failed to create GStreamer pipeline for displayType=3: " << (error ? error->message : "") << std::endl;
            if (error) g_error_free(error);
            finish_error = true;
        } else {
            appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "mysrc");
            if (!appsrc) {
                std::cerr << "Failed to get appsrc from GStreamer pipeline." << std::endl;
                finish_error = true;
            } else {
                gst_element_set_state(pipeline, GST_STATE_PLAYING);
                gst_pipeline_ready = true;
            }
        }
    }
    else if (settings.displayType == 4) {
        // GStreamer pipeline for RTSP output via rtspclientsink
        // This requires a running RTSP server accepting stream at given location.
        // appsrc -> videoconvert -> x264enc -> rtph264pay -> rtspclientsink
        GError* error = nullptr;
        pipeline = gst_parse_launch(
            "appsrc name=mysrc is-live=true format=time ! videoconvert ! x264enc tune=zerolatency bitrate=500 speed-preset=superfast ! rtph264pay config-interval=1 pt=96 ! rtspclientsink location=rtsp://127.0.0.1:8554/mystream",
            &error
        );
        if (!pipeline || error) {
            std::cerr << "Failed to create GStreamer pipeline for displayType=4: " << (error ? error->message : "") << std::endl;
            if (error) g_error_free(error);
            finish_error = true;
        } else {
            appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "mysrc");
            if (!appsrc) {
                std::cerr << "Failed to get appsrc from GStreamer pipeline." << std::endl;
                finish_error = true;
            } else {
                gst_element_set_state(pipeline, GST_STATE_PLAYING);
                gst_pipeline_ready = true;
            }
        }
    }
    else if (settings.displayType == 5) {
        // GStreamer pipeline for direct UDP streaming without RTSP:
        // appsrc -> videoconvert -> x264enc -> rtph264pay -> udpsink
        // Different from displayType=3 only in concept; you may vary parameters as needed.
        GError* error = nullptr;
        pipeline = gst_parse_launch(
            "appsrc name=mysrc is-live=true format=time ! videoconvert ! x264enc tune=zerolatency bitrate=500 speed-preset=superfast ! rtph264pay config-interval=1 pt=96 ! udpsink host=192.168.1.100 port=6000 sync=false",
            &error
        );
        if (!pipeline || error) {
            std::cerr << "Failed to create GStreamer pipeline for displayType=5: " << (error ? error->message : "") << std::endl;
            if (error) g_error_free(error);
            finish_error = true;
        } else {
            appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "mysrc");
            if (!appsrc) {
                std::cerr << "Failed to get appsrc from GStreamer pipeline." << std::endl;
                finish_error = true;
            } else {
                gst_element_set_state(pipeline, GST_STATE_PLAYING);
                gst_pipeline_ready = true;
            }
        }
    }
    else {
        std::cerr << "Invalid display type selected" << std::endl;
        finish_error = true;
    }

    auto start = std::chrono::high_resolution_clock::now();
    while (global_running and !finish_error) {
        auto startTime = std::chrono::steady_clock::now();
        frame = sharedData.displayFrame.clone();
        frame_count++;
        total_frames++;

        if (frame.empty()) {
            fc++;
            std::cout << "empty frame " << fc << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else {

            for (const auto& point : track_intf.oftdata.old_points) {
                cv::circle(frame, point, 2, cv::Scalar(255, 255, 255), -1);}
            if (track_intf.locked) {
                display_intf.draw_track(frame);
            } else if (settings.showPipper) {
                display_intf.draw_cornerbox(frame, cv::Point(track_intf.poi.x, track_intf.poi.y), track_intf.boxsize);
            }

            if (settings.searchType>0) {
                int detections = search_intf.output.size();

                for (int i = 0; i < detections; ++i) {
                    auto detection = search_intf.output[i];
                    auto box = detection.box;
                    auto classId = detection.class_id;
                    std::string classStr = search_intf.class_list[classId].c_str();
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(2) << detection.confidence;
                    std::string trimmed_confidence = oss.str();
                    //std::cout << classStr << " " << classId << " " << detection.confidence << " " << box.x+(box.width/2) << " "<< box.y+(box.height/2) << std::endl;
                    cv::rectangle(frame, box, cv::Scalar(255, 255, 255), 2);
                    cv::putText(frame, trimmed_confidence, cv::Point(box.x, box.y - 15), cv::FONT_HERSHEY_SIMPLEX, display_intf.linesize, cv::Scalar(255, 255, 255));
                    cv::putText(frame, classStr, cv::Point(box.x, box.y - 5), cv::FONT_HERSHEY_SIMPLEX, display_intf.linesize, cv::Scalar(255, 255, 255));
                }                
            }

            if (settings.showFPS) {
                if (frame_count >= 30) {
                    auto end = std::chrono::high_resolution_clock::now();
                    fps = (int)(frame_count * 1000.0 / std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
                    frame_count = 0;
                    start = std::chrono::high_resolution_clock::now();
                }
                if (fps > 0) {
                    std::ostringstream fps_label;
                    fps_label << std::fixed << std::setprecision(2);
                    fps_label << "FPS: " << fps;
                    cv::putText(frame, fps_label.str(), cv::Point(10, 25), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
                }
            }

            // display
            if (frame.cols != settings.displaySize.width) {
                cv::resize(frame, frame, settings.displaySize, 0, 0, cv::INTER_AREA);
            }

            if (settings.displayType == 1) { //opencv keyboard input
                cv::imshow("Tracking", frame);
                int keyCode = cv::waitKey(1) & 0xFF;
                if (keyCode!=255){
                    //std::cout << "KEY:" << keyCode << std::endl;
                    switch(keyCode) {

                        case 32:
                            input_intf.input_command(1);
                            break; // spc
                        case 99:
                            input_intf.input_command(2);
                            break; // c
                        case 82: 
                            input_intf.input_command(3);
                            break; // Up arrow
                        case 84: 
                            input_intf.input_command(4);
                            break; // Down arrow
                        case 83: 
                            input_intf.input_command(6);
                            break; // Right arrow
                        case 81: 
                            input_intf.input_command(5);
                            break; // Left arrow
                        case 97:
                            input_intf.input_command(7);
                            break; // a
                        case 115:
                            input_intf.input_command(8);
                            break; // s
                        case 27:
                            input_intf.input_command(9);
                            break; // c
                        default: 
                            break;

                    }
                }
                if (keyCode == 27) {break;} // Exit if ESC pressed

            } else if (settings.displayType == 2) {
                display_intf.writeImageToFramebuffer(frame);

            } else if ((settings.displayType == 3 || settings.displayType == 4 || settings.displayType == 5) && gst_pipeline_ready) {
                // Push the frame into the GStreamer pipeline via appsrc
                // Convert cv::Mat (BGR) to GstBuffer
                GstBuffer *buffer;
                GstMapInfo map;
                size_t size = frame.total() * frame.elemSize();

                buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
                gst_buffer_map(buffer, &map, GST_MAP_WRITE);
                memcpy(map.data, frame.data, size);
                gst_buffer_unmap(buffer, &map);

                // Set timestamp to maintain a proper flow in pipeline
                GstClockTime now = gst_clock_get_time(gst_element_get_clock(pipeline));
                GST_BUFFER_PTS(buffer) = now;
                GST_BUFFER_DTS(buffer) = now;
                GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, settings.displayFPS);

                // Push buffer
                GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
                if (ret != GST_FLOW_OK) {
                    std::cerr << "Failed to push buffer into GStreamer pipeline." << std::endl;
                    finish_error = true;
                }
            }

            auto endTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            auto sleepTime = frameDuration - elapsedTime;
            frame_count++;
            if (sleepTime > std::chrono::milliseconds(0)) {
                std::this_thread::sleep_for(sleepTime);
            }
        }
    }

    if (settings.displayType == 1) {
        cv::destroyAllWindows();
    }

    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }

    if (global_debug_print) {std::cout << "display thread finished" << std::endl;}
    global_running.store(false);
    return finish_error ? 1 : 0;
}
