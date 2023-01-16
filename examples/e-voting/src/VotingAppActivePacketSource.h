//
// Created by wld on 14.01.23.
//

#ifndef E_VOTING_VOTINGAPPACTIVEPACKETSOURCE_H
#define E_VOTING_VOTINGAPPACTIVEPACKETSOURCE_H

namespace voting {
    class VotingAppActivePacketSource : public inet::queueing::ActivePacketSource {
        void handleMessage(inet::cMessage *message) override;
        void scheduleProductionTimer() override;
        void producePacket() override;

    protected:
        inet::Packet *createPacket() override;

        inet::Ptr<inet::Chunk> createPacketContent() const override;

    protected:
        void initialize(int stage) override;

    };
}


#endif //E_VOTING_VOTINGAPPACTIVEPACKETSOURCE_H
