import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { FaSignOutAlt, FaUser, FaImage, FaSearch, FaFire, FaUsers, FaHashtag } from 'react-icons/fa';
import { IoMdNotifications } from 'react-icons/io';
import { BsChatLeftText, BsThreeDotsVertical } from 'react-icons/bs';

import './Home.css';
const Home = ({ username, setIsLoggedIn, setUsername }) => {
    const navigate = useNavigate();
    const [postText, setPostText] = useState('');
    const [posts, setPosts] = useState([]);
    const [filteredPosts, setFilteredPosts] = useState([]);
    const [searchQuery, setSearchQuery] = useState('');
    const [imageFile, setImageFile] = useState(null);
    const [isLoading, setIsLoading] = useState(false);
    const [error, setError] = useState('');
    const [isMenuOpen, setIsMenuOpen] = useState(false);
    const [unreadMessages, setUnreadMessages] = useState(3); // Example unread count
    const [unreadNotifications, setUnreadNotifications] = useState(2); // Example notification count
    const [trendingTopics, setTrendingTopics] = useState([
        { id: 1, content: 'New feature updates coming soon!' },
        { id: 2, content: 'Community meetup this weekend' },
        { id: 3, content: 'Best practices for social engagement' },
        { id: 4, content: '#WelcomeNewMembers' },
        { id: 5, content: 'Platform growth statistics' }
    ]);

    useEffect(() => {
        fetchPosts();
    }, []);

    const fetchPosts = async () => {
        try {
            const res = await fetch('http://localhost:8080/api/posts');
            if (!res.ok) throw new Error(`Failed to fetch posts: ${res.status}`);
            const data = await res.json();
            const reversed = data.reverse();
            setPosts(reversed);
            setFilteredPosts(reversed);
        } catch (err) {
            console.error('Error fetching posts:', err);
            setError('Failed to fetch posts. Please try again later.');
        }
    };

    useEffect(() => {
        const q = searchQuery.toLowerCase();
        if (q.trim() === '') {
            setFilteredPosts([...posts]);
            return;
        }
        
        const filtered = posts.filter(
            (post) =>
                post.author?.toLowerCase().includes(q) ||
                post.content?.toLowerCase().includes(q)
        );
        setFilteredPosts(filtered);
    }, [searchQuery, posts]);

    const handlePeopleSearch = (e) => {
        const q = e.target.value.toLowerCase();
        if (q.trim() === '') {
            setFilteredPosts([...posts]);
            return;
        }
        
        const filtered = posts.filter(p => 
            p.author?.toLowerCase().includes(q)
        );
        setFilteredPosts(filtered);
    };

    const handleLogout = () => {
        setIsLoggedIn(false);
        setUsername('');
        localStorage.removeItem('isLoggedIn');
        localStorage.removeItem('username');
        navigate('/login');
    };

    const handlePost = async () => {
        if (!postText.trim() && !imageFile) {
            setError('Post cannot be empty');
            return;
        }

        setIsLoading(true);
        setError('');

        const formData = new FormData();
        formData.append('author', username);
        formData.append('content', postText);
        if (imageFile) formData.append('image', imageFile);

        try {
            const res = await fetch('http://localhost:8080/api/posts', {
                method: 'POST',
                body: formData,
            });

            if (!res.ok) {
                const errorData = await res.json().catch(() => ({}));
                throw new Error(errorData.message || 'Failed to create post');
            }

            await fetchPosts();
            setPostText('');
            setImageFile(null);
            if (document.querySelector('.upload-btn input')) {
                document.querySelector('.upload-btn input').value = '';
            }
        } catch (err) {
            console.error('Posting error:', err);
            setError(err.message || 'Failed to create post');
        } finally {
            setIsLoading(false);
        }
    };

    const handleKeyPress = (e) => {
        if (e.key === 'Enter' && !e.shiftKey) {
            e.preventDefault();
            handlePost();
        }
    };

    return (
        <div className="home-container">
            {/* Header */}
            <header className="home-header">
                <div className="app-title" onClick={() => window.scrollTo(0, 0)}>
                    <span className="sanskrit">संस्कार</span>
                    <span className="english">Sanskriti</span>
                </div>
                
                <div className="mobile-menu-btn" onClick={() => setIsMenuOpen(!isMenuOpen)}>
                    <div className={`menu-line ${isMenuOpen ? 'open' : ''}`}></div>
                    <div className={`menu-line ${isMenuOpen ? 'open' : ''}`}></div>
                    <div className={`menu-line ${isMenuOpen ? 'open' : ''}`}></div>
                </div>

                <div className={`header-controls ${isMenuOpen ? 'open' : ''}`}>
                    <div className="user-info">
                        <FaUser className="user-icon" />
                        <span className="username">{username}</span>
                    </div>
                    <div className="header-icons">
                        <div 
                            className="icon-wrapper"
                            onClick={() => navigate('/chat')}
                            title="Chat"
                        >
                            <BsChatLeftText className="icon chat-icon" />
                            {unreadMessages > 0 && (
                                <span className="notification-badge">{unreadMessages}</span>
                            )}
                        </div>
                        
                        <div 
                            className="icon-wrapper"
                            onClick={() => navigate('/notifications')}
                            title="Notifications"
                        >
                            <IoMdNotifications className="icon notification-icon" />
                            {unreadNotifications > 0 && (
                                <span className="notification-badge">{unreadNotifications}</span>
                            )}
                        </div>

                        <div 
                            className="icon-wrapper"
                            onClick={handleLogout}
                            title="Logout"
                        >
                            <FaSignOutAlt className="icon logout-icon" />
                        </div>
                    </div>
                </div>
            </header>

            {/* Main Content */}
            <main className="feed-container">
                {/* Left Sidebar */}
                <div className="left-sidebar">
                    <div className="search-bar">
                        <FaSearch className="search-icon" />
                        <input
                            type="text"
                            placeholder="Search posts or topics..."
                            value={searchQuery}
                            onChange={(e) => setSearchQuery(e.target.value)}
                            className="search-input"
                        />
                    </div>

                    <div className="search-bar" style={{ marginTop: '1rem' }}>
                        <FaUser className="search-icon" />
                        <input
                            type="text"
                            placeholder="Search people..."
                            onChange={handlePeopleSearch}
                            className="search-input"
                        />
                    </div>
                    
                    <div className="card sidebar-card">
                        <h3 className="sidebar-title">
                            <FaUsers /> Suggested Friends
                        </h3>
                        <p style={{ color: 'var(--gray-color)', fontSize: '0.9rem' }}>
                            Connect with people you may know
                        </p>
                    </div>
                </div>

                {/* Middle Feed */}
                <div className="middle-feed-container">
                    <div className="middle-feed-content">
                    {/* Post Creator */}
                    <div className="post-creator card">
                        <textarea
                            className="post-input"
                            value={postText}
                            onChange={(e) => setPostText(e.target.value)}
                            onKeyDown={handleKeyPress}
                            placeholder={`What's on your mind, ${username}?`}
                            rows={3}
                            maxLength={500}
                        />
                        <div className="post-footer">
                            <div className="upload-row">
                                <label className="upload-btn">
                                    <FaImage /> 
                                    <span className="upload-text">Upload Image</span>
                                    <input
                                        type="file"
                                        accept="image/*"
                                        onChange={(e) => setImageFile(e.target.files[0])}
                                        hidden
                                    />
                                </label>
                                {imageFile && (
                                    <span className="filename">
                                        {imageFile.name.length > 15 
                                            ? `${imageFile.name.substring(0, 12)}...` 
                                            : imageFile.name}
                                    </span>
                                )}
                            </div>

                            <div className="post-controls">
                                <span className="char-count">{postText.length}/500</span>
                                <button
                                    className={`post-btn ${isLoading ? 'loading' : ''}`}
                                    onClick={handlePost}
                                    disabled={isLoading || (!postText.trim() && !imageFile)}
                                >
                                    {isLoading ? (
                                        <>
                                            <span className="spinner"></span>
                                            Posting...
                                        </>
                                    ) : (
                                        'Post'
                                    )}
                                </button>
                            </div>
                        </div>
                        {error && <p className="error-message">{error}</p>}
                    </div>

                    {/* Posts Feed */}
                    <div className="posts-feed">
                        {filteredPosts.length === 0 ? (
                            <div className="empty-feed card">
                                <p>No posts found. Be the first to post!</p>
                            </div>
                        ) : (
                            filteredPosts.map((post) => (
                                <div className="post-card card" key={post._id || post.id}>
                                    <div className="post-header">
                                        <div className="author-avatar">
                                            <FaUser className="post-author-icon" />
                                        </div>
                                        <div className="post-meta">
                                            <span className="post-author">{post.author || 'Unknown User'}</span>
                                            <span className="post-time">
                                                {new Date(post.createdAt || post.timestamp).toLocaleString()}
                                            </span>
                                        </div>
                                    </div>
                                    <div className="post-content">
                                        <p>{post.content}</p>
                                        {post.imageUrl && (
                                                <div className="post-image-container">
                                                    <img
                                                        src={`http://localhost:8080${post.imageUrl}`}
                                                        alt="Post content"
                                                        className="post-image"
                                                        onError={(e) => {
                                                            e.target.onerror = null;
                                                            e.target.style.display = 'none';
                                                        }}
                                                    />
                                                </div>
                                            )}
                                    </div>
                                </div>
                            ))
                        )}
                    </div>
                </div>
            </div>

                {/* Right Sidebar */}
                <div className="right-sidebar">
                    <div className="card sidebar-card">
                        <h3 className="sidebar-title">
                            <FaFire /> Trending Now
                        </h3>
                        {trendingTopics.map((topic, index) => (
                            <div className="trending-item" key={topic.id}>
                                <span className="trending-number">{index + 1}</span>
                                <span className="trending-content">{topic.content}</span>
                            </div>
                        ))}
                    </div>

                    <div className="card sidebar-card">
                        <h3 className="sidebar-title">
                            <FaHashtag /> Popular Tags
                        </h3>
                        <div className="tags-container">
                            {['#Welcome', '#Updates', '#Community', '#Events', '#Feedback'].map(tag => (
                                <span key={tag} className="tag">
                                    {tag}
                                </span>
                            ))}
                        </div>
                    </div>
                </div>
            </main>
        </div>
    );
};

export default Home;