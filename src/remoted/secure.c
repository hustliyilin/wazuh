/* Copyright (C) 2015, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#include "shared.h"
#include "os_net/os_net.h"
#include "remoted.h"
#include "wazuh_db/helpers/wdb_global_helpers.h"

#ifdef WAZUH_UNIT_TESTING
// Remove static qualifier when unit testing
#define STATIC
#else
#define STATIC static
#endif

/* Global variables */
int sender_pool;

netbuffer_t netbuffer_recv;
netbuffer_t netbuffer_send;

wnotify_t * notify = NULL;

size_t global_counter;

STATIC void handle_outgoing_data_to_tcp_socket(int sock_client);
STATIC void handle_incoming_data_from_tcp_socket(int sock_client);
STATIC void handle_incoming_data_from_udp_socket(struct sockaddr_storage * peer_info);
STATIC void handle_new_tcp_connection(wnotify_t * notify, struct sockaddr_storage * peer_info);

// Message handler thread
static void * rem_handler_main(__attribute__((unused)) void * args);

// Key reloader thread
void * rem_keyupdate_main(__attribute__((unused)) void * args);

/* Handle each message received */
STATIC void HandleSecureMessage(char *buffer, int recv_b, struct sockaddr_storage *peer_info, int sock_client, int *wdb_sock);

// Close and remove socket from keystore
int _close_sock(keystore * keys, int sock);

STATIC void * close_fp_main(void * args);

/* Status of keypolling wodle */
static char key_request_available = 0;

/* Decode hostinfo input queue */
static w_queue_t * key_request_queue;

/* Remote key request thread */
void * w_key_request_thread(__attribute__((unused)) void * args);

/* Push key request */
static void _push_request(const char *request,const char *type);
#define push_request(x, y) if (key_request_available) _push_request(x, y);

/* Connect to key polling wodle*/
#define KEY_RECONNECT_INTERVAL 300 // 5 minutes
static int key_request_connect();
static int key_request_reconnect();

/* Defines to switch according to different OS_AddSocket or, failing that, the case of using UDP protocol */
#define OS_ADDSOCKET_ERROR          0   ///< OSHash_Set_ex returns 0 on error (* see OS_AddSocket and OSHash_Set_ex)
#define OS_ADDSOCKET_KEY_UPDATED    1   ///< OSHash_Set_ex returns 1 when key existed, so it is update (*)
#define OS_ADDSOCKET_KEY_ADDED      2   ///< OSHash_Set_ex returns 2 when key didn't existed, so it is added  (*)
#define REMOTED_USING_UDP           42  ///< When using UDP, OS_AddSocket isn't called, so an arbitrary value is used

#define USING_UDP_NO_CLIENT_SOCKET  -1  ///< When using UDP, no valid client socket FD is set

/* Handle secure connections */
void HandleSecure()
{
    const int protocol = logr.proto[logr.position];
    int n_events = 0;

    struct sockaddr_storage peer_info;
    memset(&peer_info, 0, sizeof(struct sockaddr_storage));

    /* Initialize manager */
    manager_init();

    // Initialize messag equeue
    rem_msginit(logr.queue_size);

    /* Initialize the agent key table mutex */
    key_lock_init();

    /* Create shared file updating thread */
    w_create_thread(update_shared_files, NULL);

    /* Create Active Response forwarder thread */
    w_create_thread(AR_Forward, NULL);

    /* Create Security configuration assessment forwarder thread */
    w_create_thread(SCFGA_Forward, NULL);

    // Create Request listener thread
    w_create_thread(req_main, NULL);

    // Create State writer thread
    w_create_thread(rem_state_main, NULL);

    key_request_queue = queue_init(1024);

    // Create key request thread
    w_create_thread(w_key_request_thread, NULL);

    /* Create wait_for_msgs threads */
    {
        sender_pool = getDefine_Int("remoted", "sender_pool", 1, 64);

        mdebug2("Creating %d sender threads.", sender_pool);

        for (int i = 0; i < sender_pool; i++) {
            w_create_thread(wait_for_msgs, NULL);
        }
    }

    // Reset all the agents' connection status in Wazuh DB
    // The master will disconnect and alert the agents on its own DB. Thus, synchronization is not required.
    if (OS_SUCCESS != wdb_reset_agents_connection("synced", NULL))
        mwarn("Unable to reset the agents' connection status. Possible incorrect statuses until the agents get connected to the manager.");

    // Create message handler thread pool
    {
        int worker_pool = getDefine_Int("remoted", "worker_pool", 1, 16);
        // Initialize FD list and counter.
        global_counter = 0;
        rem_initList(FD_LIST_INIT_VALUE);
        while (worker_pool > 0) {
            w_create_thread(rem_handler_main, NULL);
            worker_pool--;
        }
    }

    /* Connect to the message queue
     * Exit if it fails.
     */
    if ((logr.m_queue = StartMQ(DEFAULTQUEUE, WRITE, INFINITE_OPENQ_ATTEMPTS)) < 0) {
        merror_exit(QUEUE_FATAL, DEFAULTQUEUE);
    }

    /* Read authentication keys */
    minfo(ENC_READ);

    key_lock_write();
    OS_ReadKeys(&keys, W_ENCRYPTION_KEY, 0);
    key_unlock();

    OS_StartCounter(&keys);

    // Key reloader thread
    w_create_thread(rem_keyupdate_main, NULL);

    // fp closer thread
    w_create_thread(close_fp_main, &keys);

    /* Set up peer size */
    logr.peer_size = sizeof(peer_info);

    /* Events watcher is started (is used to monitor sockets events) */
    if (notify = wnotify_init(MAX_EVENTS), !notify) {
        merror_exit("wnotify_init(): %s (%d)", strerror(errno), errno);
    }

    /* If TCP is set on the config, then the corresponding sockets is added to the watching list  */
    if (protocol & REMOTED_NET_PROTOCOL_TCP) {
        if (wnotify_add(notify, logr.tcp_sock, WO_READ) < 0) {
            merror_exit("wnotify_add(%d): %s (%d)", logr.tcp_sock, strerror(errno), errno);
        }
    }

    /* If UDP is set on the config, then the corresponding sockets is added to the watching list  */
    if (protocol & REMOTED_NET_PROTOCOL_UDP) {
        if (wnotify_add(notify, logr.udp_sock, WO_READ) < 0) {
            merror_exit("wnotify_add(%d): %s (%d)", logr.udp_sock, strerror(errno), errno);
        }
    }

    while (1) {

        /* It waits for a socket event */
        if (n_events = wnotify_wait(notify, EPOLL_MILLIS), n_events < 0) {
            if (errno != EINTR) {
                merror("Waiting for connection: %s (%d)", strerror(errno), errno);
                sleep(1);
            }

            continue;
        }

        for (int i = 0u; i < n_events; i++) {
            // Returns the fd of the socket that recived a message
            wevent_t event;
            int fd = wnotify_get(notify, i, &event);

            // In case of failure or unexpected file descriptor
            if (fd <= 0) {
                merror("Unexpected file descriptor: %d, %s (%d)", fd, strerror(errno), errno);
                continue;
            }
            // If a new TCP connection was received and TCP is enabled
            else if ((fd == logr.tcp_sock) && (protocol & REMOTED_NET_PROTOCOL_TCP)) {
                handle_new_tcp_connection(notify, &peer_info);
            }
            // If a new UDP connection was received and UDP is enabled
            else if ((fd == logr.udp_sock) && (protocol & REMOTED_NET_PROTOCOL_UDP)) {
                handle_incoming_data_from_udp_socket(&peer_info);
            }
            // If a message was received through a TCP client and tcp is enabled
            else if ((protocol & REMOTED_NET_PROTOCOL_TCP) && (event & WE_READ)) {
                handle_incoming_data_from_tcp_socket(fd);
            }
            // If a TCP client socket is ready for sending and tcp is enabled
            else if ((protocol & REMOTED_NET_PROTOCOL_TCP) && (event & WE_WRITE)) {
                handle_outgoing_data_to_tcp_socket(fd);
            }
        }
    }

    manager_free();
}

STATIC void handle_new_tcp_connection(wnotify_t * notify, struct sockaddr_storage * peer_info)
{
    int sock_client = accept(logr.tcp_sock, (struct sockaddr *) peer_info, &logr.peer_size);

    if (sock_client >= 0) {
        nb_open(&netbuffer_recv, sock_client, peer_info);
        nb_open(&netbuffer_send, sock_client, peer_info);

        rem_inc_tcp();

        mdebug1("New TCP connection [%d]", sock_client);

        if (wnotify_add(notify, sock_client, WO_READ) < 0) {
            merror("wnotify_add(%d, %d): %s (%d)", notify->fd, sock_client, strerror(errno), errno);
            _close_sock(&keys, sock_client);
        }
    } else {
        switch (errno) {
        case ECONNABORTED:
            mdebug1(ACCEPT_ERROR, strerror(errno), errno);
            break;
        default:
            merror(ACCEPT_ERROR, strerror(errno), errno);
        }
    }
}

STATIC void handle_incoming_data_from_udp_socket(struct sockaddr_storage * peer_info)
{
    char buffer[OS_MAXSTR + 1];
    memset(buffer, '\0', OS_MAXSTR + 1);

    int recv_b = recvfrom(logr.udp_sock, buffer, OS_MAXSTR, 0, (struct sockaddr *) peer_info, &logr.peer_size);

    if (recv_b > 0) {
        rem_msgpush(buffer, recv_b, peer_info, USING_UDP_NO_CLIENT_SOCKET);
        rem_add_recv((unsigned long) recv_b);
    }
}

STATIC void handle_incoming_data_from_tcp_socket(int sock_client)
{
    int recv_b = nb_recv(&netbuffer_recv, sock_client);

    switch (recv_b) {
    case -2:
        mwarn("Too big message size from socket [%d].", sock_client);
        _close_sock(&keys, sock_client);
        return;

    case -1:
        switch (errno) {
        case ECONNRESET:
        case ENOTCONN:
        case EAGAIN:
#if EAGAIN != EWOULDBLOCK
        case EWOULDBLOCK:
#endif
        case ETIMEDOUT:
            mdebug1("TCP peer [%d]: %s (%d)", sock_client, strerror(errno), errno);
            break;
        default:
            merror("TCP peer [%d]: %s (%d)", sock_client, strerror(errno), errno);
        }
        fallthrough;
    case 0:
        mdebug1("handle incoming close socket [%d].", sock_client);
        _close_sock(&keys, sock_client);
        return;

    default:
        rem_add_recv((unsigned long) recv_b);
    }
}

STATIC void handle_outgoing_data_to_tcp_socket(int sock_client)
{
    int sent_b = nb_send(&netbuffer_send, sock_client);

    switch (sent_b) {
    case -1:
        mdebug1("TCP peer [%d]: %s (%d)", sock_client, strerror(errno), errno);

        switch (errno) {
        case EAGAIN:
#if EAGAIN != EWOULDBLOCK
        case EWOULDBLOCK:
#endif
            break;
        case EPIPE:
        case EBADF:
        case ECONNRESET:
        default:
            mdebug1("handle outgoing close socket [%d].", sock_client);
            _close_sock(&keys, sock_client);
        }
        return;

    default:
        rem_add_send((unsigned long) sent_b);
    }
}

// Message handler thread
void * rem_handler_main(__attribute__((unused)) void * args) {
    message_t * message;
    char buffer[OS_MAXSTR + 1] = "";
    int wdb_sock = -1;
    mdebug1("Message handler thread started.");

    while (1) {
        message = rem_msgpop();
        if (message->sock == USING_UDP_NO_CLIENT_SOCKET || message->counter > rem_getCounter(message->sock)) {
            memcpy(buffer, message->buffer, message->size);
            HandleSecureMessage(buffer, message->size, &message->addr, message->sock, &wdb_sock);
        } else {
            rem_inc_dequeued();
        }
        rem_msgfree(message);
    }

    return NULL;
}

// Key reloader thread
void * rem_keyupdate_main(__attribute__((unused)) void * args) {
    int seconds;

    mdebug1("Key reloader thread started.");
    seconds = getDefine_Int("remoted", "keyupdate_interval", 1, 3600);

    while (1) {
        mdebug2("Checking for keys file changes.");
        check_keyupdate();
        sleep(seconds);
    }
}

// Closer rids thread
STATIC void * close_fp_main(void * args) {
    keystore * keys = (keystore *)args;
    int seconds;
    int flag;

    mdebug1("Rids closer thread started.");
    seconds = logr.rids_closing_time;

    while (1) {
        sleep(seconds);
        key_lock_write();
        flag = 1;
        while (flag) {
            w_linked_queue_node_t * first_node = keys->opened_fp_queue->first;
            mdebug2("Opened rids queue size: %d", keys->opened_fp_queue->elements);
            if (first_node) {
                int now = time(0);
                keyentry * first_node_key = (keyentry *)first_node->data;
                mdebug2("Checking rids_node of agent %s.", first_node_key->id);
                if ((now - seconds) > first_node_key->updating_time) {
                    first_node_key = (keyentry *)linked_queue_pop_ex(keys->opened_fp_queue);
                    w_mutex_lock(&first_node_key->mutex);
                    mdebug2("Pop rids_node of agent %s.", first_node_key->id);
                    if (first_node_key->fp != NULL) {
                        mdebug2("Closing rids for agent %s.", first_node_key->id);
                        fclose(first_node_key->fp);
                        first_node_key->fp = NULL;
                    }
                    first_node_key->updating_time = 0;
                    first_node_key->rids_node = NULL;
                    w_mutex_unlock(&first_node_key->mutex);
                } else {
                    flag = 0;
                }
            } else {
                flag = 0;
            }
        }
        key_unlock();
    #ifdef WAZUH_UNIT_TESTING
        break;
    #endif
    }
    return NULL;
}

STATIC void HandleSecureMessage(char *buffer, int recv_b, struct sockaddr_storage *peer_info, int sock_client, int *wdb_sock) {
    int agentid;
    const int protocol = (sock_client == USING_UDP_NO_CLIENT_SOCKET) ? REMOTED_NET_PROTOCOL_UDP : REMOTED_NET_PROTOCOL_TCP;
    char cleartext_msg[OS_MAXSTR + 1];
    char srcmsg[OS_FLSIZE + 1];
    char srcip[IPSIZE + 1] = {0};
    char agname[KEYSIZE + 1] = {0};
    char *tmp_msg;
    size_t msg_length;
    char ip_found = 0;
    int r;

    /* Set the source IP */
    switch (peer_info->ss_family) {
    case AF_INET:
        get_ipv4_string(((struct sockaddr_in *)peer_info)->sin_addr, srcip, IPSIZE);
        break;
    case AF_INET6:
        get_ipv6_string(((struct sockaddr_in6 *)peer_info)->sin6_addr, srcip, IPSIZE);
        break;
    default:
        merror("IP address family not supported.");
        return;
    }

    /* Initialize some variables */
    memset(cleartext_msg, '\0', OS_MAXSTR + 1);
    memset(srcmsg, '\0', OS_FLSIZE + 1);
    tmp_msg = NULL;

    /* Get a valid agent id */
    if (buffer[0] == '!') {
        tmp_msg = buffer;
        tmp_msg++;

        /* We need to make sure that we have a valid id
         * and that we reduce the recv buffer size
         */
        while (isdigit((int)*tmp_msg)) {
            tmp_msg++;
            recv_b--;
        }

        if (*tmp_msg != '!') {
            merror(ENCFORMAT_ERROR, "(unknown)", srcip);

            if (sock_client >= 0) {
                _close_sock(&keys, sock_client);
            }

            return;
        }

        *tmp_msg = '\0';
        tmp_msg++;
        recv_b -= 2;

        key_lock_read();
        agentid = OS_IsAllowedDynamicID(&keys, buffer + 1, srcip);

        if (agentid == -1) {
            int id = OS_IsAllowedID(&keys, buffer + 1);

            if (id < 0) {
                snprintf(agname, sizeof(agname), "unknown");
            } else {
                snprintf(agname, sizeof(agname), "%s", keys.keyentries[id]->name);
            }

            key_unlock();

            mwarn(ENC_IP_ERROR, buffer + 1, srcip, agname);

            // Send key request by id
            push_request(buffer + 1,"id");
            if (sock_client >= 0) {
                _close_sock(&keys, sock_client);
            }

            return;
        } else if ((keys.keyentries[agentid]->sock >= 0) && (keys.keyentries[agentid]->sock != sock_client)) {
            key_unlock();
            mwarn("Agent key already in use: agent ID '%s'", keys.keyentries[agentid]->id);

            if (sock_client >= 0) {
                _close_sock(&keys, sock_client);
            }
            return;
        }
    } else if (strncmp(buffer, "#ping", 5) == 0) {
            int retval = 0;
            char *msg = "#pong";
            ssize_t msg_size = strlen(msg);

            if (protocol == REMOTED_NET_PROTOCOL_UDP) {
                retval = sendto(logr.udp_sock, msg, msg_size, 0, (struct sockaddr *)peer_info, logr.peer_size) == msg_size ? 0 : -1;
            } else {
                retval = OS_SendSecureTCP(sock_client, msg_size, msg);
            }

            if (retval < 0) {
                mwarn("Ping operation could not be delivered completely (%d)", retval);
            }

            return;

    } else {
        key_lock_read();

        agentid = OS_IsAllowedIP(&keys, srcip);

        if (agentid < 0) {
            key_unlock();
            mwarn(DENYIP_WARN " Source agent ID is unknown.", srcip);

            // Send key request by ip
            push_request(srcip,"ip");
            if (sock_client >= 0) {
                _close_sock(&keys, sock_client);
            }

            return;
        } else if ((keys.keyentries[agentid]->sock >= 0) && (keys.keyentries[agentid]->sock != sock_client)) {
            key_unlock();
            mwarn("Agent key already in use: agent ID '%s'", keys.keyentries[agentid]->id);

            if (sock_client >= 0) {
                _close_sock(&keys, sock_client);
            }

            return;
        } else {
            ip_found = 1;
        }

        tmp_msg = buffer;
    }

    if (recv_b <= 0) {
        mwarn("Received message is empty");
        key_unlock();
        if (sock_client >= 0) {
            _close_sock(&keys, sock_client);
        }

        return;
    }

    /* Decrypt the message */
    if (r = ReadSecMSG(&keys, tmp_msg, cleartext_msg, agentid, recv_b - 1, &msg_length, srcip, &tmp_msg), r != KS_VALID) {
        /* If duplicated, a warning was already generated */
        key_unlock();

        if (r == KS_ENCKEY) {
            if (ip_found) {
                push_request(srcip,"ip");
            } else {
                push_request(buffer + 1, "id");
            }
        }

        if (sock_client >= 0) {
            mwarn("Decrypt the message fail, socket %d", sock_client);
            _close_sock(&keys, sock_client);
        }

        return;
    }

    /* Check if it is a control message */
    if (IsValidHeader(tmp_msg)) {

        /* We need to save the peerinfo if it is a control msg */

        keys.keyentries[agentid]->net_protocol = protocol;

        memcpy(&keys.keyentries[agentid]->peer_info, peer_info, logr.peer_size);
        keyentry * key = OS_DupKeyEntry(keys.keyentries[agentid]);
        r = (protocol == REMOTED_NET_PROTOCOL_TCP) ? OS_AddSocket(&keys, agentid, sock_client) : REMOTED_USING_UDP;
        keys.keyentries[agentid]->rcvd = time(0);

        switch (r) {
        case OS_ADDSOCKET_ERROR:
            merror("Couldn't add TCP socket to keystore.");
            break;
        case OS_ADDSOCKET_KEY_UPDATED:
            mdebug2("TCP socket %d already in keystore. Updating...", sock_client);
            break;
        case OS_ADDSOCKET_KEY_ADDED:
            mdebug2("TCP socket %d added to keystore.", sock_client);
            break;
        case REMOTED_USING_UDP:
            keys.keyentries[agentid]->sock = USING_UDP_NO_CLIENT_SOCKET;
            break;
        default:
            ;
        }

        key_unlock();

        // The critical section for readers closes within this function
        save_controlmsg(key, tmp_msg, msg_length - 3, wdb_sock);
        rem_inc_ctrl_msg();

        OS_FreeKey(key);
        return;
    }

    /* Generate srcmsg */

    snprintf(srcmsg, OS_FLSIZE, "[%s] (%s) %s", keys.keyentries[agentid]->id,
             keys.keyentries[agentid]->name, keys.keyentries[agentid]->ip->ip);

    key_unlock();

    /* If we can't send the message, try to connect to the
     * socket again. If it not exit.
     */
    if (SendMSG(logr.m_queue, tmp_msg, srcmsg,
                SECURE_MQ) < 0) {
        merror(QUEUE_ERROR, DEFAULTQUEUE, strerror(errno));

        // Try to reconnect infinitely
        logr.m_queue = StartMQ(DEFAULTQUEUE, WRITE, INFINITE_OPENQ_ATTEMPTS);

        minfo("Successfully reconnected to '%s'", DEFAULTQUEUE);

        if (SendMSG(logr.m_queue, tmp_msg, srcmsg, SECURE_MQ) < 0) {
            // Something went wrong sending a message after an immediate reconnection...
            merror(QUEUE_ERROR, DEFAULTQUEUE, strerror(errno));
        }
    } else {
        rem_inc_evt();
    }
}

// Close and remove socket from keystore
int _close_sock(keystore * keys, int sock) {
    int retval = 0;

    key_lock_read();
    retval = OS_DeleteSocket(keys, sock);
    key_unlock();

    if (!close(sock)) {
        nb_close(&netbuffer_recv, sock);
        nb_close(&netbuffer_send, sock);
        rem_dec_tcp();
    }

    rem_setCounter(sock, global_counter);
    mdebug1("TCP peer disconnected [%d]", sock);

    return retval;
}

int key_request_connect() {
#ifndef WIN32
    return OS_ConnectUnixDomain(WM_KEY_REQUEST_SOCK, SOCK_DGRAM, OS_MAXSTR);
#else
    return -1;
#endif
}

static int send_key_request(int socket,const char *msg) {
    return OS_SendUnix(socket,msg,strlen(msg));
}

static void _push_request(const char *request,const char *type) {
    char *msg = NULL;

    os_calloc(OS_MAXSTR,sizeof(char),msg);
    snprintf(msg,OS_MAXSTR,"%s:%s",type,request);

    if(queue_push_ex(key_request_queue, msg) < 0) {
        os_free(msg);
    }
}

int key_request_reconnect() {
    int socket;
    static int max_attempts = 4;
    int attempts;

    while (1) {
        for (attempts = 0; attempts < max_attempts; attempts++) {
            if (socket = key_request_connect(), socket < 0) {
                sleep(1);
            } else {
                if(OS_SetSendTimeout(socket, 5) < 0){
                    close(socket);
                    continue;
                }
                key_request_available = 1;
                return socket;
            }
        }
        mdebug1("Key-polling wodle is not available. Retrying connection in %d seconds.", KEY_RECONNECT_INTERVAL);
        sleep(KEY_RECONNECT_INTERVAL);
    }
}

void * w_key_request_thread(__attribute__((unused)) void * args) {
    char * msg = NULL;
    int socket = -1;

    while(1) {
        if (socket < 0) {
            socket = key_request_reconnect();
        }

        if (msg || (msg = queue_pop_ex(key_request_queue))) {
            int rc;

            if ((rc = send_key_request(socket, msg)) < 0) {
                if (rc == OS_SOCKBUSY) {
                    mdebug1("Key request socket busy.");
                    sleep(1);
                } else {
                    merror("Could not communicate with key request queue (%d). Is the module running?", rc);
                    if (socket >= 0) {
                        key_request_available = 0;
                        close(socket);
                        socket = -1;
                    }
                }
            } else {
                os_free(msg);
            }
        }
    }
}
