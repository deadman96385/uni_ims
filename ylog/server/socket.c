/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#ifdef ANDROID
static int create_socket_local_server_udp(int *fd, char *file) {
    int ret = -1;
    int sock = android_get_control_socket(file);
    if (sock < 0) {
        ret = socket_local_server(file,
                file[0] == '/' ? ANDROID_SOCKET_NAMESPACE_FILESYSTEM : ANDROID_SOCKET_NAMESPACE_RESERVED,
                SOCK_DGRAM); /* DGRAM no need listen */
        if (ret < 0) {
            ylog_error("socket_local_server %s failed: %s\n", file, strerror(errno));
        }
    } else {
        ret = dup(sock); /* ylog might close this fd */
    }
    *fd = ret;
    return ret < 0 ? -1 : 0;
}

static int connect_socket_local_server_udp(int *fd, char *name) {
    int ret;
    *fd = -1;
    ret = socket(PF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (ret < 0) {
        ylog_error("connect_socket_local_server_udp socket %s failed: %s\n", name, strerror(errno));
        return -1;
    }

    struct sockaddr_un un;
    memset(&un, 0, sizeof(struct sockaddr_un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, name);

    if (connect(ret, (struct sockaddr *)&un,
                sizeof(struct sockaddr_un)) < 0) {
        close(ret);
        ylog_error("connect_socket_local_server_udp connect %s failed: %s\n", name, strerror(errno));
        return -1;
    }

    *fd = ret;
    return 0;
}

static int create_socket_local_server(int *fd, char *file) {
    *fd = socket_local_server(file, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (*fd < 0) {
        ylog_error("open %s failed: %s\n", file, strerror(errno));
        return -1;
    }
    return 0;
}

static int connect_socket_local_server(int *fd, char *name) {
    *fd = socket_local_client(name, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (*fd < 0) {
        ylog_error("open %s failed: %s\n", name, strerror(errno));
        return -1;
    }
    return 0;
}
#else
static int create_socket_local_server_udp(int *fd, char *file) {
    UNUSED(file);
    *fd = -1;
    return -1;
}

static int connect_socket_local_server_udp(int *fd, char *name) {
    UNUSED(name);
    *fd = -1;
    return -1;
}

static int create_socket_local_server(int *fd, char *file) {
    struct sockaddr_un address;
    int namelen;

    /* init unix domain socket */
    *fd = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (*fd < 0) {
        ylog_error("open %s failed: %s\n", file, strerror(errno));
        return -1;
    }

    namelen = strlen(file);
    /* Test with length +1 for the *initial* '\0'. */
    if ((namelen + 1) > (int)sizeof(address.sun_path)) {
        ylog_critical("%s length is too long\n", file);
        CLOSE(*fd);
        return -1;
    }

    /* Linux-style non-filesystem Unix Domain Sockets */
    memset(&address, 0, sizeof(address));
    address.sun_family = PF_LOCAL;
    strcpy(&address.sun_path[1], file); /* local abstract socket server */

    if (bind(*fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        ylog_error("bind %s failed: %s\n", file, strerror(errno));
        CLOSE(*fd);
        return -1;
    }

    if (listen(*fd, 3) < 0) {
        ylog_error("listen %s failed: %s\n", file, strerror(errno));
        CLOSE(*fd);
        return -1;
    }

    return 0;
}

static int connect_socket_local_server(int *fd, char *name) {
    struct sockaddr_un address;
    int namelen;
    /* init unix domain socket */
    *fd = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (*fd < 0) {
        ylog_error("%s open %s failed: %s\n", __func__, name, strerror(errno));
        return -1;
    }

    namelen = strlen(name);
    /* Test with length +1 for the *initial* '\0'. */
    if ((namelen + 1) > (int)sizeof(address.sun_path)) {
        ylog_critical("%s %s length is too long\n", __func__, name);
        CLOSE(*fd);
        return -1;
    }
    /* Linux-style non-filesystem Unix Domain Sockets */
    memset(&address, 0, sizeof(address));
    address.sun_family = PF_LOCAL;
    strcpy(&address.sun_path[1], name); /* local abstract socket server */

    if (connect(*fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        ylog_error("%s connect %s failed: %s\n", __func__, name, strerror(errno));
        return -1;
    }

    return 0;
}
#endif

static int accept_client(int fd) {
    struct sockaddr addr;
    socklen_t addrlen = sizeof addr;
    return accept(fd, &addr, &addrlen);
}
