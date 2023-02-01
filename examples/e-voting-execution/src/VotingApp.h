//
// Created by wld on 27.11.22.
//

#ifndef E_VOTING_APP_H
#define E_VOTING_APP_H

#include "inet/common/INETDefs.h"
#include <inet/applications/tcpapp/TcpSessionApp.h>
#include <inet/common/socket/SocketMap.h>
#include "network/connectionService.h"
#include "inet/applications/tcpapp/TcpServerSocketIo.h"
#include "evoting/peer.h"
#include "network/syncService.h"
#include "../../../src/inetSocketAdapter.h"

namespace voting {
    class VotingApp : public inet::TcpAppBase {
        inet::cMessage *createElectionSelfMessage;
        bool isReceiving = false;
        bool doesConnect = true;
        connectionService connection_service;
        syncService sync_service;
        inetSocketAdapter socket_adapter;

        // TODO: Migrate to use peer
        std::set<std::string> nodes;
        std::map<std::string, std::string> connection_map;
        std::string nodesString;
        int down_connect_socket_id = 0;
        int down_sync_socket_id = 0;
        int up_sync_socket_id = 0;

        inet::SocketMap socketMap;

        void handleTimer(inet::cMessage *msg) override;
        void handleCrashOperation(inet::LifecycleOperation *) override;
        void handleStopOperation(inet::LifecycleOperation *) override;
        void handleStartOperation(inet::LifecycleOperation *) override;
        void handleMessageWhenUp(inet::cMessage *msg) override;
        void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override;
        void socketEstablished(inet::TcpSocket *socket) override;
        void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override;
        void socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) override;
        void initialize(int stage) override;

    public:
        void writeStateToFile(std::string file);
        void setupSocket(inet::TcpSocket* socket, int port);

    private:
        std::string received_sync_request_from;

        inet::Packet* createDataPacket(std::string send_string);
    };
}

#endif //E_VOTING_APP_H
