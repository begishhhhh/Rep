#include <iostream>
#include <winsock2.h>
#include <cstring>
#include <pthread.h> 
#include <unistd.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;


// Глобальные переменные для хранения сокетов клиентов
SOCKET client_socket1_global = INVALID_SOCKET;
SOCKET client_socket2_global = INVALID_SOCKET;

pthread_mutex_t cout_mutex = PTHREAD_MUTEX_INITIALIZER;

struct ThreadData {
    SOCKET client_socket;
    int client_number;  
};

// Функция выполняемая в отдельном потоке для обработки клиента
void* handle_client(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    SOCKET client_socket = data->client_socket;
    int client_number = data->client_number;
    char buffer[1024];
    int bytes_received;

    pthread_mutex_lock(&cout_mutex);
    cout << "Client " << client_number << " connected. Thread started." << endl;
    pthread_mutex_unlock(&cout_mutex);

    // Получаем сообщение от клиента
    memset(buffer, 0, sizeof(buffer));
    bytes_received = recv(client_socket, buffer, sizeof(buffer), 0); 
    if (bytes_received > 0) {

        pthread_mutex_lock(&cout_mutex);
        cout << "Received from client " << client_number << ": " << buffer << endl;
        pthread_mutex_unlock(&cout_mutex);
        
        // Находим целевой сокет
        SOCKET target_socket = (client_number == 1) ? client_socket2_global : client_socket1_global;
        
        // Отправляем сообщение целевому клиенту
        send(target_socket, buffer, bytes_received, 0);
    } else {
        pthread_mutex_lock(&cout_mutex);
        cerr << "Error receiving data from client " << client_number << "." << endl;
        pthread_mutex_unlock(&cout_mutex);
    }

    closesocket(client_socket);
    pthread_exit(NULL); // Завершение потока
    return NULL;
}


int main() {
    WSADATA wsaData;
    int result;
    SOCKET listening_socket = INVALID_SOCKET;
    pthread_t thread1, thread2;
    ThreadData* data1 = nullptr; // Инициализация указателей в nullptr
    ThreadData* data2 = nullptr;

    // Инициализация Winsock
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        cerr << "WSAStartup failed, result = " << result << endl;
        WSACleanup();
        return 1;
    }

    listening_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listening_socket == INVALID_SOCKET) {
        cout << "Socket creation error" << endl;
        goto clean;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);           // Порт, на котором будет работать сервер
    address.sin_addr.s_addr = INADDR_ANY;     // Любой доступный IP-адрес

    // Привязка сокета к адресу
    if (bind(listening_socket, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        cout << "Connection error" << endl;
        goto clean;
    }

    listen(listening_socket, 2);  // ожидаем максимум 2 подключения

    cout << "Server is listening..." << endl;

    // Принимаем первое подключение
    client_socket1_global = accept(listening_socket, NULL, NULL);
    if (client_socket1_global == INVALID_SOCKET) {
        cerr << "Accept failed" << endl;
        goto clean;
    }

    // Принимаем второе подключение
    client_socket2_global = accept(listening_socket, NULL, NULL);
    if (client_socket2_global == INVALID_SOCKET) {
        cerr << "Accept failed" << endl;
        goto clean;
    }

    // Создаем структуру данных для передачи в потоки
    data1 = new ThreadData;
    data1->client_socket = client_socket1_global;
    data1->client_number = 1;

    data2 = new ThreadData;
    data2->client_socket = client_socket2_global;
    data2->client_number = 2;

    // Создаем потоки
    if (pthread_create(&thread1, NULL, handle_client, data1) != 0) {
        cerr << "Failed to create thread 1" << endl;
        delete data1;
        goto clean;
    }

    if (pthread_create(&thread2, NULL, handle_client, data2) != 0) {
        cerr << "Failed to create thread 2" << endl;
        delete data1;
        delete data2;
        goto clean;
    }

    // Ожидаем завершения работы потоков
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    clean:
    closesocket(listening_socket);
    closesocket(client_socket1_global);
    closesocket(client_socket2_global);
    WSACleanup();
    return 0;
}
