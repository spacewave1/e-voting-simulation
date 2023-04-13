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

            connectAddress = par("connectAddress").stringValue();
            localAddress = par("localAddress").stringValue();

            socket_adapter.setParentComponent(this);
        }
    }

    void VotingSetupApp::handleStartOperation(inet::LifecycleOperation *) {
        // Export state at beginning
        writeStateToFile("connection/", "at_start_" + this->getFullPath().substr(0,19) + ".json");

        // Schedule events
        double tConnect = par("tConnect").doubleValue();
        double tListenStart = par("tListenStart").doubleValue();
        double tSyncInit = par("tSyncInit").doubleValue();
        double tListenDownSync = par("tListenDownSync").doubleValue();

        if (inet::simTime() <= tConnect) {
            connectSelfMessage = new inet::cMessage("timer");
            connectSelfMessage->setKind(SELF_MSGKIND_CONNECT_REQUEST);
            scheduleAt(tConnect, connectSelfMessage);
        }
        if (inet::simTime() <= tListenStart) {
            listenStartMessage = new inet::cMessage("timer");
            listenStartMessage->setKind(SELF_MSGKIND_LISTEN);
            scheduleAt(tListenStart, listenStartMessage);
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
    }

    void VotingSetupApp::handleTimer(inet::cMessage *msg) {
        if(msg->getKind() == SELF_MSGKIND_LISTEN) {
            listen_connection_socket->renewSocket();
            int localPort = par("localPort");
            setupSocket(listen_connection_socket, localPort);
            socket_adapter.setSocket(listen_connection_socket);
            connection_service.changeToListenState(socket_adapter);
        }
        if(msg->getKind() == SELF_MSGKIND_CONNECT_REQUEST) {
            socket.renewSocket();
            int localPort = par("localPort");
            // parameters
            setupSocket(&socket, localPort);

            socket_adapter.setMsgKind(APP_CONN_REQUEST);
            socket_adapter.setSocket(&socket);
            connection_service.connect(socket_adapter,connectAddress);
            connection_service.sendConnectionRequest(socket_adapter, connectAddress);

            packetsSent += 1;
            bytesSent += socket_adapter.getBytesSent();
        }
        if(msg->getKind() == SELF_MSGKIND_CONNECT_REPLY) {
            omnetpp::cMsgPar &msgPar = msg->par("socketId");
            long socketId = msgPar.longValue();
            inet::ISocket *pSocket = socketMap.getSocketById(socketId);
            auto *pTcpSocket = dynamic_cast<inet::TcpSocket *>(pSocket);
            pTcpSocket->setOutputGate(gate("socketOut"));

            socket_adapter.setMsgKind(APP_CONN_REPLY);
            socket_adapter.setSocket(pTcpSocket);
            connection_service.sendConnectionResponse(socket_adapter, "accept");
        }
        if(msg->getKind() == SELF_MSGKIND_CLOSE) {
            close();
        }
        if(msg->getKind() == SELF_MSGKIND_INIT_SYNC) {
            // parameters
            setupSocket(upSyncSocket, 5556);
            upSyncSocket->renewSocket();
            socket_adapter.setSocket(upSyncSocket);
            socket_adapter.setMsgKind(APP_SYNC_REQUEST);

            sync_service.initSync(&socket_adapter, connectAddress);
            sync_service.sendInitialSyncRequest(&socket_adapter, connectAddress, nodes, connection_map);

            packetsSent += 1;
            bytesSent += socket_adapter.getBytesSent();
        }
        if(msg->getKind() == SELF_MSGKIND_LISTEN_SYNC_DOWN){
            listen_sync_down_socket->renewSocket();

            setupSocket(listen_sync_down_socket, 5556);
            socket_adapter.setSocket(listen_sync_down_socket);
            connection_service.changeToListenState(socket_adapter);
        }
        if(msg->getKind() == SELF_MSGKIND_LISTEN_SYNC_UP){
            listen_sync_up_socket->renewSocket();

            setupSocket(listen_sync_up_socket, 5557);

            socket_adapter.setSocket(listen_sync_up_socket);
            connection_service.changeToListenState(socket_adapter);
        }
        if(msg->getKind() == SELF_MSGKIND_FORWARD_SYNC_UP){
            upSyncSocket->renewSocket();
            setupSocket(upSyncSocket, 5556);
            socket_adapter.setMsgKind(APP_SYNC_REQUEST);
            socket_adapter.setSocket(upSyncSocket);

            sync_service.forwardConnectSync(&socket_adapter, connectAddress, 5556);
            sync_service.forwardSyncRequestUp(&socket_adapter, nodes, connection_map, connectAddress, received_sync_request_from);
            packetsSent += 1;
            bytesSent += socket_adapter.getBytesSent();
        }
        if(msg->getKind() == SELF_MSGKIND_RETURN_SYNC_DOWN){
            isReturning = true;
            downSyncSocket->renewSocket();
            setupSocket(downSyncSocket, 5557);
            socket_adapter.setMsgKind(APP_SYNC_RETURN);
            socket_adapter.setSocket(downSyncSocket);
            sync_service.returnSyncRequestDown(&socket_adapter, nodes, connection_map, localAddress);
            sync_service.returnSyncRequestDownSendData(&socket_adapter, nodes, connection_map, localAddress);
            packetsSent += 1;
            bytesSent += socket_adapter.getBytesSent();
        }
        if(msg->getKind() == SELF_MSGKIND_CLOSE_SYNC_SOCKET){
            socket_adapter.setSocket(upSyncSocket);
            socket_adapter.close();
        }
    }

    void VotingSetupApp::handleCrashOperation(inet::LifecycleOperation *) {
        EV_DEBUG << "handle crash" << std::endl;
    }

    void VotingSetupApp::handleStopOperation(inet::LifecycleOperation *) {
        EV_DEBUG << "handle stop" << std::endl;
    }

    void VotingSetupApp::socketEstablished(inet::TcpSocket *socket) {
        TcpAppBase::socketEstablished(socket);;
    }

    void VotingSetupApp::handleMessageWhenUp(omnetpp::cMessage *msg) {
        if(msg->isSelfMessage()){
            handleTimer(msg);
        } else if (socket.belongsToSocket(msg)) {
            socket.processMessage(msg);
        } else if (listen_connection_socket->belongsToSocket(msg)) {
            listen_connection_socket->processMessage(msg);
        } else if (listen_sync_up_socket->belongsToSocket(msg)) {
            listen_sync_up_socket->processMessage(msg);
        } else if (listen_sync_down_socket->belongsToSocket(msg)) {
            listen_sync_down_socket->processMessage(msg);
        } else if (upSyncSocket->belongsToSocket(msg)) {
            upSyncSocket->processMessage(msg);
        } else if (downSyncSocket->belongsToSocket(msg)) {
            downSyncSocket->processMessage(msg);
        }
        //else if(socketMap.findSocketFor(msg)){
        //    socketMap.findSocketFor(msg)->processMessage(msg);
        //}
        else {
            try {
                    inet::Packet *pPacket = inet::check_and_cast<inet::Packet *>(msg);
                    socketDataArrived(inet::check_and_cast<inet::TcpSocket *>(socketMap.findSocketFor(msg)), pPacket,
                                      false);
                    return;
                } catch (std::exception &ex) {
                    EV_DEBUG << "cannot cast to packet" << std::endl;
                    EV_DEBUG << ex.what() << std::endl;
                }
            try {
                inet::TcpCommand *pCommand = inet::check_and_cast<inet::TcpCommand *>(msg->getControlInfo());
                inet::TcpCommandCode code = static_cast<inet::TcpCommandCode>(msg->getKind());
                EV_DEBUG << code << std::endl;
            } catch (std::exception &ex) {
                EV_DEBUG << "cannot cast to command" << std::endl;
                EV_DEBUG << ex.what() << std::endl;
            }
            }
        //else
        //     throw inet::cRuntimeError("Unknown incoming message: '%s'", msg->getName());
    }

    void VotingSetupApp::socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) {
        TcpAppBase::socketStatusArrived(socket, status);
    }

    void VotingSetupApp::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) {
        auto newSocket = new inet::TcpSocket(availableInfo); // create socket using received info

        if(availableInfo->getLocalPort() == 5555) {
            socket->setOutputGate(gate("socketOut"));
            socket->accept(availableInfo->getNewSocketId());
            socketMap.addSocket(newSocket); // store accepted connection
        } else if(availableInfo->getLocalPort() == 5556 && std::equal(connectAddress.begin(), connectAddress.end(), availableInfo->getRemoteAddr().str().begin())) {
            upSyncSocket->setOutputGate(gate("socketOut"));
            upSyncSocket->accept(availableInfo->getNewSocketId());
            socketMap.addSocket(newSocket); // store accepted connection
        } else if(availableInfo->getLocalPort() == 5556 && !std::equal(connectAddress.begin(), connectAddress.end(), availableInfo->getRemoteAddr().str().begin())) {
            downSyncSocket->setOutputGate(gate("socketOut"));
            downSyncSocket->accept(availableInfo->getNewSocketId());
            socketMap.addSocket(newSocket); // store accepted connection
        } else if(availableInfo->getLocalPort() == 5557 && std::equal(connectAddress.begin(), connectAddress.end(), availableInfo->getRemoteAddr().str().begin())) {
            upSyncSocket->setOutputGate(gate("socketOut"));
            upSyncSocket->accept(availableInfo->getNewSocketId());
            socketMap.addSocket(newSocket); // store accepted connection
        } else if(availableInfo->getLocalPort() == 5557 && !std::equal(connectAddress.begin(), connectAddress.end(), availableInfo->getRemoteAddr().str().begin())) {
            downSyncSocket->setOutputGate(gate("socketOut"));
            downSyncSocket->accept(availableInfo->getNewSocketId());
            socketMap.addSocket(newSocket); // store accepted connection
        }
    }

    void VotingSetupApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) {
        const inet::Ptr<const inet::BytesChunk> &intrusivePtr = msg->peekData<inet::BytesChunk>();
        uint8_t appMsgKind = intrusivePtr->getByte(0);

        const inet::Ptr<const inet::BytesChunk> &ptr = msg->peekData<inet::BytesChunk>();

        std::string content_str;
        std::transform(ptr->getBytes().begin() + 1,ptr->getBytes().end(),std::back_inserter(content_str),[](uint8_t d){
            return (char) d;
        });

        switch(appMsgKind){
            case APP_CONN_REPLY: {
                if(content_str.find("accept") != std::string::npos){
                    socket_adapter.setSocket(socket);
                    socket_adapter.addProgrammedMessage(socketMessage{std::string(content_str),connectAddress});
                    connection_service.computeConnectionReply(socket_adapter,  nodes, connection_map, localAddress);
                    socket_adapter.close();
                    writeStateToFile("connection/","after_data_received_reply_" + getFullPath().substr(0,19) + ".json");
                } else {
                    EV_DEBUG << "not found" << std::endl;
                }
            }
                break;
            case APP_CONN_REQUEST:
            {
                try{
                    socket_adapter.setSocket(socket);
                    socket_adapter.addProgrammedMessage(socketMessage{content_str, socket->getRemoteAddress().str()});

                    if(connection_service.receiveConnectionRequest(socket_adapter, content_str, nodes, connection_map, downSyncSocket->getLocalAddress().str()) == 0){
                        connectionReplyMessage = new inet::cMessage("timer");
                        auto *socketIdPar = new omnetpp::cMsgPar("socketId");
                        socketIdPar->setLongValue(socket->getSocketId());
                        connectionReplyMessage->addPar(socketIdPar);
                        connectionReplyMessage->setKind(SELF_MSGKIND_CONNECT_REPLY);
                        scheduleAt(inet::simTime() + connectionRequestReplyDelta, connectionReplyMessage);

                    } else {
                        EV_DEBUG << "send reject" << std::endl;
                        socket->send(createDataPacket("reject"));
                    }
                    writeStateToFile("connection/", "after_data_send_" + getFullPath().substr(0,19) + ".json");
                } catch(std::exception ex) {
                    EV_DEBUG << "Could not send" << std::endl;
                }
            }
            break;
            case APP_SYNC_REQUEST: {
                socket->setOutputGate(gate("socketOut"));
                socket_adapter.setSocket(socket);
                socket_adapter.addProgrammedMessage(socketMessage{content_str,socket->getRemoteAddress().str()});
                sync_service.receiveSyncRequest(socket_adapter,nodes, connection_map);
                socket_adapter.setMsgKind(APP_SYNC_REPLY);

                received_sync_request_from = socket->getRemoteAddress().str();

                sync_service.sendSyncReply(&socket_adapter);

                if(!connectAddress.empty()) {
                    forwardUpSyncMessage = new inet::cMessage("timer");
                    forwardUpSyncMessage->setKind(SELF_MSGKIND_FORWARD_SYNC_UP);
                    scheduleAt(inet::simTime() + forwardRequestDelta, forwardUpSyncMessage);
                } else {
                    returnDownSyncMessage = new inet::cMessage("timer");
                    returnDownSyncMessage->setKind(SELF_MSGKIND_RETURN_SYNC_DOWN);
                    scheduleAt(inet::simTime() + returnSyncRequestDelta, returnDownSyncMessage);
                }
            }
            break;
            case APP_SYNC_RETURN:
            {
                socket->setOutputGate(gate("socketOut"));
                socket_adapter.setSocket(socket);
                socket_adapter.addProgrammedMessage(socketMessage{content_str,socket->getRemoteAddress().str()});
                sync_service.receiveSyncRequest(socket_adapter,nodes, connection_map);
                socket_adapter.setMsgKind(APP_SYNC_REPLY);
                sync_service.sendSyncReply(&socket_adapter);

                returnDownSyncMessage = new inet::cMessage("timer");
                returnDownSyncMessage->setKind(SELF_MSGKIND_RETURN_SYNC_DOWN);
                scheduleAt(inet::simTime() + returnSyncRequestDelta, returnDownSyncMessage);

                writeStateToFile("sync/", "afterReturnReply."  + getFullPath().substr(0,19) + ".json");
            }
            break;
            case APP_SYNC_REPLY:
            {
                if(!isReturning) {
                    closeSocketMessage = new inet::cMessage("timer");
                    closeSocketMessage->setKind(SELF_MSGKIND_CLOSE_SYNC_SOCKET);
                    scheduleAt(inet::simTime(), closeSocketMessage);
                    listenUpSyncMessage = new inet::cMessage("timer");
                    listenUpSyncMessage->setKind(SELF_MSGKIND_LISTEN_SYNC_UP);
                    scheduleAt(inet::simTime() + listenUpDelta, listenUpSyncMessage);
                }
            }
            break;
        }

        TcpAppBase::socketDataArrived(socket, msg, urgent);
    }

    void VotingSetupApp::writeStateToFile(std::string directory, std::string file) {
        //const std::filesystem::path &currentPath = std::filesystem::current_path();
        connection_service.exportPeersList("./results/" + directory + "nodes/", nodes, file);
        connection_service.exportPeerConnections("./results/" + directory + "connections/", connection_map, file);
    }

    void VotingSetupApp::setupSocket(inet::TcpSocket* socket, int port) {
        socket->bind(inet::Ipv4Address(localAddress.c_str()), port);
        socket->setCallback(this);
        socket->setOutputGate(gate("socketOut"));
    }

    inet::Packet* VotingSetupApp::createDataPacket(std::string send_string) {
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
        return packet;
    }
}