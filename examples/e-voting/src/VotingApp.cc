//
// Created by wld on 27.11.22.
//

#include "VotingApp.h"
#include <algorithm>
#include <inet/networklayer/common/L3AddressResolver.h>
#include "inet/transportlayer/tcp/Tcp.h"
#include "inet/common/INETUtils.h"
#include "nlohmann/json.hpp"

#define MSGKIND_CONNECT    1
#define MSGKIND_SEND       2
#define MSGKIND_LISTEN     3
#define MSGKIND_LISTEN_END 4
#define MSGKIND_CLOSE      5


namespace voting {
    Define_Module(VotingApp);

    void VotingApp::handleTimer(inet::cMessage *msg) {
        if(msg->getKind() == MSGKIND_LISTEN) {
            EV_DEBUG << "now listen" << std::endl;
            isReceiving = true;
            socket.renewSocket();
            setupSocket();
            socket_adapter.setSocket(socket);
            connection_service.changeToListenState(socket_adapter);
            EV_DEBUG << "before connection service" << std::endl;
        }
        // TODO: Destroy connection objects, once the socket is closed
        if(msg->getKind() == MSGKIND_LISTEN_END) {
            isReceiving = false;
            listenStop();
            std::for_each(connectionSet.begin(), connectionSet.end(), [](VotingAppConnectionRequestReply * rrp){
                rrp->deleteModule();
            });
        }
        if(msg->getKind() == MSGKIND_CONNECT) {
            socket.renewSocket();
            // parameters
            setupSocket();
            connect();
        }
        if(msg->getKind() == MSGKIND_SEND) {
            EV_DEBUG << "send packet" << std::endl;
            sendPacket(createDataPacket(10));

            std::string connectAddress{par("connectAddress").stringValue()};
            std::string localAddress{par("localAddress").stringValue()};

            socket_adapter.setSocket(socket);
            connection_service.sendConnectionRequest(socket_adapter, connectAddress);
            connection_service.receiveConnectionReply(socket_adapter, connectAddress, nodes, connection_map, localAddress);
        }
        if(msg->getKind() == MSGKIND_CLOSE) {
            EV_DEBUG << "close socket" << std::endl;
            //std::for_each(connectionSet.begin(), connectionSet.end(),[](VotingAppConnectionRequestReply* rr){
                //rr->cancelAndDelete();
            //});
            close();
        }
    }

    inet::Packet *VotingApp::createDataPacket(long sendBytes) {
        EV_DEBUG << "create data" << std::endl;
        inet::Ptr<inet::Chunk> payload;
        inet::Packet *packet = new inet::Packet("data");

        const auto &bytesChunk = inet::makeShared<inet::BytesChunk>();
        const char *connectAddress = par("connectAddress").stringValue();
        std::string address{connectAddress};
        std::string sendString = connection_service.createNetworkRegistrationRequest(address);
        std::vector<uint8_t> vec;
        unsigned long length = sendString.length();

        vec.resize(sendBytes);
        for (int i = 0; i < sendBytes; i++)
            vec[i] = (bytesSent + sendString[i % length]) & 0xFF;

        bytesChunk->setBytes(vec);
        packet->insertAtBack(bytesChunk);

        return packet;
    }

    void VotingApp::handleCrashOperation(inet::LifecycleOperation *) {
        EV_DEBUG << "handle crash" << std::endl;
    }

    void VotingApp::handleStartOperation(inet::LifecycleOperation *) {
        // Export state at beginning

        EV_DEBUG << "export" << this->getFullPath().substr(0,19) << std::endl;
        writeStateToFile("peers_at_start_" + this->getFullPath().substr(0,19) + ".json");

        double tOpen = par("tOpen").doubleValue();
        double tSend = par("tSend").doubleValue();
        double tListenStart = par("tListenStart").doubleValue();
        double tListenEnd = par("tListenEnd").doubleValue();
        isReceiving = par("isReceivingAtStart").boolValue();

        //setupSocket();
        //if(isReceiving) {
        //    socket.listen();
        //}
        EV_DEBUG << "socket state: " << inet::TcpSocket::stateName(socket.getState())
            << " on " << this->getFullPath() << std::endl;

        if (inet::simTime() <= tOpen) {
            EV_DEBUG << "schedule connect at: " << tOpen << std::endl;
            connectSelfMessage = new inet::cMessage("timer");
            connectSelfMessage->setKind(MSGKIND_CONNECT);
            scheduleAt(tOpen, connectSelfMessage);
        }
        if (inet::simTime() <= tSend) {
            EV_DEBUG << "schedule send at: " << tSend << std::endl;
            sendDataSelfMessage = new inet::cMessage("timer");
            sendDataSelfMessage->setKind(MSGKIND_SEND);
            scheduleAt(tSend, sendDataSelfMessage);
        }
        if (inet::simTime() <= tListenStart) {
            EV_DEBUG << "schedule listen start at: " << tListenStart << std::endl;
            listenStartMessage = new inet::cMessage("timer");
            listenStartMessage->setKind(MSGKIND_LISTEN);
            scheduleAt(tListenStart, listenStartMessage);
        }
        if (inet::simTime() <= tListenEnd) {
            EV_DEBUG << "schedule listen end at: " << tListenEnd << std::endl;
            listenEndMessage = new inet::cMessage("timer");
            listenEndMessage->setKind(MSGKIND_LISTEN_END);
            scheduleAt(tListenEnd, listenEndMessage);
        }
    }

    void VotingApp::handleStopOperation(inet::LifecycleOperation *) {
        EV_DEBUG << "handle stop" << std::endl;
    }

    void VotingApp::socketEstablished(inet::TcpSocket *socket) {
        EV_DEBUG << "socket established" << std::endl;
        TcpAppBase::socketEstablished(socket);;
    }

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

    void VotingApp::handleMessageWhenUp(omnetpp::cMessage *msg) {
        EV_DEBUG << "message up" << std::endl;
        if(msg->isSelfMessage()){
            EV_DEBUG << "handle message up from " << this->getFullPath() << std::endl;
            EV_DEBUG << "message is:  " << msg->str() << std::endl;
            EV_DEBUG << "message displaystr:  " << msg->getDisplayString() << std::endl;
            EV_DEBUG << "message kind:  " << msg->getKind() << std::endl;

            EV_DEBUG << "socket state: " << inet::TcpSocket::stateName(socket.getState())
                     << " on " << this->getFullPath() << std::endl;
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
        EV_DEBUG << "socket data arrived" << std::endl;
        if(isReceiving) {
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
        EV_DEBUG << "stopped listenning on " << this->getFullPath() << std::endl;
        socket.renewSocket();
    }

    void VotingApp::setupSocket() {
        const char *localAddress = par("localAddress");
        int localPort = par("localPort");
        socket.bind(inet::Ipv4Address(localAddress), localPort);
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
}