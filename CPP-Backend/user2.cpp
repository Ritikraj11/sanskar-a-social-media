// user2.cpp
#include <winsock2.h>
#include <iostream>
#include <fstream>
#include <string>
#include <conio.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

class ChatUser {
protected:
    string username, password;
public:
    virtual void login() = 0;
    bool authenticate() {
        ifstream file("users.txt");
        string u, p;
        while (getline(file, u, ',') && getline(file, p)) {
            if (u == username && p == password) return true;
        }
        return false;
    }
    string getUsername() const {
        return username;
    }
};

class User2 : public ChatUser {
private:
    SOCKET sock;
    string peerUsername;
public:
    void login() override {
        cout << "User2 - Enter your username: ";
        cin >> username;
        cout << "Enter your password: ";
        cin >> password;
        cin.ignore();
    }

    bool connectToUser1() {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);

        sock = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(12345);
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            cout << "[User2] Failed to connect.\n";
            return false;
        }
        return true;
    }

    void chat() {
        string creds = username + ":" + password;
        send(sock, creds.c_str(), creds.size(), 0);

        char buffer[1024];
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        buffer[bytes] = '\0';

        if (string(buffer) == "AUTH_FAIL") {
            cout << "Authentication failed by server.\n";
            return;
        }

        // Receive peer username
        bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        buffer[bytes] = '\0';
        peerUsername = buffer;

        cout << "\n✅ Both users authenticated. Start chatting.\n\nYou: ";
        fd_set readfds;

        while (true) {
            FD_ZERO(&readfds);
            FD_SET(sock, &readfds);

            timeval tv{};
            tv.tv_sec = 0;
            tv.tv_usec = 100000;

            int activity = select(0, &readfds, NULL, NULL, &tv);
            if (activity > 0 && FD_ISSET(sock, &readfds)) {
                int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
                if (bytes <= 0) break;
                buffer[bytes] = '\0';
                cout << "\n" << peerUsername << ": " << buffer << "\nYou: ";
                cout.flush();
            }

            if (_kbhit()) {
                string msg;
                getline(cin, msg);
                send(sock, msg.c_str(), msg.size(), 0);
            }
        }

        closesocket(sock);
        WSACleanup();
    }
};

int main() {
    User2 u2;
    u2.login();
    if (!u2.authenticate()) {
        cout << "Login failed.\n";
        return 1;
    }
    if (u2.connectToUser1()) {
        u2.chat();
    }
    return 0;
}
