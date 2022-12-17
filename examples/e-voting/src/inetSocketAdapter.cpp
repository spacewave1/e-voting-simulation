//
// Created by wld on 03.12.22.
//
#include "inetSocketAdapter.h"
#include <algorithm>

void inetSocketAdapter::send(std::string payload) {
    socket.send(&sendOutPacket); // send packet using TCP socket
}

void inetSocketAdapter::recvAlt() {
    socket.listenOnce();
}

socketMessage inetSocketAdapter::recv() {
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
