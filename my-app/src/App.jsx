import React, { useState } from 'react';
import { BrowserRouter as Router, Routes, Route, Navigate } from 'react-router-dom';
import Login from './component/Login';
import Register from './component/Register';
import Home from './component/home';
import Chat from './component/Chat';
import './App.css';
import Notifications from './component/Notification';
function ProtectedRoute({ isLoggedIn, children }) {
  return isLoggedIn ? children : <Navigate to="/login" />;
}

function App() {
  const [isLoggedIn, setIsLoggedIn] = useState(() => {
    // Optional: Check localStorage for persisted login state
    return localStorage.getItem('isLoggedIn') === 'true';
  });
  const [username, setUsername] = useState(() => {
    // Optional: Get username from localStorage if available
    return localStorage.getItem('username') || '';
  });

  // Update localStorage when state changes
  React.useEffect(() => {
    localStorage.setItem('isLoggedIn', isLoggedIn);
    localStorage.setItem('username', username);
  }, [isLoggedIn, username]);

  return (
    <Router>
      <Routes>
        <Route path="/" element={
          isLoggedIn ? <Navigate to="/home" /> : <Navigate to="/login" />
        } />

        <Route path="/login" element={
          isLoggedIn ? (
            <Navigate to="/home" />
          ) : (
            <Login setIsLoggedIn={setIsLoggedIn} setUsername={setUsername} />
          )
        } />

        <Route path="/register" element={
          isLoggedIn ? (
            <Navigate to="/home" />
          ) : (
            <Register />
          )
        } />

        <Route path="/home" element={
          <ProtectedRoute isLoggedIn={isLoggedIn}>
            <Home username={username} setIsLoggedIn={setIsLoggedIn} setUsername={setUsername} />
          </ProtectedRoute>
        } />

       
          <Route path="/" element={<Home />} />
          <Route path="/chat" element={<Chat />} />
          <Route path="/notifications" element={<Notifications />} />
          </Routes>
    </Router>
  );
}

export default App;