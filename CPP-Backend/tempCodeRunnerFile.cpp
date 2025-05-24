#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <winsock2.h>
#include <windows.h> // For Windows threads

#pragma comment(lib, "ws2_32.lib")

class UserDatabase {
private:
    std::unordered_map<std::string, std::string> users;
    const std::string filename = "users.txt";

public:
    UserDatabase() {
        loadUsers();
    }

    void loadUsers() {
        std::ifstream file(filename);
        if (!file) return;

        std::string line;
        while (std::getline(file, line)) {
            size_t delimiter = line.find(',');
            if (delimiter != std::string::npos) {
                std::string username = line.substr(0, delimiter);
                std::string password = line.substr(delimiter + 1);
                users[username] = password;
            }
        }
    }

    bool registerUser(const std::string& username, const std::string& password) {
        if (users.find(username) != users.end()) {
            return false;
        }

        std::ofstream file(filename, std::ios::app);
        if (!file) return false;

        file << username << "," << password << "\n";
        users[username] = password;
        return true;
    }

    bool authenticate(const std::string& username, const std::string& password) {
        auto it = users.find(username);
        return it != users.end() && it->second == password;
    }
};

class HTTPServer {
private:
    SOCKET serverSocket;
    UserDatabase userDB;
    WSADATA wsaData;

    struct ThreadParams {
        HTTPServer* server;
        SOCKET clientSocket;
    };

    static DWORD WINAPI ClientThread(LPVOID lpParam) {
        ThreadParams* params = static_cast<ThreadParams*>(lpParam);
        params->server->handleClient(params->clientSocket);
        delete params;
        return 0;
    }

    void sendResponse(SOCKET clientSocket, int statusCode, const std::string& body) {
        std::string statusText;
        switch (statusCode) {
            case 200: statusText = "OK"; break;
            case 400: statusText = "Bad Request"; break;
            case 401: statusText = "Unauthorized"; break;
            case 404: statusText = "Not Found"; break;
            default: statusText = "Unknown"; break;
        }

        std::string response = 
            "HTTP/1.1 " + std::to_string(statusCode) + " " + statusText + "\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + std::to_string(body.length()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" + body;

        send(clientSocket, response.c_str(), response.length(), 0);
    }

    void handleClient(SOCKET clientSocket) {
        char buffer[4096];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesReceived <= 0) {
            closesocket(clientSocket);
            return;
        }

        buffer[bytesReceived] = '\0';
        std::string request(buffer);

        if (request.find("POST /api/register") != std::string::npos) {
            handleRegister(clientSocket, request);
        } 
        else if (request.find("POST /api/login") != std::string::npos) {
            handleLogin(clientSocket, request);
        }
        else if (request.find("OPTIONS") != std::string::npos) {
            sendResponse(clientSocket, 204, "");
        }
        else {
            sendResponse(clientSocket, 404, "{\"message\":\"Endpoint not found\"}");
        }

        closesocket(clientSocket);
    }

    void handleRegister(SOCKET clientSocket, const std::string& request) {
        try {
            size_t contentStart = request.find("\r\n\r\n") + 4;
            std::string jsonData = request.substr(contentStart);
            
            size_t userPos = jsonData.find("\"username\":\"");
            size_t passPos = jsonData.find("\"password\":\"");
            
            if (userPos == std::string::npos || passPos == std::string::npos) {
                throw std::runtime_error("Invalid JSON format");
            }

            std::string username = jsonData.substr(userPos + 12, 
                jsonData.find("\"", userPos + 12) - (userPos + 12));
            std::string password = jsonData.substr(passPos + 12, 
                jsonData.find("\"", passPos + 12) - (passPos + 12));

            if (!userDB.registerUser(username, password)) {
                throw std::runtime_error("Username already exists");
            }

            sendResponse(clientSocket, 200, "{\"message\":\"Registration successful\"}");
        }
        catch (const std::exception& e) {
            sendResponse(clientSocket, 400, std::string("{\"message\":\"") + e.what() + "\"}");
        }
    }

    void handleLogin(SOCKET clientSocket, const std::string& request) {
        try {
            size_t contentStart = request.find("\r\n\r\n") + 4;
            std::string jsonData = request.substr(contentStart);
            
            size_t userPos = jsonData.find("\"username\":\"");
            size_t passPos = jsonData.find("\"password\":\"");
            
            if (userPos == std::string::npos || passPos == std::string::npos) {
                throw std::runtime_error("Invalid JSON format");
            }

            std::string username = jsonData.substr(userPos + 12, 
                jsonData.find("\"", userPos + 12) - (userPos + 12));
            std::string password = jsonData.substr(passPos + 12, 
                jsonData.find("\"", passPos + 12) - (passPos + 12));

            if (userDB.authenticate(username, password)) {
                sendResponse(clientSocket, 200, "{\"message\":\"Login successful\", \"success\":true}");
            } else {
                sendResponse(clientSocket, 401, "{\"message\":\"Invalid credentials\", \"success\":false}");
            }
        }
        catch (const std::exception& e) {
            sendResponse(clientSocket, 400, std::string("{\"message\":\"") + e.what() + "\"}");
        }
    }

public:
    HTTPServer() : serverSocket(INVALID_SOCKET) {}

    ~HTTPServer() {
        if (serverSocket != INVALID_SOCKET) {
            closesocket(serverSocket);
        }
        WSACleanup();
    }

    bool start(int port) {
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed" << std::endl;
            return false;
        }

        serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
            return false;
        }

        int enable = 1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable));

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
            return false;
        }

        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
            return false;
        }

        std::cout << "Server running on port " << port << "..." << std::endl;
        return true;
    }

    void run() {
        while (true) {
            sockaddr_in clientAddr;
            int clientAddrSize = sizeof(clientAddr);
            SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
            
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
                continue;
            }

            // Create thread parameters
            ThreadParams* params = new ThreadParams{this, clientSocket};
            
            // Create Windows thread
            HANDLE hThread = CreateThread(
                NULL,                   // Default security attributes
                0,                      // Default stack size
                ClientThread,            // Thread function
                params,                 // Parameters
                0,                      // Default creation flags
                NULL                    // Don't need thread ID
            );
            
            if (hThread == NULL) {
                std::cerr << "CreateThread failed: " << GetLastError() << std::endl;
                delete params;
                closesocket(clientSocket);
            } else {
                CloseHandle(hThread); // We don't need the handle
            }
        }
    }
};

int main() {
    HTTPServer server;
    if (server.start(8080)) {
        server.run();
    }
    return 0;
}