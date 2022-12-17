//
// Created by wld on 11.12.22.
//

#include <inet/applications/common/SocketTag_m.h>
#include "VotingAppForward.h"
#include "inet/common/Simsignals.h"
#include "inet/common/packet/printer/PacketPrinter.h"
#include "inet/transportlayer/tcp/Tcp.h"
#include <algorithm>

namespace voting {
    Define_Module(VotingAppForward);
    Define_Module(VotingAppForwardThread);

    void VotingAppForwardThread::dataArrived(inet::Packet *rcvdPkt, bool urgent) {
        //TcpEchoAppThread::dataArrived(rcvdPkt, urgent);

        votingAppForward->emit(inet::packetReceivedSignal, rcvdPkt);
        int64_t rcvdBytes = rcvdPkt->getByteLength();
        EV_DEBUG << "data arrived now forward" << std::endl;

        votingAppForward->bytesRcvd += rcvdBytes;
        if (votingAppForward->echoFactor > 0 && sock->getState() == inet::TcpSocket::CONNECTED) {
            inet::Packet *outPkt = new inet::Packet(rcvdPkt->getName(), inet::TCP_C_SEND);
            // reverse direction, modify length, and send it back
            int socketId = rcvdPkt->getTag<inet::SocketInd>()->getSocketId();
            outPkt->addTag<inet::SocketReq>()->setSocketId(socketId);

            long outByteLen = rcvdBytes * votingAppForward->echoFactor;

            if (outByteLen < 1)
                outByteLen = 1;

            int64_t len = 0;
            for ( ; len + rcvdBytes <= outByteLen; len += rcvdBytes) {
                std::vector<uint8_t> vec;
                std::string addressString = getSocket()->getRemoteAddress().str();
                std::transform(addressString.begin(), addressString.end(), std::back_inserter(vec), [](char c){
                    return (uint8_t) c;
                });

                const auto &bytesChunk = inet::makeShared<inet::BytesChunk>();
                bytesChunk->setBytes(vec);

                outPkt->insertAtBack(bytesChunk);
                inet::PacketPrinter packetPrinter;
                EV_DEBUG << "received from " << getSocket()->getRemoteAddress().str() << std::endl;
                EV_DEBUG << packetPrinter.printPacketToString(rcvdPkt) << std::endl;
            }
            if (len < outByteLen) {
                std::vector<uint8_t> vec;
                std::string addressString = getSocket()->getRemoteAddress().str();
                std::transform(addressString.begin(), addressString.end(), std::back_inserter(vec), [](char c){
                    return (uint8_t) c;
                });

                const auto &bytesChunk = inet::makeShared<inet::BytesChunk>();
                bytesChunk->setBytes(vec);

                outPkt->insertAtBack(bytesChunk);
                inet::PacketPrinter packetPrinter;
                EV_DEBUG << "received from " << getSocket()->getRemoteAddress().str() << std::endl;
                EV_DEBUG << packetPrinter.printPacketToString(rcvdPkt) << std::endl;
            }

            //ASSERT(outPkt->getByteLength() == outByteLen);

            if (votingAppForward->delay == 0) {
                EV_DEBUG << "send down" << std::endl;
                votingAppForward->sendDown(outPkt);
            }
            else
                scheduleAt(inet::simTime() + votingAppForward->delay, outPkt); // send after a delay
        }
        delete rcvdPkt;
    }
}