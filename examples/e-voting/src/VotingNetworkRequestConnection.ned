package voting;

import VotingAppConnectionRequestReply;
import VotingAppActivePacketSource;
import inet.applications.contract.IApp;
import inet.queueing.contract.IPassivePacketSink;
import inet.queueing.contract.IActivePacketSource;

module VotingNetworkRequestConnection like IApp
{
    parameters:
            @display("i=block/cogwheel");
        gates:
            input socketIn;
            output socketOut;
        submodules:
            source: voting.VotingAppActivePacketSource {
                parameters:
                    packetLength = default(2b);
                    productionInterval = default(2s);
                    @display("p=100,100");
            }
            sink: <default("PassivePacketSink")> like IPassivePacketSink {
                parameters:
                    @display("p=200,100");
            }
            io: voting.VotingAppConnectionRequestReply {
                parameters:
                    @display("p=150,200");
            }
        connections:
            source.out --> io.trafficIn;
            io.socketOut --> socketOut;
            socketIn --> io.socketIn;
            io.trafficOut --> sink.in;
}