#pragma once
#include "globals.h"
#include "classes.h"
#include <string>
#include <sstream>
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
//#include <iostream>
//#include <iomanip>
//#include <cstring>
//#include <fstream>
#include <sstream>  
#include <sstream>
#include <iomanip> // For std::fixed and std::setprecision
#include <nlohmann/json.hpp>

std::string createDataPacket() {
    std::ostringstream detects;
    detects << "[";
    if (settings.searchType > 0) {
        int detections = search_intf.output.size();
        for (int i = 0; i < detections; ++i) {
            auto& detection = search_intf.output[i]; // Use reference to avoid copy
            detects << "[" << detection.class_id << "," << detection.confidence << ","
                   << "[" << detection.box.x << "," << detection.box.y << ","
                          << detection.box.width << "," << detection.box.height << "]"
                   << "]";
            if (i < detections - 1) detects << ","; // Avoid trailing comma
        }
    }
    detects << "]"; // Use JSON array syntax
    
    double data_az = track_intf.angle.x;
    double data_el = track_intf.angle.y;
    int data_mode = track_intf.target_lock;

    std::ostringstream packet;
    packet << "{ ";
    packet << "\"locked\": " << data_mode << ", ";
    packet << "\"tracking\": ["
           << std::fixed << std::setprecision(6) << data_az << ","
           << std::fixed << std::setprecision(6) << data_el << "], ";
    packet << "\"detections\": " << detects.str() << "}\n";
    
    // Debug output
    std::string result = packet.str();
    return result;
}

void handleClientRequests(int clientSocket) {
    char buffer[1024];
    while (global_running) {
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            close(clientSocket);
            return;
        }
        buffer[bytesRead] = '\0';
        std::string data = createDataPacket();
        send(clientSocket, data.c_str(), data.size(), 0);
    }
}

int output_thread(SharedData& sharedData) {
    bool finish_error;

    // insert thread sleeper here

    if (settings.outputType==1) { // socket
        int serverSocket, clientSocket;
        struct sockaddr_in serverAddr, clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            std::cerr << "Error: Unable to create server socket" << std::endl;
            finish_error = true;
            global_running.store(false);
        }
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(settings.socketport);
        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Error: Unable to bind server socket" << std::endl;
            close(serverSocket);
            finish_error = true;
            global_running.store(false);
        }
        if (listen(serverSocket, 5) < 0) {
            std::cerr << "Error: Unable to listen on server socket" << std::endl;
            close(serverSocket);
            finish_error = true;
            global_running.store(false);
        }
        std::cout << "Server live at port " << settings.socketport << std::endl;
        while (global_running) { // Accept client connections and handle them in separate threads
            clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
            //std::cout << "Client connected" << std::endl;
            if (clientSocket < 0) {
                std::cerr << "Error: Unable to accept client connection" << std::endl;
                close(serverSocket);
                finish_error = true;
                break;
            }
            std::thread clientThread(handleClientRequests, clientSocket);
            clientThread.detach(); // Detach the thread to let it run independently
        }
        close(serverSocket);
    }
    else if (settings.outputType==2) {} // serial
    else if (settings.outputType==3) { // fifo
        while (global_running) { 
            std::ofstream fifo;
            fifo.open(settings.outputPath);

            if (!fifo.is_open()) {
                std::cerr << "Error: Could not open FIFO file." << std::endl;
            }
            else {
                std::string data = createDataPacket();
                fifo << data << std::endl;
                fifo.close();
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }


    }

    if (global_debug_print) {std::cout << "output thread finished" << std::endl;}
    if (!finish_error) {return 0;}
    else {return 1;}
}