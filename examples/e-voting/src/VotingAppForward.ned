package voting;

import inet.applications.tcpapp.TcpEchoApp;

simple VotingAppForward extends TcpEchoApp
{
    parameters:
        @class(VotingAppForward);
        @signal[packetSent](type=inet::Packet);
        @signal[packetReceived](type=inet::Packet);
}

simple VotingAppForwardThread
{
}

