#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <vector>
#include <arpa/inet.h>
#include <unistd.h>
#include <zmq.hpp>
#include <signal.h>

#define PORT 5511
#define BUFFER_SIZE 1024

std::vector<std::thread> clientThreads; // 클라이언트 스레드 목록

void handleClient(int client_socket, zmq::socket_t& socket) {
    char buffer[BUFFER_SIZE];

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(client_socket, buffer, BUFFER_SIZE, 0);

        if (bytesReceived <= 0) {
            std::cerr << "클라이언트 연결 종료" << std::endl;
            close(client_socket);
            return; // 스레드 종료
        }

        std::string receivedData(buffer);
        std::cout << "수신된 메시지: " << receivedData << std::endl;

        std::string response = "Data received!\n";
        send(client_socket, response.c_str(), response.length(), 0);

        // ZeroMQ 메시지 전송
        zmq::message_t request(receivedData.size());
        memcpy(request.data(), receivedData.c_str(), receivedData.size());
        socket.send(request, zmq::send_flags::none);

        // 응답 수신
        zmq::message_t reply;
        zmq::recv_result_t result = socket.recv(reply, zmq::recv_flags::none);
        if (result.has_value()) {
            std::cout << "Received message size: " << result.value() << std::endl;
        }
        std::cout << "서버 => " << std::string(static_cast<char*>(reply.data()), reply.size()) << std::endl << std::flush;
    }
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // TCP 소켓 생성
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "소켓 생성 실패" << std::endl;
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 소켓 바인딩
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "소켓 바인딩 실패" << std::endl;
        return -1;
    }

    // 클라이언트 연결 대기
    if (listen(server_fd, 5) < 0) {  // 여러 개의 연결을 대기
        std::cerr << "클라이언트 연결 대기 실패" << std::endl;
        return -1;
    }

    std::cout << "TCP 서버 실행 중... 포트: " << PORT << std::endl;

    zmq::context_t context(3);
    zmq::socket_t socket(context, ZMQ_REQ);
    socket.connect("tcp://localhost:6511");
    std::cout << "Main Server 연결 성공!" << std::endl;

    while (true) {
        std::cout << "클라이언트 연결 대기 중..." << std::endl;
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);

        if (client_socket < 0) {
            std::cerr << "클라이언트 연결 실패" << std::endl;
            continue;
        }

        std::cout << "클라이언트 연결됨!" << std::endl;

        // 새 클라이언트 처리 스레드 생성
        clientThreads.emplace_back(std::thread(handleClient, client_socket, std::ref(socket)));
    }

    close(server_fd);
    return 0;
}
