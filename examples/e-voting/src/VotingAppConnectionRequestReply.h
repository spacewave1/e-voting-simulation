//
// Created by wld on 10.01.23.
//

#ifndef E_VOTING_VOTINGAPPCONNECTIONREQUESTREPLY_H
#define E_VOTING_VOTINGAPPCONNECTIONREQUESTREPLY_H


#include <omnetpp/csimplemodule.h>
#include <inet/transportlayer/contract/tcp/TcpSocket.h>
#include "network/connectionService.h"

namespace voting {
    class VotingAppConnectionRequestReply : public inet::cSimpleModule, public inet::TcpSocket::ICallback {

    protected:
        inet::TcpSocket *requestSocket = nullptr;
        int replyBytesSent;
        virtual void handleMessage(inet::cMessage *message) override;

    public:
        virtual inet::TcpSocket *getSocket() { return requestSocket; }
        virtual void acceptSocket(inet::TcpAvailableInfo *availableInfo);
        virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override;
        virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override {}
        virtual void socketEstablished(inet::TcpSocket *socket) override {}
        virtual void socketPeerClosed(inet::TcpSocket *socket) override {}
        virtual void socketClosed(inet::TcpSocket *socket) override {}
        virtual void socketFailure(inet::TcpSocket *socket, int code) override {}
        virtual void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override {}
        virtual void socketDeleted(inet::TcpSocket *socket) override {}
        inet::Packet *createDataPacket(std::string send_string);
    };
}


#endif //E_VOTING_VOTINGAPPCONNECTIONREQUESTREPLY_H
