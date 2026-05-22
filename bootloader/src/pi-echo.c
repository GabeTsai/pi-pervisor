#include <assert.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <poll.h>
#include <errno.h>
#include <string.h>

#include "boot-unix-io.h"
#include "pi-echo.h"

#define output(...) printf(__VA_ARGS__)

static void clean_exit(const char *msg) {
    output("%s", msg);
    exit(0);
}

static void sys_die(const char *syscall, const char *msg) {
    fprintf(stderr, "%s", msg);
    perror(syscall);
    exit(1);
}

static void notreached(void) {
    abort();
}

static int can_read_timeout(int fd, int timeout_ms) {
    struct pollfd pfd = {
        .fd = fd,
        .events = POLLIN,
    };

    int n = poll(&pfd, 1, timeout_ms);
    if(n < 0 && errno != EINTR)
        sys_die("poll", "poll failed\n");
    return n > 0 && (pfd.revents & (POLLIN | POLLERR | POLLHUP | POLLNVAL));
}

static int read_timeout(int fd, void *buf, size_t n, int timeout_ms) {
    if(!can_read_timeout(fd, timeout_ms))
        return 0;

    ssize_t got = read(fd, buf, n);
    if(got < 0)
        sys_die("read", "stdin read failed\n");
    return (int)got;
}

static int tty_gone(const char *portname) {
    return access(portname, F_OK) != 0;
}

// code credit to CS140E
// hack-y state machine to indicate when we've seen the special string
// 'DONE!!!' from the pi telling us to shutdown.
int pi_done(unsigned char *s) {
    static unsigned pos = 0;
    const char exit_string[] = "DONE!!!\n";
    const int n = sizeof exit_string - 1;

    for(; *s; s++) {
        assert(pos < n);
        if(*s != exit_string[pos++]) {
            pos = 0;
            return pi_done(s+1); // check remainder
        }
        // maybe should check if "DONE!!!" is last thing printed?
        if(pos == sizeof exit_string - 1)
            return 1;
    }
    return 0;
}

// overwrite any unprintable characters with a space.
// otherwise terminals can go haywire/bizarro.
// note, the string can contain 0's, so we send the
// size.
void remove_nonprint(uint8_t *buf, int n) {
    for(int i = 0; i < n; i++) {
        uint8_t *p = &buf[i];
        if(isprint(*p) || (isspace(*p) && *p != '\r'))
            continue;
        *p = ' ';
    }
}

// read and echo the characters from the usbtty until it closes 
// (pi rebooted) or we see a string indicating a clean shutdown.
void pi_echo(int unix_fd, int pi_fd, const char *portname) {
    assert(pi_fd);

    while(1) {
        unsigned char buf[4096];

        int n;
        if((n=read_timeout(unix_fd, buf, sizeof buf - 1, 1000))) {
            buf[n] = 0;
            // output("about to echo <%s> to pi\n", buf);
            write_exact(pi_fd, buf, n);
        }

        if(!can_read_timeout(pi_fd, 3000))
            continue;
        n = read(pi_fd, buf, sizeof buf - 1);

        if(!n) {
            // this isn't the program's fault.  so we exit(0).
            if(!portname || tty_gone(portname))
                clean_exit("pi ttyusb connection closed.  cleaning up\n");
            // so we don't keep banging on the CPU.
            usleep(1000);
        } else if(n < 0) {
            sys_die("read", "pi connection closed.  cleaning up\n");
        } else {
            buf[n] = 0;
            // if you keep getting "" "" "" it's b/c of the GET_CODE message from bootloader
            remove_nonprint(buf,n);
            output("%s", buf);

            if(pi_done(buf)) {
                // output("\nSaw done\n");
                clean_exit("\nbootloader: pi exited.  cleaning up\n");
            }
        }
    }
    notreached();
}
