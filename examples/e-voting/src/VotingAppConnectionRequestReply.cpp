//
// Created by wld on 10.01.23.
//

#include "VotingAppConnectionRequestReply.h"
#include "inet/applications/common/SocketTag_m.h"
#include "inet/applications/tcpapp/TcpServerSocketIo.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "VotingApp.h"

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
            ASSERT(requestSocket != nullptr && requestSocket->belongsToSocket(message));
            requestSocket->processMessage(message);
        }
        else if (message->arrivedOn("trafficIn"))
            requestSocket->send(inet::check_and_cast<inet::Packet *>(message));
        else
            throw inet::cRuntimeError("Unknown message");
    }

    void VotingAppConnectionRequestReply::socketDataArrived(inet::TcpSocket* socket, inet::Packet *packet, bool urgent)
    {
        cModule *connectionModule = getParentModule();
        VotingApp* appModule = inet::check_and_cast<VotingApp*>(connectionModule->getParentModule());

        appModule->addNode(socket->getRemoteAddress().str());
        appModule->addNode(socket->getLocalAddress().str());

        appModule->writeStateToFile("peers_after_data_send_" + appModule->getFullPath().substr(0,10) + ".json");
        EV_DEBUG << "parent module: " << appModule->getName() << std::endl;


        delete packet->removeTag<inet::SocketInd>();
        send(packet, "trafficOut");

        requestSocket->send(createDataPacket(10));
    }

    inet::Packet *VotingAppConnectionRequestReply::createDataPacket(long sendBytes) {
        EV_DEBUG << "create data" << std::endl;
        inet::Ptr<inet::Chunk> payload;
        inet::Packet *packet = new inet::Packet("data");

        const auto &bytesChunk = inet::makeShared<inet::BytesChunk>();
        std::string sendString = "accept";
        std::vector<uint8_t> vec;
        unsigned long length = sendString.length();

        vec.resize(sendBytes);
        for (int i = 0; i < sendBytes; i++)
            vec[i] = (replyBytesSent + sendString[i % length]) & 0xFF;

        bytesChunk->setBytes(vec);
        packet->insertAtBack(bytesChunk);

        return packet;
    }
}