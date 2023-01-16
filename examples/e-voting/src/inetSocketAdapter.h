//
// Created by wld on 03.12.22.
//

#ifndef E_VOTING_INETSOCKETADAPTER_H
#define E_VOTING_INETSOCKETADAPTER_H

#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "network/abstractSocket.h"
#include "VotingAppConnectionRequestReply.h"

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
    void setSendPacket(inet::Packet& packet);
    void setSocket(inet::TcpSocket& socket);
    void recvAlt() override;

    //void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override;

private:
    inet::Packet sendOutPacket;
    inet::TcpSocket socket;
    voting::VotingAppConnectionRequestReply * requestReplyConnection;
};


#endif //E_VOTING_INETSOCKETADAPTER_H
