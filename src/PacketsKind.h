//
// Created by wld on 17.01.23.
//

#ifndef E_VOTING_PACKETSKIND_H
#define E_VOTING_PACKETSKIND_H

enum SCHEDULE_STATE {
    SELF_MSGKIND_CONNECT,
    SELF_MSGKIND_SEND,
    SELF_MSGKIND_LISTEN,
    SELF_MSGKIND_LISTEN_END,
    SELF_MSGKIND_CLOSE,
    SELF_MSGKIND_INIT_SYNC,
    SELF_MSGKIND_FORWARD_SYNC_UP,
    SELF_MSGKIND_LISTEN_SYNC_UP,
    SELF_MSGKIND_LISTEN_SYNC_DOWN,
    SELF_MSGKIND_RETURN_SYNC_DOWN,
    SELF_MSGKIND_DISTR_PORTS_SETUP_RECEIVE,
    SELF_MSGKIND_DISTR_PORTS_SETUP_FORWARD,
    SELF_MSGKIND_DISTR_INITIAL_DIRECTION_SEND,
    SELF_MSGKIND_DISTR_DIRECTION_FORWARD,
    SELF_MSGKIND_DISTR_HOPS_SEND,
    SELF_MSGKIND_DISTR_HOPS_RECEIVE,
    SELF_MSGKIND_CREATE_ELECTION,
    SELF_MSGKIND_PLACE_VOTE,
    SELF_MSGKIND_DISTR_PUBLISH,
    SELF_MSGKIND_DISTR_SUBSCRIBE,
    SELF_MSGKIND_CLOSE_SUBSCRIBE_SOCKET,
    SELF_MSGKIND_CLOSE_PUBLISH_SOCKET,
    SELF_MSGKIND_TALLY,
    SELF_MSGKIND_REQUEST_KEY,
    SELF_MSGKIND_REQUEST_KEYS,
    SELF_MSGKIND_CONFIRM,
    SELF_MSGKIND_CLOSE_SYNC_SOCKET
};

enum APP_PACKET_KIND {
    APP_CONN_REQUEST,
    APP_CONN_REPLY,
    APP_SYNC_REQUEST,
    APP_SYNC_REPLY,
    APP_SYNC_RETURN,
    APP_DISTR_PORTS_SETUP_REQUEST,
    APP_DISTR_PORTS_SETUP_RESPONSE,
    APP_DISTR_PRE_PUBLISH_HOPS_REQUEST,
    APP_DISTR_PRE_PUBLISH_DIRECTION_REQUEST,
    APP_DISTR_PRE_PUBLISH_HOPS_RESPONSE,
    APP_DISTR_PRE_PUBLISH_DIRECTION_RESPONSE,
    APP_DISTR_PUBLISH,
    APP_REQUEST_KEY,
    APP_RESPONSE_KEY,
    APP_RESPONSE_KEY_CLOSE
};

class VotingAppTag : public omnetpp::cObject {
    public: APP_PACKET_KIND kind;
};

#endif //E_VOTING_PACKETSKIND_H
