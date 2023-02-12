//
// Created by wld on 27.11.22.
//

#ifndef E_VOTING_APP_H
#define E_VOTING_APP_H

#include "../../../src/inetSocketAdapter.h"
#include "network/connectionService.h"
#include "inet/common/INETDefs.h"
#include <inet/applications/tcpapp/TcpSessionApp.h>
#include <inet/common/socket/SocketMap.h>
#include "inet/applications/tcpapp/TcpServerSocketIo.h"
#include "evoting/peer.h"
#include "network/syncService.h"

namespace voting {
    class VotingSetupApp : public inet::TcpAppBase {
        inet::cMessage *connectSelfMessage;
        inet::cMessage *sendDataSelfMessage;
        inet::cMessage *listenStartMessage;
        inet::cMessage *listenEndMessage;
        inet::cMessage *initSyncMessage;
        inet::cMessage *listenDownSyncMessage;
        inet::cMessage *listenUpSyncMessage;
        inet::cMessage *forwardUpSyncMessage;
        inet::cMessage *returnDownSyncMessage;
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
        std::set<std::string>* getNodes();
        std::map<std::string, std::string>* getNodeConnections();
        void addNode(std::string newNode);
        void writeStateToFile(std::string directory, std::string file);
        void receiveIncomingMessages(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo);
        void listenStop();
        void setupSocket(inet::TcpSocket* socket, int port);
        void setPacketsSent(int newPacketsSent);
        void setBytesSent(int newBytesSent);
        int getBytesSent();
        int getPacketsSent();
        connectionService *getConnectionService();
        syncService *getSyncService();
        void setReceivedSyncRequestFrom(std::string requestFrom);

    private:
        std::string received_sync_request_from;

        inet::Packet* createDataPacket(std::string send_string);
    };
}

#endif //E_VOTING_APP_H
