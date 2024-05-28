#include <iostream>
#include <string>
#include <unordered_map>
#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "chat.pb.h"

void broadcast_message(int sock);
void send_private_message(int sock);
void change_status(int sock);
void list_users(int sock);
void display_user_info(int sock);
void display_help();
void exit_chat(int sock);

std::string username;
pthread_mutex_t lock;

bool register_user(const std::string& username, const std::string& server_ip, int server_port, int& sock) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return false;
    }

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip.c_str(), &server_address.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return false;
    }

    if (connect(sock, (sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return false;
    }

    chat::NewUserRequest new_user_request;
    new_user_request.set_username(username);

    chat::Request request;
    request.set_operation(chat::Operation::REGISTER_USER);
    *request.mutable_register_user() = new_user_request;

    std::string request_str;
    request.SerializeToString(&request_str);
    send(sock, request_str.c_str(), request_str.size(), 0);

    char buffer[1024];
    int bytes_read = read(sock, buffer, sizeof(buffer));
    if (bytes_read > 0) {
        chat::Response response;
        response.ParseFromArray(buffer, bytes_read);
        if (response.status_code() == chat::StatusCode::OK) {
            std::cout << "Response: " << response.message() << std::endl;
            return true;
        } else {
            std::cout << "Response: " << response.message() << std::endl;
            return false;
        }
    }

    return false;
}

void show_menu() {
    std::cout << "Chat Menu:" << std::endl;
    std::cout << "1. Chat with all users (broadcast)" << std::endl;
    std::cout << "2. Send a private message" << std::endl;
    std::cout << "3. Change status" << std::endl;
    std::cout << "4. List connected users" << std::endl;
    std::cout << "5. Display user information" << std::endl;
    std::cout << "6. Help" << std::endl;
    std::cout << "7. Exit" << std::endl;
    std::cout << "Enter your choice: ";
}

void handle_choice(int choice, int sock) {
    switch (choice) {
        case 1:
            broadcast_message(sock);
            break;
        case 2:
            send_private_message(sock);
            break;
        case 3:
            change_status(sock);
            break;
        case 4:
            list_users(sock);
            break;
        case 5:
            display_user_info(sock);
            break;
        case 6:
            display_help();
            break;
        case 7:
            exit_chat(sock);
            break;
        default:
            std::cout << "Invalid choice. Please try again." << std::endl;
    }
}

void broadcast_message(int sock) {
    std::cout << "-----Broadcast message-----" << std::endl;
    std::cout << "Enter message to broadcast: ";
    std::string message;
    std::cin.ignore();
    std::getline(std::cin, message);

    chat::SendMessageRequest send_message_request;
    send_message_request.set_content(message);

    chat::Request request;
    request.set_operation(chat::Operation::SEND_MESSAGE);
    *request.mutable_send_message() = send_message_request;

    std::string request_str;
    request.SerializeToString(&request_str);
    send(sock, request_str.c_str(), request_str.size(), 0);

    char buffer[1024];
    int bytes_read = read(sock, buffer, sizeof(buffer));
    if (bytes_read > 0) {
        chat::Response response;
        response.ParseFromArray(buffer, bytes_read);
        std::cout << "Server response: " << response.message() << std::endl;
    }
}


void send_private_message(int sock) {
    std::cout << "-----Private message functionality-----" << std::endl;
    std::cout << "Enter recipient username: ";
    std::string recipient;
    std::cin >> recipient;
    std::cout << "Enter message to send: ";
    std::string message;
    std::cin.ignore();
    std::getline(std::cin, message);

    chat::SendMessageRequest send_message_request;
    send_message_request.set_recipient(recipient);
    send_message_request.set_content(message);

    chat::Request request;
    request.set_operation(chat::Operation::SEND_MESSAGE);
    *request.mutable_send_message() = send_message_request;

    std::string request_str;
    request.SerializeToString(&request_str);
    send(sock, request_str.c_str(), request_str.size(), 0);

    char buffer[1024];
    int bytes_read = read(sock, buffer, sizeof(buffer));
    if (bytes_read > 0) {
        chat::Response response;
        response.ParseFromArray(buffer, bytes_read);
        std::cout << "Server response: " << response.message() << std::endl;
    }
}


void change_status(int sock) {
    std::cout << "-----Change status functionality-----" << std::endl;

    std::cout << "Select your new status:" << std::endl;
    std::cout << "1. ONLINE" << std::endl;
    std::cout << "2. BUSY" << std::endl;
    std::cout << "3. OFFLINE" << std::endl;
    std::cout << "Enter your choice: ";

    int status_choice;
    std::cin >> status_choice;

    chat::UserStatus new_status;
    switch (status_choice) {
        case 1:
            new_status = chat::UserStatus::ONLINE;
            break;
        case 2:
            new_status = chat::UserStatus::BUSY;
            break;
        case 3:
            new_status = chat::UserStatus::OFFLINE;
            break;
        default:
            std::cout << "Invalid status choice. Please try again." << std::endl;
            return;
    }

    chat::UpdateStatusRequest update_status_request;
    update_status_request.set_username(username);
    update_status_request.set_new_status(new_status);

    chat::Request request;
    request.set_operation(chat::Operation::UPDATE_STATUS);
    *request.mutable_update_status() = update_status_request;

    std::string request_str;
    request.SerializeToString(&request_str);
    send(sock, request_str.c_str(), request_str.size(), 0);

    char buffer[1024];
    int bytes_read = read(sock, buffer, sizeof(buffer));
    if (bytes_read > 0) {
        chat::Response response;
        response.ParseFromArray(buffer, bytes_read);
        if (response.status_code() == chat::StatusCode::OK) {
            std::cout << "Status changed successfully to " << chat::UserStatus_Name(new_status) << std::endl;
        } else {
            std::cout << "Failed to change status: " << response.message() << std::endl;
        }
    }
}

void list_users(int sock) {
    chat::Request request;
    request.set_operation(chat::Operation::GET_USERS);

    std::string request_str;
    request.SerializeToString(&request_str);
    send(sock, request_str.c_str(), request_str.size(), 0);

    char buffer[1024];
    int bytes_read = read(sock, buffer, sizeof(buffer));
    if (bytes_read > 0) {
        chat::Response response;
        response.ParseFromArray(buffer, bytes_read);
        std::cout << "-----Online users-----" << std::endl;
        for (const auto& user : response.user_list().users()) {
            std::cout << "Username: " << user.username() << std::endl;
        }
    } else {
        std::cerr << "Failed to receive response from server." << std::endl;
    }
}

void display_user_info(int sock) {
    std::cerr << "-----User Info-----" << std::endl;
    std::cout << "Enter the username of the user you want to get information about: ";
    std::string user_to_search;
    std::cin >> user_to_search;

    chat::Request request;
    request.set_operation(chat::Operation::GET_USERS);
    request.mutable_get_users()->set_username(user_to_search);

    std::string request_str;
    request.SerializeToString(&request_str);
    send(sock, request_str.c_str(), request_str.size(), 0);

    char buffer[1024];
    int bytes_read = read(sock, buffer, sizeof(buffer));
    if (bytes_read > 0) {
        chat::Response response;
        response.ParseFromArray(buffer, bytes_read);

        if (response.user_list().users_size() > 0) {
            const auto& user = response.user_list().users(0);
            std::cout << user.username() << "'s information: " << std::endl;
            std::cout << "Username: " << user.username() << std::endl;
            std::cout << "IP Address: " << user.ip_address() << std::endl;
            std::cout << "Status: " << chat::UserStatus_Name(user.status()) << std::endl;
        } else {
            std::cout << "User not found or not online." << std::endl;
        }
    } else {
        std::cerr << "Failed to receive response from server." << std::endl;
    }
}


void display_help() {
    std::cout << "-----Help-----" << std::endl;
    std::cout << "1. Chat with all users (broadcast): Send a message to all online users." << std::endl;
    std::cout << "2. Send a private message: Send a message to a specific user." << std::endl;
    std::cout << "3. Change status: Update your status (ONLINE, BUSY, OFFLINE)." << std::endl;
    std::cout << "4. List connected users: Get a list of all online users." << std::endl;
    std::cout << "5. Display user information: Get detailed information about a specific user." << std::endl;
    std::cout << "6. Help: Display this help message." << std::endl;
    std::cout << "7. Exit: Leave the chat application." << std::endl;
}


void exit_chat(int sock) {
    std::cout << "Exiting chat..." << std::endl;
    close(sock);
    exit(0);
}

void receive_messages(int sock) {
    fd_set read_fds;
    struct timeval tv;
    char buffer[1024];

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);

        // Configura un timeout para select()
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        int retval = select(sock + 1, &read_fds, NULL, NULL, &tv);
        if (retval == -1) {
            perror("select()");
            break;
        } else if (retval) {
            // Datos disponibles para leer
            int bytes_read = read(sock, buffer, sizeof(buffer));
            if (bytes_read > 0) {
                chat::Response response;
                response.ParseFromArray(buffer, bytes_read);

                if (response.operation() == chat::Operation::INCOMING_MESSAGE) {
                    const chat::IncomingMessageResponse& msg = response.incoming_message();
                    std::string message_type = (msg.type() == chat::MessageType::BROADCAST) ? "Broadcast" : "Direct";

                    std::cout << "-----New Message Incoming-----\n";
                    std::cout << "From: " << msg.sender() << "\n";
                    std::cout << "Type: " << message_type << "\n";
                    std::cout << "Content: " << msg.content() << "\n";
                    std::cout << "----------------------\n";
                }
            } else if (bytes_read == 0) {
                // El servidor cerró la conexión
                std::cout << "Server closed connection\n";
                break;
            }
        }
    }
}


void* receive_messages_wrapper(void* arg) {
    pthread_mutex_lock(&lock);
    int sock = *(int*)arg;
    receive_messages(sock);
    pthread_mutex_unlock(&lock);
    return nullptr;
}


int main(int argc, char const* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <username> <server_ip> <server_port>" << std::endl;
        return -1;
    }

    username = argv[1];
    std::string server_ip = argv[2];
    int server_port = std::stoi(argv[3]);

    int sock;
    if (!register_user(username, server_ip, server_port, sock)) {
        std::cerr << "Failed to register user" << std::endl;
        return -1;
    }

    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    pthread_mutex_init(&lock, NULL);

    pthread_t receiver_thread;
    pthread_create(&receiver_thread, nullptr, receive_messages_wrapper, &sock);

    int choice;
    while (true) {
        show_menu();
        std::cin >> choice;
        handle_choice(choice, sock);
    }

    pthread_mutex_destroy(&lock);

    return 0;
}
