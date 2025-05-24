#include <winsock2.h>
#include <iostream>
#include <fstream>
#include <string>
#include <conio.h>  // for _kbhit()
#pragma comment(lib, "ws2_32.lib")

using namespace std;

// Abstract base class
class ChatUser {
public:
    string username;
    string password;

    virtual void login() = 0;

    bool authenticate() {
        ifstream file("users.txt");
        string u, p;
        while (getline(file, u, ',') && getline(file, p)) {
            if (u == username && p == password) return true;
        }
        return false;
    }
};

// Derived class for User1
class User1 : public ChatUser {
public:
    void login() override {
        cout << "Enter your username: ";
        cin >> username;
        cout << "Enter your password: ";
        cin >> password;
        cin.ignore(); // clear newline

        if (!authenticate()) {
            cout << "Authentication failed.\n";
            exit(1);
        }
    }
};

// Function to validate user2
bool validateUser2(const string& uname, const string& pwd) {
    ifstream file("users.txt");
    string u, p;
    while (getline(file, u, ',') && getline(file, p)) {
        if (u == uname && p == pwd) return true;
    }
    return false;
}

void handleChat(SOCKET clientSocket, const string& user1Name, const string& user2Name) {
    char buffer[1024];

    cout << "\n✅ Both users authenticated. Start chatting.\n\nYou: ";
    while (true) {
        // Check for message from user2
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int result = select(0, &readfds, NULL, NULL, &tv);
        if (result > 0 && FD_ISSET(clientSocket, &readfds)) {
            int bytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0) break;
            buffer[bytes] = '\0';
            cout << "\n" << user2Name << ": " << buffer << "\nYou: ";
            cout.flush();
        }

        // Check for keyboard input
        if (_kbhit()) {
            string msg;
            getline(cin, msg);
            if (msg == "exit") break;
            send(clientSocket, msg.c_str(), msg.size(), 0);
            cout << "You: ";
        }
    }

    closesocket(clientSocket);
    WSACleanup();
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr{}, clientAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 1);

    cout << "Waiting for User2 to connect...\n";
    int clientLen = sizeof(clientAddr);
    SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
    cout << "User2 connected.\n";

    User1 user1;
    user1.login();

    // Receive credentials from User2
    char creds[1024];
    int bytes = recv(clientSocket, creds, sizeof(creds) - 1, 0);
    creds[bytes] = '\0';

    string received(creds);
    size_t delim = received.find(':');
    string user2name = received.substr(0, delim);
    string user2pass = received.substr(delim + 1);

    if (!validateUser2(user2name, user2pass)) {
        cout << "User2 failed authentication.\n";
        send(clientSocket, "AUTH_FAIL", 9, 0);
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Send OK and user1's username to user2
    send(clientSocket, "OK", 2, 0);
    send(clientSocket, user1.username.c_str(), user1.username.size(), 0);

    handleChat(clientSocket, user1.username, user2name);
    return 0;
}
