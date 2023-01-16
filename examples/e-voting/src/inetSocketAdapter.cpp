//
// Created by wld on 03.12.22.
//
#include "inetSocketAdapter.h"
#include <algorithm>
#include <inet/common/Simsignals.h>

void inetSocketAdapter::send(std::string payload) {
    inet::Packet *packet = new inet::Packet("data");

    const auto &bytesChunk = inet::makeShared<inet::BytesChunk>();
    std::vector<uint8_t> vec;
    unsigned long length = payload.length();

    int bytesSent = 0;
    int sendBytes = 0;
    int packetsSent = 0;
    int numBytes = 0;

    vec.resize(sendBytes);
    for (int i = 0; i < sendBytes; i++)
        vec[i] = (bytesSent + payload[i % length]) & 0xFF;

    bytesChunk->setBytes(vec);
    packet->insertAtBack(bytesChunk);

    //votingAppComponent->emit(inet::packetSentSignal, length);
    socket.send(packet);

   // votingAppComponent->setPacketsSent(votingAppComponent->getPacketsSent() + 1);
    //votingAppComponent->setBytesSent(votingAppComponent->getBytesSent() + numBytes);
}

void inetSocketAdapter::recvAlt() {
    socket.listen();
}

socketMessage inetSocketAdapter::recv() {
    socket.listen();
    return socketMessage();
}

socketMessage inetSocketAdapter::interruptableRecv(bool &is_L3Addressinterrupt) {
    //TODO: revc
    return socketMessage();
}

void inetSocketAdapter::disconnect(std::string protocol, std::string address, size_t port) {
    //TODO: disconnect
}

void inetSocketAdapter::connect(std::string protocol, std::string address, size_t port) {
   //socket.connect(inet::Ipv4Address(address.c_str()),port);
}

void inetSocketAdapter::bind(std::string protocol, std::string address, size_t port) {
    socket.bind(port);
}

void inetSocketAdapter::unbind(std::string protocol, std::string address, size_t port) {
    // TODO: Unbind
}

void inetSocketAdapter::close() {
    socket.close();
}

bool inetSocketAdapter::isBound() {
    return false;
}

void inetSocketAdapter::setSendPacket(inet::Packet& packet) {
    sendOutPacket = packet;
}

void inetSocketAdapter::setSocket(inet::TcpSocket& socket) {
    this->socket = socket;
}