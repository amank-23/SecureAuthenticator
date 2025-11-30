#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 8080

int main() {
    WSADATA wsa;
    auto s = INVALID_SOCKET;
    struct sockaddr_in server{};
    char buffer[1024] = {0};

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "Failed to initialize Winsock.\n";
        return 1;
    }

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        std::cerr << "Could not create socket.\n";
        WSACleanup();
        return 1;
    }

    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    std::cout << "\n";
    std::cout << "#############################################\n";
    std::cout << "#        CORPORATE LOGIN TERMINAL           #\n";
    std::cout << "#############################################\n\n";

    // --- NEW: EMAIL INPUT ---
    std::string email;
    std::cout << "Enter your Corporate Email: ";
    std::getline(std::cin, email);

    std::cout << "Connecting to Identity Provider...\n";

    if (connect(s, reinterpret_cast<struct sockaddr *>(&server), sizeof(server)) < 0) {
        std::cerr << "Error: Authentication Server is unreachable.\n";
        closesocket(s);
        WSACleanup();
        return 1;
    }

    // --- NEW: SEND EMAIL TO SERVER ---
    // We send the email immediately after connecting so the server knows who we are.
    send(s, email.c_str(), static_cast<int>(email.length()), 0);

    // 1. Receive Code
    int bytesReceived = recv(s, buffer, 1024, 0);
    if (bytesReceived > 0) {
        std::string message(buffer, bytesReceived);

        if (message.find("CODE:") != std::string::npos) {
            std::string code = message.substr(5);

            std::cout << "---------------------------------------------\n";
            std::cout << "   ACTION REQUIRED                           \n";
            std::cout << "---------------------------------------------\n";
            std::cout << "Open your Secure Authenticator app.\n";
            std::cout << "Match the following number:\n\n";
            std::cout << "           [  " << code << "  ]           \n\n";
            std::cout << "Waiting for remote approval (30s limit)...\n";
        }
    }

    // 2. Wait for Approval or Timeout
    memset(buffer, 0, 1024);
    bytesReceived = recv(s, buffer, 1024, 0);

    if (bytesReceived > 0) {
        std::string response(buffer, bytesReceived);

        if (response.find("ACCESS_GRANTED") != std::string::npos) {
            std::cout << "\n";
            std::cout << "*********************************************\n";
            std::cout << "* SUCCESS: IDENTITY VERIFIED.               *\n";
            std::cout << "* Welcome, " << email << "              *\n";
            std::cout << "* Accessing Secure Dashboard...             *\n";
            std::cout << "*********************************************\n";
        }
        else if (response.find("ACCESS_DENIED") != std::string::npos) {
            std::cout << "\n";
            std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
            std::cout << "! ERROR: LOGIN TIMED OUT OR DENIED          !\n";
            std::cout << "! The session has expired.                  !\n";
            std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        }
    } else {
        std::cout << "\n[!] Connection closed by server.\n";
    }

    std::cout << "\nPress Enter to exit...";
    std::cin.get();

    closesocket(s);
    WSACleanup();
    return 0;
}