/*!
    \file ssl_server.h
    \brief SSL server definition
    \author Ivan Shynkarenka
    \date 30.12.2016
    \copyright MIT License
*/

#ifndef CPPSERVER_ASIO_SSL_SERVER_H
#define CPPSERVER_ASIO_SSL_SERVER_H

#include "ssl_session.h"

#include <map>
#include <mutex>
#include <vector>

namespace CppServer {
namespace Asio {

template <class TServer, class TSession>
class SSLSession;

//! SSL server
/*!
    SSL server is used to connect, disconnect and manage SSL sessions.

    Thread-safe.
*/
template <class TServer, class TSession>
class SSLServer : public std::enable_shared_from_this<SSLServer<TServer, TSession>>
{
    template <class TSomeServer, class TSomeSession>
    friend class SSLSession;

public:
    //! Initialize SSL server with a given Asio service, protocol and port number
    /*!
        \param service - Asio service
        \param context - SSL context
        \param protocol - Protocol type
        \param port - Port number
    */
    explicit SSLServer(std::shared_ptr<Service> service, std::shared_ptr<asio::ssl::context> context, InternetProtocol protocol, int port);
    //! Initialize SSL server with a given Asio service, IP address and port number
    /*!
        \param service - Asio service
        \param context - SSL context
        \param address - IP address
        \param port - Port number
    */
    explicit SSLServer(std::shared_ptr<Service> service, std::shared_ptr<asio::ssl::context> context, const std::string& address, int port);
    //! Initialize SSL server with a given a given Asio service and endpoint
    /*!
        \param service - Asio service
        \param context - SSL context
        \param endpoint - Server SSL endpoint
    */
    explicit SSLServer(std::shared_ptr<Service> service, std::shared_ptr<asio::ssl::context> context, const asio::ip::tcp::endpoint& endpoint);
    SSLServer(const SSLServer&) = delete;
    SSLServer(SSLServer&&) = default;
    virtual ~SSLServer() = default;

    SSLServer& operator=(const SSLServer&) = delete;
    SSLServer& operator=(SSLServer&&) = default;

    //! Get the Asio service
    std::shared_ptr<Service>& service() noexcept { return _service; }
    //! Get the server SSL context
    std::shared_ptr<asio::ssl::context>& context() noexcept { return _context; }
    //! Get the server endpoint
    asio::ip::tcp::endpoint& endpoint() noexcept { return _endpoint; }
    //! Get the server acceptor
    asio::ip::tcp::acceptor& acceptor() noexcept { return _acceptor; }

    //! Get the number of sessions connected to the server
    uint64_t connected_sessions() const noexcept { return _sessions.size(); }
    //! Get the number of bytes sent by the server
    uint64_t bytes_sent() const noexcept { return _bytes_sent; }
    //! Get the number of bytes received by the server
    uint64_t bytes_received() const noexcept { return _bytes_received; }

    //! Get the option: no delay
    bool option_no_delay() const noexcept { return _option_no_delay; }
    //! Get the option: reuse address
    bool option_reuse_address() const noexcept { return _option_reuse_address; }
    //! Get the option: reuse port
    bool option_reuse_port() const noexcept { return _option_reuse_port; }

    //! Is the server started?
    bool IsStarted() const noexcept { return _started; }

    //! Start the server
    /*!
        \return 'true' if the server was successfully started, 'false' if the server failed to start
    */
    bool Start();
    //! Stop the server
    /*!
        \return 'true' if the server was successfully stopped, 'false' if the server is already stopped
    */
    bool Stop();
    //! Restart the server
    /*!
        \return 'true' if the server was successfully restarted, 'false' if the server failed to restart
    */
    bool Restart();

    //! Multicast data to all connected sessions
    /*!
        \param buffer - Buffer to multicast
        \param size - Buffer size
        \return 'true' if the data was successfully multicast, 'false' if the server it not started
    */
    bool Multicast(const void* buffer, size_t size);
    //! Multicast a text string to all connected sessions
    /*!
        \param text - Text string to multicast
        \return 'true' if the text string was successfully multicast, 'false' if the server it not started
    */
    bool Multicast(const std::string& text) { return Multicast(text.data(), text.size()); }

    //! Disconnect all connected sessions
    /*!
        \return 'true' if all sessions were successfully disconnected, 'false' if the server it not started
    */
    bool DisconnectAll();

    //! Setup option: no delay
    /*!
        This option will enable/disable Nagle's algorithm for TCP protocol.

        https://en.wikipedia.org/wiki/Nagle%27s_algorithm

        \param enable - Enable/disable option
    */
    void SetupNoDelay(bool enable) noexcept { _option_no_delay = enable; }
    //! Setup option: reuse address
    /*!
        This option will enable/disable SO_REUSEADDR if the OS support this feature.

        \param enable - Enable/disable option
    */
    void SetupReuseAddress(bool enable) noexcept { _option_reuse_address = enable; }
    //! Setup option: reuse port
    /*!
        This option will enable/disable SO_REUSEPORT if the OS support this feature.

        \param enable - Enable/disable option
    */
    void SetupReusePort(bool enable) noexcept { _option_reuse_port = enable; }

protected:
    //! Handle server started notification
    virtual void onStarted() {}
    //! Handle server stopped notification
    virtual void onStopped() {}

    //! Handle new session connected notification
    /*!
        \param session - Connected session
    */
    virtual void onConnected(std::shared_ptr<TSession>& session) {}
    //! Handle session disconnected notification
    /*!
        \param session - Disconnected session
    */
    virtual void onDisconnected(std::shared_ptr<TSession>& session) {}

    //! Handle error notification
    /*!
        \param error - Error code
        \param category - Error category
        \param message - Error message
    */
    virtual void onError(int error, const std::string& category, const std::string& message) {}

private:
    // Asio service
    std::shared_ptr<Service> _service;
    // Server SSL context, endpoint, acceptor and socket
    std::shared_ptr<asio::ssl::context> _context;
    asio::ip::tcp::endpoint _endpoint;
    asio::ip::tcp::acceptor _acceptor;
    asio::ip::tcp::socket _socket;
    std::atomic<bool> _started;
    // Server statistic
    uint64_t _bytes_sent;
    uint64_t _bytes_received;
    // Server sessions
    std::map<CppCommon::UUID, std::shared_ptr<TSession>> _sessions;
    // Multicast buffer
    std::mutex _multicast_lock;
    std::vector<uint8_t> _multicast_buffer;
    // Options
    bool _option_no_delay;
    bool _option_reuse_address;
    bool _option_reuse_port;

    //! Accept new connections
    void Accept();

    //! Register a new session
    std::shared_ptr<TSession> RegisterSession();
    //! Unregister the given session
    /*!
        \param id - Session Id
    */
    void UnregisterSession(const CppCommon::UUID& id);

    //! Clear multicast buffer
    void ClearBuffers();

    //! Send error notification
    void SendError(std::error_code ec);
};

/*! \example ssl_chat_server.cpp SSL chat server example */

} // namespace Asio
} // namespace CppServer

#include "ssl_server.inl"

#endif // CPPSERVER_ASIO_SSL_SERVER_H