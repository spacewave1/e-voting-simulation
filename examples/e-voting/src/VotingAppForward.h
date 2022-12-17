//
// Created by wld on 11.12.22.
//

#include <inet/applications/tcpapp/TcpEchoApp.h>

#ifndef E_VOTING_VOTINGAPPFORWARD_H
#define E_VOTING_VOTINGAPPFORWARD_H

namespace voting {
    class VotingAppForward: public inet::TcpEchoApp {
        friend class VotingAppForwardThread;
    };

    class VotingAppForwardThread : public inet::TcpEchoAppThread{
        protected:
            VotingAppForward *votingAppForward = nullptr;
        void dataArrived(inet::Packet *rcvdPkt, bool urgent) override;

        void init(inet::TcpServerHostApp *hostmodule, inet::TcpSocket *socket) override {
            inet::TcpEchoAppThread::init(hostmodule, socket);
            votingAppForward = inet::check_and_cast<VotingAppForward *>(hostmod);
        }
    };
}


#endif //E_VOTING_VOTINGAPPFORWARD_H
