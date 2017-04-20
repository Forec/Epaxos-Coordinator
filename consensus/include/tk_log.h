//
// Created by jingle on 17-3-14.
//

#ifndef TKDATABASE_TK_LOG_H
#define TKDATABASE_TK_LOG_H


#include <stdlib.h>
#include <string.h>
#include <tk_config.h>
#include <stdint.h>
#include "tk_config.h"
#include "tk_consensus.h"
#include "debug.h"
#include "tk_elog.h"
#define NOOP 0
#define CSM 1
#define CONFIG 2
#define HEAD 3

extern int prev_log_entry_head;

struct tk_log_entry{

    uint64_t idx;
    uint64_t term;
    uint64_t req_id;    /*The Request ID of the client */
    uint64_t clt_id;   /*LID of the client*/
    uint8_t  type;      /*CSM, CONFIG, NOOP, HEAD*/
    union {
        tk_command_t cmd;
        tk_cid_t cid;
        uint64_t head;
    }data;  /* The entry data */

};

typedef struct tk_log_entry tk_log_entry_t;


struct tk_log_entry_determiant {

    uint64_t idx;
    uint64_t term;
    uint64_t offset;
};

typedef struct tk_log_entry_determiant tk_log_entry_det_t;


#define MAX_NC_ENTRIES 1024 /* this implies 24 kb of memory */
struct tk_nc_buf_t {
    uint64_t len;
    tk_log_entry_det_t entries[MAX_NC_ENTRIES];
};
typedef struct tk_nc_buf_t tk_nc_buf_t;

struct log_offsets_t {
    uint64_t head;
    uint64_t apply;
    uint64_t commit;
    uint64_t end;
};
typedef struct log_offsets_t log_offsets_t;

/* The log (a circular buffer) used to replicate SM operations */
#define LOG_SIZE  16384*PAGE_SIZE
struct tk_log_t
{
    uint64_t head;  /* offset of the first entry;
                    minimum applied index in the cluster */
    uint64_t apply;  /* offset of the first not applied entry
                    the server applies all entries from apply to commit */
    uint64_t commit; /* offset of the first not committed entry
                    the leader overlaps all entries from commit to end of
                    its own log */
    uint64_t end;  /* offset after the last entry;
                    if end==len the buffer is empty;
                    if end==head the buffer is full */
    uint64_t tail;  /* offset of the last entry
                    Note: tail + sizeof(last_entry) == end */

    uint64_t len;

    /* Special buffer that stores the determinant of not committed log
     * entries */
    tk_nc_buf_t nc_buf[MAX_SERVER_COUNT];

    uint8_t entries[0];
};
typedef struct tk_log_t tk_log_t;

/* Snapshot of a generic SM */
#define PREREG_SNAPSHOT_SIZE 128*PAGE_SIZE
struct snapshot_t {
    tk_log_entry_det_t last_entry;    /* The last applied entry */
    uint32_t len;                       /* Length of data */
    char data[0];                       /* SM specific data */
};
typedef struct snapshot_t snapshot_t;

/* ================================================================== */
/* Static functions to handle the log */

/**
 * Create new log
 */
static tk_log_t*
log_new()
{
    tk_log_t* log = (tk_log_t*)malloc(sizeof(tk_log_t)+LOG_SIZE);
    if (NULL == log) {
        error(log_fp, "Cannot allocate log memory\n");
        return NULL;
    }
    /* Initialize log offsets */
    memset(log, 0, sizeof(tk_log_t)+LOG_SIZE);
    log->len  = LOG_SIZE;
    log->end  = log->len;
    log->tail = log->len;

    return log;
}

/**
 * Free log
 */
static void
log_free(tk_log_t* log )
{
    if (NULL != log) {
        free(log);
        log = NULL;
    }
}

/* ================================================================== */

/**
 * Check if the log is empty
 * Note: once the end is modified, it will always return FALSE
 * ! safe over RDMA
 */
static inline int
is_log_empty(tk_log_t* log )
{
    return (log->end == log->len);
}

/**
 * Check if the log is full
 * Note: the leader should make sure that this never happens
 */
static inline int
is_log_full(tk_log_t* log )
{
    return (log->end == log->head);
}

/**
 * Check if there are not committed entries
 * Note: called only by the leader
 * ! safe over RDMA
 */
static inline int
not_committed_entries(tk_log_t* log )
{
    return (!is_log_empty(log) && (log->commit != log->end));
}

/**
 * Check if there are not applied entries
 * Note: called only by the leader
 * ! safe over RDMA
 */
static inline int
not_applied_entries( tk_log_t* log )
{
    return (!is_log_empty(log) && (log->apply != log->commit));
}

/**
 * Check if an entry without data fits between the specified offset
 * and the buffer's end
 * ! safe over RDMA
 */
static inline int
log_fit_entry_header(tk_log_t* log, uint64_t offset )
{
    return (log->len - offset >= sizeof(tk_log_entry_t));
}

/**
 * Add a new entry at the end of the log
 * Note: called only by the leader
 * ! safe over RDMA
 * @return the new added entry
 */
static inline tk_log_entry_t*
log_add_new_entry(tk_log_t* log )
{
    if (is_log_full(log)) return NULL;
    if (is_log_empty(log) || !log_fit_entry_header(log, log->end)) {
        return (tk_log_entry_t*)(log->entries);
    }
    return (tk_log_entry_t*)(log->entries + log->end);
}


/**
 * Get the length of an entry
 * ! safe over RDMA
 */
static inline uint32_t
log_entry_len( tk_log_entry_t* entry )
{
    if (entry->type != CSM)
        return (uint32_t)sizeof(tk_log_entry_t);
    return (uint32_t)(sizeof(tk_log_entry_t) + entry->data.cmd.len);
}

/**
 * Check if an entry with data fits between the specified offset and
 * the buffer end
 * ! safe over RDMA
 */
static inline int
log_fit_entry( tk_log_t* log,
               uint64_t offset,
               tk_log_entry_t* entry )
{
    return (log->len - offset >= log_entry_len(entry));
}

/**
 * Get the distance between an offset and the end offset
 * in the circular buffer
 * Note: called by all servers
 * ! safe over RDMA
 */
static inline uint64_t
log_offset_end_distance( tk_log_t* log, uint64_t offset )
{
    uint64_t end = log->end;    // to avoid concurrency
    if (end == log->len) return 0;
    if (end >= offset) return (end - offset);
    return (log->len - (offset - end));
}

/**
 * Compare two offsets (the larger offset is closer to the end)
 * Note: called by all servers
 * ! safe over RDMA
 */
static int
log_is_offset_larger( tk_log_t* log,
                      uint64_t loffset,
                      uint64_t roffset )
{
    uint64_t end = log->end;    // to avoid concurrency
    uint64_t a = ( end == log->len ? 0 :
                   (end >= loffset ? end - loffset :
                    log->len - (loffset - end)) );
    uint64_t b = ( end == log->len ? 0 :
                   (end >= roffset ? end - roffset :
                    log->len - (roffset - end)) );
    return a < b;
}


#define TEXT_PRINT_LOG(stream, log_ptr) \
    text(stream, "#LOG [len=%"PRIu64"]: " \
                 "head=%"PRIu64 \
                 "; apply=%"PRIu64 \
                 "; commit=%"PRIu64 \
                 "; tail=%"PRIu64 \
                 "; end=%"PRIu64"\n", \
                 (log_ptr)->len, (log_ptr)->head, \
                 (log_ptr)->apply, (log_ptr)->commit,\
                 (log_ptr)->tail, (log_ptr)->end);
#define INFO_PRINT_LOG(stream, log_ptr) \
    info(stream, "LOG [%"PRIu64"]: " \
                 "h=%"PRIu64 \
                 "; a=%"PRIu64 \
                 "; c=%"PRIu64 \
                 "; t=%"PRIu64 \
                 "; e=%"PRIu64"\n", \
                 (log_ptr)->len, (log_ptr)->head, \
                 (log_ptr)->apply, (log_ptr)->commit,\
                 (log_ptr)->tail, (log_ptr)->end);



/* ================================================================== */

/**
 * Get an existing entry at a certain offset;
 * called by all servers
 * Note: the offset should not be changed remotely
 * ! safe over RDMA
 */
static tk_log_entry_t*
log_get_entry(tk_log_t* log, uint64_t *offset )
{
    if (is_log_empty(log)) {
        /* Log is empty */
        return NULL;
    }
    if (0 == log_offset_end_distance(log, *offset)) {
        /* The offset is at the end of the log */
        return NULL;
    }
    if (!log_fit_entry_header(log, *offset)) {
        /* The entry starts from the beginning */
        *offset = 0;
    }
    return (tk_log_entry_t*)(log->entries + *offset);
}

/**
 * Create log entry determinants for all not committed log entries
 * Note: called only with exclusive access to local log
 * ! safe over RDMA
 */
static void
log_entries_to_nc_buf( tk_log_t* log, tk_nc_buf_t* nc_buf )
{
    tk_log_entry_t *entry;
    uint64_t offset = log->commit;
    uint64_t len = 0;

    while ( (entry = log_get_entry(log, &offset)) != NULL ) {
        nc_buf->entries[len].idx = entry->idx;
        nc_buf->entries[len].term = entry->term;
        nc_buf->entries[len].offset = offset;
        len++;
        if (!log_fit_entry(log, offset, entry)) {
            /* Not enough place for an entry (with the command); that
             * means the log entry continues on the other side */
            offset = 0;
        }
        offset += log_entry_len(entry);
    }
    nc_buf->len = len;
}

/**
 * Seach for the last matching entry between local log and a remote NC-Buffer;
 * used during log adjustment to adjust the remote end offset;
 * called only by the leader
 * ! safe over RDMA
 */
static uint64_t
log_find_remote_end_offset( tk_log_t* log, tk_nc_buf_t* nc_buf )
{
    uint64_t i;
    tk_log_entry_t *entry;
    uint64_t offset;

    for (i = 0; i < nc_buf->len; i++) {
        offset = nc_buf->entries[i].offset;
        entry = log_get_entry(log, &offset);
        if (NULL == entry) {
            /* No more local entries */
            return offset;
        }
        if ( (entry->idx != nc_buf->entries[i].idx) ||
             (entry->term != nc_buf->entries[i].term) )
        {
            return offset;
        }
        if (!log_fit_entry(log, offset, entry)) {
            /* Not enough place for an entry (with the command); that
             * means the log entry continues on the other side */
            offset = 0;
        }
        offset += log_entry_len(entry);
    }
    return offset;
}

/**
 * Get the offset of the last entry;
 * called only by the leader or servers with exclusive access
 * to local log
 * ! safe over RDMA
 */
static uint64_t
log_get_tail(tk_log_t* log )
{
    if (log->tail != log->len) {
        return log->tail;
    }
    if (is_log_empty(log)) {
        return log->len;
    }

    tk_log_entry_t* entry;
    uint64_t offset, tail = log->len;

    /* Try to find the tail starting from the commit offset */
    offset = log->commit;
    while ( (entry = log_get_entry(log, &offset)) != NULL ) {
        tail = offset;
        if (!log_fit_entry(log, offset, entry)) {
            /* Not enough place for an entry (with the command); that
             * means the log entry continues on the other side */
            offset = 0;
        }
        offset += log_entry_len(entry);
    }
    if (tail != log->len) {
        return tail;
    }

    /* Try to find the tail starting from the apply offset */
    offset = log->apply;
    while ( (entry = log_get_entry(log, &offset)) != NULL ) {
        tail = offset;
        if (!log_fit_entry(log, offset, entry)) {
            /* Not enough place for an entry (with the command); that
             * means the log entry continues on the other side */
            offset = 0;
        }
        offset += log_entry_len(entry);
    }
    if (tail != log->len) {
        return tail;
    }

    /* Try to find the tail starting from the head offset */
    offset = log->head;
    while ( (entry = log_get_entry(log, &offset)) != NULL ) {
        tail = offset;
        if (!log_fit_entry(log, offset, entry)) {
            /* Not enough place for an entry (with the command); that
             * means the log entry continues on the other side */
            offset = 0;
        }
        offset += log_entry_len(entry);
    }
    return tail;
}

/**
 * Append an entry to the local log;
 * called only by the leader
 * as a side effect, the leader stores the tail offset;
 * note that the tail offset is reset when losing leadership
 * ! safe over RDMA
 */
static uint64_t
log_append_entry(tk_log_t* log,
                  uint64_t term,
                  uint64_t req_id,
                  uint16_t clt_id,
                  uint8_t  type,
                  void *data )
{
    tk_command_t *cmd = (tk_command_t*)data;
    tk_cid_t *cid = (tk_cid_t*)data; //for vote.
    uint64_t *head = (uint64_t*)data; //for snapshot.
    if (type != HEAD) {
        /* Avoid double HEAD */
        prev_log_entry_head = 0;
    }

    /* Compute new index */
    if (log->tail == log->len) {
        log->tail = log_get_tail(log);
    }
    uint64_t offset = log->tail;
    tk_log_entry_t *last_entry = log_get_entry(log, &offset);
    uint64_t idx = last_entry ? last_entry->idx + 1 : 1;

    /* Create new entry */
    tk_log_entry_t *entry = log_add_new_entry(log);
    if (!entry) {
        info_wtime(log_fp, "The LOG is full\n");
        return 0;
    }
    entry->idx     = idx;
    entry->term    = term;
    entry->req_id  = req_id;
    entry->clt_id  = clt_id;
    entry->type    = type;
    if (!log_fit_entry_header(log, log->end)) {
        log->end = 0;
    }

    /* Add data of the new entry */
    switch(type) {
        case CSM:
        {
//info(log_fp, "### add log entry CSM\n");
            entry->data.cmd.len = cmd->len;
            if (!log_fit_entry(log, log->end, entry)) {
                /* Not enough place for an entry (with the command) */
                log->end = 0;
                entry = log_add_new_entry(log);
                if (!entry) {
                    info_wtime(log_fp, "The LOG is full\n");
                    return 0;
                }
                entry->idx          = idx;
                entry->term         = term;
                entry->req_id       = req_id;
                entry->clt_id       = clt_id;
                entry->type         = type;
                entry->data.cmd.len = cmd->len;
            }
            /* Copy the command */
            if (cmd->len) {
                memcpy(entry->data.cmd.cmd, cmd->cmd, entry->data.cmd.len);
            }
            break;
        }
        case CONFIG:
//info(log_fp, "### add log entry CONFIG\n");
            entry->data.cid = *cid;
            break;
        case HEAD:
//info(log_fp, "### add log entry HEAD\n");
            entry->data.head = *head;
            break;
        case NOOP:
//info(log_fp, "### add log entry NOOP\n");
            break;
    }
    /* Set new tail (offset of last entry) */
    log->tail = log->end;
    /* Set new end */
    log->end += log_entry_len(entry);

    text(log_fp, "APPENDED ENTRY [%s]: ",
         (entry->type == CSM) ? "CSM" :
         (entry->type == CONFIG) ? "CONFIG" :
         (entry->type == HEAD) ? "HEAD" : "NOOP");
    TEXT_PRINT_LOG(log_fp, log);

    return idx;
}




#endif //TKDATABASE_TK_LOG_H
