#include "server.hpp"

int main() {
    ClientHandler::initialize();

    Server server(8080);
    server.run();

    ClientHandler::cleanup();
    return 0;
}