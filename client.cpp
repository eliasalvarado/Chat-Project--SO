#include <iostream>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include "chat.pb.h"

// Función para enviar la solicitud al servidor
void send_request(const chat::Request& request, chat::Response& response, const std::string& server_ip, int server_port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return;
    }

    if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return;
    }

    std::string request_str;
    request.SerializeToString(&request_str);
    send(sock, request_str.c_str(), request_str.size(), 0);

    char buffer[1024];
    int bytes_read = read(sock, buffer, sizeof(buffer));
    if (bytes_read > 0) {
        response.ParseFromArray(buffer, bytes_read);
    }

    close(sock);
}

int main(int argc, char const* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <username> <server_ip> <server_port>" << std::endl;
        return -1;
    }

    std::string username = argv[1];
    std::string server_ip = argv[2];
    int server_port = std::stoi(argv[3]);

    chat::Request request;
    request.set_operation(chat::Operation::REGISTER_USER);
    chat::NewUserRequest* new_user_request = request.mutable_register_user();
    new_user_request->set_username(username);

    chat::Response response;
    send_request(request, response, server_ip, server_port);

    if (response.status_code() == chat::StatusCode::OK) {
        std::cout << "The User " << username << " has been registered successfully." << std::endl;
    } else {
        std::cout << "Error: " << response.message() << std::endl;
        return -1;
    }

    return 0;
}
