//
// Created by wld on 03.12.22.
//
#include "inetSocketAdapter.h"
#include <algorithm>
#include <inet/common/Simsignals.h>
#include <inet/common/packet/chunk/ByteCountChunk.h>
#include <inet/applications/tcpapp/TcpAppBase.h>

void inetSocketAdapter::send(std::string payload) {

    send_out_packet = new inet::Packet("data");

    auto byteCountData = inet::makeShared<inet::BytesChunk>();
    std::vector<uint8_t> appData{ msg_kind };
    byteCountData->setBytes(appData);

    const auto dataChunk = inet::makeShared<inet::BytesChunk>();
    std::vector<uint8_t> vec;

    unsigned long sendBytes = payload.length();

    vec.resize(sendBytes);
    for (int i = 0; i < sendBytes; i++)
        vec[i] = (bytes_sent + payload[i % sendBytes]) & 0xFF;

    dataChunk->setBytes(vec);

    send_out_packet->insertAtBack(dataChunk);
    if(is_multi_package_data) {
        const auto exit_sequence_chunk = inet::makeShared<inet::BytesChunk>();
        exit_sequence_chunk->setBytes(exit_sequence);

        send_out_packet->insertAtBack(exit_sequence_chunk);
        is_multi_package_data = false;
    }

    send_out_packet->insertAtFront(byteCountData);

    parent_component->emit(inet::packetSentSignal, send_out_packet);
    std::cout << "send: " << send_out_packet->str() << std::endl;
    socket->send(send_out_packet);
}

void inetSocketAdapter::listen() {
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
    if(socket->getState() == inet::TcpSocket::CONNECTING){
        socket->renewSocket();
    }
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
    socket->destroy();
}

void inetSocketAdapter::setSocket(inet::TcpSocket* socket) {
    this->socket = socket;
}

void inetSocketAdapter::setParentComponent(inet::cComponent *component) {
    this->parent_component = component;
}

void inetSocketAdapter::addProgrammedMessage(socketMessage message) {
    programmed_message_queue.emplace(message);
}

void inetSocketAdapter::setMsgKind(uint8_t msg_kind) {
    this->msg_kind = msg_kind;
}

int inetSocketAdapter::getBytesSent() const {
    return send_out_packet->getByteLength();
}

void inetSocketAdapter::setupSocket(std::string local_address, size_t port) {
    socket->renewSocket();
    socket->bind(inet::Ipv4Address(local_address.c_str()), port);
    auto* tcpApp = reinterpret_cast<inet::TcpAppBase*>(parent_component);
    socket->setCallback(tcpApp);
    socket->setOutputGate(tcpApp->gate("socketOut"));
}

void inetSocketAdapter::setIsMultiPackageData(bool newValue) {
    this->is_multi_package_data = newValue;
}
