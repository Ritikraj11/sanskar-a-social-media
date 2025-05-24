import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import './Login.css';

const Login = ({ setIsLoggedIn, setUsername }) => {
  const navigate = useNavigate();
  const [formData, setFormData] = useState({ 
    username: '', 
    password: '' 
  });
  const [error, setError] = useState('');
  const [isLoading, setIsLoading] = useState(false);
  const [isFocused, setIsFocused] = useState({
    username: false,
    password: false
  });

  const handleChange = (e) => {
    const { name, value } = e.target;
    setFormData(prev => ({ ...prev, [name]: value }));
  };

  const handleFocus = (field) => {
    setIsFocused(prev => ({ ...prev, [field]: true }));
  };

  const handleBlur = (field) => {
    setIsFocused(prev => ({ ...prev, [field]: false }));
  };

  const handleLogin = async (e) => {
    e.preventDefault();
    setError('');
    
    if (!formData.username.trim() || !formData.password.trim()) {
      setError('Both fields are required');
      return;
    }

    setIsLoading(true);

    try {
      const requestData = {
        username: formData.username.trim(),
        password: formData.password.trim()
      };

      const response = await fetch('http://localhost:8080/api/login', {
        method: 'POST',
        headers: { 
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(requestData)
      });

      const data = await response.json();

      if (!response.ok) {
        throw new Error(data.error || 'Login failed');
      }

      setIsLoggedIn(true);
      setUsername(formData.username.trim());
      navigate('/home');
    } catch (err) {
      setError(err.message || 'Failed to connect to server');
      console.error('Login error:', err);
    } finally {
      setIsLoading(false);
    }
  }

  return (
    <div className="login-container">
      <div className="login-background">
        <div className="shape"></div>
        <div className="shape"></div>
      </div>
      
      <div className="login-card">
        <div className="login-header">
          <h1>संस्कार</h1>
          <p className="tagline">Preserve your heritage, share your traditions</p>
        </div>

        <div className="login-content">
          <h2>Welcome Back</h2>
          <p className="welcome-text">Sign in to continue your cultural journey</p>
          
          {error && (
            <div className="error-message">
              <svg className="error-icon" viewBox="0 0 24 24">
                <path fill="currentColor" d="M11,15H13V17H11V15M11,7H13V13H11V7M12,2C6.47,2 2,6.5 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2M12,20A8,8 0 0,1 4,12A8,8 0 0,1 12,4A8,8 0 0,1 20,12A8,8 0 0,1 12,20Z" />
              </svg>
              <span>{error}</span>
            </div>
          )}

          <form onSubmit={handleLogin} className="login-form">
            <div className={`form-group ${isFocused.username ? 'focused' : ''} ${formData.username ? 'has-value' : ''}`}>
              <label htmlFor="username">Username</label>
              <input
                id="username"
                type="text"
                name="username"
                value={formData.username}
                onChange={handleChange}
                onFocus={() => handleFocus('username')}
                onBlur={() => handleBlur('username')}
                disabled={isLoading}
              />
              <div className="input-decoration"></div>
            </div>
            
            <div className={`form-group ${isFocused.password ? 'focused' : ''} ${formData.password ? 'has-value' : ''}`}>
              <label htmlFor="password">Password</label>
              <input
                id="password"
                type="password"
                name="password"
                value={formData.password}
                onChange={handleChange}
                onFocus={() => handleFocus('password')}
                onBlur={() => handleBlur('password')}
                disabled={isLoading}
              />
              <div className="input-decoration"></div>
            </div>
            
            <button 
              className={`login-btn ${isLoading ? 'loading' : ''}`}
              type="submit"
              disabled={isLoading}
            >
              {isLoading ? (
                <>
                  <svg className="spinner" viewBox="0 0 50 50">
                    <circle className="path" cx="25" cy="25" r="20" fill="none" strokeWidth="5"></circle>
                  </svg>
                  <span>Signing In...</span>
                </>
              ) : (
                'Sign In'
              )}
            </button>
          </form>

          <div className="register-section">
            <p>Don't have an account?</p>
            <button 
              onClick={() => navigate('/register')} 
              className="register-link"
              disabled={isLoading}
            >
              Create Account
            </button>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Login;