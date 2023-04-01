//
// Created by wld on 27.11.22.
//

#ifndef E_VOTING_APP_H
#define E_VOTING_APP_H

#include "../../../src/inetSocketAdapter.h"
#include "network/connectionService.h"
#include "identity/didConnectionService.h"
#include "inet/common/INETDefs.h"
#include <inet/applications/tcpapp/TcpSessionApp.h>
#include <inet/common/socket/SocketMap.h>
#include "inet/applications/tcpapp/TcpServerSocketIo.h"
#include "evoting/peer.h"
#include "network/syncService.h"
#include "identity/didSyncService.h"
#include "../../../../did/src/did.h"

namespace voting {
    class DidVotingSetupApp : public inet::TcpAppBase {
        inet::cMessage *connectSelfMessage;
        inet::cMessage *sendDataSelfMessage;
        inet::cMessage *listenStartMessage;
        inet::cMessage *closeSocketMessage;
        inet::cMessage *listenUpSyncMessage;
        inet::cMessage *initSyncMessage;
        inet::cMessage *listenDownSyncMessage;
        inet::cMessage *forwardUpSyncMessage;
        inet::cMessage *returnDownSyncMessage;
        didConnectionService connection_service;
        didSyncService sync_service;
        inetSocketAdapter socket_adapter;
        identityService identity_service;

        // TODO: Migrate to use peer
        std::set<std::string> nodes;
        inMemoryStorage storage;
        std::string nodesString;
        std::stringstream message_stream;

        bool isReturning = false;

        bool isReceivingMultipackageMessage = false;
        bool hasReceivedLastPackageFromMultiMessage = false;
        uint8_t saved_package_type = 0;

        float forwardRequestDelta = 0.1f;
        float returnSyncRequestDelta = 0.5f;
        float closeSyncForwardSocketDelta = 0.01f;
        float connectionRequestReplyDelta = 0.01f;
        float listenUpDelta = 0.01f;

        inet::TcpSocket* downSyncSocket = new inet::TcpSocket();
        inet::TcpSocket* listen_connection_socket = new inet::TcpSocket();
        inet::TcpSocket* listen_sync_down_socket = new inet::TcpSocket();
        inet::TcpSocket* listen_sync_up_socket = new inet::TcpSocket();
        inet::TcpSocket* upSyncSocket = new inet::TcpSocket();

        inet::SocketMap socketMap;
        std::string local_address;
        std::string connect_address;
        did local_did;
        did connect_did;
        did received_from;

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
        void writeStateToFile(std::string directory, std::string file);
        void setupSocket(inet::TcpSocket* socket, int port);

    private:
        std::string received_sync_request_from;

        inet::Packet* createDataPacket(std::string send_string);
    };
}

#endif //E_VOTING_APP_H
