import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import './Register.css';

const Register = () => {
  const navigate = useNavigate();
  const [formData, setFormData] = useState({ 
    username: '', 
    password: '',
    confirmPassword: '' 
  });
  const [error, setError] = useState('');
  const [isLoading, setIsLoading] = useState(false);
  const [isFocused, setIsFocused] = useState({
    username: false,
    password: false,
    confirmPassword: false
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

  const handleRegister = async (e) => {
    e.preventDefault();
    setError('');

    if (!formData.username.trim() || !formData.password.trim()) {
      setError('Both fields are required');
      return;
    }

    if (formData.password.length < 6) {
      setError('Password must be at least 6 characters');
      return;
    }

    if (formData.password !== formData.confirmPassword) {
      setError('Passwords do not match');
      return;
    }

    setIsLoading(true);

    try {
      const requestData = {
        username: formData.username.trim(),
        password: formData.password.trim()
      };

      const response = await fetch('http://localhost:8080/api/register', {
        method: 'POST',
        headers: { 
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(requestData)
      });

      const responseText = await response.text();
      let data;
      
      try {
        data = JSON.parse(responseText);
      } catch {
        throw new Error('Invalid server response');
      }

      if (!response.ok) {
        throw new Error(data.message || 'Already a User');
      }

      navigate('/login', { 
        state: { 
          registrationSuccess: true,
          username: formData.username 
        } 
      });
    } catch (err) {
      setError(err.message || 'Failed to connect to server');
      console.error('Registration error:', err);
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div className="register-container">
      <div className="register-background">
        <div className="shape"></div>
        <div className="shape"></div>
      </div>
      
      <div className="register-card">
        <div className="register-header">
          <h1>संस्कार</h1>
          <p className="tagline">Preserve your heritage, share your traditions</p>
        </div>

        <div className="register-content">
          <h2>Create Account</h2>
          <p className="welcome-text">Join our community to share and preserve cultural traditions</p>
          
          {error && (
            <div className="error-message">
              <svg className="error-icon" viewBox="0 0 24 24">
                <path fill="currentColor" d="M11,15H13V17H11V15M11,7H13V13H11V7M12,2C6.47,2 2,6.5 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2M12,20A8,8 0 0,1 4,12A8,8 0 0,1 12,4A8,8 0 0,1 20,12A8,8 0 0,1 12,20Z" />
              </svg>
              <span>{error}</span>
            </div>
          )}

          <form onSubmit={handleRegister} className="register-form">
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
                minLength="6"
                disabled={isLoading}
              />
              <div className="input-decoration"></div>
            </div>

            <div className={`form-group ${isFocused.confirmPassword ? 'focused' : ''} ${formData.confirmPassword ? 'has-value' : ''}`}>
              <label htmlFor="confirmPassword">Confirm Password</label>
              <input
                id="confirmPassword"
                type="password"
                name="confirmPassword"
                value={formData.confirmPassword}
                onChange={handleChange}
                onFocus={() => handleFocus('confirmPassword')}
                onBlur={() => handleBlur('confirmPassword')}
                minLength="6"
                disabled={isLoading}
              />
              <div className="input-decoration"></div>
            </div>
            
            <button 
              className={`register-btn ${isLoading ? 'loading' : ''}`}
              type="submit"
              disabled={isLoading}
            >
              {isLoading ? (
                <>
                  <svg className="spinner" viewBox="0 0 50 50">
                    <circle className="path" cx="25" cy="25" r="20" fill="none" strokeWidth="5"></circle>
                  </svg>
                  <span>Registering...</span>
                </>
              ) : (
                'Register'
              )}
            </button>
          </form>

          <div className="login-section">
            <p>Already have an account?</p>
            <button 
              onClick={() => navigate('/login')} 
              className="login-btn"
              disabled={isLoading}
            >
              Sign In
            </button>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Register;