/*!
    \file websocket_ssl_client.cpp
    \brief WebSocket SSL client implementation
    \author Ivan Shynkarenka
    \date 11.01.2016
    \copyright MIT License
*/

#include "server/asio/websocket_ssl_client.h"

namespace CppServer {
namespace Asio {

WebSocketSSLClient::WebSocketSSLClient(std::shared_ptr<Service> service, std::shared_ptr<asio::ssl::context> context, const std::string& uri)
    : _id(CppCommon::UUID::Generate()),
      _service(service),
      _context(context),
      _uri(uri),
      _initialized(false),
      _connected(false),
      _total_received(0),
      _total_sent(0)
{
    InitAsio();
}

void WebSocketSSLClient::InitAsio()
{
    assert(!_initialized && "Asio is already initialed!");
    if (_initialized)
        return;

    // Setup WebSocket client core Asio service
    websocketpp::lib::error_code ec;
    _core.init_asio(&_service->service(), ec);
    if (ec)
    {
        onError(ec.value(), ec.category().name(), ec.message());
        return;
    }

    _initialized = true;
}

bool WebSocketSSLClient::Connect()
{
    assert(_initialized && "Asio is not initialed!");
    if (!_initialized)
        return false;

    if (IsConnected())
        return false;

    // Post the connect routine
    auto self(this->shared_from_this());
    _service->service().post([this, self]()
    {
        websocketpp::lib::error_code ec;

        // Setup WebSocket client core logging
        _core.set_access_channels(websocketpp::log::alevel::none);
        _core.set_error_channels(websocketpp::log::elevel::none);

        // Setup WebSocket server core handlers
        _core.set_open_handler([this](websocketpp::connection_hdl connection) { Connected(); });
        _core.set_close_handler([this](websocketpp::connection_hdl connection) { Disconnected(); });
        _core.set_tls_init_handler([this](websocketpp::connection_hdl connection) { return _context; });

        // Get the client connection
        _connection = _core.get_connection(_uri, ec);
        if (ec)
        {
            onError(ec.value(), ec.category().name(), ec.message());
            onDisconnected();
            return;
        }

        // Setup WebSocket client handlers
        _connection->set_message_handler([this](websocketpp::connection_hdl connection, WebSocketSSLMessage message)
        {
            size_t size = message->get_raw_payload().size();

            // Update statistic
            _total_received += size;

            // Call the message received handler
            onReceived(message);
        });
        _connection->set_fail_handler([this](websocketpp::connection_hdl connection)
        {
            WebSocketSSLServerCore::connection_ptr con = _core.get_con_from_hdl(connection);
            websocketpp::lib::error_code ec = con->get_ec();
            onError(ec.value(), ec.category().name(), ec.message());
            Disconnected();
        });

        // Note that connect here only requests a connection. No network messages are
        // exchanged until the event loop starts running in the next line.
        _connection = _core.connect(_connection);
    });

    return true;
}

void WebSocketSSLClient::Connected()
{
    // Reset statistic
    _total_received = 0;
    _total_sent = 0;

    // Update the connected flag
    _connected = true;

    // Call the client connected handler
    onConnected();
}

bool WebSocketSSLClient::Disconnect(bool dispatch, websocketpp::close::status::value code, const std::string& reason)
{
    if (!IsConnected())
        return false;

    auto self(this->shared_from_this());
    auto disconnect = [this, self, code, reason]()
    {
        // Close the client connection
        websocketpp::lib::error_code ec;
        _core.close(_connection, code, reason, ec);
        if (ec)
            onError(ec.value(), ec.category().name(), ec.message());
    };

    // Dispatch or post the disconnect routine
    if (dispatch)
        _service->Dispatch(disconnect);
    else
        _service->Post(disconnect);

    return true;
}

void WebSocketSSLClient::Disconnected()
{
    // Update the connected flag
    _connected = false;

    // Call the client disconnected handler
    onDisconnected();
}

bool WebSocketSSLClient::Reconnect()
{
    if (!Disconnect())
        return false;

    while (IsConnected())
        CppCommon::Thread::Yield();

    return Connect();
}

size_t WebSocketSSLClient::Send(const void* buffer, size_t size, websocketpp::frame::opcode::value opcode)
{
    assert((buffer != nullptr) && "Pointer to the buffer should not be equal to 'nullptr'!");
    assert((size > 0) && "Buffer size should be greater than zero!");
    if ((buffer == nullptr) || (size == 0))
        return 0;

    if (!IsConnected())
        return 0;

    websocketpp::lib::error_code ec;
    _core.send(_connection, buffer, size, opcode, ec);
    if (ec)
    {
        onError(ec.value(), ec.category().name(), ec.message());
        return 0;
    }

    // Update statistic
    _total_sent += size;

    return size;
}

size_t WebSocketSSLClient::Send(const std::string& text, websocketpp::frame::opcode::value opcode)
{
    if (!IsConnected())
        return 0;

    websocketpp::lib::error_code ec;
    _core.send(_connection, text, opcode, ec);
    if (ec)
    {
        onError(ec.value(), ec.category().name(), ec.message());
        return 0;
    }

    size_t size = text.size();

    // Update statistic
    _total_sent += size;

    return size;
}

size_t WebSocketSSLClient::Send(WebSocketSSLMessage message)
{
    if (!IsConnected())
        return 0;

    websocketpp::lib::error_code ec;
    _core.send(_connection, message, ec);
    if (ec)
    {
        onError(ec.value(), ec.category().name(), ec.message());
        return 0;
    }

    size_t size = message->get_raw_payload().size();

    // Update statistic
    _total_sent += size;

    return size;
}

} // namespace Asio
} // namespace CppServer
