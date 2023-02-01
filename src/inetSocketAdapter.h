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
    bool isBound() override;
    void setSocket(inet::TcpSocket* socket);
    void recvAlt() override;
    //void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override;
    void setParentComponent(inet::cComponent* component);
    void addProgrammedMessage(socketMessage message);
    void setMsgKind(uint8_t msgKind);
    int getBytesSent() const;

private:
    inet::Packet* sendOutPacket;
    inet::TcpSocket* socket;
    inet::cComponent* parentComponent;
    std::queue<socketMessage> programmed_message_queue;
    uint8_t msg_kind;
    logger _logger = logger::Instance();
    long bytesSent = 0;
};


#endif //E_VOTING_INETSOCKETADAPTER_H
