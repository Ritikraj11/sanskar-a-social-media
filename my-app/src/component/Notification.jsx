// Notifications.jsx
import React from 'react';
import { FaBell, FaComment, FaHeart, FaUserPlus, FaRegBell } from 'react-icons/fa';
import { useNavigate } from 'react-router-dom';
import './Notification.css';

const Notifications = () => {
    const navigate = useNavigate();
    const notifications = [
        {
            id: 1,
            type: 'like',
            user: 'John Doe',
            content: 'liked your post',
            time: '2 mins ago',
            read: false
        },
        {
            id: 2,
            type: 'comment',
            user: 'Jane Smith',
            content: 'commented on your photo',
            time: '1 hour ago',
            read: false
        },
        {
            id: 3,
            type: 'follow',
            user: 'Mike Johnson',
            content: 'started following you',
            time: '3 hours ago',
            read: true
        },
        {
            id: 4,
            type: 'mention',
            user: 'Sarah Williams',
            content: 'mentioned you in a comment',
            time: '1 day ago',
            read: true
        },
    ];

    const getNotificationIcon = (type) => {
        switch(type) {
            case 'like': return <FaHeart className="icon like" />;
            case 'comment': return <FaComment className="icon comment" />;
            case 'follow': return <FaUserPlus className="icon follow" />;
            default: return <FaRegBell className="icon default" />;
        }
    };

    return (
        <div className="notifications-container">
            <div className="notifications-header">
                <h2>Notifications</h2>
                <div className="notification-actions">
                    <span className="mark-read">Mark all as read</span>
                </div>
            </div>
            
            <div className="notifications-list">
                {notifications.map(notification => (
                    <div 
                        key={notification.id} 
                        className={`notification ${!notification.read ? 'unread' : ''}`}
                    >
                        <div className="notification-icon">
                            {getNotificationIcon(notification.type)}
                        </div>
                        <div className="notification-content">
                            <p>
                                <span className="user">{notification.user}</span> 
                                {notification.content}
                            </p>
                            <span className="time">{notification.time}</span>
                        </div>
                        {!notification.read && <div className="unread-dot"></div>}
                    </div>
                ))}
            </div>
        </div>
    );
};

export default Notifications;