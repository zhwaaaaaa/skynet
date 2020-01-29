
// Author: Ge,Jun (gejun@baidu.com)
// Date: Mon. Nov 7 14:47:36 CST 2011

#include <arpa/inet.h>                         // inet_pton, inet_ntop
#include <netdb.h>                             // gethostbyname_r
#include <unistd.h>                            // gethostname
#include <cerrno>                             // errno
#include <cstring>                            // strcpy
#include <cstdio>                             // snprintf
#include <cstdlib>                            // strtol
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include "endpoint.h"                    // ip_t

//supported since Linux 3.9.
DEFINE_bool(reuse_port, false, "Enable SO_REUSEPORT for all listened sockets");

DEFINE_bool(reuse_addr, true, "Enable SO_REUSEADDR for all listened sockets");

namespace sn {

    int str2ip(const char *ip_str, ip_t *ip) {
        // ip_str can be NULL when called by EndPoint(0, ...)
        if (ip_str != nullptr) {
            for (; isspace(*ip_str); ++ip_str);
            int rc = inet_pton(AF_INET, ip_str, ip);
            if (rc > 0) {
                return 0;
            }
        } else {
            *ip = IP_ANY;
            return 0;
        }
        return -1;
    }

    IPStr ip2str(ip_t ip) {
        IPStr str;
        if (inet_ntop(AF_INET, &ip, str._buf, INET_ADDRSTRLEN) == nullptr) {
            return ip2str(IP_NONE);
        }
        return str;
    }

    int ip2hostname(ip_t ip, char *host, size_t host_len) {
        if (host == nullptr || host_len == 0) {
            errno = EINVAL;
            return -1;
        }
        sockaddr_in sa = {};
        bzero((char *) &sa, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port = 0;    // useless since we don't need server_name
        sa.sin_addr = ip;
        if (getnameinfo((const sockaddr *) &sa, sizeof(sa),
                        host, host_len, NULL, 0, NI_NAMEREQD) != 0) {
            return -1;
        }
        return 0;
    }

    int ip2hostname(ip_t ip, std::string *host) {
        char buf[128];
        if (ip2hostname(ip, buf, sizeof(buf)) == 0) {
            host->assign(buf);
            return 0;
        }
        return -1;
    }

    EndPointStr endpoint2str(const EndPoint &point) {
        EndPointStr str;
        if (inet_ntop(AF_INET, &point.ip, str._buf, INET_ADDRSTRLEN) == nullptr) {
            return endpoint2str(EndPoint(IP_NONE, 0));
        }
        char *buf = str._buf + strlen(str._buf);
        *buf++ = ':';
        snprintf(buf, 16, "%d", point.port);
        return str;
    }

    int hostname2ip(const char *hostname, ip_t *ip) {
        char buf[256];
        if (nullptr == hostname) {
            if (gethostname(buf, sizeof(buf)) < 0) {
                return -1;
            }
            hostname = buf;
        } else {
            // skip heading space
            for (; isspace(*hostname); ++hostname);
        }

#if defined(OS_MACOSX)
        // gethostbyname on MAC is thread-safe (with current usage) since the
        // returned hostent is TLS. Check following link for the ref:
        // https://lists.apple.com/archives/darwin-dev/2006/May/msg00008.html
        struct hostent* result = gethostbyname(hostname);
        if (result == NULL) {
            return -1;
        }
#else
        char aux_buf[1024];
        int error = 0;
        struct hostent ent;
        struct hostent *result = nullptr;
        if (gethostbyname_r(hostname, &ent, aux_buf, sizeof(aux_buf),
                            &result, &error) != 0 || result == nullptr) {
            return -1;
        }
#endif // defined(OS_MACOSX)
        // Only fetch the first address here
        bcopy((char *) result->h_addr, (char *) ip, result->h_length);
        return 0;
    }

    struct MyAddressInfo {
        char my_hostname[256];
        ip_t my_ip;
        IPStr my_ip_str;

        MyAddressInfo() {
            my_ip = IP_ANY;
            if (gethostname(my_hostname, sizeof(my_hostname)) < 0) {
                my_hostname[0] = '\0';
            } else if (hostname2ip(my_hostname, &my_ip) != 0) {
                my_ip = IP_ANY;
            }
            my_ip_str = ip2str(my_ip);
        }
    };

    int str2endpoint(const char *str, EndPoint *point) {
        // Should be enough to hold ip address
        char buf[64];
        size_t i = 0;
        for (; i < sizeof(buf) && str[i] != '\0' && str[i] != ':'; ++i) {
            buf[i] = str[i];
        }
        if (i >= sizeof(buf) || str[i] != ':') {
            return -1;
        }
        buf[i] = '\0';
        if (str2ip(buf, &point->ip) != 0) {
            return -1;
        }
        ++i;
        char *end = nullptr;
        point->port = static_cast<int>(strtol(str + i, &end, 10));
        if (end == str + i) {
            return -1;
        } else if (*end) {
            for (++end; isspace(*end); ++end);
            if (*end) {
                return -1;
            }
        }
        if (point->port < 0 || point->port > 65535) {
            return -1;
        }
        return 0;
    }

    int str2endpoint(const char *ip_str, int port, EndPoint *point) {
        if (str2ip(ip_str, &point->ip) != 0) {
            return -1;
        }
        if (port < 0 || port > 65535) {
            return -1;
        }
        point->port = port;
        return 0;
    }

    int hostname2endpoint(const char *str, EndPoint *point) {
        // Should be enough to hold ip address
        char buf[64];
        size_t i = 0;
        for (; i < sizeof(buf) - 1 && str[i] != '\0' && str[i] != ':'; ++i) {
            buf[i] = str[i];
        }
        if (i == sizeof(buf) - 1) {
            return -1;
        }

        buf[i] = '\0';
        if (hostname2ip(buf, &point->ip) != 0) {
            return -1;
        }
        if (str[i] == ':') {
            ++i;
        }
        char *end = nullptr;
        point->port = static_cast<int>(strtol(str + i, &end, 10));
        if (end == str + i) {
            return -1;
        } else if (*end) {
            for (; isspace(*end); ++end);
            if (*end) {
                return -1;
            }
        }
        if (point->port < 0 || point->port > 65535) {
            return -1;
        }
        return 0;
    }

    int hostname2endpoint(const char *name_str, int port, EndPoint *point) {
        if (hostname2ip(name_str, &point->ip) != 0) {
            return -1;
        }
        if (port < 0 || port > 65535) {
            return -1;
        }
        point->port = port;
        return 0;
    }

    int endpoint2hostname(const EndPoint &point, char *host, size_t host_len) {
        if (ip2hostname(point.ip, host, host_len) == 0) {
            size_t len = strlen(host);
            if (len + 1 < host_len) {
                snprintf(host + len, host_len - len, ":%d", point.port);
            }
            return 0;
        }
        return -1;
    }

    int endpoint2hostname(const EndPoint &point, std::string *host) {
        char buf[128];
        if (endpoint2hostname(point, buf, sizeof(buf)) == 0) {
            host->assign(buf);
            return 0;
        }
        return -1;
    }

    int get_local_side(int fd, EndPoint *out) {
        struct sockaddr addr;
        socklen_t socklen = sizeof(addr);
        const int rc = getsockname(fd, &addr, &socklen);
        if (rc != 0) {
            return rc;
        }
        if (out) {
            *out = sn::EndPoint(*(sockaddr_in *) &addr);
        }
        return 0;
    }

    int get_remote_side(int fd, EndPoint *out) {
        struct sockaddr addr;
        socklen_t socklen = sizeof(addr);
        const int rc = getpeername(fd, &addr, &socklen);
        if (rc != 0) {
            return rc;
        }
        if (out) {
            *out = sn::EndPoint(*(sockaddr_in *) &addr);
        }
        return 0;
    }

    int set_tcp_no_delay(int fd, int val) {
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1) {
            return -1;
        }
        return 0;
    }

    int set_tcp_keep_alive(int fd) {
        int yes = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1) {
            return -1;
        }
        return 0;
    }


    int mark_non_block(int fd) {
        int opts;
        opts = fcntl(fd, F_GETFL);
        if (opts < 0) {
            return -1;
        }
        opts = opts | O_NONBLOCK;
        if (fcntl(fd, F_SETFL, opts) < 0) {
            return -1;
        }
        return 0;
    }

    int tcp_connect(EndPoint point, EndPoint *localAddr) {
        const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            return -1;
        }
        struct sockaddr_in serv_addr;
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr = point.ip;
        serv_addr.sin_port = htons(static_cast<uint16_t>(point.port));
        int rc = ::connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
        if (rc < 0) {
            return -1;
        }
        if (localAddr != nullptr) {
            if (get_local_side(sockfd, localAddr) == 0) {
                CHECK(false) << "Fail to get the local port of sockfd=" << sockfd;
            }
        }
        return sockfd;
    }

    int tcp_listen(EndPoint point) {
        const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            return -1;
        }

        if (FLAGS_reuse_addr) {
#if defined(SO_REUSEADDR)
            const int on = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                           &on, sizeof(on)) != 0) {
                return -1;
            }
#else
            LOG(ERROR) << "Missing def of SO_REUSEADDR while -reuse_addr is on";
        return -1;
#endif
        }

        if (FLAGS_reuse_port) {
#if defined(SO_REUSEPORT)
            const int on = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT,
                           &on, sizeof(on)) != 0) {
                LOG(WARNING) << "Fail to setsockopt SO_REUSEPORT of sockfd=" << sockfd;
            }
#else
            LOG(ERROR) << "Missing def of SO_REUSEPORT while -reuse_port is on";
        return -1;
#endif
        }

        struct sockaddr_in serv_addr;
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr = point.ip;
        serv_addr.sin_port = htons(static_cast<uint16_t>(point.port));
        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
            return -1;
        }
        if (listen(sockfd, 65535) != 0) {
            //             ^^^ kernel would silently truncate backlog to the value
            //             defined in /proc/sys/net/core/somaxconn if it is less
            //             than 65535
            return -1;
        }
        return sockfd;
    }


}  // namespace np
