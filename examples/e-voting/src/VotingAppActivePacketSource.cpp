//
// Created by wld on 14.01.23.
//


#include <inet/queueing/source/ActivePacketSource.h>
#include <inet/common/packet/chunk/ByteCountChunk.h>
#include <inet/common/packet/chunk/BitCountChunk.h>
#include <inet/applications/base/ApplicationPacket_m.h>
#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "VotingAppActivePacketSource.h"
#include "inet/common/TimeTag_m.h"


namespace voting {
        Define_Module(VotingAppActivePacketSource);

    void VotingAppActivePacketSource::handleMessage(omnetpp::cMessage *message) {
        EV_DEBUG << "handle active packet source" << std::endl;
        ActivePacketSource::handleMessage(message);
        EV_DEBUG << "after handle" << std::endl;
    }

    void VotingAppActivePacketSource::scheduleProductionTimer() {
        //ActivePacketSource::scheduleProductionTimer();
        EV_DEBUG << "after scheduling" << std::endl;
    }

    void VotingAppActivePacketSource::initialize(int stage) {
        ActivePacketSource::initialize(stage);
    }

    void VotingAppActivePacketSource::producePacket() {
        EV_DEBUG << "produce packet" << std::endl;
        auto packet = createPacket();
        EV_INFO << "Producing packet " << packet->getName() << "." << std::endl;
        pushOrSendPacket(packet, outputGate, consumer);
        updateDisplayString();
        EV_DEBUG << "after produce packet" << std::endl;
    }

    inet::Packet *VotingAppActivePacketSource::createPacket() {
        EV_DEBUG << "create content" << std::endl;
        auto packetContent = createPacketContent();
        EV_DEBUG << "add tag" << std::endl;
        packetContent->addTag<inet::CreationTimeTag>()->setCreationTime(inet::simTime());
        EV_DEBUG << "create packet name" << std::endl;
        auto packetName = createPacketName(packetContent);
        EV_DEBUG << "new packet" << std::endl;
        auto packet = new inet::Packet(packetName, packetContent);
        numProcessedPackets++;
        processedTotalLength += packet->getDataLength();
        EV_DEBUG << "emit" << std::endl;
        emit(inet::packetCreatedSignal, packet);
        return packet;
    }

    inet::Ptr<inet::Chunk> VotingAppActivePacketSource::createPacketContent() const {
        EV_DEBUG << "set packet length" << std::endl;
        auto packetLength = inet::B(packetLengthParameter->intValue());
        EV_DEBUG << "set packet data" << std::endl;
        auto packetData = packetDataParameter->intValue();
        if (!strcmp(packetRepresentation, "bitCount")){
            EV_DEBUG << "bitCount" << std::endl;
            return packetData == -1 ? inet::makeShared<inet::BitCountChunk>(packetLength) : inet::makeShared<inet::BitCountChunk>(packetLength, packetData);
        }
        else if (!strcmp(packetRepresentation, "bits")) {
            EV_DEBUG << "bits" << std::endl;
            static int total = 0;
            const auto& packetContent = inet::makeShared<inet::BitsChunk>();
            std::vector<bool> bits;
            bits.resize(inet::b(packetLength).get());
            for (int i = 0; i < (int)bits.size(); i++)
                bits[i] = packetData == -1 ? (total + i) % 2 == 0 : packetData;
            total += bits.size();
            packetContent->setBits(bits);
            return packetContent;
        }
        else if (!strcmp(packetRepresentation, "byteCount")){
            EV_DEBUG << "byteCount" << std::endl;
            EV_DEBUG << "packet length: " << packetLength << std::endl;
            EV_DEBUG << "packet data: " << packetData << std::endl;
            const inet::Ptr<inet::ByteCountChunk> &ptr = inet::makeShared<inet::ByteCountChunk>(packetLength);
            EV_DEBUG << "pointer has been cretad" << std::endl;
            return packetData == -1 ? ptr : inet::makeShared<inet::ByteCountChunk>(packetLength, packetData);
        }
        else if (!strcmp(packetRepresentation, "bytes")) {
            EV_DEBUG << "bytes" << std::endl;
            static int total = 0;
            const auto& packetContent = inet::makeShared<inet::BytesChunk>();
            std::vector<uint8_t> bytes;
            bytes.resize(inet::B(packetLength).get());
            for (int i = 0; i < (int)bytes.size(); i++)
                bytes[i] = packetData == -1 ? (total + i) % 256 : packetData;
            total += bytes.size();
            packetContent->setBytes(bytes);
            return packetContent;
        }
        else if (!strcmp(packetRepresentation, "applicationPacket")) {
            EV_DEBUG << "applicationPacket" << std::endl;
            const auto& packetContent = inet::makeShared<inet::ApplicationPacket>();
            packetContent->setChunkLength(inet::B(packetLength));
            packetContent->setSequenceNumber(numProcessedPackets);
            return packetContent;
        }
        else
            throw inet::cRuntimeError("Unknown representation");
    }
} // namespace voting
