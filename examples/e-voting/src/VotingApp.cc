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
        double tListenUpSync = par("tListenUpSync").doubleValue();
        double tListenDownSync = par("tListenDownSync").doubleValue();
        double tForwardUpSync = par("tForwardUpSync").doubleValue();
        double tReturnSyncDown = par("tReturnDownSync").doubleValue();
        isReceiving = par("isReceivingAtStart").boolValue();

        if (inet::simTime() <= tOpen) {
            EV_DEBUG << "schedule connect at: " << tOpen << std::endl;
            connectSelfMessage = new inet::cMessage("timer");
            connectSelfMessage->setKind(SELF_MSGKIND_CONNECT);
            scheduleAt(tOpen, connectSelfMessage);
        } else {
            doesConnect = false;
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

    void VotingApp::handleTimer(inet::cMessage *msg) {
        if(msg->getKind() == SELF_MSGKIND_LISTEN) {
            EV_DEBUG << "now listen" << std::endl;
            isReceiving = true;
            socket.renewSocket();
            int localPort = par("localPort");
            setupSocket(&socket, localPort);
            socket_adapter.setSocket(&socket);
            connection_service.changeToListenState(socket_adapter);
            EV_DEBUG << "before connection service" << std::endl;
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
            connection_service.connect(socket_adapter,connectAddress,nodes, connection_map);
            //connect();
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
            int syncPort = par("syncForwardPort");
            setupSocket(&socket, syncPort);
            const inet::Ipv4Address &addr = inet::Ipv4Address(connectAddress);
            socket_adapter.setMsgKind(APP_SYNC_REQUEST);

            EV_DEBUG << "set kind: " << std::to_string(socket_adapter.getMsgKind()) << std::endl;
            sync_service.initSync(&socket_adapter, connectAddress);
            sync_service.sendInitialSyncRequest(&socket_adapter, connectAddress, nodes, connection_map);
        }
        if(msg->getKind() == SELF_MSGKIND_LISTEN_SYNC_DOWN){
            EV_DEBUG << "now listen down" << std::endl;
            isReceiving = true;
            socket.renewSocket();

            int syncPort = par("syncForwardPort");
            setupSocket(&socket, syncPort);
            
            socket_adapter.setSocket(&socket);
            connection_service.changeToListenState(socket_adapter);

            EV_DEBUG << "before connection service" << std::endl;
        }
        if(msg->getKind() == SELF_MSGKIND_LISTEN_SYNC_UP){
            EV_DEBUG << "now listen up" << std::endl;
            isReceiving = true;
            socket.renewSocket();

            int syncPort = par("syncReturnPort");
            EV_DEBUG << "remote addr:" << socket.getRemoteAddress() << std::endl;
            setupSocket(&socket, syncPort);

            socket_adapter.setSocket(&socket);
            EV_DEBUG << "remote addr:" << socket.getRemoteAddress() << std::endl;
            EV_DEBUG << "state:" << inet::TcpSocket::stateName(socket.getState()) << std::endl;
            connection_service.changeToListenState(socket_adapter);
        }
        if(msg->getKind() == SELF_MSGKIND_FORWARD_SYNC_UP){
            EV_DEBUG << "now forward up" << std::endl;
            isReceiving = false;

            socket.renewSocket();
            int syncPort = par("syncForwardPort");
            setupSocket(&socket, syncPort);

            socket_adapter.setMsgKind(APP_SYNC_REQUEST);
            socket_adapter.setSocket(&socket);
            const char *connectAddress = par("connectAddress");
            EV_DEBUG << "now connect sync up" << std::endl;
            sync_service.forwardConnectSync(&socket_adapter, connectAddress);
            EV_DEBUG << "now send sync up" << std::endl;
            sync_service.forwardSyncRequestUp(&socket_adapter, nodes, connection_map, connectAddress, received_sync_request_from);
        }
        if(msg->getKind() == SELF_MSGKIND_RETURN_SYNC_DOWN){
            EV_DEBUG << "now return down" << std::endl;
            isReceiving = false;
            
            int syncPort = par("syncReturnPort");
            socket.renewSocket();
            setupSocket(&socket, syncPort);

            EV_DEBUG << "address:" << received_sync_request_from << std::endl;
            EV_DEBUG << "address:" << down_connect_socket_id << std::endl;

            socket_adapter.setMsgKind(APP_SYNC_RETURN);
            socket_adapter.setSocket(&socket);
            socket_adapter.setParentComponent(this);
            const std::string &address = socket.getLocalAddress().str();
            EV_DEBUG << "localAddress:" << address << std::endl;
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
        EV_DEBUG << msg->str() << std::endl;
        EV_DEBUG << "message when up" << std::endl;
        if(msg->isSelfMessage()){
            EV_DEBUG << "self message, kind:  " << msg->getKind() << std::endl;
            handleTimer(msg);
        } else if (socket.belongsToSocket(msg)) {
            socket.processMessage(msg);
        } else {
            EV_DEBUG << "message arrived on else" << std::endl;
            try {
                if(inet::check_and_cast<inet::Packet*>(msg)){
                    EV_DEBUG << "is packet" << std::endl;
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
            EV_DEBUG << "Socket available receiving" << std::endl;
            EV_DEBUG << "remote addr: " + availableInfo->getRemoteAddr().str() << std::endl;
            EV_DEBUG << "connect addr: " + par("connectAddress").stdstringValue() << std::endl;

            auto newSocket = new inet::TcpSocket(availableInfo); // create socket using received info
            socketMap.addSocket(newSocket); // store accepted connection

            if(availableInfo->getRemotePort() == 5555){
                EV_DEBUG << "set down connect socket" << std::endl;
                down_connect_socket_id = newSocket->getSocketId();
            } else if(availableInfo->getRemotePort() == 5556){
                EV_DEBUG << "set down sync socket" << std::endl;
                down_sync_socket_id = newSocket->getSocketId();
                received_sync_request_from = availableInfo->getRemoteAddr().str();
            } else if(availableInfo->getRemotePort() == 5557) {
                EV_DEBUG << "set up sync socket" << std::endl;
                up_sync_socket_id = newSocket->getSocketId();
            } else {

            }

            this->socket.accept(availableInfo->getNewSocketId());

            //receiveIncomingMessages(socket, availableInfo);
            //auto newSocket = new inet::TcpSocket(availableInfo);
        } else {
            EV_DEBUG << "Socket available requesting" << std::endl;
            TcpAppBase::socketAvailable(socket, availableInfo);
        }
    }

    void VotingApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) {
        EV_DEBUG << "socket data arrived: " << this->getFullPath() << std::endl;
        const inet::Ptr<const inet::BytesChunk> &intrusivePtr = msg->peekData<inet::BytesChunk>();
        uint8_t appMsgKind = intrusivePtr->getByte(0);
        EV_DEBUG << std::to_string(appMsgKind) << std::endl;

        const inet::Ptr<const inet::BytesChunk> &ptr = msg->peekData<inet::BytesChunk>();

        std::string content_str;
        std::transform(ptr->getBytes().begin() + 1,ptr->getBytes().end(),std::back_inserter(content_str),[](uint8_t d){
            return (char) d;
        });

        EV_DEBUG << content_str << std::endl;

        switch(appMsgKind){
            case APP_CONN_REPLY: {
                EV_DEBUG << "received app conn reply" << std::endl;
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
            {
                EV_DEBUG << "received connection request" << std::endl;
                auto *down_connect_socket = inet::check_and_cast<inet::TcpSocket *>(socketMap.getSocketById(down_connect_socket_id));
                try{
                    socket_adapter.setSocket(down_connect_socket);
                    socket_adapter.addProgrammedMessage(socketMessage{content_str, down_connect_socket->getRemoteAddress().str()});
                    EV_DEBUG << "set socket" << std::endl;

                    if(connection_service.receiveConnectionRequest(socket_adapter, content_str, nodes, connection_map, down_connect_socket->getLocalAddress().str()) == 0){
                        EV_DEBUG << "create accept packet" << std::endl;
                        socket->setState(inet::TcpSocket::CONNECTED);
                        inet::Packet *pPacket = createDataPacket("accept");
                        down_connect_socket->setOutputGate(gate("socketOut"));
                        down_connect_socket->send(pPacket);
                    } else {
                        socket->send(createDataPacket("reject"));
                    }
                    writeStateToFile("connection/peers_after_data_send_" + getFullPath().substr(0,19) + ".json");
                } catch(std::exception ex) {
                    EV_DEBUG << "Could not send" << std::endl;
                }
            }
            break;
            case APP_SYNC_REQUEST: {
                EV_DEBUG << "received sync request" << std::endl;
                auto *down_sync_socket = inet::check_and_cast<inet::TcpSocket *>(socketMap.getSocketById(down_sync_socket_id));
                EV_DEBUG << "got socket" << std::endl;
                EV_DEBUG << content_str << std::endl;
                socket->setState(inet::TcpSocket::CONNECTED);
                EV_DEBUG << "has set state to connected" << std::endl;
                down_sync_socket->setOutputGate(gate("socketOut"));
                EV_DEBUG << "has set output gate" << std::endl;
                socket_adapter.setSocket(down_sync_socket);

                EV_DEBUG << down_sync_socket->getRemoteAddress().str() << std::endl;

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
                EV_DEBUG << "received sync return" << std::endl;
                auto *up_sync_socket = inet::check_and_cast<inet::TcpSocket *>(socketMap.getSocketById(up_sync_socket_id));
                EV_DEBUG << content_str << std::endl;
                up_sync_socket->setOutputGate(gate("socketOut"));
                socket_adapter.setSocket(up_sync_socket);

                EV_DEBUG << up_sync_socket->getRemoteAddress().str() << std::endl;

                socket_adapter.addProgrammedMessage(socketMessage{content_str,socket->getRemoteAddress().str()});
                sync_service.receiveSyncRequest(socket_adapter,nodes, connection_map);
                socket_adapter.setMsgKind(APP_SYNC_REPLY);
                if(doesConnect){
                    sync_service.sendSyncReply(&socket_adapter);
                }
            }
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

    void VotingApp::setupSocket(inet::TcpSocket* socket, int port) {
        const char *localAddress = par("localAddress");
        socket->bind(inet::Ipv4Address(localAddress), port);
        socket->setCallback(this);
        socket->setOutputGate(gate("socketOut"));
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

    inet::Packet* VotingApp::createDataPacket(std::string send_string) {
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