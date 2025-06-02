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
            if (i < detections - 1) detects << ",";
        }
    }
    detects << "]"; // Use JSON array syntax

    double data_az = track_intf.angle.x;
    double data_el = track_intf.angle.y;
    int data_mode = track_intf.locked;

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
    // Set a 1-second timeout for recv()
    struct timeval timeout;
    timeout.tv_sec = 1;  // timeout in seconds
    timeout.tv_usec = 0;
    if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
        if (global_running) perror("setsockopt failed in handleClientRequests");
        // Optionally handle the error; here we simply continue if global_running
    }

    char buffer[1024];
    while (global_running) {
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesRead < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // Timeout occurred, loop back to check global_running
                if (!global_running) { close(clientSocket); return; }
                continue;
            } else if (errno == EINTR) { // Interrupted
                if (!global_running) { close(clientSocket); return; }
                continue; // Retry
            } else {
                if (global_running) perror("recv error in handleClientRequests");
                close(clientSocket);
                return;
            }
        }
        if (bytesRead == 0) {
            // Connection closed by the client
            close(clientSocket);
            return;
        }
        buffer[bytesRead] = '\0';
        std::string data = createDataPacket();
        send(clientSocket, data.c_str(), data.size(), 0); // Consider error handling for send
    }
    // global_running is false, close socket and exit thread
    close(clientSocket);
}

int output_thread(SharedData& sharedData) {
    bool finish_error = false;

    // insert thread sleeper here if needed

    if (settings.outputType == 1) { // socket
        int serverSocket, clientSocket;
        struct sockaddr_in serverAddr, clientAddr;
        socklen_t clientLen = sizeof(clientAddr);

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            std::cerr << "Error: Unable to create server socket" << std::endl;
            finish_error = true;
            global_running.store(false);
        }

        if (global_running && !finish_error) { // Proceed only if socket creation was successful and still running
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_addr.s_addr = INADDR_ANY;
            serverAddr.sin_port = htons(settings.outputPort);

            if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
                std::cerr << "Error: Unable to bind server socket" << std::endl;
                close(serverSocket);
                finish_error = true;
                global_running.store(false);
            }
        }

        if (global_running && !finish_error) { // Proceed only if bind was successful
            if (listen(serverSocket, 5) < 0) {
                std::cerr << "Error: Unable to listen on server socket" << std::endl;
                close(serverSocket);
                finish_error = true;
                global_running.store(false);
            }
        }
        
        if (global_running && !finish_error) {
             std::cout << "Server live at port " << settings.outputPort << std::endl;
        }

        while (global_running && !finish_error) {
            fd_set readSet;
            struct timeval timeout;
            FD_ZERO(&readSet);
            FD_SET(serverSocket, &readSet);

            // Set a 1-second timeout for select
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            int selectResult = select(serverSocket + 1, &readSet, NULL, NULL, &timeout);

            if (!global_running) break; // Exit if shutdown initiated during select

            if (selectResult < 0) {
                if (errno == EINTR && global_running) continue; // Interrupted, try again
                if (global_running) perror("select in output_thread");
                break;
            } else if (selectResult == 0) {
                // Timeout, loop back to check global_running
                continue;
            }

            if (FD_ISSET(serverSocket, &readSet)) {
                clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
                if (clientSocket < 0) {
                    if (global_running) perror("Error: Unable to accept client connection");
                    if (errno == EINTR && global_running) continue; // Interrupted, try again
                    if (!global_running) break; // If shutting down, exit loop
                    finish_error = true; // Consider this a significant error to stop the thread
                    break;
                }
                std::thread clientThread(handleClientRequests, clientSocket);
                clientThread.detach();
            }
        }
        if (serverSocket >= 0) close(serverSocket); // Ensure server socket is closed if it was opened
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
            }
            // Sleep regardless of fifo open status to avoid busy loop on error
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    if (global_debug_print) {std::cout << "output thread finished" << std::endl;}
    if (!finish_error) {return 0;}
    else {return 1;}
}