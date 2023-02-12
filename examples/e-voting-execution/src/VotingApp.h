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
#include "evoting/distributionService.h"

namespace voting {
    class VotingApp : public inet::TcpAppBase {
        inet::cMessage *createElectionSelfMessage;
        inet::cMessage *placeVoteSelfMessage;
        inet::cMessage *receive3PReqestSelfMessage;
        inet::cMessage *forwardDirectionRequestSelfMessage;
        inet::cMessage *publishSelfMessage;
        inet::cMessage *hopsSelfMessage;
        inet::cMessage *directionSelfMessage;
        inet::cMessage *subscribeSelfMessage;
        bool isReceiving = false;
        bool isInitializingDirectionDistribution = false;
        bool doesConnect = true;
        connectionService connection_service;
        distributionService distribution_service;

        inetSocketAdapter socket_up_adapter;
        inetSocketAdapter socket_down_adapter;
        inetSocketAdapter socket_no_direction_adapter;
        inetSocketAdapter subscribe_socket_adapter;
        inetSocketAdapter publish_socket_adapter;

        inet::TcpSocket* publish_socket = new inet::TcpSocket();
        inet::TcpSocket* subscribe_socket = new inet::TcpSocket();

        std::string address_up, address_down = "";
        int publish_port, subscribe_port;
        std::vector<election> election_box;

        size_t position = 0;

        std::string sendTowards = "";

        omnetpp::cMsgPar *nextDirection;
        omnetpp::cMsgPar *received3PData;

        // TODO: Migrate to use peer
        std::set<std::string> nodes;
        std::map<std::string, std::string> connection_map;
        std::string nodesString;

        int down_socket_id = 0;

        size_t current_hops = 0;
        std::string received_direction;

        inet::SocketMap socketMap;

        double pauseBeforePublish = 0.0;
        double pauseBeforeForwardPortsRequest = 0.0;

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

    private:
        std::string received_sync_request_from;
    };
}

#endif //E_VOTING_APP_H
