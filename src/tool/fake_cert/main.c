/*================================================================
*   Created：2018.06.26
*   Description：
*
================================================================*/
#include "bs.h"
#include <errno.h>
#include <getopt.h>
#include <net/if.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <fcntl.h>
#include <stdlib.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/ocsp.h>
#include <openssl/bn.h>

#include "utl/subcmd_utl.h"
#include "utl/daemon_utl.h"
#include "utl/socket_utl.h"
#include "utl/process_utl.h"
#include "utl/pidfile_utl.h"

#ifdef IN_UNIXLIKE
#include <sys/types.h> 
#include <sys/wait.h>
#endif

#include "fakecert_conf.h"
#include "fakecert_lib.h"
#include "fakecert_service.h"
#include "fakecert_license.h"
#include "fakecert_acl.h"
#include "fakecert_log.h"
#include "fakecert_dnsnames.h"

static int fakecert_run(int argc, char **argv);
static int fakecert_build(int argc, char **argv);
static int fakecert_create(int argc, char **argv);

static SUB_CMD_NODE_S g_subcmds[] = 
{
    {"run", fakecert_run, "Run service"},
    {"build", fakecert_build, "Build cert"},
    {"create", fakecert_create, "Create cert"},
    {NULL, NULL}
};

static void fakecert_help_run()
{
    printf("Usage: fakecert run [OPTIONS]\r\n");
    printf("Options:\r\n");
    printf("  -h,       Help\r\n");
    printf("  -u path,  Listen unix socket. Default /var/run/fakecert.socket. If you want to close unix socket service please use 0.\r\n");
    printf("  -p port,  Listen tcp port. Default port 4567. If you want to close tcp service please use 0.\r\n");
    printf("  -t,       Use terminal, not daemon.\r\n");
    printf("  -f file,  Acl file. Default file is fakecert_acl.txt \r\n");
    printf("  -m,       Master-Worker. Default not support. \r\n");
    return;
}

static void fakecert_help_build()
{
    printf("Usage: fackcert build [OPTIONS] HOSTNAME\r\n");
    printf("Options:\r\n");
    printf("  -h,   Help\r\n");
    return;
}

static void fakecert_help_create()
{
    printf("Usage: fackcert create [OPTIONS] HOSTNAME\r\n");
    printf("Options:\r\n");
    printf("  -h,   Help\r\n");
    return;
}

static int fakecert_worker(char *unix_path, char *acl_file, unsigned short port)
{
    fakecert_conf_init();
    fakecert_service_init();
	fakecert_dnsnames_init();

    if (acl_file) {
        fakecert_acl_init(acl_file);
    }

#ifdef IN_UNIXLIKE
    if (unix_path != NULL) {
        fakecert_service_open_unix(unix_path);
    }
#endif

    if (port != 0) {
        fakecert_service_open_tcp(port);
    }

    fakecert_service_run();

    return 0;

}

static int fakecert_master(int argc, char **argv)
{
    int pid;

    PROCESS_RenameSelf("fakecert_master");

    while (1) {
        pid = fork();
        if (pid < 0) {
            printf("Can't fork worker");
            return -1;
        } else if (pid > 0) {
            while (waitpid(pid, 0, 0) < 0);
        } else {
            break;
        }
    }

    PROCESS_RenameSelf("fakecert_worker");

    return 0;
}

static int fakecert_run(int argc, char **argv)
{
    int c;
    char *unix_path = "/var/run/fakecert.socket";
    char *acl_file = "./fakecert_acl.txt";
    unsigned short port = 4567;
    int isdaemon = 1;
    int master_worker = 0;

    if (argc < 1) {
        fakecert_help_run();
        return -1;
    }

    while ((c = getopt(argc, argv, "htmu:p:f:")) != -1) {
        switch (c) {
            case 'h':
                fakecert_help_run();
                return 0;
                break;
            case 'u':
                unix_path = optarg;
                if (unix_path[0] == '0') {
                    unix_path = NULL;
                }
                break;
            case 'p':
                port = atol(optarg);
                break;
            case 't':
                isdaemon = 0;
                break;
            case 'f':
                acl_file = optarg;
                break;
            case 'm':
                master_worker = 1;
                break;
            default:
                printf("Unknown option -%c\r\n", c);
                fakecert_help_run();
                return -1;
        }
    }

    if (isdaemon) {
        DAEMON_Init(1, 0);
    }

    if (master_worker) {
        if (fakecert_master(argc, argv) < 0) {
            return -1;
        }
    }

    return fakecert_worker(unix_path, acl_file, port);
}

static int fakecert_build(int argc, char **argv)
{
    char *hostname;
    unsigned int ip;

    if (argc < 2) {
        fakecert_help_build();
        return -1;
    }

    hostname = argv[1];

    ip = Socket_NameToIpNet(hostname);
    fakecert_build_by_dnsname(ip, htons(443), hostname);

    return 0;
}

static int fakecert_create(int argc, char **argv)
{
    char *hostname;

    if (argc < 2) {
        fakecert_help_create();
        return -1;
    }

    hostname = argv[1];

    fakecert_create_by_hostname(hostname);

    return 0;
}

static int fakecert_create_pidfile(char *progname)
{
    char *pid_filename = "fakecert.pid";

    if (PIDFILE_Lock(pid_filename)) {
        return -1;
    }

    PIDFILE_Create(pid_filename);

    return 0;
}

void record_signal_and_exit(int signal_no, siginfo_t *pinfo, void*arg)
{
    if (signal_no != SIGPIPE) {
        printf("receive signal %d and exit\n", signal_no);
        exit(-1);
    } else {
        printf("receive signala pipe\n");
    }
}

static void fakecert_statement()
{
    printf("**************************************\r\n");
    printf("Version: 1.0\r\n");
    printf("Build Date: %s %s\r\n", __DATE__, __TIME__);
    printf("**************************************\r\n");
}

int main(int argc, char **argv)
{
    fakecert_statement();

    if (fakecert_license_verify() != 0) {
        return -1;
    }

    fakecert_create_pidfile(argv[0]);

    if (0 != fakecert_init()) {
        printf("facecert init fail\n");
        return -1;
    }

 
    return SUBCMD_Do(g_subcmds, argc, argv);
}

