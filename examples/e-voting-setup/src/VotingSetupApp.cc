//
// Created by wld on 27.11.22.
//

#include "VotingSetupApp.h"
#include <algorithm>
#include <inet/networklayer/common/L3AddressResolver.h>
#include "inet/transportlayer/tcp/Tcp.h"
#include "inet/common/INETUtils.h"
#include "nlohmann/json.hpp"
#include "../../../src/PacketsKind.h"

namespace voting {
    Define_Module(VotingSetupApp);

    void VotingSetupApp::initialize(int stage) {
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

    void VotingSetupApp::handleStartOperation(inet::LifecycleOperation *) {
        // Export state at beginning
        writeStateToFile("connection/", "at_start_" + this->getFullPath().substr(0,19) + ".json");

        // Schedule events
        double tOpen = par("tOpen").doubleValue();
        double tSend = par("tSend").doubleValue();
        double tListenStart = par("tListenStart").doubleValue();
        double tListenEnd = par("tListenEnd").doubleValue();
        double tSyncInit = par("tSyncInit").doubleValue();
        double tListenUpSync = par("tListenUpSync").doubleValue();
        double tListenDownSync = par("tListenDownSync").doubleValue();
        double tForwardUpSync = par("tForwardUpSync").doubleValue();
        double tReturnSyncDown = par("tReturnDownSync").doubleValue();
        isReceiving = par("isReceivingAtStart").boolValue();

        if (inet::simTime() <= tOpen) {
            connectSelfMessage = new inet::cMessage("timer");
            connectSelfMessage->setKind(SELF_MSGKIND_CONNECT);
            scheduleAt(tOpen, connectSelfMessage);
        } else {
            doesConnect = false;
        }
        if (inet::simTime() <= tSend) {
            sendDataSelfMessage = new inet::cMessage("timer");
            sendDataSelfMessage->setKind(SELF_MSGKIND_SEND);
            scheduleAt(tSend, sendDataSelfMessage);
        }
        if (inet::simTime() <= tListenStart) {
            listenStartMessage = new inet::cMessage("timer");
            listenStartMessage->setKind(SELF_MSGKIND_LISTEN);
            scheduleAt(tListenStart, listenStartMessage);
        }
        if (inet::simTime() <= tListenEnd) {
            listenEndMessage = new inet::cMessage("timer");
            listenEndMessage->setKind(SELF_MSGKIND_LISTEN_END);
            scheduleAt(tListenEnd, listenEndMessage);
        }
        if(inet::simTime() <= tSyncInit) {
            initSyncMessage = new inet::cMessage("timer");
            initSyncMessage->setKind(SELF_MSGKIND_INIT_SYNC);
            scheduleAt(tSyncInit, initSyncMessage);
        }
        if(inet::simTime() <= tListenDownSync) {
            listenDownSyncMessage = new inet::cMessage("timer");
            listenDownSyncMessage->setKind(SELF_MSGKIND_LISTEN_SYNC_DOWN);
            scheduleAt(tListenDownSync, listenDownSyncMessage);
        }
        if(inet::simTime() <= tListenUpSync) {
            listenUpSyncMessage = new inet::cMessage("timer");
            listenUpSyncMessage->setKind(SELF_MSGKIND_LISTEN_SYNC_UP);
            scheduleAt(tListenUpSync, listenUpSyncMessage);
        }
        if(inet::simTime() <= tForwardUpSync) {
            forwardUpSyncMessage = new inet::cMessage("timer");
            forwardUpSyncMessage->setKind(SELF_MSGKIND_FORWARD_SYNC_UP);
            scheduleAt(tForwardUpSync, forwardUpSyncMessage);
        }
        if(inet::simTime() <= tReturnSyncDown) {
            returnDownSyncMessage = new inet::cMessage("timer");
            returnDownSyncMessage->setKind(SELF_MSGKIND_RETURN_SYNC_DOWN);
            scheduleAt(tReturnSyncDown, returnDownSyncMessage);
        }
    }

    void VotingSetupApp::handleTimer(inet::cMessage *msg) {
        if(msg->getKind() == SELF_MSGKIND_LISTEN) {
            isReceiving = true;
            socket.renewSocket();
            int localPort = par("localPort");
            setupSocket(&socket, localPort);
            socket_adapter.setSocket(&socket);
            connection_service.changeToListenState(socket_adapter);
        }
        // TODO: Destroy connection objects, once the socket is closed
        if(msg->getKind() == SELF_MSGKIND_LISTEN_END) {
            /*
            isReceiving = false;
            listenStop();
            std::for_each(connectionSet.begin(), connectionSet.end(), [](VotingAppConnectionRequestReply * rrp){
                rrp->deleteModule();
            });
            */
        }
        if(msg->getKind() == SELF_MSGKIND_CONNECT) {
            socket.renewSocket();
            int localPort = par("localPort");
            // parameters
            std::string connectAddress{par("connectAddress").stringValue()};
            setupSocket(&socket, localPort);

            socket_adapter.setMsgKind(APP_CONN_REQUEST);
            socket_adapter.setSocket(&socket);
            socket_adapter.setParentComponent(this);
            EV_DEBUG << connectAddress << std::endl;
            connection_service.connect(socket_adapter,connectAddress,nodes, connection_map);
            //connect();
        }
        if(msg->getKind() == SELF_MSGKIND_SEND) {
            std::string connectAddress{par("connectAddress").stringValue()};
            std::string localAddress{par("localAddress").stringValue()};

            socket_adapter.setMsgKind(APP_CONN_REQUEST);
            socket_adapter.setSocket(&socket);
            socket_adapter.setParentComponent(this);
            connection_service.sendConnectionRequest(socket_adapter, connectAddress);

            packetsSent += 1;
            bytesSent += socket_adapter.getBytesSent();
        }
        if(msg->getKind() == SELF_MSGKIND_CLOSE) {
            //std::for_each(connectionSet.begin(), connectionSet.end(),[](VotingAppConnectionRequestReply* rr){
                //rr->cancelAndDelete();
            //});
            close();
        }
        if(msg->getKind() == SELF_MSGKIND_INIT_SYNC) {
            const char *connectAddress = par("connectAddress");
            socket.renewSocket();
            // parameters
            int syncPort = par("syncForwardPort");
            setupSocket(&socket, syncPort);
            const inet::Ipv4Address &addr = inet::Ipv4Address(connectAddress);
            socket_adapter.setMsgKind(APP_SYNC_REQUEST);

            sync_service.initSync(&socket_adapter, connectAddress);
            sync_service.sendInitialSyncRequest(&socket_adapter, connectAddress, nodes, connection_map);

            packetsSent += 1;
            bytesSent += socket_adapter.getBytesSent();
        }
        if(msg->getKind() == SELF_MSGKIND_LISTEN_SYNC_DOWN){
            isReceiving = true;
            socket.renewSocket();

            int syncPort = par("syncForwardPort");
            setupSocket(&socket, syncPort);
            
            socket_adapter.setSocket(&socket);
            connection_service.changeToListenState(socket_adapter);
        }
        if(msg->getKind() == SELF_MSGKIND_LISTEN_SYNC_UP){
            isReceiving = true;
            socket.renewSocket();

            int syncPort = par("syncReturnPort");
            setupSocket(&socket, syncPort);

            socket_adapter.setSocket(&socket);
            connection_service.changeToListenState(socket_adapter);
        }
        if(msg->getKind() == SELF_MSGKIND_FORWARD_SYNC_UP){
            isReceiving = false;
            socket.renewSocket();
            int syncPort = par("syncForwardPort");
            setupSocket(&socket, syncPort);
            socket_adapter.setMsgKind(APP_SYNC_REQUEST);
            socket_adapter.setSocket(&socket);
            const char *connectAddress = par("connectAddress");
            sync_service.forwardConnectSync(&socket_adapter, connectAddress);
            sync_service.forwardSyncRequestUp(&socket_adapter, nodes, connection_map, connectAddress, received_sync_request_from);
            packetsSent += 1;
            bytesSent += socket_adapter.getBytesSent();
        }
        if(msg->getKind() == SELF_MSGKIND_RETURN_SYNC_DOWN){
            isReceiving = false;
            int syncPort = par("syncReturnPort");
            socket.renewSocket();
            setupSocket(&socket, syncPort);
            socket_adapter.setMsgKind(APP_SYNC_RETURN);
            socket_adapter.setSocket(&socket);
            socket_adapter.setParentComponent(this);
            const std::string &address = socket.getLocalAddress().str();
            sync_service.returnSyncRequestDown(&socket_adapter, nodes, connection_map, address);
            packetsSent += 1;
            bytesSent += socket_adapter.getBytesSent();
        }
    }

    void VotingSetupApp::handleCrashOperation(inet::LifecycleOperation *) {
        EV_DEBUG << "handle crash" << std::endl;
    }

    void VotingSetupApp::handleStopOperation(inet::LifecycleOperation *) {
        EV_DEBUG << "handle stop" << std::endl;
    }

    void VotingSetupApp::socketEstablished(inet::TcpSocket *socket) {
        EV_DEBUG << "socket established" << std::endl;
        TcpAppBase::socketEstablished(socket);;
    }

    void VotingSetupApp::handleMessageWhenUp(omnetpp::cMessage *msg) {
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

    void VotingSetupApp::socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) {
        EV_DEBUG << "status arrived" << std::endl;
        TcpAppBase::socketStatusArrived(socket, status);
    }

    void VotingSetupApp::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) {
        if(isReceiving) {
            auto newSocket = new inet::TcpSocket(availableInfo); // create socket using received info
            socketMap.addSocket(newSocket); // store accepted connection

            if(availableInfo->getRemotePort() == 5555){
                down_connect_socket_id = newSocket->getSocketId();
            } else if(availableInfo->getRemotePort() == 5556){
                down_sync_socket_id = newSocket->getSocketId();
                received_sync_request_from = availableInfo->getRemoteAddr().str();
            } else if(availableInfo->getRemotePort() == 5557) {
                up_sync_socket_id = newSocket->getSocketId();
            } else {

            }

            this->socket.accept(availableInfo->getNewSocketId());

            //receiveIncomingMessages(socket, availableInfo);
            //auto newSocket = new inet::TcpSocket(availableInfo);
        } else {
            TcpAppBase::socketAvailable(socket, availableInfo);
        }
    }

    void VotingSetupApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) {
        const inet::Ptr<const inet::BytesChunk> &intrusivePtr = msg->peekData<inet::BytesChunk>();
        uint8_t appMsgKind = intrusivePtr->getByte(0);
        EV_DEBUG << "Message kind arrived: " << std::to_string(appMsgKind) << std::endl;

        const inet::Ptr<const inet::BytesChunk> &ptr = msg->peekData<inet::BytesChunk>();

        std::string content_str;
        std::transform(ptr->getBytes().begin() + 1,ptr->getBytes().end(),std::back_inserter(content_str),[](uint8_t d){
            return (char) d;
        });
        EV_DEBUG << content_str << std::endl;

        switch(appMsgKind){
            case APP_CONN_REPLY: {
                if(content_str.find("accept") != std::string::npos){
                    std::string connectAddress{par("connectAddress").stringValue()};
                    std::string localAddress{par("localAddress").stringValue()};

                    connection_service.computeConnectionReply(socketMessage{std::string(content_str), connectAddress}, nodes, connection_map, localAddress);
                    writeStateToFile("connection/","after_data_received_reply_" + getFullPath().substr(0,19) + ".json");
                } else {
                    EV_DEBUG << "not found" << std::endl;
                }
            }
                break;
            case APP_CONN_REQUEST:
            {
                auto *down_connect_socket = inet::check_and_cast<inet::TcpSocket *>(socketMap.getSocketById(down_connect_socket_id));
                try{
                    socket_adapter.setSocket(down_connect_socket);
                    socket_adapter.addProgrammedMessage(socketMessage{content_str, down_connect_socket->getRemoteAddress().str()});

                    if(connection_service.receiveConnectionRequest(socket_adapter, content_str, nodes, connection_map, down_connect_socket->getLocalAddress().str()) == 0){
                        socket->setState(inet::TcpSocket::CONNECTED);

                        down_connect_socket->setOutputGate(gate("socketOut"));
                        socket_adapter.setSocket(down_connect_socket);
                        socket_adapter.setMsgKind(APP_CONN_REPLY);
                        socket_adapter.setParentComponent(this);

                        connection_service.sendConnectionResponse(socket_adapter, "accept");
                    } else {
                        socket->send(createDataPacket("reject"));
                    }
                    writeStateToFile("connection/", "after_data_send_" + getFullPath().substr(0,19) + ".json");
                } catch(std::exception ex) {
                    EV_DEBUG << "Could not send" << std::endl;
                }
            }
            break;
            case APP_SYNC_REQUEST: {
                auto *down_sync_socket = inet::check_and_cast<inet::TcpSocket *>(socketMap.getSocketById(down_sync_socket_id));
                socket->setState(inet::TcpSocket::CONNECTED);
                down_sync_socket->setOutputGate(gate("socketOut"));
                socket_adapter.setSocket(down_sync_socket);

                socket_adapter.addProgrammedMessage(socketMessage{content_str,socket->getRemoteAddress().str()});
                sync_service.receiveSyncRequest(socket_adapter,nodes, connection_map);
                socket_adapter.setMsgKind(APP_SYNC_REPLY);
                if(doesConnect){
                    sync_service.sendSyncReply(&socket_adapter);
                }
            }
            break;
            case APP_SYNC_RETURN:
            {
                auto *up_sync_socket = inet::check_and_cast<inet::TcpSocket *>(socketMap.getSocketById(up_sync_socket_id));
                up_sync_socket->setOutputGate(gate("socketOut"));
                socket_adapter.setSocket(up_sync_socket);
                socket_adapter.addProgrammedMessage(socketMessage{content_str,socket->getRemoteAddress().str()});
                sync_service.receiveSyncRequest(socket_adapter,nodes, connection_map);
                socket_adapter.setMsgKind(APP_SYNC_REPLY);
                if(doesConnect){
                    sync_service.sendSyncReply(&socket_adapter);
                }
                writeStateToFile("sync/", "afterReturnReply."  + getFullPath().substr(0,19) + ".json");
            }
            break;
        }

        TcpAppBase::socketDataArrived(socket, msg, urgent);
    }

    void VotingSetupApp::addNode(std::string newNode) {
        nodes.insert(newNode);
    }

    void VotingSetupApp::writeStateToFile(std::string directory, std::string file) {
        //const std::filesystem::path &currentPath = std::filesystem::current_path();
        EV_DEBUG << "print: " << file << std::endl;
        connection_service.exportPeersList("./results/" + directory + "nodes/", nodes, file);
        connection_service.exportPeerConnections("./results/" + directory + "connections/", connection_map, file);
    }

    void VotingSetupApp::listenStop() {
        EV_DEBUG << "stopped listening on " << this->getFullPath() << std::endl;
        socket.renewSocket();
    }

    void VotingSetupApp::setupSocket(inet::TcpSocket* socket, int port) {
        const char *localAddress = par("localAddress");
        socket->bind(inet::Ipv4Address(localAddress), port);
        socket->setCallback(this);
        socket->setOutputGate(gate("socketOut"));
    }

    int VotingSetupApp::getPacketsSent() {
        return packetsSent;
    }

    int VotingSetupApp::getBytesSent() {
        return bytesSent;
    }

    void VotingSetupApp::setPacketsSent(int newPacketsSent) {
        packetsSent = newPacketsSent;
    }

    void VotingSetupApp::setBytesSent(int newBytesSend) {
        packetsSent = newBytesSend;
    }

    inet::Packet* VotingSetupApp::createDataPacket(std::string send_string) {
        EV_DEBUG << "create data" << std::endl;
        inet::Ptr<inet::Chunk> payload;
        inet::Packet *packet = new inet::Packet("data");

        const auto &bytesChunk = inet::makeShared<inet::BytesChunk>();
        std::vector<uint8_t> vec;
        unsigned long length = send_string.length();

        vec.resize(send_string.length());
        for (int i = 0; i < send_string.length(); i++)
            vec[i] = (bytesSent + send_string[i % length]) & 0xFF;

        bytesChunk->setBytes(vec);
        packet->insertAtBack(bytesChunk);

        auto byteCountData = inet::makeShared<inet::BytesChunk>();
        std::vector<uint8_t> appData { APP_CONN_REPLY };
        byteCountData->setBytes(appData);

        packet->insertAtFront(byteCountData);
        EV_DEBUG << "return packet" << std::endl;
        return packet;
    }
}