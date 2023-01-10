package voting;

//
// This module provides TCP server socket handling for generic TCP applications.
//
// @see ~TcpServerConnection, ~TcpClientSocketIo
//
simple VotingAppConnectionRequestReply
{
    parameters:
        @display("i=block/socket");
    gates:
        input trafficIn;
        output trafficOut;
        input socketIn;
        output socketOut;
}
