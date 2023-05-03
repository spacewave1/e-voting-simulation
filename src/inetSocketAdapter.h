//
// Created by wld on 03.12.22.
//

#ifndef E_VOTING_INETSOCKETADAPTER_H
#define E_VOTING_INETSOCKETADAPTER_H

#include <queue>
#include "network/abstractSocket.h"
#include "evoting/logger.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"

class inetSocketAdapter : public abstractSocket {
public:
    void send(std::string payload) override;
    socketMessage recv() override;
    socketMessage interruptableRecv(bool &is_interrupt) override;
    void disconnect(std::string protocol, std::string address, size_t port = 0) override;
    void connect(std::string protocol, std::string address, size_t port = 0) override;
    void bind(std::string protocol, std::string address, size_t port = 0) override;
    void unbind(std::string protocol, std::string address, size_t port = 0) override;
    void close() override;
    void setSocket(inet::TcpSocket* socket);
    void listen() override;
    //void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override;
    void setParentComponent(inet::cComponent* component);
    void addProgrammedMessage(socketMessage message);
    void setMsgKind(uint8_t msg_kind);
    int getBytesSent() const;
    void setupSocket(std::string local_address, size_t port) override;
    void setIsMultiPackageData(bool newValue);

private:
    inet::Packet* send_out_packet;
    inet::TcpSocket* socket;
    inet::cComponent* parent_component;
    std::queue<socketMessage> programmed_message_queue;
    uint8_t msg_kind;
    logger _logger = logger::Instance();
    long bytes_sent = 0;
    bool is_multi_package_data = false;
    std::vector<u_int8_t> exit_sequence = { '#', '#', '#', '#'};

};


#endif //E_VOTING_INETSOCKETADAPTER_H
