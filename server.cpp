#include "server.hpp"
#include <iostream>
#include <sstream>
#include <unistd.h> 
#include <map>

using namespace std;

vector<SOCKET> ClientHandler::client_sockets;
pthread_mutex_t ClientHandler::client_sockets_mutex;
map<int, SOCKET> clientSocketsMap;

const string ADMIN_USERNAME = "admin"; 
const string ADMIN_PASSWORD = "password123";

ClientHandler::~ClientHandler() {
    closesocket(client_socket);
}

void ClientHandler::initialize() {
    pthread_mutex_init(&client_sockets_mutex, NULL);
}

void ClientHandler::cleanup() {
    pthread_mutex_destroy(&client_sockets_mutex);
}

int ClientHandler::getClientIDFromSocket(SOCKET socket) {
    pthread_mutex_lock(&client_sockets_mutex);
    for (size_t i = 0; i < client_sockets.size(); ++i) {
        if (client_sockets[i] == socket) {
            pthread_mutex_unlock(&client_sockets_mutex);
            return i+1; // IDs start from 1
        }
    }
    pthread_mutex_unlock(&client_sockets_mutex);
    return -1;  // Not found
}

SOCKET ClientHandler::findSocketByClientID(int clientID) {
    pthread_mutex_lock(&client_sockets_mutex);
    auto it = clientSocketsMap.find(clientID);
    if (it != clientSocketsMap.end()) {
        SOCKET foundSocket = it->second;
        pthread_mutex_unlock(&client_sockets_mutex);
        return foundSocket;
    }
    pthread_mutex_unlock(&client_sockets_mutex);
    return INVALID_SOCKET;
}

void ClientHandler::handle() {
    cout << "Client " << client_id << " connected. Thread started." << endl;

    stringstream ss;
    ss << "Your ID is " << client_id;
    string id_message = ss.str();
    sendMessage(client_socket, id_message.c_str(), id_message.length());

    char buffer[1024];
    int bytes_received;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes_received > 0) {
            string message(buffer, bytes_received);

            if (message.substr(0, 5) == "LOGIN") {
                size_t pos1 = message.find("{");
                size_t pos2 = message.find(":");
                size_t pos3 = message.find("}");
 
                if (pos1 != string::npos && pos2 != string::npos && pos3 != string::npos && pos2 > pos1 && pos3 > pos2) {
                    string username = message.substr(pos1 + 1, pos2 - pos1 - 1);
                    string password = message.substr(pos2 + 1, pos3 - pos2 - 1);
 
                    if (username == ADMIN_USERNAME && password == ADMIN_PASSWORD) {
                        is_admin = true;
                        cout << "Client " << client_id << " logged in as admin." << endl;
                        string admin_message = "You are logged as admin. Enter REMOVE{client ID} to disconnect client from server";
                        sendMessage(findSocketByClientID(client_id), admin_message.c_str(), admin_message.length());
                    } else {
                        cout << "Client " << client_id << " login failed." << endl;
                    }
                } else {
                    cout << "Invalid LOGIN command format." << endl;
                }
            }
            
            else if (message.substr(0, 6) == "REMOVE") {
                if(is_admin){
                size_t pos = message.find("{");
                size_t end_pos = message.find("}");
 
                if (pos != string::npos && end_pos != string::npos && end_pos > pos) {
                    string client_id_str = message.substr(pos + 1, end_pos - pos - 1);
                    try {
                        int client_id_to_remove = stoi(client_id_str);
                        removeClient(client_id_to_remove);
                        cout << "Client " << client_id_to_remove << " disconnected from server" << endl;
                    } catch (const invalid_argument& e) {
                        cerr << "Invalid client number." << endl;
                    } catch (const out_of_range& e) {
                        cerr << "Client number out of range." << endl;
                    }
                } else {
                    cout << "Invalid REMOVE command format." << endl;
                }
                }
                else {
                cout << "Client " << client_id << " not autorized to remove users" << endl;
                }
            }
 
            // проверка на приватность
            else if (message.length() > 2 && message.substr(0, 2) == "TO" && isdigit(message[2])) {
                sendPrivateMessage(message); 
            } else {
                // отправляем id отправителя
                stringstream formatted_message_stream;
                formatted_message_stream << "from " << client_id << ": " << message;
                string formatted_message = formatted_message_stream.str();

                broadcast(formatted_message.c_str(), formatted_message.length());
            }

        } else if (bytes_received == 0) {
            cout << "Client " << client_id << " disconnected." << endl;
            break;
        } else {
            cerr << "Error receiving data from client " << client_id << "." << endl;
            break;
        }
    }
    removeClient();
}

void ClientHandler::removeClient(int client_id_to_remove) {
    SOCKET socketToRemove = findSocketByClientID(client_id_to_remove);

    if(socketToRemove != INVALID_SOCKET) {
        string disconnect_message = "Server is disconnecting you.";
        sendMessage(socketToRemove, disconnect_message.c_str(), disconnect_message.length());
        closesocket(socketToRemove);
        pthread_mutex_lock(&client_sockets_mutex);
            client_sockets.erase(remove(client_sockets.begin(), client_sockets.end(), socketToRemove), client_sockets.end());

            // удаляем с map
            for (auto it = clientSocketsMap.begin(); it != clientSocketsMap.end(); ) {
                if (it->first == client_id_to_remove) {
                    it = clientSocketsMap.erase(it);
                    break;
                } else {
                    ++it;
                }
            }
        pthread_mutex_unlock(&client_sockets_mutex);
    }
    else{
        cout << "Client " << client_id_to_remove << " not found." << endl;
    }
}


void ClientHandler::sendMessage(SOCKET socket, const char* message, int length) {
    send(socket, message, length, 0);
}
void ClientHandler::broadcast(const char* message, int length) {
    pthread_mutex_lock(&client_sockets_mutex);
    for (SOCKET other_socket : client_sockets) {
        if (other_socket != client_socket) {
            sendMessage(other_socket, message, length);
        }
    }
    pthread_mutex_unlock(&client_sockets_mutex);
}

void ClientHandler::sendPrivateMessage(const string& message) {
    size_t start_pos = 2;
    size_t end_pos = message.find("}");

    if (end_pos != string::npos && end_pos > start_pos) {
        string client_id_str = message.substr(start_pos, end_pos - start_pos);
        try {
            int target_client_id = stoi(client_id_str);
           
             SOCKET target_socket = findSocketByClientID(target_client_id);

            if (target_socket != INVALID_SOCKET) {
                size_t key_start = message.find("*key*", end_pos + 1);  // ищем *key* после TO{id}
                if (key_start != string::npos) {
                    size_t key_end = message.find("*message*", key_start + strlen("*key*"));
                    if (key_end != string::npos) {
                       string key = message.substr(key_start + strlen("*key*"), key_end - (key_start + strlen("*key*")));
                       string encrypted_message = message.substr(key_end + strlen("*message*"));

                       // сообщение для пересылки в том же формате
                        stringstream message_to_forward;
                        message_to_forward << "TO{" << target_client_id << "}*key*" << key << "*message*" << encrypted_message; 
                       string final_message = message_to_forward.str();
                        sendMessage(target_socket, final_message.c_str(), final_message.length());
                   }
                }

            } else {
                cout << "Client " << target_client_id << " not found." << endl;
                      
                int senderClientID = getClientIDFromSocket(client_socket);
                string error_message = "Client " + to_string(target_client_id) + " not found";
                SOCKET sender_socket = findSocketByClientID(senderClientID);
                if (sender_socket != INVALID_SOCKET) {
                    sendMessage(sender_socket, error_message.c_str(), error_message.length());
                }
            }
        } catch (const std::invalid_argument& e) {
            std::cerr << "Invalid client ID format." << std::endl;
        } catch (const std::out_of_range& e) {
            std::cerr << "Client ID out of range." << std::endl;
        }
    } else {
        std::cerr << "Invalid private message format." << std::endl;
    }
}

void ClientHandler::removeClient() {
    pthread_mutex_lock(&client_sockets_mutex);
    client_sockets.erase(remove(client_sockets.begin(), client_sockets.end(), client_socket), client_sockets.end());
    pthread_mutex_unlock(&client_sockets_mutex);
}

void* ClientHandler::thread_entry(void* arg) {
    ClientHandler* handler = (ClientHandler*)arg;
    handler->handle();
    delete handler;
    pthread_exit(NULL);
    return nullptr;
}

void ClientHandler::start_thread(SOCKET socket, int id) {
    ClientHandler* handler = new ClientHandler(socket, id);
    pthread_t thread;
    if (pthread_create(&thread, NULL, thread_entry, handler) != 0) {
        std::cerr << "Failed to create thread." << std::endl;
        delete handler;
        closesocket(socket);
        return;
    }
    pthread_detach(thread);
}

void ClientHandler::addClientSocket(SOCKET socket) {
    pthread_mutex_lock(&client_sockets_mutex);
    client_sockets.push_back(socket);
    pthread_mutex_unlock(&client_sockets_mutex);
}


Server::Server(int port) : listening_socket(INVALID_SOCKET), address_size(sizeof(address)) {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed, result = " << result << std::endl;
        exit(1);
    }

    listening_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listening_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        exit(1);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(listening_socket, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(listening_socket);
        WSACleanup();
        exit(1);
    }

    if (listen(listening_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listening_socket);
        WSACleanup();
        exit(1);
    }

    std::cout << "Server is listening..." << std::endl;
}

Server::~Server() {
    closesocket(listening_socket);
    WSACleanup();
}

void Server::run() {
    int client_count = 0;
    while (true) {
        SOCKET client_socket = accept(listening_socket, (struct sockaddr*)&address, &address_size);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        client_count++;
        std::cout << "Client connected. Client ID: " << client_count << std::endl;

        ClientHandler::addClientSocket(client_socket);
        ClientHandler::start_thread(client_socket, client_count);
    }
}
