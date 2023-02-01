//
// Created by wld on 03.12.22.
//
#include "inetSocketAdapter.h"
#include <algorithm>
#include <inet/common/Simsignals.h>
#include <inet/common/packet/chunk/ByteCountChunk.h>

void inetSocketAdapter::send(std::string payload) {

    sendOutPacket = new inet::Packet("data");

    auto byteCountData = inet::makeShared<inet::BytesChunk>();
    std::vector<uint8_t> appData{ msg_kind };
    byteCountData->setBytes(appData);

    const auto dataChunk = inet::makeShared<inet::BytesChunk>();
    std::vector<uint8_t> vec;

    unsigned long sendBytes = payload.length();

    vec.resize(sendBytes);
    for (int i = 0; i < sendBytes; i++)
        vec[i] = (bytesSent + payload[i % sendBytes]) & 0xFF;

    dataChunk->setBytes(vec);

    sendOutPacket->insertAtBack(dataChunk);
    sendOutPacket->insertAtFront(byteCountData);

    parentComponent->emit(inet::packetSentSignal, sendOutPacket);
    socket->send(sendOutPacket);
}

void inetSocketAdapter::recvAlt() {
    socket->listen();
}

socketMessage inetSocketAdapter::recv() {
    //socket.listen();
    socketMessage message = programmed_message_queue.front();
    programmed_message_queue.pop();
    _logger.log(message.payload);
    return message;
}

socketMessage inetSocketAdapter::interruptableRecv(bool &is_L3Addressinterrupt) {
    //TODO: revc
    return socketMessage();
}

void inetSocketAdapter::disconnect(std::string protocol, std::string address, size_t port) {
    socket->abort();
    //TODO: disconnect
}

void inetSocketAdapter::connect(std::string protocol, std::string address, size_t port) {
   socket->connect(inet::Ipv4Address(address.c_str()),port);
}

void inetSocketAdapter::bind(std::string protocol, std::string address, size_t port) {
    socket->bind(port);
}

void inetSocketAdapter::unbind(std::string protocol, std::string address, size_t port) {
    // TODO: Unbind
}

void inetSocketAdapter::close() {
    socket->close();
}

bool inetSocketAdapter::isBound() {
    return false;
}

void inetSocketAdapter::setSocket(inet::TcpSocket* socket) {
    this->socket = socket;
}

void inetSocketAdapter::setParentComponent(inet::cComponent *component) {
    this->parentComponent = component;
}

void inetSocketAdapter::addProgrammedMessage(socketMessage message) {
    programmed_message_queue.emplace(message);
}

void inetSocketAdapter::setMsgKind(uint8_t msgKind) {
    this->msg_kind = msgKind;
}

int inetSocketAdapter::getBytesSent() const {
    return sendOutPacket->getByteLength();
}