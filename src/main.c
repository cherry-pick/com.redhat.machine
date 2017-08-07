#include "io.systemd.sysinfo.varlink.h"
#include "util.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <libudev.h>
#include <time.h>
#include <varlink.h>
#include <sys/utsname.h>

typedef struct {
        VarlinkServer *server;

        int epoll_fd;
        int signal_fd;
} Manager;

static void manager_free(Manager *m) {
        if (m->epoll_fd >= 0)
                close(m->epoll_fd);

        if (m->signal_fd >= 0)
                close(m->signal_fd);

        if (m->server)
                varlink_server_free(m->server);

        free(m);
}

static void manager_freep(Manager **mp) {
        if (*mp)
                manager_free(*mp);
}

static long manager_new(Manager **mp) {
        _cleanup_(manager_freep) Manager *m = NULL;

        m = calloc(1, sizeof(Manager));

        m->epoll_fd = -1;
        m->signal_fd = -1;

        *mp = m;
        m = NULL;

        return 0;
}

static char *unquote(const char *s) {
        char *p;

        if (*s == '"') {
                p = strrchr(s, '"');
                if (!p)
                        return NULL;

                if (p == s)
                        return NULL;

                return strndup(s + 1, p - 1 - s);
        }

        p = strrchr(s, '\n');
        if (p)
                return strndup(s, p - s);

        return strdup(s);
}

static long os_release(char **namep,
                       char **idp,
                       char **variantp,
                       char **versionp) {
        FILE *f;
        char line[4096];
        _cleanup_(freep) char *name = NULL;
        _cleanup_(freep) char *id = NULL;
        _cleanup_(freep) char *variant = NULL;
        _cleanup_(freep) char *version = NULL;

        f = fopen("/usr/lib/os-release", "re");
        if (!f)
                return -errno;

        while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, "NAME=", 5) == 0) {
                        free(name);
                        name = unquote(line + 5);
                } else if (strncmp(line, "ID=", 3) == 0) {
                        free(id);
                        id = unquote(line + 3);
                } else if (strncmp(line, "VARIANT=", 8) == 0) {
                        free(variant);
                        variant = unquote(line + 8);
                } else if (strncmp(line, "VERSION_ID=", 11) == 0) {
                        free(version);
                        version = unquote(line + 11);
                }
        }

        fclose(f);

        *namep = name;
        name = NULL;
        *idp = id;
        id = NULL;
        *variantp = variant;
        variant = NULL;
        *versionp = version;
        version = NULL;

        return 0;
}

static long detect_virt(char **namep) {
        FILE *f;
        char name[64];

        f = popen("systemd-detect-virt", "r");
        if (!f)
                return -errno;

        if (fgets(name, sizeof(name), f))
                *namep = unquote(name);

        pclose(f);

        return 0;
}

static long io_systemd_sysinfo_GetInfo(VarlinkServer *server,
                                       VarlinkCall *call,
                                       VarlinkObject *parameters,
                                       uint64_t flags,
                                       void *userdata) {
        _cleanup_(freep) char *name = NULL;
        _cleanup_(freep) char *id = NULL;
        _cleanup_(freep) char *variant = NULL;
        _cleanup_(freep) char *version = NULL;
        _cleanup_(freep) char *virt = NULL;
        struct utsname u;
        _cleanup_(varlink_object_unrefp) VarlinkObject *system = NULL;
        _cleanup_(varlink_object_unrefp) VarlinkObject *virtualization = NULL;
        _cleanup_(varlink_object_unrefp) VarlinkObject *reply = NULL;

        os_release(&name, &id, &variant, &version);
        detect_virt(&virt);
        uname(&u);

        varlink_object_new(&system);
        varlink_object_new(&virtualization);
        varlink_object_new(&reply);

        if (name)
                varlink_object_set_string(system, "name", name);

        if (id)
                varlink_object_set_string(system, "id", id);

        if (variant)
                varlink_object_set_string(system, "variant", variant);

        if (version)
                varlink_object_set_string(system, "version", version);

        varlink_object_set_string(system, "kernel_version", u.release);

        if (virt)
                varlink_object_set_string(virtualization, "name", virt);

        varlink_object_set_string(reply, "hostname", u.nodename);

        varlink_object_set_object(reply, "system", system);
        varlink_object_set_object(reply, "virtualization", virtualization);

        return varlink_call_reply(call, reply, 0);
}

static int make_signalfd(void) {
        sigset_t mask;

        sigemptyset(&mask);
        sigaddset(&mask, SIGTERM);
        sigaddset(&mask, SIGINT);
        sigprocmask(SIG_BLOCK, &mask, NULL);

        return signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
}

static long read_signal(int signal_fd) {
        struct signalfd_siginfo fdsi;
        long size;

        size = read(signal_fd, &fdsi, sizeof(struct signalfd_siginfo));
        if (size != sizeof(struct signalfd_siginfo))
                return -EIO;

        return fdsi.ssi_signo;
}

static long epoll_add(int epoll_fd, int fd, void *ptr) {
        struct epoll_event event = {
                .events = EPOLLIN,
                .data = {
                        .ptr = ptr
                }
        };

        return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
}

int main(int argc, char **argv) {
        _cleanup_(manager_freep) Manager *m = NULL;
        const char *address;
        int fd = -1;
        long r;

        r = manager_new(&m);
        if (r < 0)
                return EXIT_FAILURE;

        address = argv[1];
        if (!address) {
                fprintf(stderr, "Usage: %s ADDRESS\n", program_invocation_short_name);
                return EXIT_FAILURE;
        }

        /* An activator passed us our connection. */
        if (read(3, NULL, 0) == 0)
                fd = 3;

        r = varlink_server_new(&m->server, address, fd,
                               NULL,
                               &io_systemd_sysinfo_varlink, 1);
        if (r < 0) {
                fprintf(stderr, "Unable to start varlink server: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        m->signal_fd = make_signalfd();
        if (m->signal_fd < 0)
                return EXIT_FAILURE;

        m->epoll_fd = epoll_create(EPOLL_CLOEXEC);
        if (m->epoll_fd < 0 ||
            epoll_add(m->epoll_fd, varlink_server_get_fd(m->server), m->server) < 0 ||
            epoll_add(m->epoll_fd, m->signal_fd, NULL) < 0)
                return EXIT_FAILURE;

        r = varlink_server_set_method_callback(m->server, "io.systemd.sysinfo.GetInfo",
                                               io_systemd_sysinfo_GetInfo, m);
        if (r < 0)
                return EXIT_FAILURE;

        for (;;) {
                struct epoll_event event;
                int n;

                n = epoll_wait(m->epoll_fd, &event, 1, -1);
                if (n < 0) {
                        if (errno == EINTR)
                                continue;

                        return EXIT_FAILURE;
                }

                if (n == 0)
                        continue;

                if (event.data.ptr == m->server) {
                        r = varlink_server_process_events(m->server);
                        if (r < 0) {
                                fprintf(stderr, "Control: %s\n", strerror(-r));
                                if (r != -EPIPE)
                                        return EXIT_FAILURE;
                        }
                } else if (event.data.ptr == NULL) {
                        switch (read_signal(m->signal_fd)) {
                                case SIGTERM:
                                case SIGINT:
                                        return EXIT_SUCCESS;

                                default:
                                        return EXIT_FAILURE;
                        }
                }
        }

        return EXIT_SUCCESS;
}
