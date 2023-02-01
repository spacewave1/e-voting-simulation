//
// Created by wld on 27.11.22.
//

#include "VotingApp.h"
#include <algorithm>
#include <inet/networklayer/common/L3AddressResolver.h>
#include "inet/transportlayer/tcp/Tcp.h"
#include "inet/common/INETUtils.h"
#include "nlohmann/json.hpp"
#include "../../../src/PacketsKind.h"

namespace voting {
    Define_Module(VotingApp);

    void VotingApp::initialize(int stage) {
        ApplicationBase::initialize(stage);

        if (stage == inet::INITSTAGE_LOCAL) {
            numSessions = numBroken = packetsSent = packetsRcvd = bytesSent = bytesRcvd = 0;

            WATCH(numSessions);
            WATCH(numBroken);
            WATCH(packetsSent);
            WATCH(packetsRcvd);
            WATCH(bytesSent);
            WATCH(bytesRcvd);
        }
    }

    void VotingApp::handleStartOperation(inet::LifecycleOperation *) {
        // Export state at beginning
        writeStateToFile("connection/peers_at_start_" + this->getFullPath().substr(0,19) + ".json");

        // Schedule events
        double tCreateElection = par("tCreateElection").doubleValue();
        //double tInitialDistribute = par("tInitialDistribute").doubleValue();

        isReceiving = par("isReceivingAtStart").boolValue();

        if (inet::simTime() <= tCreateElection) {
            createElectionSelfMessage = new inet::cMessage("timer");
            createElectionSelfMessage->setKind(SELF_MSGKIND_CREATE_ELECTION);
            scheduleAt(tCreateElection, createElectionSelfMessage);
        }
    }

    void VotingApp::handleTimer(inet::cMessage *msg) {
        if(msg->getKind() == SELF_MSGKIND_CREATE_ELECTION) {

        }
        if(msg->getKind() == SELF_MSKIND_DISTR_3P_RECEIVE) {
            isReceiving = true;
            socket.renewSocket();
            int localPort = par(5049);
            setupSocket(&socket, localPort);
            socket_adapter.setSocket(&socket);
            //connection_service.changeToListenState(socket_adapter);
        }
        if(msg->getKind() == SELF_MSKIND_DISTR_3P_REQUEST){
            isReceiving = false;
            socket.renewSocket();
            setupSocket(&socket, 5049);

            socket_adapter.setMsgKind(APP_DISTR_3P_REQUEST);
            socket_adapter.setSocket(&socket);
            socket_adapter.setParentComponent(this);

            const std::string &address = socket.getLocalAddress().str();
            sync_service.returnSyncRequestDown(&socket_adapter, nodes, connection_map, address);
        }
    }

    void VotingApp::handleCrashOperation(inet::LifecycleOperation *) {
        EV_DEBUG << "handle crash" << std::endl;
    }

    void VotingApp::handleStopOperation(inet::LifecycleOperation *) {
        EV_DEBUG << "handle stop" << std::endl;
    }

    void VotingApp::socketEstablished(inet::TcpSocket *socket) {
        EV_DEBUG << "socket established" << std::endl;
        TcpAppBase::socketEstablished(socket);;
    }

    void VotingApp::handleMessageWhenUp(omnetpp::cMessage *msg) {
        if(msg->isSelfMessage()){
            handleTimer(msg);
        } else if (socket.belongsToSocket(msg)) {
            socket.processMessage(msg);
        } else {
            try {
                if(inet::check_and_cast<inet::Packet*>(msg)){
                    socketDataArrived(&socket, inet::check_and_cast<inet::Packet*>(msg), false);
                }
            } catch (std::exception& ex){
                EV_DEBUG << "caught exception" << std::endl;
                EV_DEBUG << ex.what() << std::endl;
            }
        }
        //else
        //     throw inet::cRuntimeError("Unknown incoming message: '%s'", msg->getName());
    }

    void VotingApp::socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) {
        EV_DEBUG << "status arrived" << std::endl;
        TcpAppBase::socketStatusArrived(socket, status);
    }

    void VotingApp::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) {
        if(isReceiving) {
            auto newSocket = new inet::TcpSocket(availableInfo); // create socket using received info
            socketMap.addSocket(newSocket); // store accepted connection

            if(availableInfo->getRemotePort() == 5050){
                EV_DEBUG << "availabel on 5050" << std::endl;
               // down_connect_socket_id = newSocket->getSocketId();
            } else if(availableInfo->getRemotePort() == 5051){
            }

            this->socket.accept(availableInfo->getNewSocketId());

        } else {
            TcpAppBase::socketAvailable(socket, availableInfo);
        }
    }

    void VotingApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) {
        const inet::Ptr<const inet::BytesChunk> &intrusivePtr = msg->peekData<inet::BytesChunk>();
        uint8_t appMsgKind = intrusivePtr->getByte(0);
        EV_DEBUG << "Message kind arrived: " << std::to_string(appMsgKind) << std::endl;

        const inet::Ptr<const inet::BytesChunk> &ptr = msg->peekData<inet::BytesChunk>();

        std::string content_str;
        std::transform(ptr->getBytes().begin() + 1,ptr->getBytes().end(),std::back_inserter(content_str),[](uint8_t d){
            return (char) d;
        });

        switch(appMsgKind){
            case APP_DISTR_3P_REQUEST: {
                EV_DEBUG << "received 3p request" << std::endl;
            }
                break;
            case APP_DISTR_3P_RESPONSE:
            {
                EV_DEBUG << "received 3p response" << std::endl;
            }
            break;
        }

        TcpAppBase::socketDataArrived(socket, msg, urgent);
    }

    void VotingApp::writeStateToFile(std::string file) {
        //const std::filesystem::path &currentPath = std::filesystem::current_path();
        EV_DEBUG << "print: " << file << std::endl;
        connection_service.exportPeersList("./results/", nodes, file);
    }

    void VotingApp::setupSocket(inet::TcpSocket* socket, int port) {
        const char *localAddress = par("localAddress");
        socket->bind(inet::Ipv4Address(localAddress), port);
        socket->setCallback(this);
        socket->setOutputGate(gate("socketOut"));
    }
}