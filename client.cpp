#include "client.hpp"
#include <iostream>
#include <string>
#include <sstream>  // для stringstream
#include <cctype>   // для isalpha, isupper
#include <cstring>  // для memset, strlen
#include <thread>   


using namespace std; 

const char* SERVER_IP = "127.0.0.1";

Client::Client() : client_socket(INVALID_SOCKET) {}

Client::~Client() {
    if (client_socket != INVALID_SOCKET) {
        closesocket(client_socket);
    }
    WSACleanup(); 
}

bool Client::connectToServer() {
    WSADATA wsaData;
    sockaddr_in server_address;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed." << endl;
        return false;
    }

    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        cerr << "Error creating socket: " << WSAGetLastError() << endl;
        WSACleanup();
        return false;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(client_socket, (SOCKADDR*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
        cerr << "connect failed with error: " << WSAGetLastError() << endl;
        closesocket(client_socket);
        WSACleanup();
        client_socket = INVALID_SOCKET;
        return false;
    }

    cout << "Connected to server." << endl;

    ReceiveThreadData* thread_data = new ReceiveThreadData; 
    thread_data->client_socket = client_socket;
    thread_data->client = this;

    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, receive_messages, thread_data) != 0) {
        cerr << "Failed to create receive thread." << endl;
        closesocket(client_socket);
        WSACleanup();
        client_socket = INVALID_SOCKET;
        delete thread_data; 
        return false;
    }

    pthread_detach(receive_thread);

    return true;
}


void* Client::receive_messages(void* arg) {
    ReceiveThreadData* data = (ReceiveThreadData*)arg;
    SOCKET client_socket = data->client_socket;
    Client* client = data->client; 
    char buffer[512];
    int bytes_read;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read > 0) {
            string received_message(buffer, bytes_read);

            // проверяем приватное ли сообщение
            size_t key_pos = received_message.find("*key*");
            if (key_pos != string::npos) {
                
                size_t key_start = key_pos + strlen("*key*");
                size_t message_pos = received_message.find("*message*", key_start);
                if (message_pos != string::npos) {
                    string key = received_message.substr(key_start, message_pos - key_start);
                    string encrypted_message = received_message.substr(message_pos + strlen("*message*"));

                    string decrypted_message = decrypt(encrypted_message, key);
                    cout << "Received private message: " << decrypted_message << endl;
                }
                 else
                {
                    cout << "Received:" << received_message << endl;
                }
            }
            else {
                cout << "Received:" << received_message << endl;
            }
        }
        else if (bytes_read == 0) {
            cout << "Server disconnected." << endl;
            break;
        }
        else {
            cerr << "recv failed: " << WSAGetLastError() << endl;
            break;
        }
    }

    closesocket(client_socket);
    delete data;
    pthread_exit(NULL); // закрываем поток
    return NULL;
}

string encrypt(const string& text, const string& key) {
    string encrypted;
    int key_length = key.length();

    for (int i = 0; i < text.length(); ++i) {
        char ch = text[i];
        int shift = (key[i % key_length] - 'A') % 26;

        if (isalpha(ch)) {
            char offset = isupper(ch) ? 'A' : 'a';
            ch = (ch - offset + shift) % 26 + offset;
        }
        encrypted += ch;
    }
    return encrypted;
}

string decrypt(const string& text, const string& key) {
    string decrypted;
    int key_length = key.length();

    for (int i = 0; i < text.length(); ++i) {
        char ch = text[i];
        int shift = (key[i % key_length] - 'A') % 26;

        if (isalpha(ch)) {
            char offset = isupper(ch) ? 'A' : 'a';
            ch = (ch - offset - shift + 26) % 26 + offset;
        }
        decrypted += ch;
    }
    return decrypted;
}
void Client::run() {
    char buffer[512];
    int bytes_read;
    pthread_t receive_thread;

    if (!connectToServer())
        return;

    memset(buffer, 0, sizeof(buffer));
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read > 0) {
        cout << "Message from server: " << buffer << endl;
    } else {
        cerr << "Error receiving data from server." << endl;
        return;
    }
    // структура данных для потока
    ReceiveThreadData* thread_data = new ReceiveThreadData;
    thread_data->client_socket = client_socket;
    thread_data->client = this;  

    if (pthread_create(&receive_thread, NULL, receive_messages, thread_data) != 0) {
        cerr << "Error creating receive thread." << endl;
        closesocket(client_socket);
        WSACleanup();
        delete thread_data;
        return;
    }
    cout << "> Enter message (TO{client ID}message for private, otherwise public). <" << endl;
    cout << "> Enter LOGIN{login:password} to connect witn status admin. <" << endl;

    // отправка сообщений в основном потоке
    while (true) {
        string message;
        getline(cin, message);

        // обработка приватных сообщений
        if (message.substr(0, 2).compare("TO") == 0) {
            string key;
            cout << "Enter encryption key: ";
            getline(cin, key);
            
            size_t pos = message.find("}");
            if (pos != string::npos && pos > 2) {
                string client_number_str = message.substr(3, pos - 2);
                int target_client_id;

                try {
                    target_client_id = stoi(client_number_str);
                    // щифрование и отправка сообщения без TO{id}
                    string private_message = message.substr(pos + 1);
                    string encrypted_message = encrypt(private_message, key);

                    string message_to_send = "TO{" + client_number_str + "}*key*" + key + "*message*" + encrypted_message;
                    send(client_socket, message_to_send.c_str(), message_to_send.length(), 0);
                } 
                catch (const invalid_argument& e) {
                    cerr << "Invalid client number." << endl;
                } 
                catch (const out_of_range& e) {
                    cerr << "Client number out of range." << endl;
                }
            } 
            else {
                cout << "Invalid private message format." << endl;
            }
        }
        // обработка обычных сообщений
        else {
            send(client_socket, message.c_str(), message.length(), 0);
        }
    }
}


void AdminClient::removeClient(int client_id_to_remove) {
    stringstream ss;
    ss << "REMOVE{" << client_id_to_remove << "}";
    string remove_message = ss.str();
    send(getSocket(), remove_message.c_str(), remove_message.length(), 0);
    cout << "Sent remove command for client " << client_id_to_remove << endl;
    }

void AdminClient::run() {
        Client::run(); // вызываем обычное поведение Client
        while (true) {
            string command;
            getline(cin, command);

            if (command.substr(0, 6) == "REMOVE") {
                size_t pos = command.find("{");
                size_t end_pos = command.find("}");

                if (pos != string::npos && end_pos != string::npos && end_pos > pos) {
                    string client_id_str = command.substr(pos + 1, end_pos - pos - 1);
                    try {
                        int client_id_to_remove = stoi(client_id_str);
                        removeClient(client_id_to_remove); // вызываем функцию удаления
                    } catch (const invalid_argument& e) {
                        cerr << "Invalid client number." << endl;
                    } catch (const out_of_range& e) {
                        cerr << "Client number out of range." << endl;
                    }
                } else {
                    cout << "Invalid REMOVE command format." << endl;
                }
            } else {
                send(getSocket(), command.c_str(), command.length(), 0); 
            }
        }
    }

