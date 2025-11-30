#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <random>
#include <atomic>
#include <chrono>
#include <cstdlib> // Required for std::system
#include "../include/sha256.h"

#pragma comment(lib, "ws2_32.lib")

#define CLIENT_PORT 8080
#define PHONE_PORT 8081

class AuthServer {
private:
    SOCKET clientSocket;
    int currentSessionCode;
    std::atomic<bool> isAuthenticated;
    std::atomic<bool> isSessionActive;
    std::atomic<int> failedAttempts;
    std::string detectedLocation;
    std::string userEmail;

    std::chrono::steady_clock::time_point sessionStartTime;
    std::atomic<bool> clientConnected;

    static int generateCode() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> distr(10, 99);
        return distr(gen);
    }

    static std::string fetchRealLocation() {
        std::cout << "[GeoIP] Connecting to location services...\n";
        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        struct hostent *host = gethostbyname("ip-api.com");
        if (host == nullptr) return "Unknown Location";

        struct sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(80);
        memcpy(&serverAddr.sin_addr.s_addr, host->h_addr, host->h_length);

        if (connect(sock, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) return "Unknown Location";

        std::string request = "GET /line/?fields=city,country HTTP/1.1\r\nHost: ip-api.com\r\nConnection: close\r\n\r\n";
        send(sock, request.c_str(), static_cast<int>(request.length()), 0);

        char buffer[4096];
        std::string response;
        int bytes;
        while ((bytes = recv(sock, buffer, 4096, 0)) > 0) response.append(buffer, bytes);
        closesocket(sock);

        size_t bodyPos = response.find("\r\n\r\n");
        if (bodyPos != std::string::npos) {
            std::string loc = response.substr(bodyPos + 4);
            std::erase(loc, '\n');
            return loc;
        }
        return "Unknown Location";
    }

    // --- REAL EMAIL SENDER (PYTHON RELAY) ---
    void sendEmailAlert(const std::string& type) {
        if (userEmail.empty()) return;

        std::cout << "\n[SMTP] Invoking Python Relay for '" << type << "' alert...\n";

        // FIX: Use "../email_sender.py" to look in the project root, not the build folder
        std::string command = "python ../email_sender.py " + type + " " + userEmail + " \"" + detectedLocation + "\"";

        // DEBUG: Print the command being run
        std::cout << "[Debug] Executing: " << command << "\n";

        // Execute system command
        int result = std::system(command.c_str());

        if (result == 0) {
            std::cout << "[SMTP] Python script executed successfully.\n";
        } else {
            std::cerr << "[SMTP] Error executing Python script. Exit Code: " << result << "\n";
            std::cerr << "[Hint] If Exit Code is 1: Check secrets.py location or python PATH.\n";
        }
    }

    [[nodiscard]] std::string getWebPage(bool isSuccess, int remainingTime, const std::string& errorMsg = "") const {
        std::string html = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
               "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"
               "<style>"
               "body { background-color: #f3f2f1; font-family: 'Segoe UI', sans-serif; height: 100vh; margin: 0; display: flex; justify-content: center; align-items: center; }"
               ".card { background: white; padding: 40px; border-radius: 8px; box-shadow: 0 4px 20px rgba(0,0,0,0.1); width: 90%; max-width: 360px; text-align: left; }"
               ".logo { color: #0078d4; font-weight: 600; margin-bottom: 20px; display: block; font-size: 18px; }"
               "h2 { font-size: 24px; color: #201f1e; margin: 0 0 10px 0; font-weight: 600; }"
               ".location { font-size: 14px; color: #605e5c; margin-bottom: 20px; display: flex; align-items: center; background: #f3f2f1; padding: 10px; border-radius: 4px; }"
               ".timer { font-size: 14px; color: #d13438; font-weight: bold; margin-bottom: 20px; text-align: right; }"
               ".error { color: #d13438; font-weight: bold; margin-bottom: 15px; font-size: 14px; background: #fde7e9; padding: 10px; border-radius: 4px; text-align: center; }"
               "input[type='number'] { width: 100%; padding: 15px; font-size: 28px; border: 1px solid #8a8886; border-radius: 2px; margin-bottom: 20px; text-align: center; letter-spacing: 5px; box-sizing: border-box; }"
               ".btn { width: 100%; padding: 12px; background-color: #0078d4; color: white; border: none; border-radius: 2px; font-size: 16px; font-weight: 600; cursor: pointer; }"
               ".btn-clear { display:block; text-align:center; margin-top:15px; text-decoration:none; color:#666; font-size:14px; }"
               "</style>"
               "<script>"
               "var seconds = " + std::to_string(remainingTime) + ";"
               "function timer() {"
               "  if(seconds <= 0) {"
               "    document.getElementById('t').innerHTML = 'Expired';"
               "    document.querySelector('input').disabled = true;"
               "    document.querySelector('.btn').disabled = true;"
               "    document.querySelector('.btn').style.background = '#ccc';"
               "    return;"
               "  }"
               "  document.getElementById('t').innerHTML = seconds + 's remaining';"
               "  seconds--;"
               "}"
               "setInterval(timer, 1000);"
               "</script>"
               "</head><body><div class='card'>";

        if (isSuccess) {
            html += "<span class='logo'>SecureAuth</span><h2>Approved</h2><p style='color:#107c10;'>Sign-in successful.</p>";
        } else {
            html += "<span class='logo'>SecureAuth</span>"
                   "<h2>Approve sign-in?</h2>"
                   "<div class='location'>&#127757; Attempt from: <strong>" + detectedLocation + "</strong></div>";
            html += "<div class='timer' id='t'>Syncing...</div>";
            if (!errorMsg.empty()) {
                html += "<div class='error'>&#9888; " + errorMsg + "</div>";
            }
            html += "<p style='font-size:14px; color:#323130;'>Enter the number shown on your screen.</p>"
                   "<form action='/check' method='GET'>"
                   "<input type='number' name='code' placeholder='--' autocomplete='off' pattern='[0-9]*' inputmode='numeric' required autofocus>"
                   "<input type='submit' value='Approve' class='btn'>"
                   "</form>";
            if (!errorMsg.empty()) {
                html += "<a href='/' class='btn-clear'>Clear & Refresh</a>";
            }
        }
        html += "</div></body></html>";
        return html;
    }

public:
    AuthServer() : clientSocket(INVALID_SOCKET), isAuthenticated(false), isSessionActive(true), failedAttempts(0), clientConnected(false) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) exit(1);
        currentSessionCode = generateCode();
        detectedLocation = fetchRealLocation();
        std::cout << "[System] Server initialized at: " << detectedLocation << "\n";
    }

    void startPhoneListener() {
        struct sockaddr_in webAddr{};
        webAddr.sin_family = AF_INET;
        webAddr.sin_addr.s_addr = INADDR_ANY;
        webAddr.sin_port = htons(PHONE_PORT);

        SOCKET webListener = socket(AF_INET, SOCK_STREAM, 0);
        if (bind(webListener, reinterpret_cast<struct sockaddr*>(&webAddr), sizeof(webAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed (Phone Listener). Error: " << WSAGetLastError() << "\n";
            closesocket(webListener);
            return;
        }
        listen(webListener, 1);

        std::cout << "[System] 2FA Web Interface Active on Port " << PHONE_PORT << "\n";

        while (isSessionActive) {
            int len = sizeof(struct sockaddr_in);
            SOCKET phone = accept(webListener, nullptr, &len);
            if (phone == INVALID_SOCKET) continue;

            char buffer[4096] = {0};
            recv(phone, buffer, 4096, 0);
            std::string request(buffer);

            if (request.find("GET /favicon.ico") != std::string::npos) {
                std::string notFound = "HTTP/1.1 404 Not Found\r\n\r\n";
                send(phone, notFound.c_str(), static_cast<int>(notFound.length()), 0);
                closesocket(phone);
                continue;
            }

            int remaining = 0;
            if (clientConnected) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - sessionStartTime).count();
                remaining = 30 - static_cast<int>(elapsed);
                if (remaining < 0) remaining = 0;
            }

            bool isSubmission = (request.find("GET /check") != std::string::npos);

            if (!clientConnected) {
                 std::string page = getWebPage(false, 0, "Waiting for Desktop App...");
                 send(phone, page.c_str(), static_cast<int>(page.length()), 0);
            }
            else if (!isSessionActive || remaining == 0) {
                if (failedAttempts >= 3) {
                    std::string page = getWebPage(false, 0, "ACCOUNT LOCKED: Too many failed attempts.");
                    send(phone, page.c_str(), static_cast<int>(page.length()), 0);
                } else {
                    std::string page = getWebPage(false, 0, "Session Expired. Restart Client.");
                    send(phone, page.c_str(), static_cast<int>(page.length()), 0);
                }
            }
            else if (isSubmission) {
                if (request.find("code=" + std::to_string(currentSessionCode)) != std::string::npos) {
                    isAuthenticated = true;
                    std::string page = getWebPage(true, remaining);
                    send(phone, page.c_str(), static_cast<int>(page.length()), 0);
                } else {
                    if (request.find("code=") != std::string::npos) {
                        failedAttempts++;
                        if (failedAttempts >= 3) {
                             isSessionActive = false;
                             sendEmailAlert("CRITICAL");
                             std::string page = getWebPage(false, 0, "ACCOUNT LOCKED: Too many failed attempts.");
                             send(phone, page.c_str(), static_cast<int>(page.length()), 0);
                        } else {
                             std::string page = getWebPage(false, remaining, "Incorrect Code. Try Again.");
                             send(phone, page.c_str(), static_cast<int>(page.length()), 0);
                        }
                    } else {
                        std::string page = getWebPage(false, remaining);
                        send(phone, page.c_str(), static_cast<int>(page.length()), 0);
                    }
                }
            } else {
                std::string page = getWebPage(false, remaining);
                send(phone, page.c_str(), static_cast<int>(page.length()), 0);
            }
            closesocket(phone);
        }
        closesocket(webListener);
    }

    void startClientListener() {
        struct sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(CLIENT_PORT);

        SOCKET serverListener = socket(AF_INET, SOCK_STREAM, 0);
        if (bind(serverListener, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed (Client Listener). Error: " << WSAGetLastError() << "\n";
            closesocket(serverListener);
            return;
        }
        listen(serverListener, 1);

        std::cout << "[System] Waiting for User Application...\n";
        clientSocket = accept(serverListener, nullptr, nullptr);

        char emailBuffer[1024] = {0};
        int bytes = recv(clientSocket, emailBuffer, 1024, 0);
        if (bytes > 0) {
            userEmail = std::string(emailBuffer, bytes);
            std::cout << "[Auth] Request from user: " << userEmail << "\n";
        }

        sessionStartTime = std::chrono::steady_clock::now();
        clientConnected = true;

        std::string msg = "CODE:" + std::to_string(currentSessionCode);
        send(clientSocket, msg.c_str(), static_cast<int>(msg.length()), 0);

        std::cout << "[Auth] Waiting for approval (30s Timeout)...\n";

        bool isTimedOut = false;

        while (!isAuthenticated) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - sessionStartTime).count();

            if (elapsed > 30) {
                isTimedOut = true;
                isSessionActive = false;
                break;
            }
            if (failedAttempts >= 3) {
                isSessionActive = false;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (failedAttempts >= 3) {
             std::cout << "[Auth] SECURITY ALERT: Brute force detected. Session terminated.\n";
             std::string failMsg = "ACCESS_DENIED:SECURITY_ALERT";
             send(clientSocket, failMsg.c_str(), static_cast<int>(failMsg.length()), 0);
        }
        else if (isTimedOut) {
            std::cout << "[Auth] Session EXPIRED. Denying access.\n";
            sendEmailAlert("FAILURE");
            std::string failMsg = "ACCESS_DENIED:TIMEOUT";
            send(clientSocket, failMsg.c_str(), static_cast<int>(failMsg.length()), 0);
        } else {
            isSessionActive = false;
            sendEmailAlert("SUCCESS");
            std::string logData = "Session:" + std::to_string(currentSessionCode) + "|Loc:" + detectedLocation + "|Result:Success";
            std::string secureHash = SHA256::hash(logData);

            std::cout << "\n[AUDIT] Session Verified. Hash: " << secureHash << "\n";
            send(clientSocket, "ACCESS_GRANTED", 14, 0);
        }

        closesocket(clientSocket);
        closesocket(serverListener);
    }

    ~AuthServer() { WSACleanup(); }
};

int main() {
    AuthServer server;
    std::thread phoneThread(&AuthServer::startPhoneListener, &server);
    server.startClientListener();
    if (phoneThread.joinable()) phoneThread.join();
    std::cout << "Server session ended. Press Enter to exit.";
    std::cin.get();
    return 0;
}