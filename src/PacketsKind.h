//
// Created by wld on 17.01.23.
//

#ifndef E_VOTING_PACKETSKIND_H
#define E_VOTING_PACKETSKIND_H

enum SCHEDULE_STATE {
    SELF_MSGKIND_CONNECT_REQUEST,                   // SELF_REQ
    SELF_MSGKIND_CONNECT_REPLY,                     // SELF_REPLY
    SELF_MSGKIND_LISTEN,                            // SELF_LISTEN
    SELF_MSGKIND_CLOSE,                             // SELF_CLOSE
    SELF_MSGKIND_INIT_SYNC,                         // SELF_INIT_SYNC
    SELF_MSGKIND_FORWARD_SYNC_UP,                   // SELF_FORWARD
    SELF_MSGKIND_LISTEN_SYNC_UP,                    // SELF_LISTEN_UP
    SELF_MSGKIND_LISTEN_SYNC_DOWN,                  // SELF_LISTEN_DOWN
    SELF_MSGKIND_RETURN_SYNC_DOWN,                  // SELF_RETURN_DOWN
    SELF_MSGKIND_CLOSE_SYNC_SOCKET,                 // SELF_CLOSE_SYNC
    SELF_MSGKIND_DISTR_PORTS_SETUP_RECEIVE,         // SELF_PORTS_REC
    SELF_MSGKIND_DISTR_PORTS_SETUP_FORWARD,         // SELF_PORTS_FORWARD
    SELF_MSGKIND_DISTR_INITIAL_DIRECTION_SEND,      // SELF_DIR_SEND
    SELF_MSGKIND_DISTR_INITIAL_PORTS_SEND_UP,       // SELF_PORTS_UP
    SELF_MSGKIND_DISTR_INITIAL_PORTS_SEND_DOWN,     // SELF_PORTS_DOWN
    SELF_MSGKIND_DISTR_DIRECTION_FORWARD,           // SELF_DIR_FORWARD
    SELF_MSGKIND_DISTR_HOPS_SEND,                   // SELF_HOPS_SEND
    SELF_MSGKIND_CREATE_ELECTION,                   // SELF_C_ELECTION
    SELF_MSGKIND_PLACE_VOTE,                        // SELF_PLACE_VOTE
    SELF_MSGKIND_DISTR_PUBLISH,                     // SELF_PUBLISH
    SELF_MSGKIND_DISTR_SUBSCRIBE,                   // SELF_SUBSCRIBE
    SELF_MSGKIND_CLOSE_SUBSCRIBE_SOCKET,            // SELF_CLOSE_SUB    
    SELF_MSGKIND_CLOSE_PUBLISH_SOCKET,              // SELF_CLOSE_PUB
    SELF_MSGKIND_TALLY,                             // SELF_TALLY
    SELF_MSGKIND_REQUEST_KEY,                       // SELF_REQ_KEY
    SELF_MSGKIND_REQUEST_KEYS,                      // SELF_REQ_KEYS
    SELF_MSGKIND_CONFIRM,                           // SELF_CONFIRM
    SELF_MSGKIND_CLOSE_SETUP_SOCKET                 // SELF_SETUP_CLOSE
};

enum APP_PACKET_KIND {
    APP_CONN_REQUEST,                               // APP_CONN_REQ
    APP_CONN_REPLY,                                 // APP_CONN_REP
    APP_SYNC_REQUEST,                               // APP_SYNC_REQ
    APP_SYNC_REPLY,                                 // APP_SYNC_REP
    APP_SYNC_RETURN,                                // APP_SYNC_RETURN
    APP_DISTR_PORTS_SETUP_REQUEST,                  // APP_PORTS_REQ
    APP_DISTR_PORTS_SETUP_RESPONSE,                 // APP_PORTS_REP
    APP_DISTR_PRE_PUBLISH_HOPS_REQUEST,             // APP_HOPS_REQ
    APP_DISTR_PRE_PUBLISH_DIRECTION_REQUEST,        // APP_DIR_REQ
    APP_DISTR_PRE_PUBLISH_HOPS_RESPONSE,            // APP_HOPS_REP
    APP_DISTR_PRE_PUBLISH_DIRECTION_RESPONSE,       // APP_DIR_REP
    APP_DISTR_PUBLISH,                              // APP_PUB
    APP_REQUEST_KEY,                                // APP_KEY_REQ
    APP_RESPONSE_KEY,                               // APP_KEY_REP
    APP_RESPONSE_KEY_CLOSE                          // APP_KEY_CLOSE
};

class VotingAppTag : public omnetpp::cObject {
    public: APP_PACKET_KIND kind;
};

#endif //E_VOTING_PACKETSKIND_H
