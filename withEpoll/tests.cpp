#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <regex>
#include <memory>
#include <functional>
#include <fstream>

struct SConnectionInfo {
public:
    std::string host;
    ushort port;
};

class Socket {
public:
    Socket() {
        sockfd_= socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ <= 0) {
            throw std::logic_error("Socket init failed");
        }
        memset(&socketAddress_, 0, sizeof(socketAddress_));
    }

    explicit Socket(int sockfd) : sockfd_(sockfd) {
        if (sockfd_ <= 0) {
            throw std::logic_error("Socket init failed");
        }
    }

    int getFD() const {
        return sockfd_;
    }

    void init(const SConnectionInfo& connectionInfo) {
        socketAddress_.sin_family = AF_INET;
        socketAddress_.sin_port = htons(connectionInfo.port);
        if (connectionInfo.host.empty()) {
            int opt = 1;
            if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
                throw std::logic_error("Socket set options failed");
            }
            socketAddress_.sin_addr.s_addr = INADDR_ANY;
            if (bind(sockfd_, (struct sockaddr*)&socketAddress_, sizeof(socketAddress_)) < 0) {
                throw std::logic_error("Socket bind failed");
            }
        } else {
            if (inet_pton(AF_INET, connectionInfo.host.c_str(), &socketAddress_.sin_addr) <= 0) {
                throw std::logic_error("Socket set host failed");
            }
        }
    }

    static int setNonBlocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) {
            return -1;
        }
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            return -1;
        }
        return 0;
    }

    void connecting() {
        if (connect(sockfd_, (struct sockaddr *)(&socketAddress_), sizeof(socketAddress_)) < 0) {
            throw std::logic_error("Socket connect failed");
        }
    }

    void accepting(std::function<void(std::shared_ptr<Socket> clientSocket, int eventFlag)> function) {
        if (listen(sockfd_, 10) < 0) {
            throw std::logic_error("Listen socket failed");
        }
        Socket epollfd(epoll_create1(0));

        struct epoll_event ev, events[EventSize];
        ev.events = EPOLLIN;
        ev.data.fd = sockfd_;

        if (epoll_ctl(epollfd.getFD(), EPOLL_CTL_ADD, sockfd_, &ev) == -1) {
            throw std::logic_error("Socket epoll ctl failed");
        }

        while (true) {
            int nfds = epoll_wait(epollfd.getFD(), events, EventSize, -1);
            if (nfds == -1) {
                throw std::logic_error("Socket epoll wait failed");
            }

            for (int n = 0; n < nfds; ++n) {
                if (events[n].data.fd == sockfd_) {
                    // Принятие нового соединения
                    int new_socket = 0;
                    int addrlen = sizeof(socketAddress_);
                    while ((new_socket = accept(sockfd_, (struct sockaddr*)&socketAddress_, (socklen_t*)&addrlen)) != -1) {
                        if (setNonBlocking(new_socket) == -1) {
                            continue;
                        }
                        ev.events = EPOLLIN | EPOLLET;
                        ev.data.fd = new_socket;
                        if (epoll_ctl(epollfd.getFD(), EPOLL_CTL_ADD, new_socket, &ev) == -1) {
                            perror("epoll_ctl: new_socket");
                            close(new_socket);
                            continue;
                        }
                        function(std::make_shared<Socket>(new_socket), 0);
                        std::cout << "Accepted new connection on descriptor " << new_socket << std::endl;
                    }
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("accept");
                    }
                } else {
                    // Обработка данных от клиента
                    int currSocket = events[n].data.fd;
                    function(std::make_shared<Socket>(currSocket), 1);
                }
            }
        }

        epollfd.deinit();
    }

    int reading(char* buffer, int BufferSize = 4096) {
        int result = -1;
        if ((result = read(sockfd_, buffer, BufferSize)) <= 0) {
            throw std::logic_error("Socket read failed");
        }
        return result;
    }

    int writing(char* buffer, int BufferSize = 4096) {
        int result = -1;
        if ((result = write(sockfd_, buffer, BufferSize)) <= 0) {
            throw std::logic_error("Socket write failed");
        }
        return result;
    }

    void deinit() {
        if (sockfd_ > 0) {
            std::cout << "close socket " << sockfd_ << std::endl;
            close(sockfd_);
            sockfd_ = 0;
        }
    }

    ~Socket() {
        
    }
private:
    static const int EventSize = 32;
    int sockfd_;
    struct sockaddr_in socketAddress_;
};

std::unique_ptr<Socket> initDbFD(const SConnectionInfo& dbInfo) {
    std::unique_ptr<Socket> dbfd = std::make_unique<Socket>();

    dbfd->init(dbInfo);
    dbfd->connecting();
    return dbfd;
}

ushort parseConnectionPort(const std::string& port) {
    if (port.empty() || std::find_if(port.begin(), port.end(), [](const char& c) { return !isdigit(c); }) != port.end()) {
        throw std::logic_error("Port is invalid: please pass the database parameters like ./proxy <db_host> <db_port> <server_port>");
    }
    return std::stoi(port);
}

std::string parseConnectionHost(const std::string& host) {
    static std::regex pattern("(\\d{1,9}(\\.\\d{1,9}){3})");
    if (host.empty() || !std::regex_match(host, pattern)) {
        throw std::logic_error("Host is invalid: please pass the database parameters like ./proxy <db_host> <db_port> <server_port>");
    }
    return host;
}

SConnectionInfo parseDatabaseArguments(const std::string& host, const std::string& port) {
    return SConnectionInfo{parseConnectionHost(host), parseConnectionPort(port)};
}

void logining(char* buffer, int bytes) {
    static int count = 0;

	std::string line(buffer, bytes - 1);
	if (line.empty())
		return;
	if (static_cast<int>(line[0]) != 81)
		return;
	std::ofstream file("log.txt", std::ios::app |  std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "Log.txt not open\n";
		return;
	}

	line = line.substr(5, bytes - 6);
	file << line << std::endl;
	file << std::endl;
	file.close();
}

int main(int argc, char** argv) {
    try {
        if (argc < 4) {
            throw std::logic_error("Please pass the database parameters like ./proxy <db_host> <db_port> <server_port>");
            return -1;
        }
        const SConnectionInfo& dbInfo = parseDatabaseArguments(argv[1], argv[2]);
        std::map<int, std::unique_ptr<Socket>> clientDatabases;
        Socket serverSock;

        serverSock.init(SConnectionInfo{"", parseConnectionPort(argv[3])});

        Socket::setNonBlocking(serverSock.getFD());

        serverSock.accepting([&dbInfo, &clientDatabases](std::shared_ptr<Socket> clientSocket, int eventFlag) {
            if (eventFlag == 0) {
                if (clientDatabases.count(clientSocket->getFD()) != 0) {
                    clientDatabases.erase(clientSocket->getFD());
                }
                clientDatabases[clientSocket->getFD()] = initDbFD(dbInfo);
                return;
            }
            auto db = std::move(clientDatabases.at(clientSocket->getFD()));
            try {
                char buf[4096];
                int bytesRead = 0;
                while ((bytesRead = clientSocket->reading(buf)) >= 0) {
                    logining(buf, bytesRead);
                    db->writing(buf, bytesRead);
                    int dbResult = db->reading(buf);
                    clientSocket->writing(buf, dbResult);
                }
            } catch (const std::exception& ex) {
                std::cerr << ex.what() << std::endl;
            }
            clientDatabases[clientSocket->getFD()] = std::move(db);
        });
        for (const auto& [key, value] : clientDatabases) {
            close(key);
            value->deinit();
        }
    } catch(const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return -1;
    }
    
    return 0;
}