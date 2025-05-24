#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <direct.h>
#include <ctime>
#include <algorithm>
#include <errno.h>


#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 65536

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

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
        if (!file) {
            std::cerr << "Note: Could not open users file (it might not exist yet)" << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            size_t delimiter = line.find(',');
            if (delimiter != std::string::npos) {
                users[line.substr(0, delimiter)] = line.substr(delimiter + 1);
            }
        }
    }

    bool registerUser(const std::string& username, const std::string& password) {
        if (users.count(username)) return false;

        std::ofstream file(filename, std::ios::app);
        if (!file) {
            std::cerr << "Error: Could not open users file for writing" << std::endl;
            return false;
        }

        file << username << "," << password << "\n";
        users[username] = password;
        return true;
    }

    bool authenticate(const std::string& username, const std::string& password) {
        return users.count(username) && users[username] == password;
    }
};

struct Post {
    std::string author;
    std::string content;
    std::string imageUrl;
    std::string timestamp;
};

class HTTPServer {
private:
    SOCKET serverSocket;
    UserDatabase userDB;
    WSADATA wsaData;
    std::vector<Post> posts;

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
            case 201: statusText = "Created"; break;
            case 204: statusText = "No Content"; break;
            case 400: statusText = "Bad Request"; break;
            case 401: statusText = "Unauthorized"; break;
            case 404: statusText = "Not Found"; break;
            case 500: statusText = "Internal Server Error"; break;
            default: statusText = "Unknown"; break;
        }

        std::string response =
            "HTTP/1.1 " + std::to_string(statusCode) + " " + statusText + "\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n\r\n" + body;

        send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
    }

    std::string getCurrentTimestamp() {
        time_t now = time(nullptr);
        char buf[80];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return std::string(buf);
    }

    int getContentLength(const std::string& request) {
        size_t pos = request.find("Content-Length:");
        if (pos == std::string::npos) return -1;
        pos += 15;
        while (pos < request.size() && (request[pos] == ' ' || request[pos] == '\t')) pos++;
        size_t end = request.find("\r\n", pos);
        if (end == std::string::npos) return -1;
        try {
            return std::stoi(request.substr(pos, end - pos));
        } catch (...) {
            return -1;
        }
    }

    std::string getBoundary(const std::string& request) {
        const std::string header = "Content-Type: multipart/form-data; boundary=";
        size_t pos = request.find(header);
        if (pos == std::string::npos) return "";
        size_t start = pos + header.length();
        size_t end = request.find("\r\n", start);
        std::string boundary = request.substr(start, end - start);
        if (boundary.substr(0, 2) != "--") {
            boundary = "--" + boundary;
        }
        return boundary;
    }

    std::string extractFieldData(const std::string& body, const std::string& boundary, const std::string& fieldName) {
        std::string searchName = "name=\"" + fieldName + "\"";
        size_t pos = body.find(searchName);
        if (pos == std::string::npos) return "";

        size_t dataStart = body.find("\r\n\r\n", pos);
        if (dataStart == std::string::npos) return "";
        dataStart += 4;

        size_t dataEnd = body.find(boundary, dataStart);
        if (dataEnd == std::string::npos) return "";

        std::string data = body.substr(dataStart, dataEnd - dataStart);

        while (!data.empty() && (data.back() == '\r' || data.back() == '\n')) {
            data.pop_back();
        }

        return data;
    }

    bool extractFileData(const std::string& body, const std::string& boundary, const std::string& fieldName,
                         std::string& outFilename, std::string& outFileData) {
        std::string searchName = "name=\"" + fieldName + "\"; filename=\"";
        size_t pos = body.find(searchName);
        if (pos == std::string::npos) return false;

        size_t filenameStart = pos + searchName.size();
        size_t filenameEnd = body.find("\"", filenameStart);
        if (filenameEnd == std::string::npos) return false;

        outFilename = body.substr(filenameStart, filenameEnd - filenameStart);

        size_t fileDataStart = body.find("\r\n\r\n", filenameEnd);
        if (fileDataStart == std::string::npos) return false;
        fileDataStart += 4;

        size_t fileDataEnd = body.find(boundary, fileDataStart);
        if (fileDataEnd == std::string::npos) return false;

        outFileData = body.substr(fileDataStart, fileDataEnd - fileDataStart);

        if (outFileData.size() >= 2 &&
            outFileData[outFileData.size() - 2] == '\r' &&
            outFileData[outFileData.size() - 1] == '\n') {
            outFileData.resize(outFileData.size() - 2);
        }

        return true;
    }

    bool saveImageFile(const std::string& filename, const std::string& data) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) return false;
        file.write(data.data(), data.size());
        return true;
    }

    bool recvFullRequest(SOCKET clientSocket, std::string& outRequest) {
        char buffer[BUFFER_SIZE];
        int totalReceived = 0;

        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) return false;
        buffer[bytesReceived] = 0;
        outRequest = buffer;

        int contentLength = getContentLength(outRequest);
        if (contentLength <= 0) return true;

        size_t headerEnd = outRequest.find("\r\n\r\n");
        if (headerEnd == std::string::npos) return false;
        size_t bodyStart = headerEnd + 4;

        int bodyBytesReceived = (int)(outRequest.size() - bodyStart);

        while (bodyBytesReceived < contentLength) {
            bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived <= 0) return false;
            outRequest.append(buffer, bytesReceived);
            bodyBytesReceived += bytesReceived;
        }

        return true;
    }

    // --- New: JSON String escape function ---
    std::string escapeJsonString(const std::string& input) {
        std::string output;
        output.reserve(input.size() + 10); // Rough reserve

        for (char c : input) {
            switch (c) {
                case '\"': output += "\\\""; break;
                case '\\': output += "\\\\"; break;
                case '\b': output += "\\b"; break;
                case '\f': output += "\\f"; break;
                case '\n': output += "\\n"; break;
                case '\r': output += "\\r"; break;
                case '\t': output += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        // Control character, use \u00XX
                        char buf[7];
                        snprintf(buf, sizeof(buf), "\\u%04x", c);
                        output += buf;
                    } else {
                        output += c;
                    }
            }
        }

        return output;
    }
    void handleGetImage(SOCKET clientSocket, const std::string& request) {
    size_t pathStart = request.find("GET /uploads/") + 5; // Keep the /uploads/ part
    size_t pathEnd = request.find(" HTTP/1.1");
    if (pathEnd == std::string::npos) {
        sendResponse(clientSocket, 400, "Bad request");
        return;
    }
    
    std::string filePath = request.substr(pathStart, pathEnd - pathStart);
    
    // Security check - prevent directory traversal
    if (filePath.find("..") != std::string::npos) {
        sendResponse(clientSocket, 403, "Forbidden");
        return;
    }
    
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        sendResponse(clientSocket, 404, "Not found");
        return;
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read file data
    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    
    // Determine content type
    std::string contentType = "application/octet-stream";
    if (filePath.find(".jpg") != std::string::npos || filePath.find(".jpeg") != std::string::npos) {
        contentType = "image/jpeg";
    } else if (filePath.find(".png") != std::string::npos) {
        contentType = "image/png";
    }
    
    // Send response
    std::string header = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: " + contentType + "\r\n"
        "Content-Length: " + std::to_string(fileSize) + "\r\n"
        "Connection: close\r\n\r\n";
    
    send(clientSocket, header.c_str(), header.size(), 0);
    send(clientSocket, buffer.data(), fileSize, 0);
}

    void handleClient(SOCKET clientSocket) {
        std::string request;
        if (!recvFullRequest(clientSocket, request)) {
            closesocket(clientSocket);
            return;
        }
        else if (request.find("GET /uploads/") != std::string::npos) {
        handleGetImage(clientSocket, request);
        }

        std::cout << "Received request:\n" << request.substr(0, 500) << "...\n";

        if (request.find("POST /api/register") != std::string::npos) {
            handleRegister(clientSocket, request);
        } else if (request.find("POST /api/login") != std::string::npos) {
            handleLogin(clientSocket, request);
        } else if (request.find("POST /api/posts") != std::string::npos) {
            handleCreatePost(clientSocket, request);
        } else if (request.find("GET /api/posts") != std::string::npos) {
            handleGetPosts(clientSocket);
        } else if (request.find("OPTIONS") != std::string::npos) {
            sendResponse(clientSocket, 204, "");
        } else {
            sendResponse(clientSocket, 404, "{\"error\":\"Endpoint not found\"}");
        }

        closesocket(clientSocket);
    }

    void handleRegister(SOCKET clientSocket, const std::string& request) {
        size_t jsonStart = request.find("\r\n\r\n");
        if (jsonStart == std::string::npos) {
            sendResponse(clientSocket, 400, "{\"error\":\"Bad request\"}");
            return;
        }
        std::string body = request.substr(jsonStart + 4);

        size_t userPos = body.find("\"username\"");
        size_t passPos = body.find("\"password\"");
        if (userPos == std::string::npos || passPos == std::string::npos) {
            sendResponse(clientSocket, 400, "{\"error\":\"Missing username or password\"}");
            return;
        }

        auto extractJsonValue = [](const std::string& str, size_t pos) -> std::string {
            size_t colon = str.find(':', pos);
            if (colon == std::string::npos) return "";
            size_t quote1 = str.find('\"', colon);
            if (quote1 == std::string::npos) return "";
            size_t quote2 = str.find('\"', quote1 + 1);
            if (quote2 == std::string::npos) return "";
            return str.substr(quote1 + 1, quote2 - quote1 - 1);
        };

        std::string username = extractJsonValue(body, userPos);
        std::string password = extractJsonValue(body, passPos);

        if (username.empty() || password.empty()) {
            sendResponse(clientSocket, 400, "{\"error\":\"Empty username or password\"}");
            return;
        }

        if (userDB.registerUser(username, password)) {
            sendResponse(clientSocket, 201, "{\"message\":\"User registered successfully\"}");
        } else {
            sendResponse(clientSocket, 400, "{\"error\":\"User already exists\"}");
        }
    }

    void handleLogin(SOCKET clientSocket, const std::string& request) {
    size_t jsonStart = request.find("\r\n\r\n");
    if (jsonStart == std::string::npos) {
        sendResponse(clientSocket, 400, "{\"success\":false,\"error\":\"Bad request\"}");
        return;
    }
    std::string body = request.substr(jsonStart + 4);

    size_t userPos = body.find("\"username\"");
    size_t passPos = body.find("\"password\"");
    if (userPos == std::string::npos || passPos == std::string::npos) {
        sendResponse(clientSocket, 400, "{\"success\":false,\"error\":\"Missing username or password\"}");
        return;
    }

    auto extractJsonValue = [](const std::string& str, size_t pos) -> std::string {
        size_t colon = str.find(':', pos);
        if (colon == std::string::npos) return "";
        size_t quote1 = str.find('\"', colon);
        if (quote1 == std::string::npos) return "";
        size_t quote2 = str.find('\"', quote1 + 1);
        if (quote2 == std::string::npos) return "";
        return str.substr(quote1 + 1, quote2 - quote1 - 1);
    };

    std::string username = extractJsonValue(body, userPos);
    std::string password = extractJsonValue(body, passPos);

    if (username.empty() || password.empty()) {
        sendResponse(clientSocket, 400, "{\"success\":false,\"error\":\"Empty username or password\"}");
        return;
    }

    if (userDB.authenticate(username, password)) {
        sendResponse(clientSocket, 200, "{\"success\":true,\"message\":\"Login successful\"}");
    } else {
        sendResponse(clientSocket, 401, "{\"success\":false,\"error\":\"Invalid username or password\"}");
    }
}

    void handleCreatePost(SOCKET clientSocket, const std::string& request) {
        std::string boundary = getBoundary(request);
        if (boundary.empty()) {
            sendResponse(clientSocket, 400, "{\"error\":\"Missing boundary in multipart/form-data\"}");
            return;
        }

        size_t bodyStart = request.find("\r\n\r\n");
        if (bodyStart == std::string::npos) {
            sendResponse(clientSocket, 400, "{\"error\":\"Bad request\"}");
            return;
        }

        std::string body = request.substr(bodyStart + 4);

        std::string author = extractFieldData(body, boundary, "author");
        std::string content = extractFieldData(body, boundary, "content");

        std::string filename, fileData;
        bool hasImage = extractFileData(body, boundary, "image", filename, fileData);

        std::string imageUrl;
                    if (hasImage) {
                std::string safeFilename = "uploads/" + filename;
                _mkdir("uploads");
                if (!saveImageFile(safeFilename, fileData)) {
                    sendResponse(clientSocket, 500, "{\"error\":\"Failed to save image\"}");
                    return;
                }
                // Change this line to include the full accessible path
                imageUrl = "/" + safeFilename;  // Now it will be "/uploads/filename.jpg"
            }

        if (author.empty() || content.empty()) {
            sendResponse(clientSocket, 400, "{\"error\":\"Author or content missing\"}");
            return;
        }

        Post newPost{author, content, imageUrl, getCurrentTimestamp()};
        posts.push_back(newPost);

        std::string response = "{";
        response += "\"author\":\"" + escapeJsonString(author) + "\",";
        response += "\"content\":\"" + escapeJsonString(content) + "\",";
        response += "\"imageUrl\":\"" + escapeJsonString(imageUrl) + "\",";
        response += "\"timestamp\":\"" + newPost.timestamp + "\"";
        response += "}";

        sendResponse(clientSocket, 201, response);
    }

    void handleGetPosts(SOCKET clientSocket) {
        std::string json = "[";
        for (size_t i = 0; i < posts.size(); ++i) {
            const Post& p = posts[i];
            json += "{";
            json += "\"author\":\"" + escapeJsonString(p.author) + "\",";
            json += "\"content\":\"" + escapeJsonString(p.content) + "\",";
            json += "\"imageUrl\":\"" + escapeJsonString(p.imageUrl) + "\",";
            json += "\"timestamp\":\"" + p.timestamp + "\"";
            json += "}";
            if (i != posts.size() - 1) json += ",";
        }
        json += "]";
        sendResponse(clientSocket, 200, json);
    }

public:
    HTTPServer() : serverSocket(INVALID_SOCKET) {}

    bool start(const char* ip, unsigned short port) {
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "WSAStartup failed: " << result << std::endl;
            return false;
        }

        serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Failed to create socket: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return false;
        }

        sockaddr_in service{};
        service.sin_family = AF_INET;
        service.sin_addr.s_addr = inet_addr(ip);
        service.sin_port = htons(port);

        if (bind(serverSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
            std::cerr << "bind() failed: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return false;
        }

        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "listen() failed: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return false;
        }

        std::cout << "Server listening on " << ip << ":" << port << std::endl;
        return true;
    }

    void run() {
        while (true) {
            SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "accept() failed: " << WSAGetLastError() << std::endl;
                continue;
            }

            ThreadParams* params = new ThreadParams{this, clientSocket};
            HANDLE threadHandle = CreateThread(nullptr, 0, ClientThread, params, 0, nullptr);
            if (threadHandle) {
                CloseHandle(threadHandle);
            } else {
                std::cerr << "Failed to create client thread: " << GetLastError() << std::endl;
                closesocket(clientSocket);
                delete params;
            }
        }
    }

    ~HTTPServer() {
        if (serverSocket != INVALID_SOCKET) {
            closesocket(serverSocket);
        }
        WSACleanup();
    }
};

int main() {
    HTTPServer server;
    if (!server.start("127.0.0.1", 8080)) {
        return 1;
    }
    server.run();
    return 0;
}
