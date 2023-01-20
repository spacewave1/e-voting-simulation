//
// Created by wld on 10.01.23.
//

#include "VotingAppConnectionRequestReply.h"
#include "inet/applications/common/SocketTag_m.h"
#include "inet/applications/tcpapp/TcpServerSocketIo.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "VotingApp.h"
#include "PacketsKind.h"

namespace voting {
    Define_Module(VotingAppConnectionRequestReply);

    void VotingAppConnectionRequestReply::acceptSocket(inet::TcpAvailableInfo *availableInfo)
    {
        EV_DEBUG << "accepted socket on vacrr" << std::endl;
        Enter_Method_Silent();
        requestSocket = new inet::TcpSocket(availableInfo);
        requestSocket->setOutputGate(gate("socketOut"));
        requestSocket->setCallback(this);
        requestSocket->accept(availableInfo->getNewSocketId());
    }

    void VotingAppConnectionRequestReply::handleMessage(inet::cMessage *message)
    {
        if (message->arrivedOn("socketIn")) {
            EV_DEBUG << "data arrived on socket in" << std::endl;
            ASSERT(requestSocket != nullptr && requestSocket->belongsToSocket(message));
            requestSocket->processMessage(message);
        }
        else if (message->arrivedOn("trafficIn")){
            EV_DEBUG << "data arrived on traffic in" << std::endl;
            requestSocket->send(inet::check_and_cast<inet::Packet *>(message));
        }
        else
            throw inet::cRuntimeError("Unknown message");
    }

    void VotingAppConnectionRequestReply::socketDataArrived(inet::TcpSocket* socket, inet::Packet *packet, bool urgent)
    {
        EV_DEBUG << "data has arrived" << std::endl;
        cModule *connectionModule = getParentModule();
        VotingApp* appModule = inet::check_and_cast<VotingApp*>(connectionModule->getParentModule());

        const inet::Ptr<const inet::BytesChunk> &ptr = packet->peekData<inet::BytesChunk>();
        std::string content_str;
        std::transform(ptr->getBytes().begin(),ptr->getBytes().end(),std::back_inserter(content_str),[](uint8_t d){
            return (char) d;
        });
        EV_DEBUG << content_str << std::endl;
        EV_DEBUG << packet->getKind() << std::endl;

        if(packet->getKind() == APP_CONN_REQUEST){
            inetSocketAdapter socket_adapter;
            socket_adapter.setSocket(socket);
            socket_adapter.addProgrammedMessage(socketMessage{content_str,socket->getRemoteAddress().str()});
            auto &abstract_socket_adapter = reinterpret_cast<abstractSocket &>(socket_adapter);

            if(appModule->getConnectionService()->receiveConnectionRequest(abstract_socket_adapter, content_str, *appModule->getNodes(), *appModule->getNodeConnections(), socket->getLocalAddress().str()) == 0){
                delete packet->removeTag<inet::SocketInd>();
                send(packet, "trafficOut");
                requestSocket->send(createDataPacket("accept"));
            } else {
                delete packet->removeTag<inet::SocketInd>();
                send(packet, "trafficOut");
                requestSocket->send(createDataPacket("reject"));
            }
            appModule->writeStateToFile("connection/peers_after_data_send_" + appModule->getFullPath().substr(0,19) + ".json");

            EV_DEBUG << "parent module: " << appModule->getName() << std::endl;
            EV_DEBUG << packet->str() << std::endl;
        }

        if(packet->getKind() == APP_SYNC_REQUEST){
            EV_DEBUG << "now forward sync" << std::endl;
            inetSocketAdapter socket_adapter;
            socket_adapter.setSocket(socket);
            socket_adapter.addProgrammedMessage(socketMessage{content_str,socket->getRemoteAddress().str()});
            auto &abstract_socket_adapter = reinterpret_cast<abstractSocket &>(socket_adapter);

            if(appModule->getSyncService()->receiveSyncRequest(&socket_adapter, *appModule->getNodes(), *appModule->getNodeConnections())){
                delete packet->removeTag<inet::SocketInd>();
                send(packet, "trafficOut");
            }
        }
    }

    inet::Packet *VotingAppConnectionRequestReply::createDataPacket(std::string send_string) {
        EV_DEBUG << "create data" << std::endl;
        inet::Ptr<inet::Chunk> payload;
        inet::Packet *packet = new inet::Packet("data");

        const auto &bytesChunk = inet::makeShared<inet::BytesChunk>();
        std::vector<uint8_t> vec;
        unsigned long length = send_string.length();

        vec.resize(send_string.length());
        for (int i = 0; i < send_string.length(); i++)
            vec[i] = (replyBytesSent + send_string[i % length]) & 0xFF;

        bytesChunk->setBytes(vec);
        packet->insertAtBack(bytesChunk);
        packet->setKind(APP_CONN_REPLY);
        return packet;
    }
}