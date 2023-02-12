//
// Created by wld on 27.11.22.
//

#include "VotingApp.h"
#include <algorithm>
#include "inet/transportlayer/tcp/Tcp.h"
#include "inet/common/INETUtils.h"
#include "inet/applications/common/SocketTag_m.h"
#include "nlohmann/json.hpp"
#include "evoting/electionBuilder.h"
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

            pauseBeforePublish = par("pauseBeforePublish").doubleValue();
            pauseBeforeForwardPortsRequest = par("pauseBeforeForwardPorts").doubleValue();

            connection_service.importPeersList(nodes, "./");
            connection_service.importPeerConnections(connection_map, "./");
        }
    }

    void VotingApp::handleStartOperation(inet::LifecycleOperation *) {
        // Export state at beginning
        writeStateToFile("voting/peers_at_start_" + this->getFullPath().substr(0,19) + ".json");

        // Schedule events
        double tCreateElection = par("tCreateElection").doubleValue();
        double tPlaceVote = par("tPlaceVote").doubleValue();
        double tReceive3PRequest = par("tThreePReceive").doubleValue();
        EV_DEBUG << "place vote on: " << tPlaceVote << std::endl;
        //double tInitialDistribute = par("tInitialDistribute").doubleValue();

        isReceiving = par("isReceivingAtStart").boolValue();

        if (inet::simTime() <= tCreateElection) {
            createElectionSelfMessage = new inet::cMessage("timer");
            createElectionSelfMessage->setKind(SELF_MSGKIND_CREATE_ELECTION);
            scheduleAt(tCreateElection, createElectionSelfMessage);
        }
        if (inet::simTime() <= tPlaceVote) {
            placeVoteSelfMessage = new inet::cMessage("timer");
            placeVoteSelfMessage->setKind(SELF_MSGKIND_PLACE_VOTE);
            scheduleAt(tPlaceVote, placeVoteSelfMessage);
        }
        if (inet::simTime() <= tReceive3PRequest) {
            receive3PReqestSelfMessage = new inet::cMessage("timer");
            receive3PReqestSelfMessage->setKind(SELF_MSGKIND_DISTR_3P_RECEIVE);
            scheduleAt(tReceive3PRequest, receive3PReqestSelfMessage);
        }
    }

    void VotingApp::handleTimer(inet::cMessage *msg) {
        if(msg->getKind() == SELF_MSGKIND_CREATE_ELECTION) {
            electionBuilder builder{1};
            
            std::map<size_t, std::string> voteOptions;
            voteOptions[0] = "a";
            voteOptions[1] = "b";
            voteOptions[2] = "c";

            const std::string &localAddress = par("localAddress").stdstringValue();

            election el = builder
                    .withSequenceNumber(1)
                    .withVoteOptions(voteOptions);

            election_box.push_back(el);
        }
        if(msg->getKind() == SELF_MSGKIND_PLACE_VOTE){
            isInitializingDirectionDistribution = true;
            if(!election_box.empty()){
                isReceiving = false;
                election &election = election_box.front();
                const std::string &localAddress = par("localAddress").stdstringValue();

                election.prepareForDistribtion(nodes);
                election.placeVote(localAddress, "1");

                position = distribution_service.calculatePosition(connection_map, localAddress);

                distribution_service.getDistributionParams(connection_map,localAddress,address_up, address_down);

                publish_port = 5050;
                if(!address_up.empty()) {
                    socket_up_adapter.setSocket(&socket);
                    socket_up_adapter.setMsgKind(APP_DISTR_3P_REQUEST);
                    socket_up_adapter.setParentComponent(this);
                }
                if(!address_down.empty()) {
                    socket_down_adapter.setSocket(&socket);
                    socket_down_adapter.setMsgKind(APP_DISTR_3P_REQUEST);
                    socket_down_adapter.setParentComponent(this);
                }

                distribution_service.sendInitial3PRequest(&socket_up_adapter, &socket_down_adapter,localAddress,position, address_up, address_down);
            }
            //place vote on election
            //setup distribution
            //send towards extreme
            //send back
        }
        if(msg->getKind() == SELF_MSGKIND_DISTR_3P_RECEIVE) {
            std::string localAddress = par("localAddress").stdstringValue();
            isReceiving = true;
            socket.renewSocket();
            socket_down_adapter.setParentComponent(this);
            socket_down_adapter.setSocket(&socket);
            socket_down_adapter.setupSocket(localAddress, 5049);

            connection_service.changeToListenState(socket_down_adapter);
        }
        if(msg->getKind() == SELF_MSGKIND_DISTR_HOPS_SEND) {
            std::string localAddress = par("localAddress").stdstringValue();
            inet::cMsgPar directionPar = msg->par("direction");
            std::string direction = directionPar.stringValue();
            if(direction.find("up") != std::string::npos){
                socket_up_adapter.setSocket(&socket);
                socket_up_adapter.setMsgKind(APP_DISTR_PRE_PUBLISH_HOPS_REQUEST);
                socket_up_adapter.setParentComponent(this);
                distribution_service.sendDirectionRequestNumberOfHops(&socket_up_adapter, current_hops);
            } else if (direction.find("down") != std::string::npos) {
                socket_down_adapter.setSocket(&socket);
                socket_down_adapter.setMsgKind(APP_DISTR_PRE_PUBLISH_HOPS_REQUEST);
                socket_down_adapter.setParentComponent(this);
                distribution_service.sendDirectionRequestNumberOfHops(&socket_down_adapter, current_hops);
            }
        }
        if(msg->getKind() == SELF_MSGKIND_DISTR_3P_FORWARD) {
            isReceiving = false;
            inet::cMsgPar contentPar = msg->par("content");
            std::string content_str = contentPar.stringValue();
            nlohmann::json data = nlohmann::json::parse(content_str);

            std::string localAddress = par("localAddress").stdstringValue();
            socket.renewSocket();
            socket_up_adapter.setSocket(&socket);
            socket_up_adapter.setMsgKind(APP_DISTR_3P_REQUEST);
            socket_up_adapter.setParentComponent(this);
            socket_up_adapter.setupSocket(localAddress, 5049);

            distribution_service.getDistributionParams(connection_map,localAddress,address_up, address_down);
            position = distribution_service.calculatePosition(connection_map, localAddress);
            distribution_service.sendDirectionRequest(&socket_up_adapter,data, position, address_down, address_up);

        }
        if(msg->getKind() == SELF_MSGKIND_DISTR_SUBSCRIBE) {
            EV_DEBUG << "now subscribing" << std::endl;
            std::string localAddress = par("localAddress").stdstringValue();
            isReceiving = true;
            subscribe_socket->renewSocket();
            socket_down_adapter.setParentComponent(this);
            socket_down_adapter.setSocket(subscribe_socket);
            socket_down_adapter.setupSocket(localAddress, subscribe_port);
            connection_service.changeToListenState(socket_down_adapter);
            EV_DEBUG << "now listenning" << std::endl;
        }
        if(msg->getKind() == SELF_MSGKIND_DISTR_PUBLISH) {
            EV_DEBUG << "now publish" << std::endl;
            EV_DEBUG << "election: " << election_box[0] << std::endl;
            publish_socket->renewSocket();
            //publish_socket->setTCPAlgorithmClass("TcpNoCongestionControl");

            if(sendTowards.find("up") != std::string::npos) {
                std::string localAddress = par("localAddress").stdstringValue();
                socket_up_adapter.setParentComponent(this);
                socket_up_adapter.setSocket(publish_socket);
                socket_up_adapter.setupSocket(localAddress, publish_port);
                socket_up_adapter.setMsgKind(APP_DISTR_PUBLISH);
                publish_socket->connect(inet::Ipv4Address(address_up.c_str()), publish_port);
                distribution_service.sendElection(&socket_up_adapter, election_box[0], publish_port);
            } else if(sendTowards.find("down") != std::string::npos) {
                socket_down_adapter.setMsgKind(APP_DISTR_PUBLISH);
                distribution_service.sendElection(&socket_down_adapter, election_box[0], publish_port);
            }
            isInitializingDirectionDistribution = false;
        }
        if(msg->getKind() == SELF_MSGKIND_DISTR_INITIAL_DIRECTION_SEND) {
            std::string localAddress = par("localAddress").stdstringValue();
            if(!address_up.empty()){
                socket_up_adapter.setSocket(&socket);
                socket_up_adapter.setParentComponent(this);
                socket_up_adapter.setMsgKind(APP_DISTR_PRE_PUBLISH_DIRECTION_REQUEST);
            }

            if(!address_down.empty()) {
                socket_down_adapter.setParentComponent(this);
                socket_down_adapter.setMsgKind(APP_DISTR_PRE_PUBLISH_DIRECTION_REQUEST);
                inet::TcpSocket *socket = dynamic_cast<inet::TcpSocket *>(socketMap.getSocketById(down_socket_id));
                socket_down_adapter.setSocket(socket);
            }

            sendTowards = distribution_service.sendInitialDistributionRequestDirection(&socket_up_adapter,
                                                                         &socket_down_adapter,
                                                                         address_up,
                                                                         address_down,
                                                                         nodes.size(), position);
        }
        if(msg->getKind() == SELF_MSGKIND_DISTR_DIRECTION_FORWARD) {
            std::string localAddress = par("localAddress").stdstringValue();

            inet::cMsgPar directionPar = msg->par("receivedFrom");
            std::string directionFrom = directionPar.stringValue();

            EV_DEBUG << "direction: " << directionFrom << std::endl;

            //TODO: Figure out direction here
            if (directionFrom.find("down") != -1) {
                EV_DEBUG << "Socketstate: " << socket.stateName(socket.getState()) << std::endl;
                socket_up_adapter.setSocket(&socket);
                socket_up_adapter.setParentComponent(this);
                socket_up_adapter.setMsgKind(APP_DISTR_PRE_PUBLISH_DIRECTION_REQUEST);
            }

            if (directionFrom.find("up") != -1) {
                socket_down_adapter.setParentComponent(this);
                socket_down_adapter.setMsgKind(APP_DISTR_PRE_PUBLISH_DIRECTION_REQUEST);
                inet::TcpSocket *socket = dynamic_cast<inet::TcpSocket *>(socketMap.getSocketById(down_socket_id));
                socket_down_adapter.setSocket(socket);
            }

            EV_DEBUG << "down: " << address_down << ", up: " << address_up << std::endl;
            EV_DEBUG << "Socketstate: " << socket.stateName(socket.getState()) << std::endl;
            EV_DEBUG << "nodes size: " << nodes.size() << ", position: " << position << std::endl;
            EV_DEBUG << "direction from: " << directionFrom << std::endl;
            sendTowards = distribution_service.sendForwardistributionRequestDirection(&socket_up_adapter,
                                                                                       &socket_down_adapter,
                                                                                       address_up,
                                                                                       address_down,
                                                                                       current_hops,
                                                                                       nodes.size(),
                                                                                       position,
                                                                                       directionFrom);

            EV_DEBUG << sendTowards << std::endl;
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
        EV_DEBUG << "msg up" << std::endl;
        EV_DEBUG << msg->str() << std::endl;
        if(msg->isSelfMessage()){
            handleTimer(msg);
        } else if (socket.belongsToSocket(msg)) {
            inet::TagSet &set = inet::getTags(msg);
            inet::SocketInd *pInd = set.findTag<inet::SocketInd>();
            EV_DEBUG << "ind: " << pInd->getSocketId() << std::endl;
            EV_DEBUG << "connId: " << socket.getSocketId() << std::endl;
            socket.processMessage(msg);
        } else if (subscribe_socket->belongsToSocket(msg)) {
            EV_DEBUG << "subscribe handle up" << std::endl;
            subscribe_socket->processMessage(msg);
        } else {
            try {
                inet::Packet *pPacket = inet::check_and_cast<inet::Packet *>(msg);
                socketDataArrived(&socket, pPacket, false);
                return;
            } catch (std::exception& ex){
                EV_DEBUG << "cannot cast to packet" << std::endl;
                EV_DEBUG << ex.what() << std::endl;
            }
            try {
                inet::Indication *pIndication = inet::check_and_cast<inet::Indication *>(msg);
                EV_DEBUG << "is indication" << std::endl;
                EV_DEBUG << pIndication->str() << std::endl;
                
                try {
                    inet::TcpAvailableInfo *pInfo = inet::check_and_cast<inet::TcpAvailableInfo *>(
                            pIndication->getControlInfo());
                    EV_DEBUG << "remote: " << pInfo->getRemoteAddr() << ":" << pInfo->getRemotePort() << std::endl;
                    EV_DEBUG << "local: " << pInfo->getLocalAddr() << ":"<< pInfo->getLocalPort() << std::endl;
                    EV_DEBUG << "user id: " << pInfo->getUserId() << std::endl;
                    EV_DEBUG << "new socket id: " << pInfo->getNewSocketId() << std::endl;

                    inet::TagSet &set = inet::getTags(msg);
                    inet::SocketInd *pInd = set.findTag<inet::SocketInd>();
                    EV_DEBUG << "ind: " << pInd->getSocketId() << std::endl;
                    EV_DEBUG << "connId: " << socket.getSocketId() << std::endl;

                    std::for_each(socketMap.getMap().begin(), socketMap.getMap().end(), [this](std::pair<int, inet::ISocket*> idSocketPair){
                        inet::TcpSocket *pSocket = inet::check_and_cast<inet::TcpSocket *>(idSocketPair.second);
                        EV_DEBUG << idSocketPair.first << ": " << pSocket->getLocalAddress() << ":" << pSocket->getLocalPort()
                            << "-" << pSocket->getRemoteAddress() << ":" << pSocket->getRemotePort() << ", open=" <<  pSocket->isOpen() << ")" << std::endl;
                        
                    });


                    //inet::ISocket *pSocket = socketMap.findSocketFor(msg);
                    //EV_DEBUG << "socket id: " << pSocket->getSocketId() << std::endl;
                    return;
                } catch(std::exception& ex) {
                    EV_DEBUG << "cannot cast to available info" << std::endl;
                }
                try {
                    inet::TcpConnectInfo *pInfo = inet::check_and_cast<inet::TcpConnectInfo *>(
                            pIndication->getControlInfo());
                    EV_DEBUG << "remote port: " << pInfo->getRemotePort() << std::endl;
                    EV_DEBUG << "local port: " << pInfo->getLocalPort() << std::endl;
                    EV_DEBUG << "user id: " << pInfo->getUserId() << std::endl;
                    return;
                } catch(std::exception& ex) {
                    EV_DEBUG << "cannot cast to connect info" << std::endl;
                }

            } catch (std::exception& ex){
                EV_DEBUG << "cannot cast to indication" << std::endl;
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
        EV_DEBUG << "user id: " << availableInfo->getUserId() << std::endl;
        EV_DEBUG << "remote port: " << availableInfo->getRemotePort() << std::endl;
        EV_DEBUG << "local port: " << availableInfo->getRemotePort() << std::endl;
        EV_DEBUG << "is receiving: " << isReceiving << std::endl;
        auto newSocket = new inet::TcpSocket(availableInfo); // create socket using received info
        socketMap.addSocket(newSocket); // store accepted connection
        if(socket->getLocalPort() == 5049){
            down_socket_id = availableInfo->getNewSocketId();
        }
        this->socket.accept(availableInfo->getNewSocketId());
    }

    void VotingApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) {
        const inet::Ptr<const inet::BytesChunk> &intrusivePtr = msg->peekData<inet::BytesChunk>();
        uint8_t appMsgKind = intrusivePtr->getByte(0);
        EV_DEBUG << "Message kind arrived: " << std::to_string(appMsgKind) << std::endl;

        const inet::Ptr<const inet::BytesChunk> &ptr = msg->peekData<inet::BytesChunk>();
        EV_DEBUG << "as bytes: ";
        std::string content_str;
        std::transform(ptr->getBytes().begin() + 1,ptr->getBytes().end(),std::back_inserter(content_str),[this](uint8_t d){
            EV_DEBUG << std::to_string(d) << " ";
            return (char) d;
        });
        EV_DEBUG << std::endl;
        EV_DEBUG << "as string: " << content_str << std::endl;


        switch(appMsgKind){
            case APP_DISTR_3P_REQUEST: {
                auto *down_socket = inet::check_and_cast<inet::TcpSocket *>(socketMap.getSocketById(down_socket_id));
                down_socket->setOutputGate(gate("socketOut"));
                socket_down_adapter.setSocket(down_socket);
                socket_down_adapter.setMsgKind(APP_DISTR_3P_RESPONSE);
                socket_down_adapter.setParentComponent(this);
                distribution_service.sendSuccessResponse(&socket_down_adapter);
                
                nlohmann::json setup_info = nlohmann::json::parse(content_str);
                subscribe_port = setup_info["origin_publish_port"];
                publish_port = setup_info["origin_publish_port"].get<int>() % 5050 == 0 ? 5051 : 5050;


                forwardDirectionRequestSelfMessage = new inet::cMessage("timer");
                received3PData = new omnetpp::cMsgPar("content");
                received3PData->setStringValue(content_str.c_str());
                forwardDirectionRequestSelfMessage->addPar(received3PData);
                forwardDirectionRequestSelfMessage->setKind(SELF_MSGKIND_DISTR_3P_FORWARD);
                scheduleAt(inet::simTime() + pauseBeforeForwardPortsRequest, forwardDirectionRequestSelfMessage);
            }
                break;
            case APP_DISTR_3P_RESPONSE:
            {
                if(isInitializingDirectionDistribution) {
                    if(content_str.find("accept") != -1) {
                        hopsSelfMessage = new inet::cMessage("timer");
                        nextDirection = new omnetpp::cMsgPar("direction");
                        nextDirection->setStringValue("up");
                        hopsSelfMessage->addPar(nextDirection);
                        hopsSelfMessage->setKind(SELF_MSGKIND_DISTR_HOPS_SEND);
                        scheduleAt(inet::simTime() + 0.1, hopsSelfMessage);
                    }
                }
                break;
            }
            case APP_DISTR_PRE_PUBLISH_HOPS_REQUEST:
            {
                auto *down_socket = inet::check_and_cast<inet::TcpSocket *>(socketMap.getSocketById(down_socket_id));
                current_hops = std::stoi(content_str);
                current_hops++;
                down_socket->setOutputGate(gate("socketOut"));
                socket_down_adapter.setSocket(down_socket);
                socket_down_adapter.setMsgKind(APP_DISTR_PRE_PUBLISH_HOPS_RESPONSE);
                socket_down_adapter.setParentComponent(this);
                distribution_service.sendSuccessResponse(&socket_down_adapter);
            }
                break;
            case APP_DISTR_PRE_PUBLISH_DIRECTION_REQUEST:
            {
                EV_DEBUG << "received publish direction request " << std::endl;
                auto *down_socket = inet::check_and_cast<inet::TcpSocket *>(socketMap.getSocketById(down_socket_id));
                received_direction = content_str;
                down_socket->setOutputGate(gate("socketOut"));
                socket_down_adapter.setSocket(down_socket);
                socket_down_adapter.setMsgKind(APP_DISTR_PRE_PUBLISH_DIRECTION_RESPONSE);
                socket_down_adapter.setParentComponent(this);
                distribution_service.sendSuccessResponse(&socket_down_adapter);

                socket_down_adapter.close();

                subscribeSelfMessage = new inet::cMessage("timer");
                subscribeSelfMessage->setKind(SELF_MSGKIND_DISTR_SUBSCRIBE);
                scheduleAt(inet::simTime(), subscribeSelfMessage);

                if(!address_up.empty()) {
                    socket_up_adapter.setSocket(socket);
                    socket_up_adapter.setMsgKind(APP_DISTR_PRE_PUBLISH_DIRECTION_REQUEST);
                    socket_up_adapter.setParentComponent(this);
                }
            }
                break;
            case APP_DISTR_PRE_PUBLISH_HOPS_RESPONSE:
            {
                if(content_str.find("accept") != -1) {
                    directionSelfMessage = new inet::cMessage("timer");
                    nextDirection = new omnetpp::cMsgPar("receivedFrom");
                    nextDirection->setStringValue("down");
                    directionSelfMessage->addPar(nextDirection);
                    if(isInitializingDirectionDistribution) {
                        directionSelfMessage->setKind(SELF_MSGKIND_DISTR_INITIAL_DIRECTION_SEND);
                    } else {
                        directionSelfMessage->setKind(SELF_MSGKIND_DISTR_DIRECTION_FORWARD);
                    }
                    scheduleAt(inet::simTime() + 0.1, directionSelfMessage);
                }
            }
            break;
            case APP_DISTR_PRE_PUBLISH_DIRECTION_RESPONSE:
            {
                this->socket_no_direction_adapter.setParentComponent(this);
                this->socket_no_direction_adapter.setSocket(socket);
                this->socket_no_direction_adapter.close();

                if(content_str.find("accept") != -1) {
                    publishSelfMessage = new inet::cMessage("timer");
                    publishSelfMessage->setKind(SELF_MSGKIND_DISTR_PUBLISH);
                    scheduleAt(inet::simTime() + pauseBeforePublish, publishSelfMessage);
                }
                this->socket.renewSocket();
                this->socket.bind(5049);
                this->socket.listen();
                isReceiving = true;
            }
            break;
            case APP_DISTR_PUBLISH:
            {
                inetSocketAdapter& socket_adapter = socket_up_adapter;

                char vt = 11;
                std::stringstream test(content_str);
                std::string segment;
                std::vector<std::string> seglist;

                while(std::getline(test, segment, vt))
                {
                    socket_adapter.addProgrammedMessage(socketMessage{segment, socket->getRemoteAddress().str()});
                    seglist.push_back(segment);
                }
                election el = distribution_service.receiveElection(&socket_adapter);

                election_box.push_back(el);

                if (current_hops < nodes.size() - 1) {
                    hopsSelfMessage = new inet::cMessage("timer");
                    nextDirection = new omnetpp::cMsgPar("direction");
                    nextDirection->setStringValue("up");
                    hopsSelfMessage->addPar(nextDirection);
                    hopsSelfMessage->setKind(SELF_MSGKIND_DISTR_HOPS_SEND);
                    scheduleAt(inet::simTime() + 0.1, hopsSelfMessage);
                }
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
}