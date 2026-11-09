#include <errno.h>
#include <stdio.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <getopt.h>
#include <arpa/inet.h>
#include "myfw_ctl.h"
#include "myfw_genl_ipc.h"

static const char *argv0;


static int parse_ip(const char *ip_str, uint32_t *ip)
{
    struct in_addr addr;

    if (!ip_str || !ip)
        return -EINVAL;

    if (inet_pton(AF_INET, ip_str, &addr) <= 0) {
        fprintf(stderr, "Invalid IP address: %s\n", ip_str);
        return -EINVAL;
    }

    *ip = addr.s_addr;

    return 0;
}

static int parse_proto(const char *proto_str, uint8_t *proto)
{
    if (!proto_str || !proto)
        return -EINVAL;

    if (strcmp(proto_str, MYFW_PROTOCOL_TCP_STR) == 0) {
        *proto = IPPROTO_TCP;
        return 0;
    }

    if (strcmp(proto_str, MYFW_PROTOCOL_UDP_STR) == 0) {
        *proto = IPPROTO_UDP;
        return 0;
    }

    if (strcmp(proto_str, MYFW_PROTOCOL_ICMP_STR) == 0) {
        *proto = IPPROTO_ICMP;
        return 0;
    }

    fprintf(stderr, "Invalid protocol: %s\n", proto_str);
    return -EINVAL;
}

static int parse_action(const char *action_str, uint8_t *action)
{
    if (!action_str || !action)
        return -EINVAL;

    if (strcmp(action_str, MYFW_ACTION_DROP_STR) == 0) {
        *action = MYFW_ACTION_DROP;
        return 0;
    }

    if (strcmp(action_str, MYFW_ACTION_ACCEPT_STR) == 0) {
        *action = MYFW_ACTION_ACCEPT;
        return 0;
    }

    fprintf(stderr, "Invalid action: %s\n", action_str);
    return -EINVAL;
}

static int print_rule_cb(struct nl_msg *msg, void *arg);

static int print_stats_cb(struct nl_msg *msg, void *arg);

int myfw_cmd_add(struct myfw_state *state,
                        struct nl_cb *cb,
                        struct nl_msg *msg,
                        int argc, char **argv);

int myfw_cmd_del(struct myfw_state *state,
                        struct nl_cb *cb,
                        struct nl_msg *msg,
                        int argc, char **argv);

int myfw_cmd_show(struct myfw_state *state,
                        struct nl_cb *cb,
                        struct nl_msg *msg,
                        int argc, char **argv);

int myfw_cmd_stat_show(struct myfw_state *state,
                        struct nl_cb *cb,
                        struct nl_msg *msg,
                        int argc, char **argv);

int myfw_cmd_stat_reset(struct myfw_state *state,
                        struct nl_cb *cb,
                        struct nl_msg *msg,
                        int argc, char **argv);

static const struct myfw_cmd commands[] = {
    {
        .name = "rule add",
        .args = "<src_ip> <dst_ip> <src_port> <dst_port> <protocol> <action>",
        .help = "Add a firewall rule",
        .cmd_id = MYFW_CMD_ADD_RULE,
        .nl_msg_flags = NLM_F_REQUEST | NLM_F_ACK,
        .handler = myfw_cmd_add,
    },
    {
        .name = "rule del",
        .args = "<src_ip> <dst_ip> <src_port> <dst_port> <protocol>",
        .help = "Delete a firewall rule",
        .cmd_id = MYFW_CMD_DEL_RULE,
        .nl_msg_flags = NLM_F_REQUEST | NLM_F_ACK,
        .handler = myfw_cmd_del,
    },
    {
        .name = "rule show",
        .args = "",
        .help = "Show firewall rules",
        .cmd_id = MYFW_CMD_GET_RULES,
        .nl_msg_flags = NLM_F_REQUEST | NLM_F_DUMP,
        .handler = myfw_cmd_show,
    },
    {
        .name = "stat show",
        .args = "",
        .help = "Show firewall statistics",
        .cmd_id = MYFW_CMD_GET_STATS,
        .nl_msg_flags = NLM_F_REQUEST,
        .handler = myfw_cmd_stat_show,
    },
    {
        .name = "stat reset",
        .args = "",
        .help = "Reset firewall statistics",
        .cmd_id = MYFW_CMD_RESET_STATS,
        .nl_msg_flags = NLM_F_REQUEST,
        .handler = myfw_cmd_stat_reset,
    },
};

static int print_rule_cb(struct nl_msg *msg, void *arg)
{
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    struct nlattr *attrs[MYFW_ATTR_MAX + 1];
    uint32_t src_ip = 0, dst_ip = 0;
    uint16_t src_port = 0, dst_port = 0;
    uint8_t proto = 0, action = 0;

    if (genlmsg_parse(nlh, 0, attrs, MYFW_ATTR_MAX, NULL) < 0)
        return NL_SKIP;

    if (attrs[MYFW_ATTR_SRC_IP])
        src_ip   = nla_get_u32(attrs[MYFW_ATTR_SRC_IP]);
    if (attrs[MYFW_ATTR_DST_IP])
        dst_ip   = nla_get_u32(attrs[MYFW_ATTR_DST_IP]);
    if (attrs[MYFW_ATTR_SRC_PORT])
        src_port = nla_get_u16(attrs[MYFW_ATTR_SRC_PORT]);
    if (attrs[MYFW_ATTR_DST_PORT])
        dst_port = nla_get_u16(attrs[MYFW_ATTR_DST_PORT]);
    if (attrs[MYFW_ATTR_PROTO])
        proto    = nla_get_u8(attrs[MYFW_ATTR_PROTO]);
    if (attrs[MYFW_ATTR_ACTION])
        action   = nla_get_u8(attrs[MYFW_ATTR_ACTION]);

    char src_str[INET_ADDRSTRLEN];
    char dst_str[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &src_ip, src_str, sizeof(src_str));
    inet_ntop(AF_INET, &dst_ip, dst_str, sizeof(dst_str));

    printf("Rule -> Src: %15s:%-5u | Dst: %15s:%-5u | Proto: %3u | Action: %s\n",
           src_str, ntohs(src_port), dst_str, ntohs(dst_port), proto,
           action == MYFW_ACTION_DROP ? "DROP" : "ACCEPT");

    return NL_OK;
}

static int print_stats_cb(struct nl_msg *msg, void *arg)
{
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    struct nlattr *attrs[MYFW_ATTR_MAX + 1];
    int64_t rx_bytes = 0, rx_pkts = 0, drops = 0;

    if (genlmsg_parse(nlh, 0, attrs, MYFW_ATTR_MAX, NULL) < 0)
        return NL_SKIP;

    if (attrs[MYFW_ATTR_RX_BYTES])
        rx_bytes = nla_get_s64(attrs[MYFW_ATTR_RX_BYTES]);
    if (attrs[MYFW_ATTR_RX_PACKETS])
        rx_pkts = nla_get_s64(attrs[MYFW_ATTR_RX_PACKETS]);
    if (attrs[MYFW_ATTR_DROPPED_PACKETS])
        drops = nla_get_s64(attrs[MYFW_ATTR_DROPPED_PACKETS]);

    printf("======================================\n");
    printf("         MYFW HOOK STATISTICS     \n");
    printf("======================================\n");
    printf(" Rx Packets   : %lld\n", (long long)rx_pkts);
    printf(" Rx Bytes     : %lld\n", (long long)rx_bytes);
    printf(" Dropped Pkts : %lld\n", (long long)drops);
    printf("======================================\n");

    return NL_OK;
}

int myfw_cmd_add(struct myfw_state *state,
                        struct nl_cb *cb,
                        struct nl_msg *msg,
                        int argc, char **argv)
{
    uint32_t src_ip = 0, dst_ip = 0;
    uint16_t src_port = 0, dst_port = 0;
    uint8_t proto = 0, action = 0;

    for (int i = 0; i < argc; i++){
        if (strcmp(argv[i], "--src-ip") == 0 && i + 1 < argc) {
            if (parse_ip(argv[i+1], &src_ip) < 0)
                return -EINVAL;

            if (src_ip == 0)
                return -EINVAL;

            nla_put_u32(msg, MYFW_ATTR_SRC_IP, src_ip);
            i++;
        }
        else if (strcmp(argv[i], "--dst-ip") == 0 && i + 1 < argc) {
           if (parse_ip(argv[i+1], &dst_ip) < 0)
                return -EINVAL;
            nla_put_u32(msg, MYFW_ATTR_DST_IP, dst_ip);
            i++;
        }
        else if (strcmp(argv[i], "--src-port") == 0 && i + 1 < argc) {
            src_port = htons((uint16_t)atoi(argv[i+1]));
            nla_put_u16(msg, MYFW_ATTR_SRC_PORT, src_port);
            i++;
        }
        else if (strcmp(argv[i], "--dst-port") == 0 && i + 1 < argc) {
            dst_port = htons((uint16_t)atoi(argv[i+1]));
            nla_put_u16(msg, MYFW_ATTR_DST_PORT, dst_port);
            i++;
        }
        else if (strcmp(argv[i], "--proto") == 0 && i + 1 < argc) {
            if (parse_proto(argv[i+1], &proto) < 0)
                return -EINVAL;

            nla_put_u8(msg, MYFW_ATTR_PROTO, proto);
            i++;
        }
        else if (strcmp(argv[i], "--action") == 0 && i + 1 < argc) {
            if (parse_action(argv[i+1], &action) < 0)
                return -EINVAL;

            nla_put_u8(msg, MYFW_ATTR_ACTION, action);
            i++;
        }
        else {
            fprintf(stderr, "myfw_cmd_add error: %s\n", argv[i]);
            return -EINVAL;
        }
    }
    return 0;
}

int myfw_cmd_del(struct myfw_state *state,
                        struct nl_cb *cb,
                        struct nl_msg *msg,
                        int argc, char **argv)
{
    uint32_t src_ip = 0, dst_ip = 0;
    uint16_t src_port = 0, dst_port = 0;
    uint8_t proto = 0;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--src-ip") == 0 && i + 1 < argc) {
            if (parse_ip(argv[i+1], &src_ip) < 0)
                return -EINVAL;

            nla_put_u32(msg, MYFW_ATTR_SRC_IP, src_ip);
            i++;
        }
        else if (strcmp(argv[i], "--dst-ip") == 0 && i + 1 < argc) {
            if (parse_ip(argv[i+1], &dst_ip) < 0)
                return -EINVAL;

            nla_put_u32(msg, MYFW_ATTR_DST_IP, dst_ip);
            i++;
        }
        else if (strcmp(argv[i], "--src-port") == 0 && i + 1 < argc) {
            src_port = htons((uint16_t)atoi(argv[i+1]));
            nla_put_u16(msg, MYFW_ATTR_SRC_PORT, src_port);
            i++;
        }
        else if (strcmp(argv[i], "--dst-port") == 0 && i + 1 < argc) {
            dst_port = htons((uint16_t)atoi(argv[i+1]));
            nla_put_u16(msg, MYFW_ATTR_DST_PORT, dst_port);
            i++;
        }
        else if (strcmp(argv[i], "--proto") == 0 && i + 1 < argc) {
            if (parse_proto(argv[i+1], &proto) < 0) return -EINVAL;
            nla_put_u8(msg, MYFW_ATTR_PROTO, proto);
            i++;
        }
        else {
            fprintf(stderr, "Invalid option for rule del: %s\n", argv[i]);
            return -EINVAL;
        }
    }
    return 0;
}

int myfw_cmd_show(struct myfw_state *state,
                        struct nl_cb *cb,
                        struct nl_msg *msg,
                        int argc, char **argv)
{
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, print_rule_cb, NULL);
    return 0;
}

int myfw_cmd_stat_show(struct myfw_state *state,
                        struct nl_cb *cb,
                        struct nl_msg *msg,
                        int argc, char **argv)
{
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, print_stats_cb, NULL);
    return 0;
}

int myfw_cmd_stat_reset(struct myfw_state *state,
                        struct nl_cb *cb,
                        struct nl_msg *msg,
                        int argc, char **argv)
{

    return 0;
}

static void usage(void)
{
    printf("======== MYFW Firewall CLI Control Tool ========\n");
    printf("  %s -fw rule add --src-ip <ip> --dst-ip <ip> --src-port <port> --dst-port <port> --proto <tcp|udp|icmp> --action <drop|accept>\n", argv0);
    printf("  %s -fw rule del --src-ip <ip> --dst-ip <ip> --src-port <port> --dst-port <port> --proto <tcp|udp|icmp>\n", argv0);
    printf("  %s -fw rule show\n", argv0);
    printf("  %s -fw stat show\n", argv0);
    printf("  %s -fw stat reset\n", argv0);
    printf("================================================\n");
}

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
			 void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

static int myfw_init(struct myfw_state *state)
{
    int err;
    state->nl_sock = nl_socket_alloc();
    if (!state->nl_sock) {
        fprintf(stderr, "Failed to allocate netlink socket\n");
        return -ENOMEM;
    }

    nl_socket_set_buffer_size(state->nl_sock, 8192, 8192);

    if (genl_connect(state->nl_sock)) {
		fprintf(stderr, "Failed to connect to generic netlink.\n");
		err = -ENOLINK;
		goto out_handle_destroy;
	}

    state->myfw_id = genl_ctrl_resolve(state->nl_sock, "MYFW_GENL");
    if (state->myfw_id < 0) {
        fprintf(stderr, "Failed to resolve MYFW family.\n");
        err = -ENOENT;
        goto out_handle_destroy;
    }

    return 0;

out_handle_destroy:
    nl_socket_free(state->nl_sock);
    return err;
}

static int handle_cmd(struct myfw_state *state,
                      int cmd_id,
                      int argc, char **argv)
{
    const struct myfw_cmd *cmd = NULL;
    struct nl_msg *msg = NULL;
    struct nl_cb *cb = NULL;
    struct nl_cb *s_cb = NULL;
    int err;

    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (cmd_id == commands[i].cmd_id) {
            cmd = &commands[i];
            break;
        }
    }

    if (!cmd)
        return -EINVAL;

    msg = nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "Failed to allocate netlink message\n");
        return -ENOMEM;
    }

    cb = nl_cb_alloc(NL_CB_DEFAULT);
    s_cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb || !s_cb) {
        fprintf(stderr, "Failed to allocate netlink callback\n");
        nlmsg_free(msg);
        return -ENOMEM;
    }

    genlmsg_put(msg, 0, 0, state->myfw_id, 0, cmd->nl_msg_flags, cmd->cmd_id, 1);

    err = cmd->handler(state, cb, msg, argc, argv);
    if (err < 0) {
        goto out;
    }

    nl_socket_set_cb(state->nl_sock, s_cb);

    err = nl_send_auto(state->nl_sock, msg);
    if (err < 0) {
        fprintf(stderr, "Failed to send netlink message: %s\n", nl_geterror(err));
        goto out;
    }

    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);

    while (err > 0)
        nl_recvmsgs(state->nl_sock, cb);

out:
    nl_cb_put(cb);
    nlmsg_free(msg);
    return err;
}

int main(int argc, char **argv)
{
    struct myfw_state state;
    int err;

    argc--;
    argv0 = *argv++;

    if (argc == 0 || strcmp(*argv, "--help") == 0) {
        usage();
        return 0;
    }

    err = myfw_init(&state);

    if (err)
        return err;

    argc--;
    argv++;

    if (strcmp(*argv, "rule") == 0) {
        argc--;
        argv++;

        if (argc > 0 && strcmp(*argv, "add") == 0)
        {
            argc--;
            argv++;
            err = handle_cmd(&state, MYFW_CMD_ADD_RULE, argc, argv);
        }
        else if (argc > 0 && strcmp(*argv, "del") == 0)
        {
            argc--;
            argv++;
            err = handle_cmd(&state, MYFW_CMD_DEL_RULE, argc, argv);
        }
        else if (argc > 0 && strcmp(*argv, "show") == 0)
        {
            argc--;
            argv++;
            err = handle_cmd(&state, MYFW_CMD_GET_RULES, argc, argv);
        }
        else
            goto failure;
    }
    else if (strcmp(*argv, "stat") == 0) {
        argc--;
        argv++;

        if (argc > 0 && strcmp(*argv, "show") == 0)
        {
            argc--;
            argv++;
            err = handle_cmd(&state, MYFW_CMD_GET_STATS, argc, argv);
        }
        else if (argc > 0 && strcmp(*argv, "reset") == 0)
        {
            argc--;
            argv++;
            err = handle_cmd(&state, MYFW_CMD_RESET_STATS, argc, argv);
        }
        else
            goto failure;
    }
    else
        goto failure;

    return 0;

failure:
    usage();
    err = -EINVAL;
    return err;
}