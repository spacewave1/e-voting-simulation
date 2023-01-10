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
#define MSGKIND_CLOSE      3


namespace voting {
    Define_Module(VotingApp);

    void VotingApp::handleTimer(inet::cMessage *msg) {
        EV_DEBUG << "timer" << std::endl;
        if(!isReceiving){
            if(msg->getKind() == MSGKIND_CONNECT) {
                const inet::L3Address &connectAddress = inet::L3Address(par("connectAddress"));
                int connectPort = 5555;
                connect();
            }
            else if(msg->getKind() == MSGKIND_SEND) {
                EV_DEBUG << "send packet" << std::endl;
                sendPacket(createDataPacket(10));
            }
        } else {
            if(msg->isSelfMessage()) {
                EV_DEBUG << "listen is self message" << std::endl;
            }
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

        if(isReceiving) {
            const char *localAddress = par("localAddress");
            int localPort = par("localPort");
            socket.bind(inet::Ipv4Address(localAddress), localPort);
            const std::vector<const char *> &vector = getGateNames();
            socket.setOutputGate(gate("socketOut"));
            socket.setCallback(this);
            socket.listen();
        }
        double tOpen = par("tOpen").doubleValue();
        double tSend = par("tSend").doubleValue();
        isReceiving = par("isReceiving").boolValue();
        if(isReceiving){
        } else {
            if (inet::simTime() <= tOpen) {
                EV_DEBUG << "schedule connect at: " << tOpen << std::endl;
                connectSelfMessage = new inet::cMessage("timer");
                connectSelfMessage->setKind(MSGKIND_CONNECT);
                scheduleAt(tOpen, connectSelfMessage);
            }
            if (inet::simTime() <= tSend) {
                EV_DEBUG << "schedule send" << std::endl;
                sendDataSelfMessage = new inet::cMessage("timer");
                sendDataSelfMessage->setKind(MSGKIND_SEND);
                scheduleAt(tSend, sendDataSelfMessage);
            }
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
        isReceiving = par("isReceiving").boolValue();

         if(!isReceiving) {
            if (stage == inet::INITSTAGE_LOCAL) {
                numSessions = numBroken = packetsSent = packetsRcvd = bytesSent = bytesRcvd = 0;

                WATCH(numSessions);
                WATCH(numBroken);
                WATCH(packetsSent);
                WATCH(packetsRcvd);
                WATCH(bytesSent);
                WATCH(bytesRcvd);
            }
            else if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
                // parameters
                const char *localAddress = par("localAddress");
                int localPort = par("localPort");
                socket.bind(inet::Ipv4Address(localAddress), localPort);
                socket.setCallback(this);
                socket.setOutputGate(gate("socketOut"));
            }
        }

        EV_DEBUG << "stage: " << stage << std::endl;
        EV_DEBUG << "initialize" << std::endl;
    }

    void VotingApp::handleMessageWhenUp(omnetpp::cMessage *msg) {
        EV_DEBUG << "handle message up: " << std::endl;
        if(isReceiving) {
            if (socket.belongsToSocket(msg))
                socket.processMessage(msg);
            else
                 throw inet::cRuntimeError("Unknown incoming message: '%s'", msg->getName());
        } else {
            TcpAppBase::handleMessageWhenUp(msg);
        }
    }

    void VotingApp::socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) {
        TcpAppBase::socketStatusArrived(socket, status);
        EV_DEBUG << "status arrived" << std::endl;
    }

    void VotingApp::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) {
        if(isReceiving) {
            EV_DEBUG << "Socket available receiving" << std::endl;
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
}