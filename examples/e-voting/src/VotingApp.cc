//
// Created by wld on 27.11.22.
//

#include "VotingApp.h"
#include "inetSocketAdapter.h"
#include "network/connectionService.h"
#include "inet/common/TimeTag_m.h"
#include "inet/applications/tcpapp/GenericAppMsg_m.h"
#include "inet/transportlayer/tcp/Tcp.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include <string>

#define CONNECT    0
#define RECEIVE    1

namespace voting {
    Define_Module(VotingApp);

    inet::Packet * VotingApp::createDataPacket(long sendBytes) {
        const char *dataTransferMode = par("dataTransferMode");
        inet::Ptr<inet::Chunk> payload;
        inet::Packet *packet = new inet::Packet("data");

        const auto &bytesChunk = inet::makeShared<inet::BytesChunk>();
        const char *connectAddress = par("connectAddress");
        std::string address {connectAddress };
        std::string sendString = connection_service.createNetworkRegistrationRequest(address);
        std::vector<uint8_t> vec;
        bytesChunk->setBytes(vec);
        unsigned long length = sendString.length();

        EV_INFO << "sizeof: " << length << std::endl;

        vec.resize(sendBytes);
        for (int i = 0; i < sendBytes; i++)
            vec[i] = (bytesSent + sendString[i%length]) & 0xFF;

        EV_INFO << "PacketBytes: ";
        for (uint8_t number : vec)
            EV_INFO << number;
        EV_INFO << std::endl;

        bytesChunk->setBytes(vec);
        packet->insertAtBack(bytesChunk);
        //auto tcpHeader = inet::makeShared<inet::tcp::TcpHeader>(); // create new TCP header
        //tcpHeader->setCrcMode(inet::CRC_COMPUTED);
        //packet->insertAtFront(tcpHeader);

        return packet;
    }

    void VotingApp::initialize(int stage) {
        TcpAppBase::initialize(stage);
        if (stage == inet::INITSTAGE_LOCAL) {
            activeOpen = par("active");
            tOpen = par("tOpen");
            tSend = par("tSend");
            tClose = par("tClose");
            sendBytes = par("connectAddress").stdstringValue().size();
            commandIndex = 0;

            const char *script = par("sendScript");
            parseScript(script);

            if (sendBytes > 0 && !commands.empty())
                throw omnetpp::cRuntimeError("Cannot use both sendScript and tSend+sendBytes");
            if (sendBytes > 0)
                commands.push_back(Command(tSend, sendBytes));
            if (commands.empty())
                throw omnetpp::cRuntimeError("sendScript is empty");
            timeoutMsg = new omnetpp::cMessage("timer");
        }
    }
}