#include <iostream>
#include <string>
#include <unordered_map>
#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include "chat.pb.h"

struct UserSession {
    std::string username;
    std::string ip_address;
    chat::UserStatus status;
    int socket;
    std::chrono::system_clock::time_point last_activity;
};

std::unordered_map<std::string, UserSession> users;
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;
int inactivity_timeout = 30;

void handle_register_user(const chat::NewUserRequest& request, chat::Response& response, int socket, const std::string& client_ip) {
    pthread_mutex_lock(&users_mutex);
    if (users.find(request.username()) != users.end()) {
        response.set_status_code(chat::StatusCode::BAD_REQUEST);
        response.set_message("Username already taken");
    } else {
        UserSession session;
        session.username = request.username();
        session.ip_address = client_ip;
        session.status = chat::UserStatus::ONLINE;
        session.socket = socket;
        session.last_activity = std::chrono::system_clock::now();

        users[session.username] = session;
        response.set_status_code(chat::StatusCode::OK);
        response.set_message("User registered successfully");
        
        std::cout << "User registered: " << session.username << " with IP: " << session.ip_address << std::endl;
    }
    pthread_mutex_unlock(&users_mutex);
}

void handle_update_status(const chat::UpdateStatusRequest& request, chat::Response& response) {
    pthread_mutex_lock(&users_mutex);
    auto it = users.find(request.username());
    if (it != users.end()) {
        it->second.status = request.new_status();
        it->second.last_activity = std::chrono::system_clock::now(); 
        response.set_status_code(chat::StatusCode::OK);
        response.set_message("Status updated successfully");
        std::cout << "User " << request.username() << " changed status to " << chat::UserStatus_Name(request.new_status()) << std::endl;
    } else {
        response.set_status_code(chat::StatusCode::BAD_REQUEST);
        response.set_message("User not found");
        
        std::cout << "Failed to update status. Registered users:" << std::endl;
        for (const auto& user : users) {
            std::cout << "User: " << user.second.username << ", IP: " << user.second.ip_address << std::endl;
        }
        std::cout << "Attempted to change status for user: " << request.username() << std::endl;
    }
    pthread_mutex_unlock(&users_mutex);
}

void handle_client_disconnection(int client_sock) {
    pthread_mutex_lock(&users_mutex);
    for (auto it = users.begin(); it != users.end(); ++it) {
        if (it->second.socket == client_sock) {
            users.erase(it);
            break;
        }
    }
    pthread_mutex_unlock(&users_mutex);
    close(client_sock);
}

void handle_get_users(const chat::UserListRequest& user_list_request, chat::UserListResponse* user_list_response) {
    // std::cout << "handle get users: " << user_list_request.username() << std::endl;
    if (user_list_request.username().empty()) {
        for (const auto& user : users) {
            if (user.second.status == chat::UserStatus::ONLINE) {
                chat::User* user_proto = user_list_response->add_users();
                user_proto->set_username(user.first);
            }
        }
    } else {
        auto it = users.find(user_list_request.username());
        if (it != users.end()) {
            user_list_response->clear_users();
            chat::User* user_proto = user_list_response->add_users();
            user_proto->set_username(it->first);
            user_proto->set_ip_address(it->second.ip_address);
            user_proto->set_status(it->second.status);
        }
    }
}


void handle_client(int socket) {
    char buffer[1024];

    while (true) {
        int bytes_read = read(socket, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            handle_client_disconnection(socket);
            return;
        }

        chat::Request request;
        request.ParseFromArray(buffer, bytes_read);

        chat::Response response;
        switch (request.operation()) {
            case chat::Operation::REGISTER_USER:
                handle_register_user(request.register_user(), response, socket, "TODO: Get IP Address");
                break;
            case chat::Operation::UPDATE_STATUS:
                handle_update_status(request.update_status(), response);
                break;
            case chat::Operation::GET_USERS:
                handle_get_users(request.get_users(), response.mutable_user_list());
                break;
            default:
                response.set_status_code(chat::StatusCode::BAD_REQUEST);
                response.set_message("Unknown operation");
        }

        pthread_mutex_lock(&users_mutex);
        for (auto& user : users) {
            if (user.second.socket == socket && chat::Operation::UPDATE_STATUS != request.operation()) {
                std::cout << "The user: " << user.second.username << " will be updated to ONLINE" << std::endl;
                user.second.last_activity = std::chrono::system_clock::now();
                user.second.status = chat::UserStatus::ONLINE;
                break;
            }
        }
        pthread_mutex_unlock(&users_mutex);

        std::string response_str;
        response.SerializeToString(&response_str);
        send(socket, response_str.c_str(), response_str.size(), 0);
    }

    close(socket);
}


void* handle_client_wrapper(void* client_socket) {
    int socket = *(int*)client_socket;
    free(client_socket);
    handle_client(socket);
    return NULL;
}

void* inactivity_monitor(void*) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto now = std::chrono::system_clock::now();

        pthread_mutex_lock(&users_mutex);
        for (auto& user : users) {
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - user.second.last_activity).count();
            if (duration >= inactivity_timeout && user.second.status != chat::UserStatus::OFFLINE) {
                user.second.status = chat::UserStatus::OFFLINE;
                std::cout << "User " << user.second.username << " set to OFFLINE due to inactivity" << std::endl;
            }
        }
        pthread_mutex_unlock(&users_mutex);
    }
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

    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, inactivity_monitor, NULL);

    printf("The server is listening on port: %d\n", port);

    while (true) {
        sockaddr_in client_address;
        socklen_t client_addrlen = sizeof(client_address);
        int client_socket = accept(server_fd, (sockaddr*)&client_address, &client_addrlen);

        int* new_sock = new int;
        *new_sock = client_socket;
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client_wrapper, (void*)new_sock);
    }

    close(server_fd);
    return 0;
}
