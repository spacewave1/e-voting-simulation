//
// Created by wld on 27.11.22.
//

#ifndef E_VOTING_APP_H
#define E_VOTING_APP_H

#include "inet/common/INETDefs.h"
#include <inet/applications/tcpapp/TcpSessionApp.h>
#include <inet/common/socket/SocketMap.h>
#include "inetSocketAdapter.h"
#include "network/connectionService.h"
#include "inet/applications/tcpapp/TcpServerSocketIo.h"
#include "VotingAppConnectionRequestReply.h"
#include "evoting/peer.h"

namespace voting {
class VotingApp: public inet::TcpAppBase {
        inet::cMessage* connectSelfMessage;
        inet::cMessage* sendDataSelfMessage;
        std::set<VotingAppConnectionRequestReply *> connectionSet;
        bool isReceiving;
        connectionService connection_service;

        // TODO: Migrate to use peer
        std::set<std::string> nodes;
        std::string nodesString;

    void handleTimer(inet::cMessage *msg) override;
        void handleCrashOperation(inet::LifecycleOperation*) override;
        void handleStopOperation(inet::LifecycleOperation*) override;
        void handleStartOperation(inet::LifecycleOperation*) override;
        void handleMessageWhenUp(inet::cMessage *msg) override;

        void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override;
        void socketEstablished(inet::TcpSocket *socket) override;
        void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override;
        void socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) override;

        void initialize(int stage) override;

        inet::Packet *createDataPacket(long sendBytes);

public:
    void addNode(std::string newNode);

    void writeStateToFile(std::string file);
};
}

#endif //E_VOTING_APP_H
