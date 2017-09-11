//
// Created by haoba on 17-7-28.
//

#ifndef TKDATABASE_PROTO_H
#define TKDATABASE_PROTO_H

#ifdef __cplusplus
extern "C" {
#endif

/* predefined xid's values recognized as special by the server */
#define WATCHER_EVENT_XID -1
#define PING_XID -2
#define AUTH_XID -4
#define SET_WATCHES_XID -8

/** zookeeper return constants **/

enum ZOO_ERRORS {
    ZOK = 0, /*!< Everything is OK */

    /** System and server-side errors.
     * This is never thrown by the server, it shouldn't be used other than
     * to indicate a range. Specifically error codes greater than this
     * value, but lesser than {@link #ZAPIERROR}, are system errors. */
            ZSYSTEMERROR = -1,
    ZRUNTIMEINCONSISTENCY = -2, /*!< A runtime inconsistency was found */
    ZDATAINCONSISTENCY = -3, /*!< A data inconsistency was found */
    ZCONNECTIONLOSS = -4, /*!< Connection to the server has been lost */
    ZMARSHALLINGERROR = -5, /*!< Error while marshalling or unmarshalling data */
    ZUNIMPLEMENTED = -6, /*!< Operation is unimplemented */
    ZOPERATIONTIMEOUT = -7, /*!< Operation timeout */
    ZBADARGUMENTS = -8, /*!< Invalid arguments */
    ZINVALIDSTATE = -9, /*!< Invliad zhandle state */

    /** API errors.
     * This is never thrown by the server, it shouldn't be used other than
     * to indicate a range. Specifically error codes greater than this
     * value are API errors (while values less than this indicate a
     * {@link #ZSYSTEMERROR}).
     */
            ZAPIERROR = -100,
    ZNONODE = -101, /*!< Node does not exist */
    ZNOAUTH = -102, /*!< Not authenticated */
    ZBADVERSION = -103, /*!< Version conflict */
    ZNOCHILDRENFOREPHEMERALS = -108, /*!< Ephemeral nodes may not have children */
    ZNODEEXISTS = -110, /*!< The node already exists */
    ZNOTEMPTY = -111, /*!< The node has children */
    ZSESSIONEXPIRED = -112, /*!< The session has been expired by the server */
    ZINVALIDCALLBACK = -113, /*!< Invalid callback specified */
    ZINVALIDACL = -114, /*!< Invalid ACL specified */
    ZAUTHFAILED = -115, /*!< Client authentication failed */
    ZCLOSING = -116, /*!< ZooKeeper is closing */
    ZNOTHING = -117, /*!< (not error) no server responses to process */
    ZSESSIONMOVED = -118 /*!<session moved to another server, so operation is ignored */
};

enum CreateMode{
    PERSISTENT = 0,
    EPHEMERAL = 1,
    PERSISTENT_SEQUENTIAL = 2,
    EPHEMERAL_SEQUENTIAL = 3
};

enum KeeperState{
/* zookeeper state constants */
    ZOO_EXPIRED_SESSION_STATE = -112,
    ZOO_AUTH_FAILED_STATE = -113,
    ZOO_CONNECTING_STATE = 1,
    ZOO_ASSOCIATING_STATE = 2,
    ZOO_CONNECTED_STATE = 3,
    ZOO_NOTCONNECTED_STATE = 999
};

enum ZOO_EVENT_TYPE{
    /* zookeeper event type constants */
    ZOO_CREATED_EVENT = 1,
    ZOO_DELETED_EVENT = 2,
    ZOO_CHANGED_EVENT = 3,
    ZOO_CHILD_EVENT = 4,
    ZOO_SESSION_EVENT = -1,
    ZOO_NOTWATCHING_EVENT = -2
};

#define ZOO_NOTIFY_OP 0
#define ZOO_CREATE_OP 1
#define ZOO_DELETE_OP 2
#define ZOO_EXISTS_OP 3
#define ZOO_GETDATA_OP 4
#define ZOO_SETDATA_OP 5
#define ZOO_GETACL_OP 6
#define ZOO_SETACL_OP 7
#define ZOO_GETCHILDREN_OP 8
#define ZOO_SYNC_OP 9
#define ZOO_PING_OP 11
#define ZOO_GETCHILDREN2_OP 12
#define ZOO_CHECK_OP 13
#define ZOO_MULTI_OP 14
#define ZOO_CLOSE_OP -11
#define ZOO_SETAUTH_OP 100
#define ZOO_SETWATCHES_OP 101
#define ZOO_SASL_OP = 102
#define ZOO_CREATESESSION_OP = -10




#ifdef __cplusplus
}
#endif

#endif //TKDATABASE_PROTO_H
