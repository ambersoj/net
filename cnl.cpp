#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <cstdlib>

#define PORT_BASE 7000

class UDPChannel {
public:
    UDPChannel() = default;
    UDPChannel(const std::string& ip, int port) : ip(ip), port(port) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("Socket creation failed");
            exit(1);
        }
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        addr.sin_port = htons(port);
        if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("Bind failed");
            exit(1);
        }
        std::cout << "UDPChannel bound to " << ip << ":" << port << " with sockfd " << sockfd << std::endl;
    }

    UDPChannel(const UDPChannel&) = delete;
    UDPChannel& operator=(const UDPChannel&) = delete;

    UDPChannel(UDPChannel&& other) noexcept
        : sockfd(other.sockfd), ip(std::move(other.ip)), port(other.port) {
        other.sockfd = -1;
    }

    UDPChannel& operator=(UDPChannel&& other) noexcept {
        if (this != &other) {
            close(sockfd);
            sockfd = other.sockfd;
            ip = std::move(other.ip);
            port = other.port;
            other.sockfd = -1;
        }
        return *this;
    }

    void send(const std::string& dst_ip, int dst_port, const std::string& message) {
        if (sockfd < 0) {
            std::cerr << "Socket not initialized properly!" << std::endl;
            return;
        }
        sockaddr_in dest{};
        dest.sin_family = AF_INET;
        dest.sin_addr.s_addr = inet_addr(dst_ip.c_str());
        dest.sin_port = htons(dst_port);
        std::cout << "Sending to " << dst_ip << ":" << dst_port << " via socket " << sockfd << std::endl;
        ssize_t bytes_sent = sendto(sockfd, message.c_str(), message.size(), 0, (struct sockaddr*)&dest, sizeof(dest));
        if (bytes_sent < 0) {
            perror("sendto failed");
        }
    }

    std::string recv() {
        char buffer[1024] = {0};
        sockaddr_in sender{};
        socklen_t sender_len = sizeof(sender);
        ssize_t len = recvfrom(sockfd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT, (struct sockaddr*)&sender, &sender_len);
        if (len > 0) {
            buffer[len] = '\0';
            return std::string(buffer);
        }
        return "";
    }

    ~UDPChannel() {
        if (sockfd >= 0) {
            close(sockfd);
        }
    }

private:
    int sockfd;
    std::string ip;
    int port;
};

class Command {
public:
    virtual void execute() = 0;
    virtual ~Command() = default;
};

class SendCommand : public Command {
public:
    SendCommand(UDPChannel& channel, const std::string& dst_ip, int dst_port, const std::string& message)
        : channel(channel), dst_ip(dst_ip), dst_port(dst_port), message(message) {}
    
    void execute() override { channel.send(dst_ip, dst_port, message); }
    
private:
    UDPChannel& channel;
    std::string dst_ip;
    int dst_port;
    std::string message;
};

class RecvCommand : public Command {
public:
    RecvCommand(UDPChannel& channel) : channel(channel) {}
    
    void execute() override {
        std::string msg = channel.recv();
        if (!msg.empty()) std::cout << "Received: " << msg << std::endl;
        else std::cout << "> No data available" << std::endl;
    }
    
private:
    UDPChannel& channel;
};

class CommandInvoker {
public:
    void addCommand(Command* cmd) {
        commandQueue.push(cmd);
    }
    
    void executeCommands() {
        while (!commandQueue.empty()) {
            Command* cmd = commandQueue.front();
            commandQueue.pop();
            cmd->execute();
            std::cout << "> ";
            delete cmd;
        }
    }
    
private:
    std::queue<Command*> commandQueue;
};

void run(std::unordered_map<int, UDPChannel>& channels, CommandInvoker& invoker) {
    struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };
    std::cout << "> ";
    while (true) {
//        std::cout << "> ";
        std::cout.flush();
        invoker.executeCommands();
        if (poll(&pfd, 1, 100) > 0) {
            std::string line;
            std::getline(std::cin, line);
            if (line == "exit") break;
            
            std::vector<std::string> tokens;
            std::istringstream iss(line);
            std::string token;
            while (iss >> token) tokens.push_back(token);
            
            if (tokens.empty()) continue;
            if (tokens[0] == "send" && tokens.size() >= 5) {
                int ch = std::stoi(tokens[1]);
                if (channels.find(ch) != channels.end()) {
                    int dst_port = std::stoi(tokens[3]);
                    std::string message = line.substr(line.find(tokens[4]));
                    invoker.addCommand(new SendCommand(channels[ch], tokens[2], dst_port, message));
                } else {
                    std::cerr << "> Invalid channel: " << ch << std::endl;
                }
            } else if (tokens[0] == "recv" && tokens.size() == 2) {
                int ch = std::stoi(tokens[1]);
                if (channels.find(ch) != channels.end()) {
                    invoker.addCommand(new RecvCommand(channels[ch]));
                } else {
                    std::cerr << "> Invalid channel: " << ch << std::endl;
                }
            } else {
                std::cout << "> Invalid command" << std::endl;
            }
        }
    }
}

int main() {
    int base_port = PORT_BASE;
    std::unordered_map<int, UDPChannel> channels;
    CommandInvoker invoker;
    for (int i = 0; i < 6; ++i) {
        channels.emplace(i, UDPChannel("127.0.0.1", base_port + i));
    }
    run(channels, invoker);
    return 0;
}
