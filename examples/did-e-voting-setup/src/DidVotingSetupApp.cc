//
// Created by wld on 27.11.22.
//

#include "DidVotingSetupApp.h"
#include <algorithm>
#include <inet/networklayer/common/L3AddressResolver.h>
#include "inet/transportlayer/tcp/Tcp.h"
#include "inet/common/INETUtils.h"
#include "nlohmann/json.hpp"
#include "../../../src/PacketsKind.h"

namespace voting {
    Define_Module(DidVotingSetupApp);

    void DidVotingSetupApp::initialize(int stage) {
        ApplicationBase::initialize(stage);

        if (stage == inet::INITSTAGE_LOCAL) {
            numSessions = numBroken = packetsSent = packetsRcvd = bytesSent = bytesRcvd = 0;

            WATCH(numSessions);
            WATCH(numBroken);
            WATCH(packetsSent);
            WATCH(packetsRcvd);
            WATCH(bytesSent);
            WATCH(bytesRcvd);

            connect_address = par("connectAddress").stringValue();
            local_address = par("localAddress").stringValue();

            const auto p1 = std::chrono::system_clock::now();

            std::time_t today_time = std::chrono::system_clock::to_time_t(p1);
            local_did = identity_service.createLocalDid(today_time, local_address, "abcs");
        }
    }

    void DidVotingSetupApp::handleStartOperation(inet::LifecycleOperation *) {
        // Export state at beginning
        writeStateToFile("connection/", "at_start_" + this->getFullPath().substr(0,19) + ".json");

        // Schedule events
        double tOpen = par("tOpen").doubleValue();
        double tSend = par("tSend").doubleValue();
        double tListenStart = par("tListenStart").doubleValue();
        double tSyncInit = par("tSyncInit").doubleValue();
        double tListenDownSync = par("tListenDownSync").doubleValue();

        if (inet::simTime() <= tOpen) {
            connectSelfMessage = new inet::cMessage("timer");
            connectSelfMessage->setKind(SELF_MSGKIND_CONNECT);
            scheduleAt(tOpen, connectSelfMessage);
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

    void DidVotingSetupApp::handleTimer(inet::cMessage *msg) {
        if(msg->getKind() == SELF_MSGKIND_LISTEN) {
            listen_connection_socket->renewSocket();
            int localPort = par("localPort");
            setupSocket(listen_connection_socket, localPort);
            socket_adapter.setSocket(listen_connection_socket);
            socket_adapter.listen();
        }
        if(msg->getKind() == SELF_MSGKIND_CONNECT) {
            socket.renewSocket();
            int localPort = par("localPort");
            // parameters
            setupSocket(&socket, localPort);

            socket_adapter.setMsgKind(APP_CONN_REQUEST);
            socket_adapter.setSocket(&socket);
            socket_adapter.setParentComponent(this);
            connection_service.connect(socket_adapter,connect_address,nodes, storage);
            //connect();
        }
        if(msg->getKind() == SELF_MSGKIND_SEND) {
            socket_adapter.setMsgKind(APP_CONN_REQUEST);
            socket_adapter.setSocket(&socket);
            socket_adapter.setParentComponent(this);

            std::stringstream local_did_stream;
            local_did_stream << local_did;
            std::string local_did_string = local_did_stream.str();
            connection_service.sendConnectionRequest(socket_adapter, local_did_string);

            packetsSent += 1;
            bytesSent += socket_adapter.getBytesSent();
        }
        if(msg->getKind() == SELF_MSGKIND_CLOSE) {
            close();
        }
        if(msg->getKind() == SELF_MSGKIND_INIT_SYNC) {
            upSyncSocket->renewSocket();
            setupSocket(upSyncSocket, 5556);
            socket_adapter.setSocket(upSyncSocket);
            socket_adapter.setMsgKind(APP_SYNC_REQUEST);
            socket_adapter.setParentComponent(this);
            socket_adapter.setIsMultiPackageData(true);
            storage.fetchResource(connect_did);

            sync_service.initSync(&socket_adapter, connect_did, storage);
            sync_service.sendInitialSyncRequest(&socket_adapter, local_did, storage);

            packetsSent += 1;
            bytesSent += socket_adapter.getBytesSent();
        }
        if(msg->getKind() == SELF_MSGKIND_LISTEN_SYNC_DOWN){
            listen_sync_down_socket->renewSocket();

            setupSocket(listen_sync_down_socket, 5556);
            socket_adapter.setSocket(listen_sync_down_socket);
            socket_adapter.listen();
        }
        if(msg->getKind() == SELF_MSGKIND_LISTEN_SYNC_UP){
            listen_sync_up_socket->renewSocket();
            setupSocket(listen_sync_up_socket, 5557);

            socket_adapter.setSocket(listen_sync_up_socket);
            socket_adapter.listen();
        }
        if(msg->getKind() == SELF_MSGKIND_FORWARD_SYNC_UP){
            upSyncSocket->renewSocket();
            setupSocket(upSyncSocket, 5556);
            socket_adapter.setParentComponent(this);
            socket_adapter.setMsgKind(APP_SYNC_REQUEST);
            socket_adapter.setSocket(upSyncSocket);
            socket_adapter.setIsMultiPackageData(true);
            sync_service.forwardConnectSync(&socket_adapter, connect_did, storage);
            sync_service.forwardSyncRequestUp(&socket_adapter, storage, connect_did, received_sync_request_from);
            packetsSent += 1;
            bytesSent += socket_adapter.getBytesSent();
        }
        if(msg->getKind() == SELF_MSGKIND_RETURN_SYNC_DOWN){
            isReturning = true;
            downSyncSocket->renewSocket();
            setupSocket(downSyncSocket, 5557);
            socket_adapter.setParentComponent(this);
            socket_adapter.setMsgKind(APP_SYNC_RETURN);
            socket_adapter.setSocket(downSyncSocket);
            socket_adapter.setIsMultiPackageData(true);

            writeStateToFile("connection/","before_return_" + getFullPath().substr(0,19) + ".json");
            EV_DEBUG << "now connect return" << std::endl;
            sync_service.returnSyncRequestDown(&socket_adapter, nodes, storage, local_did);
            EV_DEBUG << "now send data return" << std::endl;
            sync_service.returnSyncRequestDownData(&socket_adapter, nodes, storage, local_did);
            EV_DEBUG << "has send data" << std::endl;

            packetsSent += 1;
            bytesSent += socket_adapter.getBytesSent();
        }
        if(msg->getKind() == SELF_MSGKIND_CLOSE_SYNC_SOCKET){
            socket_adapter.setSocket(upSyncSocket);
            socket_adapter.setParentComponent(this);
            socket_adapter.close();
        }
    }

    void DidVotingSetupApp::handleCrashOperation(inet::LifecycleOperation *) {
        EV_DEBUG << "handle crash" << std::endl;
    }

    void DidVotingSetupApp::handleStopOperation(inet::LifecycleOperation *) {
        EV_DEBUG << "handle stop" << std::endl;
    }

    void DidVotingSetupApp::socketEstablished(inet::TcpSocket *socket) {
        EV_DEBUG << "socket established" << std::endl;
        TcpAppBase::socketEstablished(socket);;
    }

    void DidVotingSetupApp::handleMessageWhenUp(omnetpp::cMessage *msg) {
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

    void DidVotingSetupApp::socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) {
        EV_DEBUG << "status arrived" << std::endl;
        TcpAppBase::socketStatusArrived(socket, status);
    }

    void DidVotingSetupApp::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) {
        auto newSocket = new inet::TcpSocket(availableInfo); // create socket using received info

        if(availableInfo->getLocalPort() == 5555) {
            socket->setOutputGate(gate("socketOut"));
            socket->accept(availableInfo->getNewSocketId());
            socketMap.addSocket(newSocket); // store accepted connection
        } else if(availableInfo->getLocalPort() == 5556 && std::equal(connect_address.begin(), connect_address.end(), availableInfo->getRemoteAddr().str().begin())) {
            upSyncSocket->setOutputGate(gate("socketOut"));
            upSyncSocket->accept(availableInfo->getNewSocketId());
            socketMap.addSocket(newSocket); // store accepted connection
        } else if(availableInfo->getLocalPort() == 5556 && !std::equal(connect_address.begin(), connect_address.end(), availableInfo->getRemoteAddr().str().begin())) {
            downSyncSocket->setOutputGate(gate("socketOut"));
            downSyncSocket->accept(availableInfo->getNewSocketId());
            socketMap.addSocket(newSocket); // store accepted connection
        } else if(availableInfo->getLocalPort() == 5557 && std::equal(connect_address.begin(), connect_address.end(), availableInfo->getRemoteAddr().str().begin())) {
            EV_DEBUG << "available on upsyncsocket 5557" << std::endl;
            upSyncSocket->setOutputGate(gate("socketOut"));
            upSyncSocket->accept(availableInfo->getNewSocketId());
            socketMap.addSocket(newSocket); // store accepted connection
        } else if(availableInfo->getLocalPort() == 5557 && !std::equal(connect_address.begin(), connect_address.end(), availableInfo->getRemoteAddr().str().begin())) {
            EV_DEBUG << "available on downsyncsocket 5557" << std::endl;
            downSyncSocket->setOutputGate(gate("socketOut"));
            downSyncSocket->accept(availableInfo->getNewSocketId());
            socketMap.addSocket(newSocket); // store accepted connection
        }
    }

    void DidVotingSetupApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) {
        EV_DEBUG << "socket data arrived" << std::endl;
        const inet::Ptr<const inet::BytesChunk> &intrusivePtr = msg->peekData<inet::BytesChunk>();
        uint8_t appMsgKind = intrusivePtr->getByte(0);
        const inet::Ptr<const inet::BytesChunk> &ptr = msg->peekData<inet::BytesChunk>();
        int offset = 1;

        // TODO: Logik in service verlagern
        if(isReceivingMultipackageMessage){
            EV_DEBUG << "is receiving multi package" << std::endl;
            appMsgKind = saved_package_type;
            offset = 0;
        }

        std::string content_str;
        std::transform(ptr->getBytes().begin() + offset,ptr->getBytes().end(),std::back_inserter(content_str),[](uint8_t d){
            return (char) d;
        });

        if(isReceivingMultipackageMessage){
            int count = 0;
            std::for_each(ptr->getBytes().rbegin(), ptr->getBytes().rbegin() + 5,[&count](char c){
               if(c == '#') {
                   count++;
               };
            });
            EV_DEBUG << "count: " << count << std::endl;
            if(count > 3){
                EV_DEBUG << "found exit sequence" << std::endl;
                content_str = "";
                std::transform(ptr->getBytes().begin() + offset,ptr->getBytes().end() - count,std::back_inserter(content_str),[](uint8_t d){
                    return (char) d;
                });
                hasReceivedLastPackageFromMultiMessage = true;
            }
        }

        switch(appMsgKind){
            case APP_CONN_REPLY: {
                if(did::validateDID(content_str)){
                    socket_adapter.setSocket(socket);
                    socket_adapter.addProgrammedMessage(socketMessage{std::string(content_str),connect_address});
                    std::set<did> addresses = storage.getAllDIDs();
                    connection_service.computeConnectionReply(socket_adapter,storage, local_did);
                    socket_adapter.close();
                    connect_did = did(content_str);
                    writeStateToFile("connection/","after_data_received_reply_" + getFullPath().substr(0,19) + ".json");
                } else {
                    EV_DEBUG << "not found" << std::endl;
                }
            }
                break;
            case APP_CONN_REQUEST:
            {
                EV_DEBUG << "received connection request" << std::endl;
                try{
                    socket_adapter.setSocket(socket);
                    socket_adapter.addProgrammedMessage(socketMessage{content_str, socket->getRemoteAddress().str()});

                    if(connection_service.receiveConnectionRequest(socket_adapter,  storage, local_did) == 0){
                        socket->setOutputGate(gate("socketOut"));
                        socket_adapter.setSocket(socket);
                        socket_adapter.setMsgKind(APP_CONN_REPLY);
                        socket_adapter.setParentComponent(this);

                        connection_service.sendConnectionSuccess(socket_adapter, local_did);
                    } else {
                        // TODO: Connection failure, or ask for authentication
                        socket->send(createDataPacket("reject"));
                    }
                    writeStateToFile("connection/", "after_data_send_" + getFullPath().substr(0,19) + ".json");
                } catch(std::exception ex) {
                    EV_DEBUG << "Could not send" << std::endl;
                }
            }
            break;
            case APP_SYNC_REQUEST: {
                EV_DEBUG << "received sync request" << std::endl;
                if(!isReceivingMultipackageMessage && !hasReceivedLastPackageFromMultiMessage) {
                    isReceivingMultipackageMessage = true;
                    saved_package_type = APP_SYNC_REQUEST;
                    message_stream << content_str;
                } else if(isReceivingMultipackageMessage && hasReceivedLastPackageFromMultiMessage) {
                    message_stream << content_str;
                    socket->setOutputGate(gate("socketOut"));
                    socket_adapter.setSocket(socket);
                    EV_DEBUG << message_stream.str() << std::endl;
                    socket_adapter.addProgrammedMessage(socketMessage{message_stream.str(),socket->getRemoteAddress().str()});
                    sync_service.receiveSyncRequest(socket_adapter,storage);
                    socket_adapter.setMsgKind(APP_SYNC_REPLY);
                    received_sync_request_from = socket->getRemoteAddress().str();
                    sync_service.sendSyncReply(&socket_adapter);
                    isReceivingMultipackageMessage = false;
                    hasReceivedLastPackageFromMultiMessage = false;
                    message_stream.str(std::string());

                    if(!connect_address.empty()) {
                        forwardUpSyncMessage = new inet::cMessage("timer");
                        forwardUpSyncMessage->setKind(SELF_MSGKIND_FORWARD_SYNC_UP);
                        scheduleAt(inet::simTime() + forwardRequestDelta, forwardUpSyncMessage);
                    } else {
                        EV_DEBUG << "now return" << std::endl;
                        returnDownSyncMessage = new inet::cMessage("timer");
                        returnDownSyncMessage->setKind(SELF_MSGKIND_RETURN_SYNC_DOWN);
                        scheduleAt(inet::simTime() + returnSyncRequestDelta, returnDownSyncMessage);
                    }
                } else if(isReceivingMultipackageMessage && !hasReceivedLastPackageFromMultiMessage){
                    EV_DEBUG << "package in the middle" << std::endl;
                    message_stream << content_str;
                }
            }
            break;
            case APP_SYNC_RETURN:
            {
                if(!isReceivingMultipackageMessage && !hasReceivedLastPackageFromMultiMessage) {
                    EV_DEBUG << "received initial package" << std::endl;
                    isReceivingMultipackageMessage = true;
                    hasReceivedLastPackageFromMultiMessage = false;
                    saved_package_type = APP_SYNC_RETURN;
                    EV_DEBUG << "message stream" << message_stream.str() << std::endl;
                    message_stream << content_str;
                } else if(isReceivingMultipackageMessage && hasReceivedLastPackageFromMultiMessage) {
                    message_stream << content_str;
                    socket->setOutputGate(gate("socketOut"));
                    EV_DEBUG << "now save from return" << std::endl;
                    EV_DEBUG << message_stream.str() << std::endl;
                    socket_adapter.setSocket(socket);
                    socket_adapter.addProgrammedMessage(socketMessage{message_stream.str(),socket->getRemoteAddress().str()});
                    sync_service.receiveSyncRequest(socket_adapter,storage);
                    socket_adapter.setMsgKind(APP_SYNC_REPLY);
                    socket_adapter.setParentComponent(this);
                    sync_service.sendSyncReply(&socket_adapter);

                    isReceivingMultipackageMessage = false;
                    hasReceivedLastPackageFromMultiMessage = false;
                    message_stream.str(std::string());


                    if(storage.hasIdDown(local_did)) {
                        returnDownSyncMessage = new inet::cMessage("timer");
                        returnDownSyncMessage->setKind(SELF_MSGKIND_RETURN_SYNC_DOWN);
                        scheduleAt(inet::simTime() + returnSyncRequestDelta, returnDownSyncMessage);
                    }

                    writeStateToFile("sync/", "afterReturnReply."  + getFullPath().substr(0,19) + ".json");
                } else if(isReceivingMultipackageMessage && !hasReceivedLastPackageFromMultiMessage){
                    EV_DEBUG << "package in the middle" << std::endl;
                    message_stream << content_str;
                }
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

    void DidVotingSetupApp::writeStateToFile(std::string directory, std::string file) {
        //const std::filesystem::path &currentPath = std::filesystem::current_path();
        EV_DEBUG << "print: " << file << std::endl;
        //connection_service.exportPeersList("./results/" + directory + "nodes/", nodes, file);
        connection_service.exportDidRegistry("./results/" + directory + "connections/", storage, file);
    }

    void DidVotingSetupApp::setupSocket(inet::TcpSocket* socket, int port) {
        socket->bind(inet::Ipv4Address(local_address.c_str()), port);
        socket->setCallback(this);
        socket->setOutputGate(gate("socketOut"));
    }

    inet::Packet* DidVotingSetupApp::createDataPacket(std::string send_string) {
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