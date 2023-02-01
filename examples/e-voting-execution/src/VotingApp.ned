package voting;

import inet.applications.contract.IApp;

simple VotingApp like IApp
{
    parameters:
        string localAddress = default("");
        int localPort = default(-1);  // local port
        bool active = default(true);
        bool isReceivingAtStart = default(false);
        string connectAddress;
        int threePDownPort = default(5049);
        int threePUpPort = default(5051);
        double tThreePRequest @unit(s) = default(1.5s);
        double tThreePReceive @unit(s) = default(1s);
        double tCreateElection @unit(s) = default(-1s);
        int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        @display("i=block/app");
        @lifecycleSupport;
        double stopOperationExtraTime @unit(s) = default(-1s);    // extra time after lifecycle stop operation finished
        double stopOperationTimeout @unit(s) = default(2s);    // timeout value for lifecycle stop operation
        string serverConnectionModuleType = default("voting.VotingNetworkRequestConnection"); // module type of "connection" module to launch on incoming connections
        @signal[connect](type=long);
        @class(VotingApp);
        @signal[packetSent](type=inet::Packet);
        @signal[packetReceived](type=inet::Packet);
        @statistic[packetReceived](title="packets received"; source=packetReceived; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[packetSent](title="packets sent"; source=packetSent; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[endToEndDelay](title="end-to-end delay"; source="dataAge(packetReceived)"; unit=s; record=histogram,weightedHistogram,vector; interpolationmode=none);
    gates:
        input socketIn @labels(TcpCommand/up);
        output socketOut @labels(TcpCommand/down);
}