package voting;

import inet.applications.contract.IApp;

simple VotingSetupApp like IApp
{
    parameters:
        string localAddress = default("");
        int localPort = default(-1);  // local port
        bool active = default(true);
        bool isReceivingAtStart = default(false);
        string connectAddress;
        int connectPort = default(5555);
        int syncForwardPort = default(5556);
        int syncReturnPort = default(5557);
        double tOpen @unit(s) = default(1s);
        double tSend @unit(s) = default(1s);
        double tClose @unit(s) = default(2s);
        double tListenStart @unit(s) = default(0s);
        double tListenEnd @unit(s) = default(5s);
        double tSyncInit @unit(s) = default(3s);
        double tListenUpSync @unit(s) = default(3s);
        double tListenDownSync @unit(s) = default(3s);
        double tForwardUpSync @unit(s) = default(3s);
        double tReturnDownSync @unit(s) = default(3s);
        int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        @display("i=block/app");
        @lifecycleSupport;
        double stopOperationExtraTime @unit(s) = default(-1s);    // extra time after lifecycle stop operation finished
        double stopOperationTimeout @unit(s) = default(2s);    // timeout value for lifecycle stop operation

        @signal[connect](type=long);
        @class(VotingSetupApp);
        @signal[packetSent](type=inet::Packet);
        @signal[packetReceived](type=inet::Packet);
        @statistic[packetReceived](title="packets received"; source=packetReceived; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[packetSent](title="packets sent"; source=packetSent; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[endToEndDelay](title="end-to-end delay"; source="dataAge(packetReceived)"; unit=s; record=histogram,weightedHistogram,vector; interpolationmode=none);
    gates:
        input socketIn @labels(TcpCommand/up);
        output socketOut @labels(TcpCommand/down);
}