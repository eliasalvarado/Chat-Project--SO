#include <iostream>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "chat.pb.h"

struct UserSession {
    std::string username;
    chat::UserStatus status;
    int socket;
};

std::map<std::string, UserSession> users;
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

void print_registered_users() {
    std::cout << "Registered Users:" << std::endl;
    std::cout << "Users count: " << users.size() << std::endl;
    for (const auto& user_pair : users) {
        std::cout << "Username: " << user_pair.second.username << std::endl;
    }
    std::cout << "-----------------" << std::endl;
}

void handle_register_user(const chat::NewUserRequest& request, chat::Response& response) {
    pthread_mutex_lock(&users_mutex);
    if (users.find(request.username()) != users.end()) {
        response.set_status_code(chat::StatusCode::BAD_REQUEST);
        response.set_message("Username already taken");
    } else {
        UserSession session;
        session.username = request.username();
        session.status = chat::UserStatus::ONLINE;
        // TODO: Set socket

        users[session.username] = session;
        response.set_status_code(chat::StatusCode::OK);
        response.set_message("User registered successfully");
    }
    print_registered_users();
    pthread_mutex_unlock(&users_mutex);
}

void handle_client(int client_socket) {
    char buffer[1024];
    while (true) {
        int bytes_read = read(client_socket, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            close(client_socket);
            pthread_exit(NULL);
        }

        chat::Request request;
        request.ParseFromArray(buffer, bytes_read);

        chat::Response response;
        switch (request.operation()) {
            case chat::Operation::REGISTER_USER:
                handle_register_user(request.register_user(), response);
                break;
            // Add cases for other operations
            default:
                response.set_status_code(chat::StatusCode::BAD_REQUEST);
                response.set_message("Unknown operation");
        }

        std::string response_str;
        response.SerializeToString(&response_str);
        send(client_socket, response_str.c_str(), response_str.size(), 0);
    }

    close(client_socket);
}

void* handle_client_wrapper(void* client_socket) {
    handle_client(*(int*)client_socket);
    delete (int*)client_socket;
    return NULL;
}

int main(int argc, char const* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return -1;
    }

    int port = std::stoi(argv[1]);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);

    printf("SUCCESS: listening on port-> %d\n", port);

    while (true) {
        sockaddr_in client_address;
        socklen_t client_addrlen = sizeof(client_address);
        int client_socket = accept(server_fd, (sockaddr*)&client_address, &client_addrlen);

        int* pclient = new int;
        *pclient = client_socket;
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client_wrapper, (void*)pclient);
    }

    close(server_fd);
    return 0;
}