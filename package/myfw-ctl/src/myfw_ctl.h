#ifndef _MYFW_CTL_H_
#define _MYFW_CTL_H_

#define MYFW_ACTION_DROP_STR "drop"
#define MYFW_ACTION_ACCEPT_STR "accept"
#define MYFW_PROTOCOL_TCP_STR "tcp"
#define MYFW_PROTOCOL_UDP_STR "udp"
#define MYFW_PROTOCOL_ICMP_STR "icmp"

enum myfw_action {
    MYFW_ACTION_DROP = 0,
    MYFW_ACTION_ACCEPT = 1,
};

struct myfw_state{
    struct nl_sock *nl_sock;
    int myfw_id;
};

struct myfw_cmd{
    const char *name;
    const char *args;
    const char *help;
    int cmd_id;
    int nl_msg_flags;

    int (*handler)(struct myfw_state *state,
                   struct nl_cb *cb,
                   struct nl_msg *msg,
                   int argc,
                   char **argv);
};

#endif /* _MYFW_CTL_H_ */