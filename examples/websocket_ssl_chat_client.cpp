/*!
    \file websocket_ssl_chat_client.cpp
    \brief WebSocket SSL chat client example
    \author Ivan Shynkarenka
    \date 11.01.2016
    \copyright MIT License
*/

#include "server/asio/websocket_ssl_client.h"

#include "asio_service.h"

#include <iostream>

class ChatClient : public CppServer::Asio::WebSocketSSLClient
{
public:
    using CppServer::Asio::WebSocketSSLClient::WebSocketSSLClient;

protected:
    void onConnected() override
    {
        std::cout << "Chat WebSocket SSL client connected a new session with Id " << id() << std::endl;
    }
    void onDisconnected() override
    {
        std::cout << "Chat WebSocket SSL client disconnected a session with Id " << id() << std::endl;

        // Wait for a while...
        CppCommon::Thread::Sleep(1000);

        // Try to connect again
        Connect();
    }

    void onReceived(CppServer::Asio::WebSocketSSLMessage message) override
    {
        std::cout << "Incoming: " << message->get_raw_payload() << std::endl;
    }

    void onError(int error, const std::string& category, const std::string& message) override
    {
        std::cout << "Chat WebSocket SSL client caught an error with code " << error << " and category '" << category << "': " << message << std::endl;
    }
};

int main(int argc, char** argv)
{
    // WebSocket SSL server address
    std::string address = "127.0.0.1";
    if (argc > 1)
        address = argv[1];

    // WebSocket SSL server port
    int port = 5555;
    if (argc > 2)
        port = std::atoi(argv[2]);

    // WebSocket SSL server uri
    std::string uri = "wss://" + address + ":" + std::to_string(port);

    std::cout << "WebSocket SSL server address: " << address << std::endl;
    std::cout << "WebSocket SSL server port: " << port << std::endl;
    std::cout << "WebSocket SSL server uri: " << uri << std::endl;
    std::cout << "Press Enter to stop..." << std::endl;

    // Create a new Asio service
    auto service = std::make_shared<AsioService>();

    // Start the service
    service->Start();

    // Create and prepare a new SSL client context
    std::shared_ptr<asio::ssl::context> context = std::make_shared<asio::ssl::context>(asio::ssl::context::sslv23);
    context->set_verify_mode(asio::ssl::verify_peer);
    context->load_verify_file("../tools/certificates/ca.pem");

    // Create a new WebSocket SSL chat client
    auto client = std::make_shared<ChatClient>(service, context, uri);

    // Connect the client
    client->Connect();

    // Perform text input
    std::string line;
    while (getline(std::cin, line))
    {
        if (line.empty())
            break;

        // Send the entered text to the chat server
        client->Send(line);
    }

    // Disconnect the client
    client->Disconnect();

    // Stop the service
    service->Stop();

    return 0;
}
