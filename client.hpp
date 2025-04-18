#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <string>
#include <winsock2.h> 
#include <ws2tcpip.h>
#include <pthread.h> 

#pragma comment(lib, "ws2_32.lib")


const int PORT = 8080;
extern const char* SERVER_IP; 

class Client {
private:
    SOCKET client_socket;

    struct ReceiveThreadData {
        SOCKET client_socket;
        Client* client; 
    };

    static void* receive_messages(void* arg);

protected:
    
    SOCKET getSocket() const { return client_socket; }

public:
    Client();
    ~Client();
    bool connectToServer();
    virtual void run();
};
class AdminClient : public Client {
    public:
        void removeClient(int client_id);
        void run() override;
};

std::string encrypt(const std::string& text, const std::string& key);
std::string decrypt(const std::string& text, const std::string& key);

#endif
