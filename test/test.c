#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include "util.h"
#include "net.h"
#include "ip.h"

#include "driver/loopback.h"

#include "test.h"

static volatile sig_atomic_t terminate;

static void
on_signal(int signum)
{
    (void)signum;
    terminate = 1;
}

static int
setup(void)
{
    struct sigaction sa = {0};
    struct net_device *dev;
    struct ip_iface *iface;

    sa.sa_handler = on_signal;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        errorf("sigaction() %s", strerror(errno));
        return -1;
    }
    infof("setup protocol stack...");
    if (net_init() == -1) {
        errorf("net_init() failure");
        return -1;
    }
    dev = loopback_init();
    if (!dev) {
        errorf("loopback_init() failure");
        return -1;
    }
    iface = ip_iface_alloc(LOOPBACK_IP_ADDR, LOOPBACK_NETMASK);
    if (!iface) {
        errorf("ip_iface_alloc() failure");
        return -1;
    }
    if (ip_iface_register(dev, iface) == -1) {
        errorf("ip_iface_register() failure");
        return -1;
    }
    if (net_run() == -1) {
        errorf("net_run() failure");
        return -1;
    }
    return 0;
}

static int
cleanup(void)
{
    infof("cleanup protocol stack...");
    if (net_shutdown() == -1) {
        errorf("net_shutdown() failure");
        return -1;
    }
    return 0;
}

static int
app_main(void)
{
    ip_addr_t src, dst;
    size_t offset = IP_HDR_SIZE_MIN;

    ip_addr_pton(LOOPBACK_IP_ADDR, &src);
    dst = src;
    debugf("press Ctrl+C to terminate");
    while (!terminate) {
        if (ip_output(1, test_data + offset, sizeof(test_data) - offset, src, dst) == -1) {
            errorf("ip_output() failure");
            break;
        }
        sleep(1);
    }
    debugf("terminate");
    return 0;
}

int
main(void)
{
    int ret;

    if (setup() == -1) {
        errorf("setup() failure");
        return -1;
    }
    ret = app_main();
    if (cleanup() == -1) {
        errorf("cleanup() failure");
        return -1;
    }
    return ret;
}
