//
// Created by wld on 27.11.22.
//

#include <inet/applications/tcpapp/TcpSessionApp.h>
#include "inetSocketAdapter.h"
#include "network/connectionService.h"

#ifndef E_VOTING_APP_H
#define E_VOTING_APP_H

namespace voting {
    class VotingApp: public inet::TcpSessionApp{
        void initialize(int stage) override;
        inet::Packet * createDataPacket(long sendBytes) override;
        connectionService connection_service;
        inetSocketAdapter socketAdapter;

        inet::Packet sendOutPacket = inet::Packet(nullptr, 0);
        inet::Ptr<inet::BytesChunk> ptr = inet::makeShared<inet::BytesChunk>();
    };
}

#endif //E_VOTING_APP_H
