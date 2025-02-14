#include <iostream>
#include <winsock2.h>
#include <cstring>
#include <string>
#include <pthread.h> 
#include <unistd.h>
#include <fstream>   // для ofstream

#pragma comment(lib, "ws2_32.lib")

using namespace std;

string encrypt(const string& text, const string& key) {
    string encrypted;
    int key_length = key.length();

    for (int i=0; i < text.length(); ++i) {
        char ch = text[i];
        int shift = (key[i%key_length]-'A') % 26;

        if (isalpha(ch)) {
            char offset = isupper(ch) ? 'A' : 'a';
            ch = (ch - offset+shift)% 26 + offset;}
        encrypted += ch;
    }
    return encrypted;
}

string decrypt(const string& text, const string& key) {
    string decrypted;
    int key_length = key.length();

    for (int i=0; i < text.length(); ++i) {
        char ch = text[i];
        int shift = (key[i%key_length] - 'A') % 26;

        if (isalpha(ch)) {
            char offset = isupper(ch)?'A' : 'a';
            ch = (ch - offset - shift+26)% 26 + offset;}
        decrypted += ch;
    }
    return decrypted;
}

// Mutex чтобы не перебивали друг друга
pthread_mutex_t cout_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cin_mutex = PTHREAD_MUTEX_INITIALIZER;

// Структура для передачи данных в поток
struct ThreadData {
    SOCKET socket;
    sockaddr_in server_address;
    int client_id;
};

// Функция выполняемая в потоке для обработки клиента
void* handle_client(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    SOCKET client_socket = data->socket;
    sockaddr_in server_address = data->server_address;
    int client_id = data->client_id;
    string key;
    string message;
    
    char buffer[512];
    int bytes_read;

    // Освобождаем память выделенную для ThreadData в main()
    delete data;

    pthread_mutex_lock(&cout_mutex);
    cout << "Client " << client_id << ": Thread started." << endl;
    pthread_mutex_unlock(&cout_mutex);


    pthread_mutex_lock(&cout_mutex);
    cout << "Client " << client_id << ": Enter a key: ";
    pthread_mutex_lock(&cin_mutex);
    if (!getline(cin, key) || key != "YEBAH") {
        pthread_mutex_lock(&cout_mutex);
        cout << "Client " << client_id << ": Incorrect key" << endl;
        pthread_mutex_unlock(&cout_mutex);
        closesocket(client_socket);
    }
    pthread_mutex_unlock(&cin_mutex);
    pthread_mutex_unlock(&cout_mutex);

    // Отправка сообщений
    pthread_mutex_lock(&cout_mutex);
    cout << "Client " << client_id << ": Enter a message to other client: ";
    if (!getline(cin, message)) {
        pthread_mutex_lock(&cout_mutex);
        cerr << "Client " << client_id << ": Error reading message." << endl;
        pthread_mutex_unlock(&cout_mutex);
        closesocket(client_socket);
    }
    pthread_mutex_unlock(&cout_mutex);

    message = encrypt(message, key);
    auto result = send(client_socket, message.c_str(), message.length(), 0);
    if (result == SOCKET_ERROR) {
        pthread_mutex_lock(&cout_mutex);
        cerr << "Client " << client_id << ": send failed with error: " << WSAGetLastError() << endl;
        pthread_mutex_unlock(&cout_mutex);
        closesocket(client_socket);
    }
    //sleep(2);

    memset(buffer, 0, sizeof(buffer));
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read > 0) {
        string response(buffer, 0, bytes_read);

        pthread_mutex_lock(&cout_mutex);
        cout << "Client " << client_id << ": Enter a key for received message: ";
        if (!getline(cin, key) || key != "YEBAH") {
            cout << "Client " << client_id << ": Incorrect key" << endl;
            closesocket(client_socket);
       }
        pthread_mutex_unlock(&cout_mutex);

        pthread_mutex_lock(&cout_mutex);
        cout << "Client " << client_id << ": Message from other client: " << decrypt(response, key) << endl;
        ofstream outputFile("output.txt", ios_base::app);
        if (outputFile.is_open())
        {
            outputFile << "from client " << client_id << ": " << decrypt(response, key) << endl;
            outputFile.close();
        }
         else 
        {
            cerr << "Unable to open file for writing." << endl;
        }
        pthread_mutex_unlock(&cout_mutex);

    } else {
        pthread_mutex_lock(&cout_mutex);
        cerr << "Client " << client_id << ": Reading data error" << endl;
        pthread_mutex_unlock(&cout_mutex);
        closesocket(client_socket);
    }

    pthread_mutex_lock(&cout_mutex);
    cout << "Client " << client_id << ": Thread finished." << endl;
    pthread_mutex_unlock(&cout_mutex);

    pthread_exit(NULL); // Завершение потока
    return NULL;
}

int main() {
    WSADATA wsaData;
    int result;
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        cerr << "WSAStartup failed, result = " << result << endl;
        return 1;
    }

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    pthread_t thread1, thread2;

    // Создание структуры для передачи данных в поток 1
    ThreadData* data1 = new ThreadData; 
    data1->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (data1->socket == INVALID_SOCKET) {
        cerr << "Socket1 creation error" << endl;
        WSACleanup();
        return 1;
    }
    data1->server_address = server_address;
    data1->client_id = 1;
    

    //Создание структуры для передачи данных в поток 2
    ThreadData* data2 = new ThreadData;  
    data2->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (data2->socket == INVALID_SOCKET) {
        cerr << "Socket2 creation error" << endl;
        closesocket(data1->socket);
        WSACleanup();
        return 1;
    }
    data2->server_address = server_address;
    data2->client_id = 2;

    //Подключение к серверу.
    connect(data1->socket, (sockaddr*)&server_address, sizeof(server_address));
    connect(data2->socket, (sockaddr*)&server_address, sizeof(server_address));

    // Создание потоков
    if (pthread_create(&thread1, NULL, handle_client, data1) != 0) {
        cerr << "Failed to create thread1" << endl;
        closesocket(data1->socket);  
        delete data1; 
        WSACleanup();
        return 1;
    }
    if (pthread_create(&thread2, NULL, handle_client, data2) != 0) {
        cerr << "Failed to create thread2" << endl;
        delete data1;
        closesocket(data2->socket); 
        delete data2; 
        WSACleanup();
        closesocket(data1->socket);  
        pthread_cancel(thread1);  //Отменяем поток 1.
        return 1;
    }


    // Ожидание завершения потоков
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    delete data1;
    closesocket(data2->socket); 
    delete data2; 
    WSACleanup();
    closesocket(data1->socket);  
    pthread_cancel(thread1); 

    return 0;
}
