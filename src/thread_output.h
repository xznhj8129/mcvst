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

void handleClientRequests(int clientSocket) {
    char buffer[1024];
    while (global_running) {
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            close(clientSocket);
            return;
        }
        buffer[bytesRead] = '\0';
        double data_az = track_intf.angle.x; // might need to do atomic for thread safety
        double data_el = track_intf.angle.y; //atom_elevation.load();
        int data_mode = track_intf.target_lock;
        std::string data = std::to_string(data_mode) + "," + std::to_string(data_az) + ","+std::to_string(data_el);
        send(clientSocket, data.c_str(), data.size(), 0);
    }
}

int output_thread(SharedData& sharedData) {
    bool finish_error;

    // insert thread sleeper here
    if (settings.outputType==1) {} // serial

    else if (settings.outputType==2) { // socket
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
            std::cout << "Client connected" << std::endl;
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
    else if (settings.outputType==3) { // fifo
        while (global_running) { 
            std::ofstream fifo;
            std::string fifo_path = "/tmp/mcvst_output";
            fifo.open(fifo_path);

            if (!fifo.is_open()) {
                std::cerr << "Error: Could not open FIFO file." << std::endl;
            }
            else {
                double data_az = track_intf.angle.x; // might need to do atomic for thread safety
                double data_el = track_intf.angle.y; //atom_elevation.load();
                int data_mode = track_intf.locked;
                std::string data = std::to_string(data_mode) + "," + std::to_string(data_az) + ","+std::to_string(data_el);
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