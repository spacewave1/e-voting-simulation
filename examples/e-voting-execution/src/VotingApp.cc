//
// Created by wld on 27.11.22.
//

#include "VotingApp.h"
#include <algorithm>
#include "inet/transportlayer/tcp/Tcp.h"
#include "inet/common/INETUtils.h"
#include "inet/applications/common/SocketTag_m.h"
#include "nlohmann/json.hpp"
#include "evoting/model/electionBuilder.h"
#include "../../../src/PacketsKind.h"

namespace voting {
    Define_Module(VotingApp);

    void VotingApp::initialize(int stage) {
        tally_service = new tallyService(hill_encryption_service);
        election_service = new electionService(hill_encryption_service);

        ApplicationBase::initialize(stage);

        if (stage == inet::INITSTAGE_LOCAL) {
            numSessions = numBroken = packetsSent = packetsRcvd = bytesSent = bytesRcvd = 0;

            WATCH(numSessions);
            WATCH(numBroken);
            WATCH(packetsSent);
            WATCH(packetsRcvd);
            WATCH(bytesSent);
            WATCH(bytesRcvd);

            local_address = par("localAddress").stdstringValue();

            pauseBeforePublish = par("pauseBeforePublish").doubleValue();
            pauseBeforeForwardPortsRequest = par("pauseBeforeForwardPorts").doubleValue();

            connection_service.importPeersList(nodes, "./");
            connection_service.importPeerConnections(connection_map, "./");

            if (!connection_map[local_address].empty()) {
                EV_DEBUG << "has address up" << std::endl;
                address_up = connection_map[local_address];
            }
            std::map<std::string, std::string> reversed_connection_map = distribution_service.getReversedConnectionTable(
                    connection_map);
            if (!reversed_connection_map[local_address].empty()) {
                EV_DEBUG << "has address down" << std::endl;
                address_down = reversed_connection_map[local_address];
            }
            position = distribution_service.calculatePosition(connection_map, local_address);

            distribution_service.getDistributionParams(connection_map, local_address, address_up, address_down);
        }
    }

    void VotingApp::handleStartOperation(inet::LifecycleOperation *) {
        // Export state at beginning
        writeStateToFile("voting/peers_at_start_" + this->getFullPath().substr(0, 19) + ".json");

        // Schedule events
        double tCreateElection = par("tCreateElection").doubleValue();
        double tPlaceVote = par("tPlaceVote").doubleValue();
        double tReceive3PRequest = par("tThreePReceive").doubleValue();
        double tConfirmVoteAt = par("tConfirmVoteAt").doubleValue();
        double tRequestKeysAt = par("tRequestKeys").doubleValue();
        double tTallyAt = par("tTallyAt").doubleValue();
        EV_DEBUG << "place vote on: " << tPlaceVote << std::endl;

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
            receive3PReqestSelfMessage->setKind(SELF_MSGKIND_DISTR_PORTS_SETUP_RECEIVE);
            scheduleAt(tReceive3PRequest, receive3PReqestSelfMessage);
        }
        if (inet::simTime() <= tConfirmVoteAt) {
            tConfirmAtMessage = new inet::cMessage("timer");
            tConfirmAtMessage->setKind(SELF_MSGKIND_CONFIRM);
            scheduleAt(tConfirmVoteAt, tConfirmAtMessage);
        }
        if (inet::simTime() <= tRequestKeysAt) {
            tRequestKeysAtMessage = new inet::cMessage("timer");
            tRequestKeysAtMessage->setKind(SELF_MSGKIND_REQUEST_KEYS);
            scheduleAt(tRequestKeysAt, tRequestKeysAtMessage);
        }
        if (inet::simTime() <= tTallyAt) {
            tTallyAtMessage = new inet::cMessage("timer");
            tTallyAtMessage->setKind(SELF_MSGKIND_TALLY);
            scheduleAt(tTallyAt, tTallyAtMessage);
        }
    }

    void VotingApp::initElectionDistribution(election& election) {
        current_hops = 0;
        publish_port = 5050;
        subscribe_port = 5051;

        if (!address_up.empty()) {
            socket_up_adapter.setSocket(request_up_socket);
            socket_up_adapter.setMsgKind(APP_DISTR_PORTS_SETUP_REQUEST);
            socket_up_adapter.setParentComponent(this);
        }
        if (!address_down.empty()) {
            socket_down_adapter.setSocket(request_down_socket);
            socket_down_adapter.setMsgKind(APP_DISTR_PORTS_SETUP_REQUEST);
            socket_down_adapter.setParentComponent(this);
        }
        distribution_service.sendInitialPortsSetupRequest(&socket_up_adapter, &socket_down_adapter,
                                                          local_address, position, address_up, address_down);
    }

    void VotingApp::handleTimer(inet::cMessage *msg) {
        if (msg->getKind() == SELF_MSGKIND_CREATE_ELECTION) {
            EV_DEBUG << "create election" << std::endl;
            electionBuilder builder{1};

            std::map<size_t, std::string> voteOptions;
            voteOptions[0] = "a";
            voteOptions[1] = "b";
            voteOptions[2] = "c";

            election el = builder
                    .withSequenceNumber(1)
                    .withVoteOptions(voteOptions);

            election_box.push_back(el);
        }
        if (msg->getKind() == SELF_MSGKIND_PLACE_VOTE) {
            EV_DEBUG << "place vote" << std::endl;
            isInitializingDirectionDistribution = true;
            if (!election_box.empty()) {
                isReceiving = false;
                election &election = election_box.front();

                if (!election.isPreparedForDistribution()) {
                    election.prepareForDistribtion(nodes);
                }
                bool foundKey = false;

                std::srand(std::time(0)); //use current time as seed for random generator
                int random_variable = std::rand();
                int selected_option = random_variable % election.getOptions().size();

                while(!foundKey) {
                    foundKey = election_service->placeEncryptedVote(std::to_string(selected_option), election, own_election_keys, local_address);
                }
                initElectionDistribution(election);
            }
            //place vote on election
            //setup distribution
            //send towards extreme
            //send back
        }
        if (msg->getKind() == SELF_MSGKIND_REQUEST_KEY) {
            EV_DEBUG << "send election id to request key" << std::endl;
            std::string request_address = msg->par("request_address").stringValue();
            request_keys_socket.setSocket(request_key_socket);
            request_keys_socket.setParentComponent(this);
            request_keys_socket.setupSocket(local_address, 50061);
            request_keys_socket.setMsgKind(APP_REQUEST_KEY);
            tally_service->requestKey(request_keys_socket, election_box[0], local_address, request_address);
        }
        if (msg->getKind() == SELF_MSGKIND_TALLY) {
            EV_DEBUG << "now tally" << std::endl;
            EV_DEBUG << election_box[0] << std::endl;

            std::for_each(received_election_keys[election_box[0].getId()].begin(), received_election_keys[election_box[0].getId()].end(), [this](std::string received_key){
                EV_DEBUG << "key: " << received_key << std::endl;
            });

            election_service->decryptWithKeys(election_box[0], local_address, received_election_keys[election_box[0].getId()]);
            isInitializingDirectionDistribution = true;
            isReceiving = false;
            initElectionDistribution(election_box[0]);
        }
        if (msg->getKind() == SELF_MSGKIND_REQUEST_KEYS) {
            election &election = election_box[0];
            const std::vector<std::string> &addresses_without_self = tally_service->findGroupAndFilterOwnIdentity(
                    election, local_address);

            float timeAdd = 0.0;
            std::for_each(addresses_without_self.begin(), addresses_without_self.end(), [this, &timeAdd](std::string address){
                // TODO: What happens with this, is this memory leak??
                inet::cMessage *pMessage = new inet::cMessage();
                omnetpp::cMsgPar *pPar = new omnetpp::cMsgPar("request_address");
                pPar->setStringValue(address.c_str());
                pMessage->addPar(pPar);
                pMessage->setKind(SELF_MSGKIND_REQUEST_KEY);

                tKeyMessages.push_back(pMessage);
                addressPars.push_back(pPar);
                scheduleAt(inet::simTime() + timeAdd, pMessage);
                timeAdd += 0.3;
            });
        }
        if (msg->getKind() == SELF_MSGKIND_CONFIRM) {
            election &election = election_box[0];
            tally_service->prepareTally(election,
                                        is_evaluated_votes_map,
                                        local_address,
                                        own_election_keys,
                                        election_keys_to_send
                                       );
            isInitializingDirectionDistribution = true;
            isReceiving = false;
            initElectionDistribution(election);

            // Setup reply key listen
            socket_no_direction_adapter.setSocket(&reply_key_socket);
            socket_no_direction_adapter.setParentComponent(this);
            socket_no_direction_adapter.setupSocket(local_address, 50061);
            connection_service.changeToListenState(socket_no_direction_adapter);
        }
        if (msg->getKind() == SELF_MSGKIND_DISTR_PORTS_SETUP_RECEIVE) {
            isReceiving = true;
            socket_no_direction_adapter.setParentComponent(this);
            socket_no_direction_adapter.setSocket(&listen_socket);
            socket_no_direction_adapter.setupSocket(local_address, 5049);
            connection_service.changeToListenState(socket_no_direction_adapter);
        }
        if (msg->getKind() == SELF_MSGKIND_DISTR_HOPS_SEND) {
            std::string direction = nextDirection->stringValue();
            if (direction.find("up") != std::string::npos) {
                socket_up_adapter.setSocket(request_up_socket);
                socket_up_adapter.setMsgKind(APP_DISTR_PRE_PUBLISH_HOPS_REQUEST);
                socket_up_adapter.setParentComponent(this);
                if (request_up_socket->getState() != inet::TcpSocket::CONNECTED) {
                    socket_up_adapter.connect("tcp", address_up, 5049);
                }
                distribution_service.sendDirectionRequestNumberOfHops(&socket_up_adapter, current_hops);
            } else if (direction.find("down") != std::string::npos) {
                socket_down_adapter.setSocket(request_down_socket);
                socket_down_adapter.setMsgKind(APP_DISTR_PRE_PUBLISH_HOPS_REQUEST);
                socket_down_adapter.setParentComponent(this);
                if (request_down_socket->getState() != inet::TcpSocket::CONNECTED) {
                    socket_down_adapter.connect("tcp", address_down, 5049);
                }
                distribution_service.sendDirectionRequestNumberOfHops(&socket_down_adapter, current_hops);
            }
        }
        if (msg->getKind() == SELF_MSGKIND_DISTR_PORTS_SETUP_FORWARD) {
            isReceiving = false;
            inet::cMsgPar contentPar = msg->par("content");
            std::string content_str = contentPar.stringValue();

            nlohmann::json data = nlohmann::json::parse(content_str);
            distribution_service.getDistributionParams(connection_map, local_address, address_up, address_down);
            position = distribution_service.calculatePosition(connection_map, local_address);
            size_t originPosition = std::stoi(data["originPosition"].dump());

            if (!address_up.empty() && originPosition > position) {
                request_up_socket->renewSocket();
                socket_up_adapter.setSocket(request_up_socket);
                socket_up_adapter.setMsgKind(APP_DISTR_PORTS_SETUP_REQUEST);
                socket_up_adapter.setParentComponent(this);
                socket_up_adapter.setupSocket(local_address, 5049);
                socket_up_adapter.connect("tcp", address_up, 5049);
                distribution_service.sendDirectionRequest(&socket_up_adapter, data, position, address_down, address_up);
            } else if (!address_down.empty() && originPosition < position) {
                request_down_socket->renewSocket();
                socket_down_adapter.setSocket(request_down_socket);
                socket_down_adapter.setMsgKind(APP_DISTR_PORTS_SETUP_REQUEST);
                socket_down_adapter.setParentComponent(this);
                socket_down_adapter.setupSocket(local_address, 5049);
                socket_down_adapter.connect("tcp", address_down, 5049);
                distribution_service.sendDirectionRequest(&socket_down_adapter, data, position, address_down, address_up);
            }
        }
        if (msg->getKind() == SELF_MSGKIND_DISTR_INITIAL_DIRECTION_SEND) {
            if (!address_up.empty()) {
                socket_up_adapter.setSocket(request_up_socket);
                socket_up_adapter.setParentComponent(this);
                socket_up_adapter.setMsgKind(APP_DISTR_PRE_PUBLISH_DIRECTION_REQUEST);
            }

            if (!address_down.empty()) {
                socket_down_adapter.setSocket(request_down_socket);
                socket_down_adapter.setParentComponent(this);
                socket_down_adapter.setMsgKind(APP_DISTR_PRE_PUBLISH_DIRECTION_REQUEST);
            }
            sendTowards = distribution_service.sendInitialDistributionRequestDirection(&socket_up_adapter,
                                                                                       &socket_down_adapter,
                                                                                       local_address,
                                                                                       address_up,
                                                                                       address_down,
                                                                                       nodes.size(), position);
        }
        if (msg->getKind() == SELF_MSGKIND_DISTR_SUBSCRIBE) {
            isReceiving = true;
            subscribe_socket->renewSocket();
            socket_no_direction_adapter.setParentComponent(this);
            socket_no_direction_adapter.setSocket(subscribe_socket);
            socket_no_direction_adapter.setupSocket(local_address, subscribe_port);
            if (subscribe_socket->getState() != inet::TcpSocket::LISTENING) {
                connection_service.changeToListenState(socket_no_direction_adapter);
            }
            printSocketMap();
        }
        if (msg->getKind() == SELF_MSGKIND_DISTR_DIRECTION_FORWARD) {
            inet::cMsgPar directionPar = msg->par("direction");
            std::string direction = directionPar.stringValue();

            //TODO: Figure out direction here
            printSocketMap();

            if (direction.find("up") != -1) {
                socket_up_adapter.setSocket(request_up_socket);
                socket_up_adapter.setParentComponent(this);
                socket_up_adapter.setMsgKind(APP_DISTR_PRE_PUBLISH_DIRECTION_REQUEST);
                if (request_up_socket->getState() == inet::TcpSocket::CONNECTED) {
                } else {
                    try {
                        socket_up_adapter.setupSocket(local_address, 5049);
                        socket_up_adapter.connect("tcp", address_up, 5049);
                    } catch (std::exception ex) {
                        EV_DEBUG << "already has connection" << std::endl;
                        request_up_socket->setState(inet::TcpSocket::CONNECTED);
                    }
                }
            }

            if (direction.find("down") != -1) {
                EV_DEBUG << "state is: " << inet::TcpSocket::stateName(request_down_socket->getState()) << std::endl;
                socket_down_adapter.setSocket(request_down_socket);
                socket_down_adapter.setParentComponent(this);
                socket_down_adapter.setMsgKind(APP_DISTR_PRE_PUBLISH_DIRECTION_REQUEST);
                if (request_down_socket->getState() == inet::TcpSocket::CONNECTED) {
                } else {
                    try {
                        socket_down_adapter.setupSocket(local_address, 5049);
                        socket_down_adapter.connect("tcp", address_down, 5049);
                    } catch (std::exception ex) {
                        EV_DEBUG << "already has connection" << std::endl;
                        request_down_socket->setState(inet::TcpSocket::CONNECTED);
                    }
                }
            }

            distribution_service.sendForwardistributionRequestDirection(&socket_up_adapter,
                                                                        &socket_down_adapter,
                                                                        address_up,
                                                                        address_down,
                                                                        current_hops,
                                                                        nodes.size(),
                                                                        position,
                                                                        direction);

        }
        if (msg->getKind() == SELF_MSGKIND_DISTR_PUBLISH) {
            //publish_socket->setTCPAlgorithmClass("TcpNoCongestionControl");
            printSocketMap();

            if (sendTowards =="up") {
                socket_up_adapter.setParentComponent(this);
                socket_up_adapter.setSocket(publish_socket);
                socket_up_adapter.setupSocket(local_address, publish_port);
                socket_up_adapter.setMsgKind(APP_DISTR_PUBLISH);
                socket_up_adapter.connect("tcp", address_up, publish_port);
                //publish_socket->connect(inet::Ipv4Address(address_up.c_str()), publish_port);
                distribution_service.sendElection(&socket_up_adapter, election_box[0], publish_port);
            } else if (sendTowards =="down") {
                socket_down_adapter.setParentComponent(this);
                socket_down_adapter.setMsgKind(APP_DISTR_PUBLISH);
                socket_down_adapter.setSocket(publish_socket);
                socket_down_adapter.setupSocket(local_address, publish_port);
                socket_down_adapter.connect("tcp", address_down, publish_port);
                //publish_socket->connect(inet::Ipv4Address(address_down.c_str()), publish_port);
                distribution_service.sendElection(&socket_down_adapter, election_box[0], publish_port);
            }
            closePublishMessage = new inet::cMessage();
            closePublishMessage->setKind(SELF_MSGKIND_CLOSE_PUBLISH_SOCKET);
            scheduleAt(inet::simTime() + 0.1, closePublishMessage);
            isInitializingDirectionDistribution = false;
        }
        if (msg->getKind() == SELF_MSGKIND_CLOSE_SUBSCRIBE_SOCKET) {
            subscribe_socket_adapter.setSocket(subscribe_socket);
            subscribe_socket_adapter.setParentComponent(this);

            try {
                printSocketMap();
                subscribe_socket->close();
            } catch (std::exception &ex) {
                EV_DEBUG << "couldnt close" << std::endl;
                EV_DEBUG << ex.what() << std::endl;
            }
        }
        if (msg->getKind() == SELF_MSGKIND_CLOSE_PUBLISH_SOCKET) {
            publish_socket_adapter.setSocket(publish_socket);
            publish_socket_adapter.setParentComponent(this);
            printSocketMap();
            try {
                publish_socket_adapter.close();
            } catch (std::exception &ex) {
                EV_DEBUG << "couldnt close" << std::endl;
                EV_DEBUG << ex.what() << std::endl;
            }
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
        if (msg->isSelfMessage()) {
            handleTimer(msg);
        } else if (request_up_socket->belongsToSocket(msg)) {
            inet::TagSet &set = inet::getTags(msg);
            inet::SocketInd *pInd = set.findTag<inet::SocketInd>();
            request_up_socket->processMessage(msg);
        } else if (request_down_socket->belongsToSocket(msg)) {
            inet::TagSet &set = inet::getTags(msg);
            inet::SocketInd *pInd = set.findTag<inet::SocketInd>();
            request_down_socket->processMessage(msg);
        } else if (listen_socket.belongsToSocket(msg)) {
            listen_socket.processMessage(msg);
        } else if(reply_key_socket.belongsToSocket(msg)){
            reply_key_socket.processMessage(msg);
        } else if (subscribe_socket->belongsToSocket(msg)) {
            subscribe_socket->processMessage(msg);
        } else if(publish_socket->belongsToSocket(msg)) {
            publish_socket->processMessage(msg);
        } else if (request_key_socket->belongsToSocket(msg)) {
            request_key_socket->processMessage(msg);
        } else {
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
                inet::Indication *pIndication = inet::check_and_cast<inet::Indication *>(msg);
                try {
                    inet::TcpAvailableInfo *pInfo = inet::check_and_cast<inet::TcpAvailableInfo *>(
                            pIndication->getControlInfo());

                    inet::TagSet &set = inet::getTags(msg);
                    inet::SocketInd *pInd = set.findTag<inet::SocketInd>();

                    printSocketMap();

                    //inet::ISocket *pSocket = socketMap.findSocketFor(msg);
                    return;
                } catch (std::exception &ex) {
                    EV_DEBUG << "cannot cast to available info" << std::endl;
                    EV_DEBUG << ex.what() << std::endl;
                }
                try {
                    inet::TcpConnectInfo *pInfo = inet::check_and_cast<inet::TcpConnectInfo *>(
                            pIndication->getControlInfo());
                    return;
                } catch (std::exception &ex) {
                    EV_DEBUG << "cannot cast to connect info" << std::endl;
                    EV_DEBUG << ex.what() << std::endl;
                }
                try {
                    inet::TcpCommand *pCommand = inet::check_and_cast<inet::TcpCommand *>(
                            pIndication->getControlInfo());
                    return;
                } catch (std::exception &ex) {
                    EV_DEBUG << "cannot cast to command" << std::endl;
                    EV_DEBUG << ex.what() << std::endl;
                }

            } catch (std::exception &ex) {
                EV_DEBUG << "cannot cast to indication" << std::endl;
                EV_DEBUG << ex.what() << std::endl;
            }
        }
        //else
        //     throw inet::cRuntimeError("Unknown incoming message: '%s'", msg->getName());
    }

    void VotingApp::socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) {
        TcpAppBase::socketStatusArrived(socket, status);
    }

    void VotingApp::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) {
        auto newSocket = new inet::TcpSocket(availableInfo); // create socket using received info
        socketMap.addSocket(newSocket); // store accepted connection
        if (socket->getLocalPort() == 5049) {
            request_socket_id = availableInfo->getNewSocketId();
        }
        if(availableInfo->getRemotePort() == 50061) {
            reply_key_socket.setOutputGate(gate("socketOut"));
            reply_key_socket.accept(availableInfo->getNewSocketId());
        } else if (std::equal(address_up.begin(), address_up.end(), availableInfo->getRemoteAddr().str().begin())) {
            if (availableInfo->getRemotePort() == 5049) {
                this->request_up_socket->setOutputGate(gate("socketOut"));
                this->request_up_socket->accept(availableInfo->getNewSocketId());
            } else if (availableInfo->getLocalPort() == subscribe_port) {
                this->subscribe_socket->accept(availableInfo->getNewSocketId());
            }
        } else if (std::equal(address_down.begin(), address_down.end(), availableInfo->getRemoteAddr().str().begin())) {
            if (availableInfo->getRemotePort() == 5049) {
                this->request_down_socket->setOutputGate(gate("socketOut"));
                this->request_down_socket->accept(availableInfo->getNewSocketId());
            } else if (availableInfo->getLocalPort() == subscribe_port) {
                this->subscribe_socket->accept(availableInfo->getNewSocketId());
            }
        }
    }

    void VotingApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) {
        const inet::Ptr<const inet::BytesChunk> &intrusivePtr = msg->peekData<inet::BytesChunk>();
        uint8_t appMsgKind = intrusivePtr->getByte(0);

        const inet::Ptr<const inet::BytesChunk> &ptr = msg->peekData<inet::BytesChunk>();
        EV_DEBUG << "as bytes: ";
        std::string content_str;
        std::transform(ptr->getBytes().begin() + 1, ptr->getBytes().end(), std::back_inserter(content_str),
                       [this](uint8_t d) {
                           EV_DEBUG << std::to_string(d) << " ";
                           return (char) d;
                       });
        EV_DEBUG << std::endl;
        EV_DEBUG << "as string: " << content_str << std::endl;


        switch (appMsgKind) {
            case APP_DISTR_PORTS_SETUP_REQUEST: {
                socket->setOutputGate(gate("socketOut"));
                socket_no_direction_adapter.setSocket(socket);
                socket_no_direction_adapter.setMsgKind(APP_DISTR_PORTS_SETUP_RESPONSE);
                socket_no_direction_adapter.setParentComponent(this);
                distribution_service.sendSuccessResponse(&socket_no_direction_adapter);

                nlohmann::json setup_info = nlohmann::json::parse(content_str);
                subscribe_port = setup_info["origin_publish_port"];
                publish_port = setup_info["origin_publish_port"].get<int>() % 5050 == 0 ? 5051 : 5050;

                forwardDirectionRequestSelfMessage = new inet::cMessage("timer");
                received3PData = new omnetpp::cMsgPar("content");
                received3PData->setStringValue(content_str.c_str());
                forwardDirectionRequestSelfMessage->addPar(received3PData);
                forwardDirectionRequestSelfMessage->setKind(SELF_MSGKIND_DISTR_PORTS_SETUP_FORWARD);
                scheduleAt(inet::simTime() + pauseBeforeForwardPortsRequest, forwardDirectionRequestSelfMessage);
            }
                break;
            case APP_DISTR_PORTS_SETUP_RESPONSE: {
                if (content_str.find("accept") != -1) {

                    socket_no_direction_adapter.setSocket(socket);
                    socket_no_direction_adapter.close();

                    if (isInitializingDirectionDistribution) {
                        directionSelfMessage = new inet::cMessage("timer");
                        nextDirection = new omnetpp::cMsgPar("direction");

                        if (address_up.empty()) {
                            sendTowards = "down";
                            nextDirection->setStringValue("down");
                        } else if (address_down.empty()) {
                            sendTowards = "up";
                            nextDirection->setStringValue("up");
                        } else if (position < nodes.size() / 2) {
                            sendTowards = "up";
                            nextDirection->setStringValue("up");
                        } else {
                            sendTowards = "down";
                            nextDirection->setStringValue("down");
                        }

                        directionSelfMessage->addPar(nextDirection);
                        directionSelfMessage->setKind(SELF_MSGKIND_DISTR_INITIAL_DIRECTION_SEND);

                        isInitializingDirectionDistribution = false;
                        scheduleAt(inet::simTime() + 0.1, directionSelfMessage);
                    }
                }
                break;
            }
            case APP_DISTR_PRE_PUBLISH_DIRECTION_REQUEST: {
                received_from_direction = distribution_service.invertDirection(content_str);
                socket_no_direction_adapter.setSocket(socket);
                socket_no_direction_adapter.setMsgKind(APP_DISTR_PRE_PUBLISH_DIRECTION_RESPONSE);
                socket_no_direction_adapter.setParentComponent(this);
                socket->setOutputGate(gate("socketOut"));
                distribution_service.sendSuccessResponse(&socket_no_direction_adapter);
                EV_DEBUG << "now close " << std::endl;
            }
                break;
            case APP_DISTR_PRE_PUBLISH_DIRECTION_RESPONSE: {
                hopsSelfMessage = new inet::cMessage("timer");
                hopsSelfMessage->setKind(SELF_MSGKIND_DISTR_HOPS_SEND);
                scheduleAt(inet::simTime() + 0.1, hopsSelfMessage);
                isReceiving = true;
            }
                break;
            case APP_DISTR_PRE_PUBLISH_HOPS_REQUEST: {
                current_hops = std::stoi(content_str);
                current_hops++;

                socket_no_direction_adapter.setSocket(socket);
                socket_no_direction_adapter.setMsgKind(APP_DISTR_PRE_PUBLISH_HOPS_RESPONSE);
                socket_no_direction_adapter.setParentComponent(this);
                socket->setOutputGate(gate("socketOut"));
                distribution_service.sendSuccessResponse(&socket_no_direction_adapter);
                socket_no_direction_adapter.close();

                subscribeSelfMessage = new inet::cMessage("timer");
                subscribeSelfMessage->setKind(SELF_MSGKIND_DISTR_SUBSCRIBE);
                scheduleAt(inet::simTime(), subscribeSelfMessage);
            }
                break;
            case APP_DISTR_PRE_PUBLISH_HOPS_RESPONSE: {

                this->socket_no_direction_adapter.setSocket(socket);
                this->socket_no_direction_adapter.close();

                if (content_str.find("accept") != -1) {
                    publishSelfMessage = new inet::cMessage("timer");
                    publishSelfMessage->setKind(SELF_MSGKIND_DISTR_PUBLISH);
                    scheduleAt(inet::simTime() + pauseBeforePublish, publishSelfMessage);
                }
            }
                break;
            case APP_DISTR_PUBLISH: {

                char vt = 11;
                std::stringstream test(content_str);
                std::string segment;
                std::vector<std::string> seglist;

                while (std::getline(test, segment, vt)) {
                    subscribe_socket_adapter.addProgrammedMessage(
                            socketMessage{segment, socket->getRemoteAddress().str()});
                    seglist.push_back(segment);
                }
                election el = distribution_service.receiveElection(&subscribe_socket_adapter);

                distribution_service.updateElectionBox(el, election_box);

                if (address_up.empty() xor address_down.empty()) {
                    if (current_hops < nodes.size() - 1) {
                        directionSelfMessage = new inet::cMessage("timer");
                        nextDirection = new omnetpp::cMsgPar("direction");

                        if (address_up.empty()) {
                            nextDirection->setStringValue("down");
                            sendTowards = "down";
                        } else if (address_down.empty()) {
                            nextDirection->setStringValue("up");
                            sendTowards = "up";
                        }

                        directionSelfMessage->addPar(nextDirection);
                        directionSelfMessage->setKind(SELF_MSGKIND_DISTR_DIRECTION_FORWARD);
                        scheduleAt(inet::simTime() + 0.2, directionSelfMessage);
                    }
                } else if (!address_up.empty() and !address_down.empty()) {
                    directionSelfMessage = new inet::cMessage("timer");
                    nextDirection = new omnetpp::cMsgPar("direction");
                    nextDirection->setStringValue(
                            distribution_service.invertDirection(received_from_direction).c_str());
                    sendTowards = distribution_service.invertDirection(received_from_direction);
                    directionSelfMessage->addPar(nextDirection);
                    directionSelfMessage->setKind(SELF_MSGKIND_DISTR_DIRECTION_FORWARD);
                    scheduleAt(inet::simTime() + 0.2, directionSelfMessage);
                } else {
                    directionSelfMessage = new inet::cMessage("timer");
                    nextDirection = new omnetpp::cMsgPar("direction");
                    nextDirection->setStringValue(received_from_direction.c_str());
                    sendTowards = received_from_direction;
                    directionSelfMessage->addPar(nextDirection);
                    directionSelfMessage->setKind(SELF_MSGKIND_DISTR_DIRECTION_FORWARD);
                    scheduleAt(inet::simTime() + 0.2, directionSelfMessage);
                }
                closeSubscribeMessage = new inet::cMessage("timer");
                closeSubscribeMessage->setKind(SELF_MSGKIND_CLOSE_SUBSCRIBE_SOCKET);
                scheduleAt(inet::simTime(), closeSubscribeMessage);
            }
                break;
            case APP_REQUEST_KEY: {
                socket_no_direction_adapter.addProgrammedMessage(socketMessage{content_str, socket->getRemoteAddress().str()});
                socket_no_direction_adapter.setSocket(socket);

                size_t election_id = tally_service->receiveKeyRequest(socket_no_direction_adapter);

                std::queue<std::string> &queue = election_keys_to_send[election_id];
                socket_no_direction_adapter.setMsgKind(APP_RESPONSE_KEY);
                socket_no_direction_adapter.setParentComponent(this);
                socket->setOutputGate(gate("socketOut"));
                tally_service->keyResponse(socket_no_direction_adapter, election_id, &election_keys_to_send);

            }
            break;
            case APP_RESPONSE_KEY: {
                socket_no_direction_adapter.setSocket(socket);
                socket_no_direction_adapter.setMsgKind(APP_RESPONSE_KEY_CLOSE);
                socket_no_direction_adapter.setParentComponent(this);
                socket_no_direction_adapter.addProgrammedMessage(socketMessage{content_str, socket->getRemoteAddress().str()});
                tally_service->receiveKey(socket_no_direction_adapter, election_box[0].getId(), received_election_keys);
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

    void VotingApp::printSocketMap() {
        std::for_each(socketMap.getMap().begin(), socketMap.getMap().end(),
                      [this](std::pair<int, inet::ISocket *> idSocketPair) {
                          inet::TcpSocket *pSocket = inet::check_and_cast<inet::TcpSocket *>(idSocketPair.second);
                          EV_DEBUG
                          << idSocketPair.first << ": " << pSocket->getLocalAddress() << ":" << pSocket->getLocalPort()
                          << "-" << pSocket->getRemoteAddress() << ":" << pSocket->getRemotePort() << ", open="
                          << pSocket->isOpen() << ", isConnected=" << std::to_string(pSocket->getState() ==
                          inet::TcpSocket::CONNECTED) << ")" << std::endl;
                      });
    }
}