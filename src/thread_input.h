#pragma once
#include "globals.h"
#include "classes.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <arpa/inet.h>

/*
Inputs:
- Lock (bool, momentary)
- Reset (bool, momentary)
- Updown (-1 to 1)
- Leftright (-1 to 1)
- Boxsize (-1, 0, 1) (smaller or bigger)
- Shutdown

*/

std::string read_line_from_socket(int socket_fd) {
    std::string line;
    char ch;
    while (global_running) { // Check global_running
        ssize_t n = recv(socket_fd, &ch, 1, 0); // clientSocket needs SO_RCVTIMEO
        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) { // Timeout
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Avoid busy spin
                continue; // Re-check global_running
            }
            return ""; // Other errors
        }
        if (n == 0) { // Disconnection
            return "";
        }
        if (ch == '\n') {
            break;
        }
        if (ch != '\r') { // Append if not CR
            line += ch;
        }
    }
    return line;
}

TrackInputs read_from_fifo(std::string FIFO_PATH) {
    std::ifstream fifo(FIFO_PATH);
    std::string line;
    TrackInputs inputs;

    if (fifo) {
        std::getline(fifo, line);
        std::stringstream ss(line);
        std::string token;

        if (std::getline(ss, token, ',')) {
            inputs.lock = std::stoi(token);
        }
        if (std::getline(ss, token, ',')) {
            inputs.unlock = std::stoi(token);
        }
        if (std::getline(ss, token, ',')) {
            inputs.updown = std::stof(token);
        }
        if (std::getline(ss, token, ',')) {
            inputs.leftright = std::stof(token);
        }
        if (std::getline(ss, token, ',')) {
            inputs.boxsize = std::stoi(token);
        }
        if (std::getline(ss, token, ',')) {
            inputs.exit = std::stoi(token);
        }
        inputs.valid = true;
    }

    return inputs;
}


int input_thread(SharedData& sharedData) {
    bool finish_error = false;

    //insert thread sleeper here

    if (settings.inputType==0) {
        if (settings.displayType == 1) {} //do nothing, handled by opencv

        else if (settings.displayType == 2) { //framebuffer
            display_intf.setTerminalMode(true);
            char c;
            while (global_running) {
                c = getchar();
                int keyCode = 0;
                if (c == ' ') {
                    input_intf.input_command(1);}
                else if (c == 27) { // ESC sequence for arrow keys
                    getchar(); // Skip the '['
                    c = getchar();
                    switch(c) {
                        case 'A':
                            input_intf.input_command(3);
                            break; // Up arrow
                        case 'B':
                            input_intf.input_command(4);
                            break; // Down arrow
                        case 'C':
                            input_intf.input_command(6);
                            break; // Right arrow
                        case 'D':
                            input_intf.input_command(5);
                            break; // Left arrow
                        default:
                            keyCode = 27;
                            global_running.store(false);
                            break; // ESC pressed
                    }
                } else if (c == 'c') {input_intf.input_command(2);}
                else if (c=='a') {input_intf.input_command(7);}
                else if (c=='s') {input_intf.input_command(8);}

                if (keyCode != 0) { // Reset after processing
                    latestKeyCode.store(0); }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            display_intf.setTerminalMode(false); // Restore terminal settings
        }
    }
    else if (settings.inputType == 1) { // socket
        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            std::cerr << "Error: Unable to create server socket" << std::endl;
            finish_error = true;
            global_running.store(false);
            return 1;
        }
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(settings.inputPort);

        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Error: Unable to bind server socket" << std::endl;
            close(serverSocket);
            finish_error = true;
            global_running.store(false);
            return 1;
        }

        if (listen(serverSocket, 5) < 0) {
            std::cerr << "Error: Unable to listen on server socket" << std::endl;
            close(serverSocket);
            finish_error = true;
            global_running.store(false);
            return 1;
        }

        std::cout << "Input server listening on port " << settings.inputPort << std::endl;

        while (global_running) {
            fd_set readSet;
            struct timeval timeout;
            FD_ZERO(&readSet);
            FD_SET(serverSocket, &readSet);
            timeout.tv_sec = 1;    // 1-second timeout
            timeout.tv_usec = 0;

            int selectResult = select(serverSocket + 1, &readSet, NULL, NULL, &timeout);

            if (!global_running) break; // Exit if shutdown initiated during select

            if (selectResult < 0) {
                perror("select");
                break;
            } else if (selectResult == 0) {
                // Timeout occurred; no client is waiting. Loop again.
                continue;
            }

            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
            if (clientSocket < 0) {
                if (global_running) {
                    perror("Error: Unable to accept client connection");
                }
                if (errno == EINTR && global_running) continue; // Interrupted, try again if running
                if (!global_running) break; // If shutting down, exit loop
                continue; // Try accepting another connection
            }

            // Set timeout for clientSocket operations
            struct timeval clientTimeout;
            clientTimeout.tv_sec = 1;
            clientTimeout.tv_usec = 0;
            if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&clientTimeout, sizeof(clientTimeout)) < 0) {
                if (global_running) perror("setsockopt SO_RCVTIMEO for clientSocket in input_thread");
                close(clientSocket);
                if (!global_running) break;
                continue;
            }

            int last_btn1 = 0;
            while (global_running) {
                std::string line = read_line_from_socket(clientSocket);
                if (line.empty()) {
                    break; // Client disconnected or global_running is false
                }

                try {
                    nlohmann::json j = nlohmann::json::parse(line);
                    TrackInputs inputs;
                    inputs.lock = j["lock"].get<int>();
                    inputs.unlock = j["reset"].get<int>();
                    inputs.leftright = j["lr"].get<float>();
                    inputs.updown = j["ud"].get<float>();
                    inputs.boxsize = j["boxsize"].get<int>(); // Assuming integer input
                    inputs.exit = j["shutdown"].get<int>();
                    inputs.valid = true;

                    input_intf.input_vec(inputs, last_btn1);
                    last_btn1 = inputs.lock;
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing JSON: " << e.what() << std::endl;
                }
                send(clientSocket, "x", 1, 0); // Consider error handling for send
            }

            close(clientSocket);
            //std::cout << "Client disconnected" << std::endl;
        }

        close(serverSocket);
    }

    else if (settings.inputType==2) //serial
    {
        // to implement
    }
    else if (settings.inputType==3) //fifo
    {
        int last_btn1 = 0;
        while (global_running) {
            try {
                TrackInputs inputs = read_from_fifo(settings.inputPath);
                if (inputs.valid) {
                    input_intf.input_vec(inputs, last_btn1);
                    last_btn1 = inputs.lock;
                }
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    else if (settings.inputType==4) //test loop
    {
        int set_frame = settings.test_frame;
        int set_x = settings.test_x;
        int set_y = settings.test_y;
        int set_boxsize = settings.test_boxsize;
        bool lock = false;
        int lostframe;

        auto startTime = std::chrono::steady_clock::now();
        while (global_running) {
            //std::cout << framecounter << std::endl;
            if (framecounter == set_frame && !lock) {
                track_intf.boxsize = set_boxsize;
                std::cout << "Test lock" << std::endl;
                startTime = std::chrono::steady_clock::now();  // Use steady_clock for consistency
                track_intf.lock(set_x, set_y);
                lock = true;
            }
            if (framecounter > set_frame + 3 && lock && !track_intf.locked) {
                lostframe = framecounter;
                std::cout << "Track lost at " << lostframe << std::endl;
                auto endTime = std::chrono::steady_clock::now();
                auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
                std::cout << "Time: " << elapsedTime.count() << " ms" << std::endl;
                std::cout << "FPS: " << (lostframe - set_frame) / (elapsedTime.count() / 1000.0) << std::endl;
                lock = false;
                global_running.store(false);

            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

    }

    if (global_debug_print) {std::cout << "input thread finished" << std::endl;}
    if (!finish_error) {return 0;}
    else {return 1;}
}