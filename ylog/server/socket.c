/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

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
        close(*fd);
        return -1;
    }

    /* Linux-style non-filesystem Unix Domain Sockets */
    memset(&address, 0, sizeof(address));
    address.sun_family = PF_LOCAL;
    strcpy(&address.sun_path[1], file); /* local abstract socket server */

    if (bind(*fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        ylog_error("bind %s failed: %s\n", file, strerror(errno));
        close(*fd);
        return -1;
    }

    if (listen(*fd, 3) < 0) {
        ylog_error("listen %s failed: %s\n", file, strerror(errno));
        close(*fd);
        return -1;
    }

    return 0;
}

static int accept_client(int fd) {
    struct sockaddr addr;
    socklen_t addrlen = sizeof addr;
    return accept(fd, &addr, &addrlen);
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
        close(*fd);
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
