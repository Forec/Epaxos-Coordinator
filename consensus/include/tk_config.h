//
// Created by jingle on 17-2-14.
//

#ifndef TKDATABASE_TK_CONFIG_H
#define TKDATABASE_TK_CONFIG_H

#include <tk_consensus.h>
#include <cstdint>


/*Stable configuration: Only one size specified */
#define CID_STABLE 0
#define CID_TRANSIT 1
#define CID_EXTENDED 2
#define CID_IS_SERVER_ON(cid, idx) ((cid).bitmask & (1 << (idx)))
#define CID_SERVER_ADD(cid, idx)   (cid).bitmask |= 1 << (idx)
#define CID_SERVER_RM(cid, idx)    (cid).bitmask &= ~(1 << (idx))


struct tk_cid_t {
    uint64_t  epoch;
    uint8_t  size[2];
    uint8_t  state;
    uint8_t  pad[1];
    uint32_t  bitmask;
};


typedef struct tk_cid_t tk_cid_t;

static int
equal_cid(tk_cid_t left_cid, tk_cid_t right_cid)
{
    if(left_cid.epoch != right_cid.epoch) return 0;
    if(left_cid.state != right_cid.state) return 0;
    if(left_cid.bitmask != right_cid.bitmask) return 0;
    if(left_cid.size[0] != right_cid.size[0]) return 0;
    if(left_cid.size[1] != right_cid.size[1]) return 0;
    return 1;
}

typedef struct server_t server_t;

struct server_config_t {
    tk_cid_t cid;
    uint64_t cid_offset;

    uint64_t cid_idx;

    uint64_t req_id;

    server_t *servers;

    uint16_t clt_cid;

    uint8_t  idx;

    uint8_t  len;
};

/* Get the maximum size including the extra added servers */

static uint8_t
get_extended_group_size (server_config_t config)
{
    if(CID_STABLE == config.cid.state)
        return config.cid.size[0];
    if(config.cid.size[0] < config.cid.size[1])
        return config.cid.size[1];

    return config.cid.size[0];
}

/* Get the maximum size ignoring the extra added servers */

static uint8_t
get_group_size(server_config_t config)
{
    if(CID_TRANSIT != config.cid.state)
        return config.cid.size[0];
    if(config.cid.size[0] < config.cid.size[1])
        return config.cid.size[1];

    return config.cid.size[0];
}

#define PRINT_CID(cid) text(log_fp,     \
    " [E%"PRIu64":%02"PRIu8"|%02"PRIu8"|%d|%03"PRIu32"] ", \
    (cid).epoch, (cid).size[0], (cid).size[1], (cid).state, (cid).bitmask)
#define PRINT_CID_(cid) PRINT_CID(cid); text(log_fp, "\n");

#define PRINT_CONF_TRANSIT(old_cid, new_cid) \
    info_wtime(log_fp, "(%s:%d) Configuration transition: " \
        "[E%"PRIu64":%02"PRIu8"|%02"PRIu8"|%d|%03"PRIu32"] -> " \
        "[E%"PRIu64":%02"PRIu8"|%02"PRIu8"|%d|%03"PRIu32"]\n", \
        __func__, __LINE__, \
        (old_cid).epoch, (old_cid).size[0], (old_cid).size[1], \
        (old_cid).state, (old_cid).bitmask, \
        (new_cid).epoch, (new_cid).size[0], (new_cid).size[1], \
        (new_cid).state, (new_cid).bitmask)


#endif //TKDATABASE_TK_CONFIG_H
