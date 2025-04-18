#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <winsock2.h>
#include <pthread.h>
#include <vector>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

class ClientHandler {
private:
    SOCKET client_socket;
    int client_id;
    static std::vector<SOCKET> client_sockets;
    static pthread_mutex_t client_sockets_mutex;
    bool is_admin = false;

public:
    ClientHandler(SOCKET socket, int id) : client_socket(socket), client_id(id) {}
    ~ClientHandler(); 

    static void initialize();
    static void cleanup();

    void handle();

private:
    void broadcast(const char* message, int length);
    void removeClient(int client_id);
    void removeClient();
    void sendMessage(SOCKET socket, const char* message, int length);
    void sendPrivateMessage(const std::string& message); 

    static void* thread_entry(void* arg);

public:
    static void start_thread(SOCKET socket, int id);
    static void addClientSocket(SOCKET socket);
    static int getClientIDFromSocket(SOCKET socket);
    static SOCKET findSocketByClientID(int clientID);
};

class Server {
private:
    SOCKET listening_socket;
    sockaddr_in address;
    int address_size;

public:
    Server(int port);
    ~Server();

    void run();
};

#endif // SERVER_HPP
