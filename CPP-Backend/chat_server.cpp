#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <process.h>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cctype>

#pragma comment(lib, "ws2_32.lib")

CRITICAL_SECTION cs;

struct User {
    std::string username;
    std::string password;
};

struct Message {
    std::string sender;
    std::string receiver;
    std::string content;
    std::string timestamp;
};

std::vector<User> users;
std::vector<Message> messages;
std::unordered_map<std::string, SOCKET> active_users;

void loadUsers() {
    std::cout << "Loading users from file...\n";
    std::ifstream file("users.txt");
    if (!file.is_open()) {
        std::cerr << "Warning: users.txt not found. Creating empty user list.\n";
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        size_t comma = line.find(',');
        if (comma != std::string::npos) {
            User user;
            user.username = line.substr(0, comma);
            user.password = line.substr(comma + 1);
            users.push_back(user);
            std::cout << "Loaded user: " << user.username << "\n";
        }
    }
    std::cout << "Total users loaded: " << users.size() << "\n";
}

void loadMessages() {
    std::cout << "Loading messages...\n";
    std::ifstream file("messages.txt");
    if (!file.is_open()) {
        std::cerr << "Warning: messages.txt not found. Creating empty message list.\n";
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string part;
        
        while (std::getline(ss, part, '|')) {
            parts.push_back(part);
        }
        
        if (parts.size() == 4) {
            Message msg;
            msg.sender = parts[0];
            msg.receiver = parts[1];
            msg.timestamp = parts[2];
            msg.content = parts[3];
            messages.push_back(msg);
            std::cout << "Loaded message: " << msg.sender << "->" << msg.receiver 
                      << ": " << msg.content << " at " << msg.timestamp << "\n";
        } else {
            std::cerr << "Invalid message format (expected 4 parts, got " << parts.size() << "): " << line << "\n";
        }
    }
    std::cout << "Total messages loaded: " << messages.size() << "\n";
}

void saveMessage(const Message& msg) {
    EnterCriticalSection(&cs);
    std::cout << "Saving message: " << msg.sender << "|" << msg.receiver << "|" 
              << msg.timestamp << "|" << msg.content << "\n";
    
    std::ofstream file("messages.txt", std::ios::app);
    if (file.is_open()) {
        file << msg.sender << "|" << msg.receiver << "|" 
             << msg.timestamp << "|" << msg.content << "\n";
        file.close();
    } else {
        std::cerr << "Error: Could not open messages.txt for writing\n";
    }
    LeaveCriticalSection(&cs);
}

std::string getCurrentTime() {
    time_t now = time(0);
    tm* localtm = localtime(&now);
    if (localtm == nullptr) {
        return "1970-01-01 00:00:00";
    }
    char buffer[80];
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", localtm);
    return std::string(buffer);
}

std::string urlDecode(const std::string &str) {
    std::string result;
    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] == '+') {
            result += ' ';
        } else if (str[i] == '%' && i + 2 < str.size()) {
            int val;
            std::stringstream ss(str.substr(i + 1, 2));
            ss >> std::hex >> val;
            result += static_cast<char>(val);
            i += 2;
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string parseQueryParam(const std::string& query, const std::string& param) {
    size_t param_pos = query.find(param + "=");
    if (param_pos == std::string::npos) return "";
    
    size_t value_start = param_pos + param.length() + 1;
    size_t value_end = query.find('&', value_start);
    if (value_end == std::string::npos) {
        return urlDecode(query.substr(value_start));
    }
    return urlDecode(query.substr(value_start, value_end - value_start));
}
// std::string parseQueryParam(const std::string& query, const std::string& param) {
//     return "Hello Everyone";
// }


void testParseQueryParam() {
    std::string test = "GET /getmessages?user1=Alice&user2=Bob HTTP/1.1";
    size_t qmark = test.find('?');
    std::string query = test.substr(qmark + 1);
    
    std::cout << "user1=" << parseQueryParam(query, "user1") << "\n";
    std::cout << "user2=" << parseQueryParam(query, "user2") << "\n";
}


std::string handleRequest(const std::string& request) {
    std::cout << "\nReceived request:\n" << request << "\n";

    std::stringstream response;

    // Add CORS headers to all responses
    response << "HTTP/1.1 200 OK\r\n"
         << "Access-Control-Allow-Origin: *\r\n"
         << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
         << "Access-Control-Allow-Headers: Content-Type\r\n"
         << "Content-Type: text/plain\r\n"
         << "\r\n";

    if (request.find("GET /getusers") != std::string::npos) {
        std::cout << "Handling /getusers request\n";
        response << "Content-Type: text/plain\r\n\r\n";
        EnterCriticalSection(&cs);
        for (const auto& user : users) {
            response << user.username << "\n";
            std::cout << "Sending user: " << user.username << "\n";
        }
        LeaveCriticalSection(&cs);
    }
    else if (request.find("GET /getmessages") != std::string::npos) {
    size_t qmark = request.find('?');
    std::string query = qmark != std::string::npos ? request.substr(qmark + 1) : "";
    std::string user1 = parseQueryParam(query, "user1");
    std::string user2 = parseQueryParam(query, "user2");

    response << "Content-Type: text/plain\r\n\r\n";
    EnterCriticalSection(&cs);
    for (const auto& msg : messages) {
        if ((msg.sender == user1 && msg.receiver == user2) ||
            (msg.sender == user2 && msg.receiver == user1)) {
            // Send in format: sender|content|timestamp
            response << msg.sender << "|" << msg.content << "|" << msg.timestamp << "\n";
        }
    }
    LeaveCriticalSection(&cs);
}
// Add login/logout endpoints
else if (request.find("GET /login") != std::string::npos) {
    size_t qmark = request.find('?');
    std::string query = qmark != std::string::npos ? request.substr(qmark + 1) : "";
    std::string username = parseQueryParam(query, "username");
    
    EnterCriticalSection(&cs);
    active_users[username] = client_socket; // You'll need to pass client_socket
    LeaveCriticalSection(&cs);
    
    response << "Content-Type: text/plain\r\n\r\nOK";
}
else if (request.find("GET /logout") != std::string::npos) {
    size_t qmark = request.find('?');
    std::string query = qmark != std::string::npos ? request.substr(qmark + 1) : "";
    std::string username = parseQueryParam(query, "username");
    
    EnterCriticalSection(&cs);
    active_users.erase(username);
    LeaveCriticalSection(&cs);
    
    response << "Content-Type: text/plain\r\n\r\nOK";
}
    else if (request.find("GET /send") != std::string::npos) {
        std::cout << "Handling /send request\n";
        size_t qmark = request.find('?');
        std::string query = (qmark != std::string::npos) ? request.substr(qmark + 1) : "";
        
        size_t space_pos = query.find(' ');
        if (space_pos != std::string::npos) {
            query = query.substr(0, space_pos);
        }

        std::string from = parseQueryParam(query, "from");
        std::string to = parseQueryParam(query, "to");
        std::string content = parseQueryParam(query, "content");

        std::cout << "Message from " << from << " to " << to << ": " << content << "\n";

        if (!from.empty() && !to.empty() && !content.empty()) {
            Message msg{from, to, content, getCurrentTime()};
            EnterCriticalSection(&cs);
            messages.push_back(msg);
            saveMessage(msg);
            
            if (active_users.count(to)) {
                std::string notify = "NEWMSG " + from + " " + content + "\n";
                send(active_users[to], notify.c_str(), notify.size(), 0);
            }
            LeaveCriticalSection(&cs);
            response << "Content-Type: text/plain\r\n\r\nOK";
        } else {
            response << "HTTP/1.1 400 Bad Request\r\n\r\nMissing parameters";
        }
    }
    else {
        std::cout << "Unknown request\n";
        response << "HTTP/1.1 404 Not Found\r\n\r\nInvalid endpoint";
    }

    std::string responseStr = response.str();
    std::cout << "Sending response:\n" << responseStr << "\n";
    return responseStr;
}

unsigned __stdcall handleClient(void* lpParam) {
    SOCKET client_socket = (SOCKET)lpParam;
    char buffer[4096] = {0};
    
    int valread = recv(client_socket, buffer, sizeof(buffer), 0);
    if (valread <= 0) {
        std::cout << "Client disconnected or error occurred\n";
        closesocket(client_socket);
        return 0;
    }

    std::string request(buffer, valread);
    std::cout << "Raw request from client:\n" << request << "\n";

    size_t end_of_request = request.find("\r\n");
    if (end_of_request != std::string::npos) {
        request = request.substr(0, end_of_request);
    }

    std::string response = handleRequest(request);
    
    send(client_socket, response.c_str(), response.size(), 0);
    closesocket(client_socket);
    
    return 0;
}

int main() {
    InitializeCriticalSection(&cs);
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    loadUsers();
    loadMessages();

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        std::cerr << "setsockopt failed: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8000);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "listen failed: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    std::cout << "Server successfully started on port 8000\n";
    std::cout << "Loaded " << users.size() << " users and " << messages.size() << " messages\n";

    while (true) {
        SOCKET new_socket = accept(server_fd, nullptr, nullptr);
        if (new_socket == INVALID_SOCKET) {
            std::cerr << "accept failed: " << WSAGetLastError() << "\n";
            continue;
        }

        HANDLE hThread = (HANDLE)_beginthreadex(
            nullptr, 0, &handleClient, (void*)new_socket, 0, nullptr);
        if (hThread == nullptr) {
            std::cerr << "Failed to create thread\n";
            closesocket(new_socket);
        } else {
            CloseHandle(hThread);
        }
    }

    closesocket(server_fd);
    DeleteCriticalSection(&cs);
    WSACleanup();
    return 0;
}