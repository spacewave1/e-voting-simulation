//
// Created by wld on 27.11.22.
//

#include "VotingApp.h"
#include <algorithm>
#include <inet/networklayer/common/L3AddressResolver.h>
#include "inet/transportlayer/tcp/Tcp.h"
#include "inet/common/INETUtils.h"
#include "nlohmann/json.hpp"
#include "PacketsKind.h"

std::string_view transformPacketDataToString(inet::Ptr<const inet::BytesChunk> ptr) {
    std::string contentStr;
    std::transform(ptr->getBytes().begin(),ptr->getBytes().end(),std::back_inserter(contentStr),[](uint8_t d){
        return (char) d;
    });
    return contentStr;
}


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
        double tOpen = par("tOpen").doubleValue();
        double tSend = par("tSend").doubleValue();
        double tListenStart = par("tListenStart").doubleValue();
        double tListenEnd = par("tListenEnd").doubleValue();
        double tSyncInit = par("tSyncInit").doubleValue();
        double tForwardSync = par("tForwardSync").doubleValue();
        isReceiving = par("isReceivingAtStart").boolValue();

        if (inet::simTime() <= tOpen) {
            EV_DEBUG << "schedule connect at: " << tOpen << std::endl;
            connectSelfMessage = new inet::cMessage("timer");
            connectSelfMessage->setKind(SELF_MSGKIND_CONNECT);
            scheduleAt(tOpen, connectSelfMessage);
        }
        if (inet::simTime() <= tSend) {
            EV_DEBUG << "schedule send at: " << tSend << std::endl;
            sendDataSelfMessage = new inet::cMessage("timer");
            sendDataSelfMessage->setKind(SELF_MSGKIND_SEND);
            scheduleAt(tSend, sendDataSelfMessage);
        }
        if (inet::simTime() <= tListenStart) {
            EV_DEBUG << "schedule listen start at: " << tListenStart << std::endl;
            listenStartMessage = new inet::cMessage("timer");
            listenStartMessage->setKind(SELF_MSGKIND_LISTEN);
            scheduleAt(tListenStart, listenStartMessage);
        }
        if (inet::simTime() <= tListenEnd) {
            EV_DEBUG << "schedule listen end at: " << tListenEnd << std::endl;
            listenEndMessage = new inet::cMessage("timer");
            listenEndMessage->setKind(SELF_MSGKIND_LISTEN_END);
            scheduleAt(tListenEnd, listenEndMessage);
        }
        if(inet::simTime() <= tSyncInit) {
            initSyncMessage = new inet::cMessage("timer");
            initSyncMessage->setKind(SELF_MSGKIND_INIT_SYNC);
            scheduleAt(tSyncInit, initSyncMessage);
        }
        if(inet::simTime() <= tForwardSync) {
            forwardSyncMessage = new inet::cMessage("timer");
            forwardSyncMessage->setKind(SELF_MSGKIND_FORWARD_SYNC);
            scheduleAt(tForwardSync, forwardSyncMessage);
        }
    }

    void VotingApp::handleTimer(inet::cMessage *msg) {
        if(msg->getKind() == SELF_MSGKIND_LISTEN) {
            EV_DEBUG << "now listen" << std::endl;
            isReceiving = true;
            socket.renewSocket();
            int localPort = par("localPort");
            setupSocket(localPort);
            socket_adapter.setSocket(&socket);
            connection_service.changeToListenState(socket_adapter);
            EV_DEBUG << "before connection service" << std::endl;
        }
        // TODO: Destroy connection objects, once the socket is closed
        if(msg->getKind() == SELF_MSGKIND_LISTEN_END) {
            isReceiving = false;
            listenStop();
            std::for_each(connectionSet.begin(), connectionSet.end(), [](VotingAppConnectionRequestReply * rrp){
                rrp->deleteModule();
            });
        }
        if(msg->getKind() == SELF_MSGKIND_CONNECT) {
            socket.renewSocket();
            int localPort = par("localPort");
            // parameters
            setupSocket(localPort);
            //connection_service.connect(socket_adapter)
            connect();
        }
        if(msg->getKind() == SELF_MSGKIND_SEND) {
            EV_DEBUG << "send packet" << std::endl;

            std::string connectAddress{par("connectAddress").stringValue()};
            std::string localAddress{par("localAddress").stringValue()};

            socket_adapter.setMsgKind(APP_CONN_REQUEST);
            socket_adapter.setSocket(&socket);
            socket_adapter.setParentComponent(this);
            connection_service.sendConnectionRequest(socket_adapter, connectAddress);

        }
        if(msg->getKind() == SELF_MSGKIND_CLOSE) {
            EV_DEBUG << "close socket" << std::endl;
            //std::for_each(connectionSet.begin(), connectionSet.end(),[](VotingAppConnectionRequestReply* rr){
                //rr->cancelAndDelete();
            //});
            close();
        }
        if(msg->getKind() == SELF_MSGKIND_INIT_SYNC) {
            const char *connectAddress = par("connectAddress");
            EV_DEBUG<< "init sync" << std::endl;
            socket.renewSocket();
            EV_DEBUG<< "has renewed" << std::endl;
            // parameters
            int syncPort = par("syncPort");
            setupSocket(syncPort);
            const inet::Ipv4Address &addr = inet::Ipv4Address(connectAddress);
            //socket.connect(addr, 5556);
            socket_adapter.setMsgKind(APP_SYNC_REQUEST);

            EV_DEBUG << "set kind: " << socket_adapter.getMsgKind() << std::endl;
            sync_service.initSync(&socket_adapter, connectAddress);
            sync_service.sendInitialSyncRequest(&socket_adapter, connectAddress, nodes, connection_map);
        }
        if(msg->getKind() == SELF_MSGKIND_FORWARD_SYNC){
            EV_DEBUG << "now listen" << std::endl;
            isReceiving = true;
            socket.renewSocket();
            int syncPort = par("syncPort");
            setupSocket(syncPort);
            socket_adapter.setSocket(&socket);
            connection_service.changeToListenState(socket_adapter);
            EV_DEBUG << "before connection service" << std::endl;
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
            EV_DEBUG << "self message, kind:  " << msg->getKind() << std::endl;
            handleTimer(msg);
        } else if (socket.belongsToSocket(msg)) {
            socket.processMessage(msg);
        }
        //else
        //     throw inet::cRuntimeError("Unknown incoming message: '%s'", msg->getName());
    }

    void VotingApp::socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) {
        EV_DEBUG << "status arrived" << std::endl;
        TcpAppBase::socketStatusArrived(socket, status);
    }

    void VotingApp::receiveIncomingMessages(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo){
        const char *serverConnectionModuleType = par("serverConnectionModuleType");
        inet::cModuleType *moduleType = inet::cModuleType::get(serverConnectionModuleType);
        cModule *submodule = getParentModule()->getSubmodule("connection", 0);
        int submoduleIndex = submodule == nullptr ? 0 : submodule->getVectorSize();
        auto connection = moduleType->create("connection", this, submoduleIndex + 1, submoduleIndex);
        connection->finalizeParameters();
        connection->buildInside();
        connection->callInitialize();
        auto dispatcher = inet::check_and_cast<cSimpleModule *>(gate("socketIn")->getPathStartGate()->getOwnerModule());
        dispatcher->setGateSize("in", dispatcher->gateSize("in") + 1);
        dispatcher->setGateSize("out", dispatcher->gateSize("out") + 1);
        connection->gate("socketOut")->connectTo(dispatcher->gate("in", dispatcher->gateSize("in") - 1));
        dispatcher->gate("out", dispatcher->gateSize("out") - 1)->connectTo(connection->gate("socketIn"));
        auto requestReplyConnection = inet::check_and_cast<VotingAppConnectionRequestReply *>(connection->gate("socketIn")->getPathEndGate()->getOwnerModule());
        requestReplyConnection->acceptSocket(availableInfo);
        connectionSet.insert(requestReplyConnection);
    }

    void VotingApp::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) {
        if(isReceiving) {
            EV_DEBUG << "Socket available receiving" << std::endl;
            receiveIncomingMessages(socket, availableInfo);
        } else {
            EV_DEBUG << "Socket available requesting" << std::endl;
            TcpAppBase::socketAvailable(socket, availableInfo);
        }
    }

    void VotingApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) {
        EV_DEBUG << "socket data arrived: " << this->getFullPath() << std::endl;

        switch(msg->getKind()){
            case APP_CONN_REPLY: {
                const inet::Ptr<const inet::BytesChunk> &ptr = msg->peekData<inet::BytesChunk>();
                std::string content_str;
                std::transform(ptr->getBytes().begin(),ptr->getBytes().end(),std::back_inserter(content_str),[](uint8_t d){
                    return (char) d;
                });
                EV_DEBUG << content_str << std::endl;
                if(content_str.find("accept") != std::string::npos){
                    std::string connectAddress{par("connectAddress").stringValue()};
                    std::string localAddress{par("localAddress").stringValue()};

                    connection_service.computeConnectionReply(socketMessage{std::string(content_str), connectAddress}, nodes, connection_map, localAddress);
                    writeStateToFile("connection/peers_after_data_received_reply_" + getFullPath().substr(0,19) + ".json");

                    EV_DEBUG << "accepted address" << std::endl;
                } else {
                    EV_DEBUG << "not found" << std::endl;
                }
            }
                EV_DEBUG << "received connection reply" << std::endl;
                break;
            case APP_CONN_REQUEST:
                EV_DEBUG << "received connection request" << std::endl;
                break;
        }

        if(isReceiving) {
            EV_DEBUG << "send back message: " << this->getFullPath() << std::endl;
            TcpAppBase::socketDataArrived(socket, msg, urgent);
        } else {
            TcpAppBase::socketDataArrived(socket, msg, urgent);
        }
    }

    void VotingApp::addNode(std::string newNode) {
        nodes.insert(newNode);
    }

    void VotingApp::writeStateToFile(std::string file) {
        //const std::filesystem::path &currentPath = std::filesystem::current_path();
        EV_DEBUG << "print: " << file << std::endl;
        connection_service.exportPeersList("./results/", nodes, file);
    }

    void VotingApp::listenStop() {
        EV_DEBUG << "stopped listening on " << this->getFullPath() << std::endl;
        socket.renewSocket();
    }

    void VotingApp::setupSocket(int port) {
        const char *localAddress = par("localAddress");
        socket.bind(inet::Ipv4Address(localAddress), port);
        socket.setCallback(this);
        socket.setOutputGate(gate("socketOut"));
        const inet::L3Address &connectAddress = inet::L3Address(par("connectAddress"));
    }

    int VotingApp::getPacketsSent() {
        return packetsSent;
    }

    int VotingApp::getBytesSent() {
        return bytesSent;
    }

    void VotingApp::setPacketsSent(int newPacketsSent) {
        packetsSent = newPacketsSent;
    }

    void VotingApp::setBytesSent(int newBytesSend) {
        packetsSent = newBytesSend;
    }

    connectionService *VotingApp::getConnectionService() {
        return &connection_service;
    }

    std::map<std::string, std::string> *VotingApp::getNodeConnections() {
        return &connection_map;
    }

    std::set<std::string> *VotingApp::getNodes() {
        return &nodes;
    }

    syncService *VotingApp::getSyncService() {
        return &sync_service;
    }
}