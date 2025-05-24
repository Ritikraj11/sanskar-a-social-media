import React, { useState, useEffect, useRef } from 'react';
import './Chat.css';

const Chat = () => {
  const [currentUser, setCurrentUser] = useState('');
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [message, setMessage] = useState('');
  const [messages, setMessages] = useState([]);
  const [users, setUsers] = useState([]);
  const [selectedUser, setSelectedUser] = useState('');
  const [error, setError] = useState('');
  const messagesEndRef = useRef(null);

  const handleLogin = async (e) => {
    e.preventDefault();
    setError('');
    
    try {
      const response = await fetch('http://localhost:8080/api/login', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ username, password }),
      });
      
      const data = await response.json();
      
      if (data.success) {
        setCurrentUser(username);
      } else {
        setError(data.error || 'Login failed');
      }
    } catch (err) {
      setError('Failed to connect to server');
      console.error('Login error:', err);
    }
  };

  const fetchUsers = async () => {
    try {
      const response = await fetch('http://localhost:8080/api/users');
      if (response.ok) {
        const userList = await response.json();
        setUsers(userList.filter(u => u !== currentUser));
      }
    } catch (err) {
      console.error('Failed to fetch users:', err);
    }
  };

  const fetchMessages = async () => {
    if (!currentUser || !selectedUser) return;
    
    try {
      const response = await fetch(
        `http://localhost:8080/api/messages?user1=${currentUser}&user2=${selectedUser}`
      );
      
      if (response.ok) {
        const messages = await response.json();
        setMessages(messages);
      }
    } catch (err) {
      console.error('Fetch error:', err);
    }
  };

  const sendMessage = async () => {
    if (!message.trim() || !selectedUser) return;
    
    try {
      const response = await fetch('http://localhost:8080/api/messages', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          sender: currentUser,
          recipient: selectedUser,
          content: message
        }),
      });
      
      if (response.ok) {
        setMessage('');
        await fetchMessages();
      }
    } catch (err) {
      console.error('Failed to send message:', err);
    }
  };

  useEffect(() => {
    if (currentUser) {
      fetchUsers();
      const userInterval = setInterval(fetchUsers, 5000);
      return () => clearInterval(userInterval);
    }
  }, [currentUser]);

  useEffect(() => {
    if (currentUser && selectedUser) {
      fetchMessages();
      const messageInterval = setInterval(fetchMessages, 2000);
      return () => clearInterval(messageInterval);
    }
  }, [currentUser, selectedUser]);

  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [messages]);

  if (!currentUser) {
    return (
      <div className="login-container">
        <form onSubmit={handleLogin}>
          <h2>Login</h2>
          {error && <div className="error">{error}</div>}
          <input
            type="text"
            placeholder="Username"
            value={username}
            onChange={(e) => setUsername(e.target.value)}
            required
          />
          <input
            type="password"
            placeholder="Password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            required
          />
          <button type="submit">Login</button>
        </form>
      </div>
    );
  }

  return (
    <div className="chat-app">
      <div className="sidebar">
        <div className="header">
          <h3>Logged in as: {currentUser}</h3>
          <button onClick={() => setCurrentUser('')}>Logout</button>
        </div>
        <div className="user-list">
          <h4>Users</h4>
          {users.map((user) => (
            <div
              key={user}
              className={`user ${selectedUser === user ? 'active' : ''}`}
              onClick={() => setSelectedUser(user)}
            >
              {user}
            </div>
          ))}
        </div>
      </div>

      <div className="chat-area">
        {selectedUser ? (
          <>
            <div className="messages">
              {messages.length > 0 ? (
                messages.map((msg, i) => (
                  <div 
                    key={i} 
                    className={`message ${msg.sender === currentUser ? 'sent' : 'received'}`}
                  >
                    <div className="message-content">{msg.content}</div>
                    <div className="message-meta">
                      <span className="sender">{msg.sender}</span>
                      <span className="timestamp">
                        {new Date(msg.timestamp).toLocaleString()}
                      </span>
                    </div>
                  </div>
                ))
              ) : (
                <div className="no-messages">
                  No messages yet. Start the conversation!
                </div>
              )}
              <div ref={messagesEndRef} />
            </div>

            <div className="message-input">
              <input
                type="text"
                value={message}
                onChange={(e) => setMessage(e.target.value)}
                onKeyPress={(e) => e.key === 'Enter' && sendMessage()}
                placeholder="Type a message..."
              />
              <button onClick={sendMessage}>Send</button>
            </div>
          </>
        ) : (
          <div className="select-user">
            <p>Select a user to start chatting</p>
          </div>
        )}
      </div>
    </div>
  );
};

export default Chat;