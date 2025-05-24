# Sanskar - A Social Media Platform 👥

![Project Banner](/client/public/banner.png) <!-- Add a banner image if available -->

A modern social media application with React.js frontend and high-performance C++ backend, connected via WinSockets for real-time communication.

## 🌟 Features

- **User Authentication** (Register/Login)
- **Real-time Chat** with other users
- **Notifications** system
- **Responsive UI** built with React
- **High-performance backend** in C++
- **JSON API** for frontend-backend communication
- **Windows Sockets** for network connectivity

## 🏗️ Architecture


sanskar-a-social-media/
├── client/ # React Frontend
│ ├── public/ # Static assets
│ └── src/ # React components
│ ├── auth/ # Authentication
│ │ ├── Login.jsx
│ │ └── Register.jsx
│ ├── chat/ # Chat functionality
│ │ └── Chat.jsx
│ ├── home/ # Main dashboard
│ │ └── Home.jsx
│ └── notifications/ # Notifications
│ └── Notification.jsx
└── server/ # C++ Backend
├── include/ # Header files
├── src/ # Source files
└── build/ # Build output




## 🚀 Getting Started

### Prerequisites

- Node.js (for React frontend)
- C++ compiler (MSVC or MinGW for Windows)
- CMake (for building C++ server)

### Installation

1. **Frontend Setup**:
```bash
cd my-app
npm install
npm run dev

1. **Backend Setup**:

cd cpp-backend
mkdir build && cd build
cmake ..
cmake --build .
g++ server.cpp -o server.exe -lws2_32
./server.exe

**Frontend: **

React.js

React Router

Axios (for HTTP requests)

Material-UI/SASS (for styling)

** Backend: **

C++17

WinSock2 (for networking)

JSON for Modern C++ (nlohmann/json)


Project Link: https://github.com/Ritikraj11/sanskar-a-social-media