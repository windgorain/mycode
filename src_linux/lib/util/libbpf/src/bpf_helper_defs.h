/* This is auto-generated file. See bpf_doc.py for details. */

/* Forward declarations of BPF structs */
struct bpf_fib_lookup;
struct bpf_sk_lookup;
struct bpf_perf_event_data;
struct bpf_perf_event_value;
struct bpf_pidns_info;
struct bpf_redir_neigh;
struct bpf_sock;
struct bpf_sock_addr;
struct bpf_sock_ops;
struct bpf_sock_tuple;
struct bpf_spin_lock;
struct bpf_sysctl;
struct bpf_tcp_sock;
struct bpf_tunnel_key;
struct bpf_xfrm_state;
struct linux_binprm;
struct pt_regs;
struct sk_reuseport_md;
struct sockaddr;
struct tcphdr;
struct seq_file;
struct tcp6_sock;
struct tcp_sock;
struct tcp_timewait_sock;
struct tcp_request_sock;
struct udp6_sock;
struct unix_sock;
struct task_struct;
struct cgroup;
struct __sk_buff;
struct sk_msg_md;
struct xdp_md;
struct path;
struct btf_ptr;
struct inode;
struct socket;
struct file;
struct bpf_timer;
struct mptcp_sock;
struct bpf_dynptr;
struct iphdr;
struct ipv6hdr;


static void *(*bpf_map_lookup_elem)(void *map, const void *key) = (void *) 1;


static long (*bpf_map_update_elem)(void *map, const void *key, const void *value, __u64 flags) = (void *) 2;


static long (*bpf_map_delete_elem)(void *map, const void *key) = (void *) 3;


static long (*bpf_probe_read)(void *dst, __u32 size, const void *unsafe_ptr) = (void *) 4;


static __u64 (*bpf_ktime_get_ns)(void) = (void *) 5;


static long (*bpf_trace_printk)(const char *fmt, __u32 fmt_size, ...) = (void *) 6;


static __u32 (*bpf_get_prandom_u32)(void) = (void *) 7;


static __u32 (*bpf_get_smp_processor_id)(void) = (void *) 8;


static long (*bpf_skb_store_bytes)(struct __sk_buff *skb, __u32 offset, const void *from, __u32 len, __u64 flags) = (void *) 9;


static long (*bpf_l3_csum_replace)(struct __sk_buff *skb, __u32 offset, __u64 from, __u64 to, __u64 size) = (void *) 10;


static long (*bpf_l4_csum_replace)(struct __sk_buff *skb, __u32 offset, __u64 from, __u64 to, __u64 flags) = (void *) 11;


static long (*bpf_tail_call)(void *ctx, void *prog_array_map, __u32 index) = (void *) 12;


static long (*bpf_clone_redirect)(struct __sk_buff *skb, __u32 ifindex, __u64 flags) = (void *) 13;


static __u64 (*bpf_get_current_pid_tgid)(void) = (void *) 14;


static __u64 (*bpf_get_current_uid_gid)(void) = (void *) 15;


static long (*bpf_get_current_comm)(void *buf, __u32 size_of_buf) = (void *) 16;


static __u32 (*bpf_get_cgroup_classid)(struct __sk_buff *skb) = (void *) 17;


static long (*bpf_skb_vlan_push)(struct __sk_buff *skb, __be16 vlan_proto, __u16 vlan_tci) = (void *) 18;


static long (*bpf_skb_vlan_pop)(struct __sk_buff *skb) = (void *) 19;


static long (*bpf_skb_get_tunnel_key)(struct __sk_buff *skb, struct bpf_tunnel_key *key, __u32 size, __u64 flags) = (void *) 20;


static long (*bpf_skb_set_tunnel_key)(struct __sk_buff *skb, struct bpf_tunnel_key *key, __u32 size, __u64 flags) = (void *) 21;


static __u64 (*bpf_perf_event_read)(void *map, __u64 flags) = (void *) 22;


static long (*bpf_redirect)(__u32 ifindex, __u64 flags) = (void *) 23;


static __u32 (*bpf_get_route_realm)(struct __sk_buff *skb) = (void *) 24;


static long (*bpf_perf_event_output)(void *ctx, void *map, __u64 flags, void *data, __u64 size) = (void *) 25;


static long (*bpf_skb_load_bytes)(const void *skb, __u32 offset, void *to, __u32 len) = (void *) 26;


static long (*bpf_get_stackid)(void *ctx, void *map, __u64 flags) = (void *) 27;


static __s64 (*bpf_csum_diff)(__be32 *from, __u32 from_size, __be32 *to, __u32 to_size, __wsum seed) = (void *) 28;


static long (*bpf_skb_get_tunnel_opt)(struct __sk_buff *skb, void *opt, __u32 size) = (void *) 29;


static long (*bpf_skb_set_tunnel_opt)(struct __sk_buff *skb, void *opt, __u32 size) = (void *) 30;


static long (*bpf_skb_change_proto)(struct __sk_buff *skb, __be16 proto, __u64 flags) = (void *) 31;


static long (*bpf_skb_change_type)(struct __sk_buff *skb, __u32 type) = (void *) 32;


static long (*bpf_skb_under_cgroup)(struct __sk_buff *skb, void *map, __u32 index) = (void *) 33;


static __u32 (*bpf_get_hash_recalc)(struct __sk_buff *skb) = (void *) 34;


static __u64 (*bpf_get_current_task)(void) = (void *) 35;


static long (*bpf_probe_write_user)(void *dst, const void *src, __u32 len) = (void *) 36;


static long (*bpf_current_task_under_cgroup)(void *map, __u32 index) = (void *) 37;


static long (*bpf_skb_change_tail)(struct __sk_buff *skb, __u32 len, __u64 flags) = (void *) 38;


static long (*bpf_skb_pull_data)(struct __sk_buff *skb, __u32 len) = (void *) 39;


static __s64 (*bpf_csum_update)(struct __sk_buff *skb, __wsum csum) = (void *) 40;


static void (*bpf_set_hash_invalid)(struct __sk_buff *skb) = (void *) 41;


static long (*bpf_get_numa_node_id)(void) = (void *) 42;


static long (*bpf_skb_change_head)(struct __sk_buff *skb, __u32 len, __u64 flags) = (void *) 43;


static long (*bpf_xdp_adjust_head)(struct xdp_md *xdp_md, int delta) = (void *) 44;


static long (*bpf_probe_read_str)(void *dst, __u32 size, const void *unsafe_ptr) = (void *) 45;


static __u64 (*bpf_get_socket_cookie)(void *ctx) = (void *) 46;


static __u32 (*bpf_get_socket_uid)(struct __sk_buff *skb) = (void *) 47;


static long (*bpf_set_hash)(struct __sk_buff *skb, __u32 hash) = (void *) 48;


static long (*bpf_setsockopt)(void *bpf_socket, int level, int optname, void *optval, int optlen) = (void *) 49;


static long (*bpf_skb_adjust_room)(struct __sk_buff *skb, __s32 len_diff, __u32 mode, __u64 flags) = (void *) 50;


static long (*bpf_redirect_map)(void *map, __u64 key, __u64 flags) = (void *) 51;


static long (*bpf_sk_redirect_map)(struct __sk_buff *skb, void *map, __u32 key, __u64 flags) = (void *) 52;


static long (*bpf_sock_map_update)(struct bpf_sock_ops *skops, void *map, void *key, __u64 flags) = (void *) 53;


static long (*bpf_xdp_adjust_meta)(struct xdp_md *xdp_md, int delta) = (void *) 54;


static long (*bpf_perf_event_read_value)(void *map, __u64 flags, struct bpf_perf_event_value *buf, __u32 buf_size) = (void *) 55;


static long (*bpf_perf_prog_read_value)(struct bpf_perf_event_data *ctx, struct bpf_perf_event_value *buf, __u32 buf_size) = (void *) 56;


static long (*bpf_getsockopt)(void *bpf_socket, int level, int optname, void *optval, int optlen) = (void *) 57;


static long (*bpf_override_return)(struct pt_regs *regs, __u64 rc) = (void *) 58;


static long (*bpf_sock_ops_cb_flags_set)(struct bpf_sock_ops *bpf_sock, int argval) = (void *) 59;


static long (*bpf_msg_redirect_map)(struct sk_msg_md *msg, void *map, __u32 key, __u64 flags) = (void *) 60;


static long (*bpf_msg_apply_bytes)(struct sk_msg_md *msg, __u32 bytes) = (void *) 61;


static long (*bpf_msg_cork_bytes)(struct sk_msg_md *msg, __u32 bytes) = (void *) 62;


static long (*bpf_msg_pull_data)(struct sk_msg_md *msg, __u32 start, __u32 end, __u64 flags) = (void *) 63;


static long (*bpf_bind)(struct bpf_sock_addr *ctx, struct sockaddr *addr, int addr_len) = (void *) 64;


static long (*bpf_xdp_adjust_tail)(struct xdp_md *xdp_md, int delta) = (void *) 65;


static long (*bpf_skb_get_xfrm_state)(struct __sk_buff *skb, __u32 index, struct bpf_xfrm_state *xfrm_state, __u32 size, __u64 flags) = (void *) 66;


static long (*bpf_get_stack)(void *ctx, void *buf, __u32 size, __u64 flags) = (void *) 67;


static long (*bpf_skb_load_bytes_relative)(const void *skb, __u32 offset, void *to, __u32 len, __u32 start_header) = (void *) 68;


static long (*bpf_fib_lookup)(void *ctx, struct bpf_fib_lookup *params, int plen, __u32 flags) = (void *) 69;


static long (*bpf_sock_hash_update)(struct bpf_sock_ops *skops, void *map, void *key, __u64 flags) = (void *) 70;


static long (*bpf_msg_redirect_hash)(struct sk_msg_md *msg, void *map, void *key, __u64 flags) = (void *) 71;


static long (*bpf_sk_redirect_hash)(struct __sk_buff *skb, void *map, void *key, __u64 flags) = (void *) 72;


static long (*bpf_lwt_push_encap)(struct __sk_buff *skb, __u32 type, void *hdr, __u32 len) = (void *) 73;


static long (*bpf_lwt_seg6_store_bytes)(struct __sk_buff *skb, __u32 offset, const void *from, __u32 len) = (void *) 74;


static long (*bpf_lwt_seg6_adjust_srh)(struct __sk_buff *skb, __u32 offset, __s32 delta) = (void *) 75;


static long (*bpf_lwt_seg6_action)(struct __sk_buff *skb, __u32 action, void *param, __u32 param_len) = (void *) 76;


static long (*bpf_rc_repeat)(void *ctx) = (void *) 77;


static long (*bpf_rc_keydown)(void *ctx, __u32 protocol, __u64 scancode, __u32 toggle) = (void *) 78;


static __u64 (*bpf_skb_cgroup_id)(struct __sk_buff *skb) = (void *) 79;


static __u64 (*bpf_get_current_cgroup_id)(void) = (void *) 80;


static void *(*bpf_get_local_storage)(void *map, __u64 flags) = (void *) 81;


static long (*bpf_sk_select_reuseport)(struct sk_reuseport_md *reuse, void *map, void *key, __u64 flags) = (void *) 82;


static __u64 (*bpf_skb_ancestor_cgroup_id)(struct __sk_buff *skb, int ancestor_level) = (void *) 83;


static struct bpf_sock *(*bpf_sk_lookup_tcp)(void *ctx, struct bpf_sock_tuple *tuple, __u32 tuple_size, __u64 netns, __u64 flags) = (void *) 84;


static struct bpf_sock *(*bpf_sk_lookup_udp)(void *ctx, struct bpf_sock_tuple *tuple, __u32 tuple_size, __u64 netns, __u64 flags) = (void *) 85;


static long (*bpf_sk_release)(void *sock) = (void *) 86;


static long (*bpf_map_push_elem)(void *map, const void *value, __u64 flags) = (void *) 87;


static long (*bpf_map_pop_elem)(void *map, void *value) = (void *) 88;


static long (*bpf_map_peek_elem)(void *map, void *value) = (void *) 89;


static long (*bpf_msg_push_data)(struct sk_msg_md *msg, __u32 start, __u32 len, __u64 flags) = (void *) 90;


static long (*bpf_msg_pop_data)(struct sk_msg_md *msg, __u32 start, __u32 len, __u64 flags) = (void *) 91;


static long (*bpf_rc_pointer_rel)(void *ctx, __s32 rel_x, __s32 rel_y) = (void *) 92;


static long (*bpf_spin_lock)(struct bpf_spin_lock *lock) = (void *) 93;


static long (*bpf_spin_unlock)(struct bpf_spin_lock *lock) = (void *) 94;


static struct bpf_sock *(*bpf_sk_fullsock)(struct bpf_sock *sk) = (void *) 95;


static struct bpf_tcp_sock *(*bpf_tcp_sock)(struct bpf_sock *sk) = (void *) 96;


static long (*bpf_skb_ecn_set_ce)(struct __sk_buff *skb) = (void *) 97;


static struct bpf_sock *(*bpf_get_listener_sock)(struct bpf_sock *sk) = (void *) 98;


static struct bpf_sock *(*bpf_skc_lookup_tcp)(void *ctx, struct bpf_sock_tuple *tuple, __u32 tuple_size, __u64 netns, __u64 flags) = (void *) 99;


static long (*bpf_tcp_check_syncookie)(void *sk, void *iph, __u32 iph_len, struct tcphdr *th, __u32 th_len) = (void *) 100;


static long (*bpf_sysctl_get_name)(struct bpf_sysctl *ctx, char *buf, unsigned long buf_len, __u64 flags) = (void *) 101;


static long (*bpf_sysctl_get_current_value)(struct bpf_sysctl *ctx, char *buf, unsigned long buf_len) = (void *) 102;


static long (*bpf_sysctl_get_new_value)(struct bpf_sysctl *ctx, char *buf, unsigned long buf_len) = (void *) 103;


static long (*bpf_sysctl_set_new_value)(struct bpf_sysctl *ctx, const char *buf, unsigned long buf_len) = (void *) 104;


static long (*bpf_strtol)(const char *buf, unsigned long buf_len, __u64 flags, long *res) = (void *) 105;


static long (*bpf_strtoul)(const char *buf, unsigned long buf_len, __u64 flags, unsigned long *res) = (void *) 106;


static void *(*bpf_sk_storage_get)(void *map, void *sk, void *value, __u64 flags) = (void *) 107;


static long (*bpf_sk_storage_delete)(void *map, void *sk) = (void *) 108;


static long (*bpf_send_signal)(__u32 sig) = (void *) 109;


static __s64 (*bpf_tcp_gen_syncookie)(void *sk, void *iph, __u32 iph_len, struct tcphdr *th, __u32 th_len) = (void *) 110;


static long (*bpf_skb_output)(void *ctx, void *map, __u64 flags, void *data, __u64 size) = (void *) 111;


static long (*bpf_probe_read_user)(void *dst, __u32 size, const void *unsafe_ptr) = (void *) 112;


static long (*bpf_probe_read_kernel)(void *dst, __u32 size, const void *unsafe_ptr) = (void *) 113;


static long (*bpf_probe_read_user_str)(void *dst, __u32 size, const void *unsafe_ptr) = (void *) 114;


static long (*bpf_probe_read_kernel_str)(void *dst, __u32 size, const void *unsafe_ptr) = (void *) 115;


static long (*bpf_tcp_send_ack)(void *tp, __u32 rcv_nxt) = (void *) 116;


static long (*bpf_send_signal_thread)(__u32 sig) = (void *) 117;


static __u64 (*bpf_jiffies64)(void) = (void *) 118;


static long (*bpf_read_branch_records)(struct bpf_perf_event_data *ctx, void *buf, __u32 size, __u64 flags) = (void *) 119;


static long (*bpf_get_ns_current_pid_tgid)(__u64 dev, __u64 ino, struct bpf_pidns_info *nsdata, __u32 size) = (void *) 120;


static long (*bpf_xdp_output)(void *ctx, void *map, __u64 flags, void *data, __u64 size) = (void *) 121;


static __u64 (*bpf_get_netns_cookie)(void *ctx) = (void *) 122;


static __u64 (*bpf_get_current_ancestor_cgroup_id)(int ancestor_level) = (void *) 123;


static long (*bpf_sk_assign)(void *ctx, void *sk, __u64 flags) = (void *) 124;


static __u64 (*bpf_ktime_get_boot_ns)(void) = (void *) 125;


static long (*bpf_seq_printf)(struct seq_file *m, const char *fmt, __u32 fmt_size, const void *data, __u32 data_len) = (void *) 126;


static long (*bpf_seq_write)(struct seq_file *m, const void *data, __u32 len) = (void *) 127;


static __u64 (*bpf_sk_cgroup_id)(void *sk) = (void *) 128;


static __u64 (*bpf_sk_ancestor_cgroup_id)(void *sk, int ancestor_level) = (void *) 129;


static long (*bpf_ringbuf_output)(void *ringbuf, void *data, __u64 size, __u64 flags) = (void *) 130;


static void *(*bpf_ringbuf_reserve)(void *ringbuf, __u64 size, __u64 flags) = (void *) 131;


static void (*bpf_ringbuf_submit)(void *data, __u64 flags) = (void *) 132;


static void (*bpf_ringbuf_discard)(void *data, __u64 flags) = (void *) 133;


static __u64 (*bpf_ringbuf_query)(void *ringbuf, __u64 flags) = (void *) 134;


static long (*bpf_csum_level)(struct __sk_buff *skb, __u64 level) = (void *) 135;


static struct tcp6_sock *(*bpf_skc_to_tcp6_sock)(void *sk) = (void *) 136;


static struct tcp_sock *(*bpf_skc_to_tcp_sock)(void *sk) = (void *) 137;


static struct tcp_timewait_sock *(*bpf_skc_to_tcp_timewait_sock)(void *sk) = (void *) 138;


static struct tcp_request_sock *(*bpf_skc_to_tcp_request_sock)(void *sk) = (void *) 139;


static struct udp6_sock *(*bpf_skc_to_udp6_sock)(void *sk) = (void *) 140;


static long (*bpf_get_task_stack)(struct task_struct *task, void *buf, __u32 size, __u64 flags) = (void *) 141;


static long (*bpf_load_hdr_opt)(struct bpf_sock_ops *skops, void *searchby_res, __u32 len, __u64 flags) = (void *) 142;


static long (*bpf_store_hdr_opt)(struct bpf_sock_ops *skops, const void *from, __u32 len, __u64 flags) = (void *) 143;


static long (*bpf_reserve_hdr_opt)(struct bpf_sock_ops *skops, __u32 len, __u64 flags) = (void *) 144;


static void *(*bpf_inode_storage_get)(void *map, void *inode, void *value, __u64 flags) = (void *) 145;


static int (*bpf_inode_storage_delete)(void *map, void *inode) = (void *) 146;


static long (*bpf_d_path)(struct path *path, char *buf, __u32 sz) = (void *) 147;


static long (*bpf_copy_from_user)(void *dst, __u32 size, const void *user_ptr) = (void *) 148;


static long (*bpf_snprintf_btf)(char *str, __u32 str_size, struct btf_ptr *ptr, __u32 btf_ptr_size, __u64 flags) = (void *) 149;


static long (*bpf_seq_printf_btf)(struct seq_file *m, struct btf_ptr *ptr, __u32 ptr_size, __u64 flags) = (void *) 150;


static __u64 (*bpf_skb_cgroup_classid)(struct __sk_buff *skb) = (void *) 151;


static long (*bpf_redirect_neigh)(__u32 ifindex, struct bpf_redir_neigh *params, int plen, __u64 flags) = (void *) 152;


static void *(*bpf_per_cpu_ptr)(const void *percpu_ptr, __u32 cpu) = (void *) 153;


static void *(*bpf_this_cpu_ptr)(const void *percpu_ptr) = (void *) 154;


static long (*bpf_redirect_peer)(__u32 ifindex, __u64 flags) = (void *) 155;


static void *(*bpf_task_storage_get)(void *map, struct task_struct *task, void *value, __u64 flags) = (void *) 156;


static long (*bpf_task_storage_delete)(void *map, struct task_struct *task) = (void *) 157;


static struct task_struct *(*bpf_get_current_task_btf)(void) = (void *) 158;


static long (*bpf_bprm_opts_set)(struct linux_binprm *bprm, __u64 flags) = (void *) 159;


static __u64 (*bpf_ktime_get_coarse_ns)(void) = (void *) 160;


static long (*bpf_ima_inode_hash)(struct inode *inode, void *dst, __u32 size) = (void *) 161;


static struct socket *(*bpf_sock_from_file)(struct file *file) = (void *) 162;


static long (*bpf_check_mtu)(void *ctx, __u32 ifindex, __u32 *mtu_len, __s32 len_diff, __u64 flags) = (void *) 163;


static long (*bpf_for_each_map_elem)(void *map, void *callback_fn, void *callback_ctx, __u64 flags) = (void *) 164;


static long (*bpf_snprintf)(char *str, __u32 str_size, const char *fmt, __u64 *data, __u32 data_len) = (void *) 165;


static long (*bpf_sys_bpf)(__u32 cmd, void *attr, __u32 attr_size) = (void *) 166;


static long (*bpf_btf_find_by_name_kind)(char *name, int name_sz, __u32 kind, int flags) = (void *) 167;


static long (*bpf_sys_close)(__u32 fd) = (void *) 168;


static long (*bpf_timer_init)(struct bpf_timer *timer, void *map, __u64 flags) = (void *) 169;


static long (*bpf_timer_set_callback)(struct bpf_timer *timer, void *callback_fn) = (void *) 170;


static long (*bpf_timer_start)(struct bpf_timer *timer, __u64 nsecs, __u64 flags) = (void *) 171;


static long (*bpf_timer_cancel)(struct bpf_timer *timer) = (void *) 172;


static __u64 (*bpf_get_func_ip)(void *ctx) = (void *) 173;


static __u64 (*bpf_get_attach_cookie)(void *ctx) = (void *) 174;


static long (*bpf_task_pt_regs)(struct task_struct *task) = (void *) 175;


static long (*bpf_get_branch_snapshot)(void *entries, __u32 size, __u64 flags) = (void *) 176;


static long (*bpf_trace_vprintk)(const char *fmt, __u32 fmt_size, const void *data, __u32 data_len) = (void *) 177;


static struct unix_sock *(*bpf_skc_to_unix_sock)(void *sk) = (void *) 178;


static long (*bpf_kallsyms_lookup_name)(const char *name, int name_sz, int flags, __u64 *res) = (void *) 179;


static long (*bpf_find_vma)(struct task_struct *task, __u64 addr, void *callback_fn, void *callback_ctx, __u64 flags) = (void *) 180;


static long (*bpf_loop)(__u32 nr_loops, void *callback_fn, void *callback_ctx, __u64 flags) = (void *) 181;


static long (*bpf_strncmp)(const char *s1, __u32 s1_sz, const char *s2) = (void *) 182;


static long (*bpf_get_func_arg)(void *ctx, __u32 n, __u64 *value) = (void *) 183;


static long (*bpf_get_func_ret)(void *ctx, __u64 *value) = (void *) 184;


static long (*bpf_get_func_arg_cnt)(void *ctx) = (void *) 185;


static int (*bpf_get_retval)(void) = (void *) 186;


static int (*bpf_set_retval)(int retval) = (void *) 187;


static __u64 (*bpf_xdp_get_buff_len)(struct xdp_md *xdp_md) = (void *) 188;


static long (*bpf_xdp_load_bytes)(struct xdp_md *xdp_md, __u32 offset, void *buf, __u32 len) = (void *) 189;


static long (*bpf_xdp_store_bytes)(struct xdp_md *xdp_md, __u32 offset, void *buf, __u32 len) = (void *) 190;


static long (*bpf_copy_from_user_task)(void *dst, __u32 size, const void *user_ptr, struct task_struct *tsk, __u64 flags) = (void *) 191;


static long (*bpf_skb_set_tstamp)(struct __sk_buff *skb, __u64 tstamp, __u32 tstamp_type) = (void *) 192;


static long (*bpf_ima_file_hash)(struct file *file, void *dst, __u32 size) = (void *) 193;


static void *(*bpf_kptr_xchg)(void *map_value, void *ptr) = (void *) 194;


static void *(*bpf_map_lookup_percpu_elem)(void *map, const void *key, __u32 cpu) = (void *) 195;


static struct mptcp_sock *(*bpf_skc_to_mptcp_sock)(void *sk) = (void *) 196;


static long (*bpf_dynptr_from_mem)(void *data, __u32 size, __u64 flags, struct bpf_dynptr *ptr) = (void *) 197;


static long (*bpf_ringbuf_reserve_dynptr)(void *ringbuf, __u32 size, __u64 flags, struct bpf_dynptr *ptr) = (void *) 198;


static void (*bpf_ringbuf_submit_dynptr)(struct bpf_dynptr *ptr, __u64 flags) = (void *) 199;


static void (*bpf_ringbuf_discard_dynptr)(struct bpf_dynptr *ptr, __u64 flags) = (void *) 200;


static long (*bpf_dynptr_read)(void *dst, __u32 len, const struct bpf_dynptr *src, __u32 offset, __u64 flags) = (void *) 201;


static long (*bpf_dynptr_write)(const struct bpf_dynptr *dst, __u32 offset, void *src, __u32 len, __u64 flags) = (void *) 202;


static void *(*bpf_dynptr_data)(const struct bpf_dynptr *ptr, __u32 offset, __u32 len) = (void *) 203;


static __s64 (*bpf_tcp_raw_gen_syncookie_ipv4)(struct iphdr *iph, struct tcphdr *th, __u32 th_len) = (void *) 204;


static __s64 (*bpf_tcp_raw_gen_syncookie_ipv6)(struct ipv6hdr *iph, struct tcphdr *th, __u32 th_len) = (void *) 205;


static long (*bpf_tcp_raw_check_syncookie_ipv4)(struct iphdr *iph, struct tcphdr *th) = (void *) 206;


static long (*bpf_tcp_raw_check_syncookie_ipv6)(struct ipv6hdr *iph, struct tcphdr *th) = (void *) 207;


static __u64 (*bpf_ktime_get_tai_ns)(void) = (void *) 208;


static long (*bpf_user_ringbuf_drain)(void *map, void *callback_fn, void *ctx, __u64 flags) = (void *) 209;


static void *(*bpf_cgrp_storage_get)(void *map, struct cgroup *cgroup, void *value, __u64 flags) = (void *) 210;


static long (*bpf_cgrp_storage_delete)(void *map, struct cgroup *cgroup) = (void *) 211;


