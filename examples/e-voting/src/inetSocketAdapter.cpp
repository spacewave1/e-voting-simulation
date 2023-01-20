//
// Created by wld on 03.12.22.
//
#include "inetSocketAdapter.h"
#include "VotingApp.h"
#include <algorithm>
#include <inet/common/Simsignals.h>

void inetSocketAdapter::send(std::string payload) {

    inet::Packet *packet = new inet::Packet("data");
    packet->setKind(msg_kind);

    const auto &bytesChunk = inet::makeShared<inet::BytesChunk>();
    std::vector<uint8_t> vec;
    unsigned long length = payload.length();

    int bytesSent = 0;
    int sendBytes = length;

    vec.resize(sendBytes);
    for (int i = 0; i < sendBytes; i++)
        vec[i] = (bytesSent + payload[i % length]) & 0xFF;

    bytesChunk->setBytes(vec);
    packet->insertAtBack(bytesChunk);

    parentComponent->emit(inet::packetSentSignal, packet);
    socket->send(packet);

    auto voting_app_component = dynamic_cast<voting::VotingApp*>(parentComponent);

    voting_app_component->setPacketsSent(voting_app_component->getPacketsSent() + 1);
    voting_app_component->setBytesSent(voting_app_component->getBytesSent() + packet->getByteLength());
}

void inetSocketAdapter::recvAlt() {
    socket->listen();
}

socketMessage inetSocketAdapter::recv() {
    //socket.listen();
    socketMessage &message = programmed_message_queue.front();
    programmed_message_queue.pop();
    return message;
}

socketMessage inetSocketAdapter::interruptableRecv(bool &is_L3Addressinterrupt) {
    //TODO: revc
    return socketMessage();
}

void inetSocketAdapter::disconnect(std::string protocol, std::string address, size_t port) {
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

void inetSocketAdapter::setSendPacket(inet::Packet& packet) {
    sendOutPacket = packet;
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

inet::TcpSocket *inetSocketAdapter::getSocket() {
    return socket;
}

void inetSocketAdapter::setMsgKind(short msgKind) {
    this->msg_kind = msgKind;
}

short inetSocketAdapter::getMsgKind() const {
    return msg_kind;
}
