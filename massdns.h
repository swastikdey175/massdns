#ifndef MASSDNS_MASSDNS_H
#define MASSDNS_MASSDNS_H

#include <stdint.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#define PCAP_SUPPORT
#ifdef PCAP_SUPPORT
#include <pcap.h>
#endif

#include "list.h"
#include "module.h"
#include "net.h"
#include "hashmap.h"
#include "dns.h"
#include "timed_ring.h"

#define MAXIMUM_MODULE_COUNT 0xFF
#define COMMON_UNPRIVILEGED_USER "nobody"

const uint32_t OUTPUT_BINARY_VERSION = 0x00;

typedef struct
{
    size_t answers;
    size_t noerr;
    size_t formerr;
    size_t servfail;
    size_t nxdomain;
    size_t notimp;
    size_t refused;
    size_t yxdomain;
    size_t yxrrset;
    size_t nxrrset;
    size_t notauth;
    size_t notzone;
    size_t timeout;
    size_t mismatch;
    size_t other;
    size_t qsent;
    size_t numreplies;
    size_t fakereplies; // used for resolver plausibility checks (wrong records)
} resolver_stats_t;

typedef struct {
    size_t fork_index;
    size_t numdomains;
    size_t numreplies;
    size_t finished;
    size_t finished_success;
    size_t mismatch_domain;
    size_t mismatch_id;
    size_t timeouts[0x100];
    size_t all_rcodes[5];
    size_t final_rcodes[5];
    size_t current_rate;
    size_t numparsed;
} stats_exchange_t;

typedef struct
{
    struct sockaddr_storage address;
    resolver_stats_t stats; // To be used to track resolver bans or non-replying resolvers
} resolver_t;

typedef struct
{
    char *domain;
    dns_record_type type;
} lookup_key_t;

typedef struct
{
    unsigned char tries;
    uint16_t transaction;
    void **ring_entry; // pointer to the entry within the timed ring for entry invalidation
    resolver_t *resolver;
    lookup_key_t *key;
} lookup_t;

typedef enum
{
    STATE_WARMUP, // Before the hash map size has been reached
    STATE_QUERYING,
    STATE_COOLDOWN,
    STATE_DONE
} state_t;

typedef enum
{
    OUTPUT_TEXT_FULL,
    OUTPUT_TEXT_SIMPLE,
    OUTPUT_BINARY
} output_t;

const char *default_interfaces[] = {""};

typedef struct
{
    buffer_t resolvers;

    struct
    {
        massdns_module_t handlers[MAXIMUM_MODULE_COUNT]; // we only support up to 255 modules
        size_t count;
    } modules;

    struct cmd_args
    {
        bool root;
        char *resolvers;
        char *domains;
        uint8_t resolve_count;
        size_t hashmap_size;
        unsigned int interval_ms;
        bool norecurse;
        bool finalstats;
        bool quiet;
        int sndbuf;
        int rcvbuf;
        char *drop_user;
        dns_record_type record_type;
        size_t timed_ring_buckets;
        int extreme; // Do not remove EPOLLOUT after warmup
        output_t output;
        bool retry_codes[0xFFFF]; // Fast lookup map for DNS reply codes that are unacceptable and require a retry
        bool retry_codes_set;
        single_list_t bind_addrs4;
        single_list_t bind_addrs6;
        bool sticky;
        int argc;
        char **argv;
        void (*help_function)();
        bool flush;
        bool predictable_resolver;
        bool use_pcap;
        size_t num_processes;
    } cmd_args;

    struct
    {
        buffer_t interfaces4; // Sockets used for receiving queries
        buffer_t interfaces6; // Sockets used for receiving queries
        buffer_t queries; // Sockets used for sending out queries
        int *pipes;
        socket_info_t read_pipe;
        socket_info_t write_pipe;
        socket_info_t *master_pipes_read;
    } sockets;

    FILE* outfile;
    FILE* logfile;
    FILE* domainfile;
    ssize_t domainfile_size;
    int epollfd;
    Hashmap *map;
    state_t state;
    timed_ring_t ring; // handles timeouts
    size_t lookup_index;
    size_t fork_index;
    struct
    {
        struct timespec start_time;
        size_t mismatch;
        size_t other;
        size_t qsent;
        size_t numreplies;
        size_t numparsed;
        size_t numdomains;
        struct timespec last_print;
        size_t current_rate;
        size_t timeouts[0x100];
        size_t final_rcodes[0x10000];
        size_t all_rcodes[0x10000];
        size_t finished;
        size_t finished_success;
        size_t mismatch_id;
        size_t mismatch_domain;
    } stats;
    stats_exchange_t *stat_messages;
#ifdef PCAP_SUPPORT
    pcap_t *pcap;
    char pcap_error[PCAP_ERRBUF_SIZE];
    char *pcap_dev;
    socket_info_t pcap_info;
    uint16_t ether_type_ip;
    uint16_t ether_type_ip6;
    struct bpf_program pcap_filter;
#endif
} massdns_context_t;

massdns_context_t context;

#endif //MASSDNS_MASSDNS_H
