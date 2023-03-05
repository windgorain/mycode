/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang
* Description: 
* History:     
******************************************************************************/
#include "klc/klc_base.h"

#define KLC_MODULE_NAME "initiator/osbase"
KLC_DEF_MODULE();

static inline void _osbase_init_os_helper_one(int id, char *name, u64 err)
{
    void *func = NULL;

    if (name) {
        func = KLCHLP_KAllSymsLookupName(name);
        if (! func) {
//            KLCHLP_KoPrint("KLC can't find sym %s \n", name, 0, 0);
        }
    }

    KLCHLP_SetOsHelper(id, func, err);
}

#define OSBASSE_INIT_HELPER(id, name, err) do { \
    char _name[] = name; \
    _osbase_init_os_helper_one(id, _name, err); \
}while(0)

static inline void _osbase_init_os_helper()
{
    OSBASSE_INIT_HELPER(1, "bpf_map_lookup_elem", 0);
    OSBASSE_INIT_HELPER(2, "bpf_map_update_elem", -1);
    OSBASSE_INIT_HELPER(3, "bpf_map_delete_elem", -1);
    OSBASSE_INIT_HELPER(4, "bpf_probe_read", -1);
    OSBASSE_INIT_HELPER(5, "bpf_ktime_get_ns", 0);
    OSBASSE_INIT_HELPER(6, "bpf_trace_printk", -1);
    OSBASSE_INIT_HELPER(7, "bpf_user_rnd_u32", 0);
    OSBASSE_INIT_HELPER(8, "bpf_get_smp_processor_id", 0);
    OSBASSE_INIT_HELPER(9, "bpf_skb_store_bytes", -1);
    OSBASSE_INIT_HELPER(10, "bpf_l3_csum_replace", -1);
    OSBASSE_INIT_HELPER(11, "bpf_l4_csum_replace", -1);
    _osbase_init_os_helper_one(12, NULL, -1); /* bpf_tail_call */
    OSBASSE_INIT_HELPER(13, "bpf_clone_redirect", -1);
    OSBASSE_INIT_HELPER(14, "bpf_get_current_pid_tgid", 0);
    OSBASSE_INIT_HELPER(15, "bpf_get_current_uid_gid", 0);
    OSBASSE_INIT_HELPER(16, "bpf_get_current_comm", -1);
    OSBASSE_INIT_HELPER(17, "bpf_get_cgroup_classid", 0);
    OSBASSE_INIT_HELPER(18, "bpf_skb_vlan_push", -1);
    OSBASSE_INIT_HELPER(19, "bpf_skb_vlan_pop", -1);
    OSBASSE_INIT_HELPER(20, "bpf_skb_get_tunnel_key", -1);
    OSBASSE_INIT_HELPER(21, "bpf_skb_set_tunnel_key", -1);
    OSBASSE_INIT_HELPER(22, "bpf_perf_event_read", -1);
    OSBASSE_INIT_HELPER(23, "bpf_redirect", 0);
    OSBASSE_INIT_HELPER(24, "bpf_get_route_realm", 0);
    OSBASSE_INIT_HELPER(25, "bpf_perf_event_output", -1);
    OSBASSE_INIT_HELPER(26, "bpf_skb_load_bytes", -1);
    OSBASSE_INIT_HELPER(27, "bpf_get_stackid", -1);
    OSBASSE_INIT_HELPER(28, "bpf_csum_diff", -1);
    OSBASSE_INIT_HELPER(29, "bpf_skb_get_tunnel_opt", 0);
    OSBASSE_INIT_HELPER(30, "bpf_skb_set_tunnel_opt", -1);
    OSBASSE_INIT_HELPER(31, "bpf_skb_change_proto", -1);
    OSBASSE_INIT_HELPER(32, "bpf_skb_change_type", -1);
    OSBASSE_INIT_HELPER(33, "bpf_skb_under_cgroup", -1);
    OSBASSE_INIT_HELPER(34, "bpf_get_hash_recalc", 0);
    OSBASSE_INIT_HELPER(35, "bpf_get_current_task", 0);
    OSBASSE_INIT_HELPER(36, "bpf_probe_write_user", -1);
    OSBASSE_INIT_HELPER(37, "bpf_current_task_under_cgroup", -1);
    OSBASSE_INIT_HELPER(38, "bpf_skb_change_tail", -1);
    OSBASSE_INIT_HELPER(39, "bpf_skb_pull_data", -1);
    OSBASSE_INIT_HELPER(40, "bpf_csum_update", -1);
    OSBASSE_INIT_HELPER(41, "bpf_set_hash_invalid", 0);
    OSBASSE_INIT_HELPER(42, "bpf_get_numa_node_id", 0);
    OSBASSE_INIT_HELPER(43, "bpf_skb_change_head", -1);
    OSBASSE_INIT_HELPER(44, "bpf_xdp_adjust_head", -1);
    OSBASSE_INIT_HELPER(45, "bpf_probe_read_str", -1);
    OSBASSE_INIT_HELPER(46, "bpf_get_socket_cookie", 0);
    OSBASSE_INIT_HELPER(47, "bpf_get_socket_uid", 65534);
    OSBASSE_INIT_HELPER(48, "bpf_set_hash", 0);
    OSBASSE_INIT_HELPER(49, "bpf_setsockopt", -1);
    OSBASSE_INIT_HELPER(50, "bpf_skb_adjust_room", -1);
    OSBASSE_INIT_HELPER(51, "bpf_xdp_redirect_map", 0);
    OSBASSE_INIT_HELPER(52, "bpf_sk_redirect_map", 0);
    OSBASSE_INIT_HELPER(53, "bpf_sock_map_update", -1);
    OSBASSE_INIT_HELPER(54, "bpf_xdp_adjust_meta", -1);
    OSBASSE_INIT_HELPER(55, "bpf_perf_event_read_value", -1);
    OSBASSE_INIT_HELPER(56, "bpf_perf_prog_read_value", -1);
    OSBASSE_INIT_HELPER(57, "bpf_getsockopt", -1);
    OSBASSE_INIT_HELPER(58, "bpf_override_return", 0);
    OSBASSE_INIT_HELPER(59, "bpf_sock_ops_cb_flags_set", -1);
    OSBASSE_INIT_HELPER(60, "bpf_msg_redirect_map", 0);
    OSBASSE_INIT_HELPER(61, "bpf_msg_apply_bytes", 0);
    OSBASSE_INIT_HELPER(62, "bpf_msg_cork_bytes", 0);
    OSBASSE_INIT_HELPER(63, "bpf_msg_pull_data", -1);
    OSBASSE_INIT_HELPER(64, "bpf_bind", -1);
    OSBASSE_INIT_HELPER(65, "bpf_xdp_adjust_tail", -1);
    OSBASSE_INIT_HELPER(66, "bpf_skb_get_xfrm_state", -1);
    OSBASSE_INIT_HELPER(67, "bpf_get_stack", -1);
    OSBASSE_INIT_HELPER(68, "bpf_skb_load_bytes_relative", -1);
    _osbase_init_os_helper_one(69, NULL, -1); /* bpf_skb_fib_lookup or bpf_xdp_fib_lookup 不知道该使用哪个 */
    OSBASSE_INIT_HELPER(70, "bpf_sock_hash_update", -1);
    OSBASSE_INIT_HELPER(71, "bpf_msg_redirect_hash", 0);
    OSBASSE_INIT_HELPER(72, "bpf_sk_redirect_hash", 0);
    OSBASSE_INIT_HELPER(73, "bpf_lwt_push_encap", -1);
    OSBASSE_INIT_HELPER(74, "bpf_lwt_seg6_store_bytes", -1);
    OSBASSE_INIT_HELPER(75, "bpf_lwt_seg6_adjust_srh", -1);
    OSBASSE_INIT_HELPER(76, "bpf_lwt_seg6_action", -1);
    OSBASSE_INIT_HELPER(77, "bpf_rc_repeat", 0);
    OSBASSE_INIT_HELPER(78, "bpf_rc_keydown", 0);
    OSBASSE_INIT_HELPER(79, "bpf_skb_cgroup_id", 0);
    OSBASSE_INIT_HELPER(80, "bpf_get_current_cgroup_id", 0);
    OSBASSE_INIT_HELPER(81, "bpf_get_local_storage", 0);
    OSBASSE_INIT_HELPER(82, "sk_select_reuseport", -1);
    OSBASSE_INIT_HELPER(83, "bpf_skb_ancestor_cgroup_id", 0);
    OSBASSE_INIT_HELPER(84, "bpf_sk_lookup_tcp", 0);
    OSBASSE_INIT_HELPER(85, "bpf_sk_lookup_udp", 0);
    OSBASSE_INIT_HELPER(86, "bpf_sk_release", -1);
    OSBASSE_INIT_HELPER(87, "bpf_map_push_elem", -1);
    OSBASSE_INIT_HELPER(88, "bpf_map_pop_elem", -1);
    OSBASSE_INIT_HELPER(89, "bpf_map_peek_elem", -1);
    OSBASSE_INIT_HELPER(90, "bpf_msg_push_data", -1);
    OSBASSE_INIT_HELPER(91, "bpf_msg_pop_data", -1);
    OSBASSE_INIT_HELPER(92, "bpf_rc_pointer_rel", 0);
    OSBASSE_INIT_HELPER(93, "bpf_spin_lock", 0);
    OSBASSE_INIT_HELPER(94, "bpf_spin_unlock", 0);
    OSBASSE_INIT_HELPER(95, "bpf_sk_fullsock", 0);
    OSBASSE_INIT_HELPER(96, "bpf_tcp_sock", 0);
    OSBASSE_INIT_HELPER(97, "bpf_skb_ecn_set_ce", 0);
    OSBASSE_INIT_HELPER(98, "bpf_get_listener_sock", 0);
    OSBASSE_INIT_HELPER(99, "bpf_skc_lookup_tcp", 0);
    OSBASSE_INIT_HELPER(100, "bpf_tcp_check_syncookie", -1);
    OSBASSE_INIT_HELPER(101, "bpf_sysctl_get_name", -22);
    OSBASSE_INIT_HELPER(102, "bpf_sysctl_get_current_value", -22);
    OSBASSE_INIT_HELPER(103, "bpf_sysctl_get_new_value", -22);
    OSBASSE_INIT_HELPER(104, "bpf_sysctl_set_new_value", -22);
    OSBASSE_INIT_HELPER(105, "bpf_strtol", -22);
    OSBASSE_INIT_HELPER(106, "bpf_strtoul", -22);
    OSBASSE_INIT_HELPER(107, "bpf_sk_storage_get", 0);
    OSBASSE_INIT_HELPER(108, "bpf_sk_storage_delete", -1);
    OSBASSE_INIT_HELPER(109, "bpf_send_signal", -22);
    OSBASSE_INIT_HELPER(110, "bpf_tcp_gen_syncookie", -22);
    OSBASSE_INIT_HELPER(111, "bpf_skb_output", -1);
    OSBASSE_INIT_HELPER(112, "bpf_probe_read_user", -1);
    OSBASSE_INIT_HELPER(113, "bpf_probe_read_kernel", -1);
    OSBASSE_INIT_HELPER(114, "bpf_probe_read_user_str", -1);
    OSBASSE_INIT_HELPER(115, "bpf_probe_read_kernel_str", -1);
    OSBASSE_INIT_HELPER(116, "bpf_tcp_send_ack", -1);
    OSBASSE_INIT_HELPER(117, "bpf_send_signal_thread", -1);
    OSBASSE_INIT_HELPER(118, "bpf_jiffies64", 0);
    OSBASSE_INIT_HELPER(119, "bpf_read_branch_records", -1);
    OSBASSE_INIT_HELPER(120, "bpf_get_ns_current_pid_tgid", -1);
    OSBASSE_INIT_HELPER(121, "bpf_xdp_output", -1);
    OSBASSE_INIT_HELPER(122, "bpf_get_netns_cookie", -1);
    OSBASSE_INIT_HELPER(123, "bpf_get_current_ancestor_cgroup_id", 0);
    OSBASSE_INIT_HELPER(124, "bpf_sk_assign", -1);
    OSBASSE_INIT_HELPER(125, "bpf_ktime_get_boot_ns", 0);
    OSBASSE_INIT_HELPER(126, "bpf_seq_printf", -1);
    OSBASSE_INIT_HELPER(127, "bpf_seq_write", -1);
    OSBASSE_INIT_HELPER(128, "bpf_sk_cgroup_id", 0);
    OSBASSE_INIT_HELPER(129, "bpf_sk_ancestor_cgroup_id", 0);
    OSBASSE_INIT_HELPER(130, "bpf_ringbuf_output", -1);
    OSBASSE_INIT_HELPER(131, "bpf_ringbuf_reserve", 0);
    OSBASSE_INIT_HELPER(132, "bpf_ringbuf_submit", 0);
    OSBASSE_INIT_HELPER(133, "bpf_ringbuf_discard", 0);
    OSBASSE_INIT_HELPER(134, "bpf_ringbuf_query", 0);
    OSBASSE_INIT_HELPER(135, "bpf_csum_level", -1);
    OSBASSE_INIT_HELPER(136, "bpf_skc_to_tcp6_sock", 0);
    OSBASSE_INIT_HELPER(137, "bpf_skc_to_tcp_sock", 0);
    OSBASSE_INIT_HELPER(138, "bpf_skc_to_tcp_timewait_sock", 0);
    OSBASSE_INIT_HELPER(139, "bpf_skc_to_tcp_request_sock", 0);
    OSBASSE_INIT_HELPER(140, "bpf_skc_to_udp6_sock", 0);
    OSBASSE_INIT_HELPER(141, "bpf_get_task_stack", -1);
    OSBASSE_INIT_HELPER(142, "bpf_load_hdr_opt", -1);
    OSBASSE_INIT_HELPER(143, "bpf_store_hdr_opt", -1);
    OSBASSE_INIT_HELPER(144, "bpf_reserve_hdr_opt", -1);
    OSBASSE_INIT_HELPER(145, "bpf_inode_storage_get", 0);
    OSBASSE_INIT_HELPER(146, "bpf_inode_storage_delete", -1);
    OSBASSE_INIT_HELPER(147, "bpf_d_path", -1);
    OSBASSE_INIT_HELPER(148, "bpf_copy_from_user", -1);
    OSBASSE_INIT_HELPER(149, "bpf_snprintf_btf", -1);
    OSBASSE_INIT_HELPER(150, "bpf_seq_printf_btf", -1);
    OSBASSE_INIT_HELPER(151, "bpf_skb_cgroup_classid", 0);
    OSBASSE_INIT_HELPER(152, "bpf_redirect_neigh", 2);
    OSBASSE_INIT_HELPER(153, "bpf_per_cpu_ptr", 0);
    OSBASSE_INIT_HELPER(154, "bpf_this_cpu_ptr", 0);
    OSBASSE_INIT_HELPER(155, "bpf_redirect_peer", 2);
    OSBASSE_INIT_HELPER(156, "bpf_task_storage_get", 0);
    OSBASSE_INIT_HELPER(157, "bpf_task_storage_delete", -1);
    OSBASSE_INIT_HELPER(158, "bpf_get_current_task_btf", 0);
    OSBASSE_INIT_HELPER(159, "bpf_bprm_opts_set", -1);
    OSBASSE_INIT_HELPER(160, "bpf_ktime_get_coarse_ns", 0);
    OSBASSE_INIT_HELPER(161, "bpf_ima_inode_hash", -1);
    OSBASSE_INIT_HELPER(162, "bpf_sock_from_file", 0);
    OSBASSE_INIT_HELPER(163, "bpf_check_mtu", -1);
    OSBASSE_INIT_HELPER(164, "bpf_for_each_map_elem", -1);
    OSBASSE_INIT_HELPER(165, "bpf_snprintf", -16);
    OSBASSE_INIT_HELPER(166, "bpf_sys_bpf", -1);
    OSBASSE_INIT_HELPER(167, "bpf_btf_find_by_name_kind", 0);
    OSBASSE_INIT_HELPER(168, "bpf_sys_close", -1);
    OSBASSE_INIT_HELPER(169, "bpf_timer_init", -1);
    OSBASSE_INIT_HELPER(170, "bpf_timer_set_callback", -22);
    OSBASSE_INIT_HELPER(171, "bpf_timer_start", -22);
    OSBASSE_INIT_HELPER(172, "bpf_timer_cancel", -22);
    OSBASSE_INIT_HELPER(173, "bpf_get_func_ip", 0);
    OSBASSE_INIT_HELPER(174, "bpf_get_attach_cookie", 0);
    OSBASSE_INIT_HELPER(175, "bpf_task_pt_regs", 0);
    OSBASSE_INIT_HELPER(176, "bpf_get_branch_snapshot", -22);
    OSBASSE_INIT_HELPER(177, "bpf_trace_vprintk", -1);
    OSBASSE_INIT_HELPER(178, "bpf_skc_to_unix_sock", 0);
    OSBASSE_INIT_HELPER(179, "bpf_kallsyms_lookup_name", -22);
    OSBASSE_INIT_HELPER(180, "bpf_find_vma", -22);
    OSBASSE_INIT_HELPER(181, "bpf_loop", -22);
    OSBASSE_INIT_HELPER(182, "bpf_strncmp", -22);
    OSBASSE_INIT_HELPER(183, "bpf_get_func_arg", -22);
    OSBASSE_INIT_HELPER(184, "bpf_get_func_ret", -1);
    OSBASSE_INIT_HELPER(185, "bpf_get_func_arg_cnt", 0);
    OSBASSE_INIT_HELPER(186, "bpf_get_retval", -1);
    OSBASSE_INIT_HELPER(187, "bpf_set_retval", -1);
    OSBASSE_INIT_HELPER(188, "bpf_xdp_get_buff_len", 0);
    OSBASSE_INIT_HELPER(189, "bpf_xdp_load_bytes", -1);
    OSBASSE_INIT_HELPER(190, "bpf_xdp_store_bytes", -1);
    OSBASSE_INIT_HELPER(191, "bpf_copy_from_user_task", -1);
}

static inline void _osbase_init_os_func_one(int id, char *name)
{
    void *sym= NULL;

    if (name) {
        sym = KLCHLP_KAllSymsLookupName(name);
        if (! sym) {
            KLCHLP_KoPrint("KLC can't find sym %s \n", (long)name, 0, 0);
        }
    }

    KLCHLP_SetSym(id, sym);
}

#define OSBASSE_INIT_FUNC(id, name) do { \
    char _name[] = name; \
    _osbase_init_os_func_one(id, _name); \
}while(0)

static inline void _osbase_init_os_funcs()
{
    OSBASSE_INIT_FUNC(KLCSYM_ID_ip_local_out, "ip_local_out");
    OSBASSE_INIT_FUNC(KLCSYM_ID_ip_route_output_flow, "ip_route_output_flow");
    OSBASSE_INIT_FUNC(KLCSYM_ID_init_net, "init_net");
    OSBASSE_INIT_FUNC(KLCSYM_ID___alloc_skb, "__alloc_skb");
    OSBASSE_INIT_FUNC(KLCSYM_ID___kmalloc, "__kmalloc");
    OSBASSE_INIT_FUNC(KLCSYM_ID_kfree, "kfree");
    OSBASSE_INIT_FUNC(KLCSYM_ID_kfree_skb, "kfree_skb");
    OSBASSE_INIT_FUNC(KLCSYM_ID_sg_init_one, "sg_init_one");
    OSBASSE_INIT_FUNC(KLCSYM_ID_wait_for_completion, "wait_for_completion");
    
}

/* 定义一个name function */
SEC("klc/namefunc/osbase_init")
u64 _osbase_init()
{
    /* 初始化随机数生成器 */
    KLCHLP_SysCall("bpf_user_rnd_init_once", 0, 0, 0);

    /* 建立os bpf helper表 */
    _osbase_init_os_helper();

    /* 建立一些常用的函数表 */
    _osbase_init_os_funcs();

    return 0;
}

/* 定义init段, 指定初始化调用函数 */
KLC_FUNC_INIT_S _init[] SEC("klc/init") = {
    {.func = "osbase_init"}
};


