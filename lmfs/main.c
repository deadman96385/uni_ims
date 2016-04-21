#define LOG_TAG "lmfs"
#define LOG_NDEBUG 0

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/netlink.h>

#include <cutils/sockets.h>
#include <cutils/log.h>

#define LMFS_NETLINK_PROTO NETLINK_USERSOCK
#define LMFS_NETLINK_GROUP 21
#define LMFS_NETLINK_MAX_NAME_LENGTH 100


struct killed_app_info{
    int uid;
    int pid;
    int adj;
};
static int lmfs_netlink_socket;
static int lmfs_unix_socket;
static int lmfs_epoll_fd;
static int lmfs_unix_client_socket;

static void handle_lmfs_unix_socket_connect(){
    struct sockaddr addr;
    socklen_t len = sizeof(addr);
    
    if(lmfs_unix_client_socket > 0){
        close(lmfs_unix_client_socket);
    }
    lmfs_unix_client_socket = accept(lmfs_unix_socket,&addr,&len);

    if(lmfs_unix_client_socket < 0) {
        ALOGE("lmfs control socket can't accept,err=%d",errno);
        lmfs_unix_client_socket = 0;
        return;
    }
    ALOGD("unix socket is connected:%d",lmfs_unix_client_socket);
}

static void handle_killing(struct killed_app_info* app_info){
    char buffer[10];
    memset(buffer,0,sizeof(buffer));
    int nameLen = 0;

    if(lmfs_unix_client_socket > 0) {
        nameLen = snprintf(buffer,9,"%d",app_info->pid); 
        if (nameLen < 0){
            return;
        }
        buffer[nameLen]='\n';
        write(lmfs_unix_client_socket,buffer,nameLen+1);
    }
    ALOGD("handle killing done");
}

static void handle_lmfs_netlink_socket_action(){
    struct sockaddr_nl addr;
    struct msghdr msg;
    struct iovec vec;
    char buff[200];
    int ret;
    struct killed_app_info* app_info;

    vec.iov_base=(void*)buff;
    vec.iov_len=sizeof(buff);
    msg.msg_name=(void*)&addr;
    msg.msg_namelen=sizeof(addr);
    msg.msg_iov=&vec;
    msg.msg_iovlen=1;

    ret=recvmsg(lmfs_netlink_socket,&msg,0);
    if(ret < 0){
        ALOGE("lmfs netlink recv error,err=%d",errno);
    }else{
        app_info=(struct killed_app_info*)NLMSG_DATA((struct nlmsghdr*)buff);
        ALOGD("received payload:%d,%d,%d\n",app_info->uid,app_info->pid,app_info->adj);
        handle_killing(app_info);
    }
}

static int lmfs_init_netlink_socket(){
    struct sockaddr_nl addr;
    int group = LMFS_NETLINK_GROUP;
    struct epoll_event event;

    lmfs_netlink_socket = socket(AF_NETLINK,SOCK_RAW,LMFS_NETLINK_PROTO);
    if(lmfs_netlink_socket < 0){
        ALOGE("can't create lmfs netlink socket,err=%d",errno);
        return 0;
    }

    memset(&addr,0,sizeof(struct sockaddr_nl));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    
    if(bind(lmfs_netlink_socket,(struct sockaddr*)&addr,sizeof(addr)) < 0){
        ALOGE("can't bind lmfs netlink socket,err=%d",errno);
        return 0;
    }

    if(setsockopt(lmfs_netlink_socket,270,NETLINK_ADD_MEMBERSHIP,&group,sizeof(group)) < 0){
        ALOGE("can't add lmfs netlink socket to group,err=%d",errno);
        return 0;
    }

    event.events = EPOLLIN;
    event.data.ptr = (void*)handle_lmfs_netlink_socket_action;

    if(epoll_ctl(lmfs_epoll_fd,EPOLL_CTL_ADD,lmfs_netlink_socket,&event) < 0){
        ALOGE("can't add lmfs netlink socket to epoll,err=%d",errno);
        return 0;
    }
    return 1;
    
}

static int lmfs_init_unix_socket(){
    int rv = 0;
    struct epoll_event event;
    lmfs_unix_socket = android_get_control_socket("lmfs");
    if(lmfs_unix_socket < 0){
        ALOGE("can't get lmfs socket");
        return 0;
    }

    rv = listen(lmfs_unix_socket,1);
    if(rv < -1){
        ALOGE("lmfs can't listen,err=%d",errno);
        return 0;
    }

    event.events = EPOLLIN;
    event.data.ptr = (void*)handle_lmfs_unix_socket_connect;
    if(epoll_ctl(lmfs_epoll_fd,EPOLL_CTL_ADD,lmfs_unix_socket,&event) < 0){
        ALOGE("can't add lmfs to epoll,err=%d",errno);
        return 0;
    }
    return 1;
}

static int lmfs_init_epoll(){
    lmfs_epoll_fd = epoll_create(2);
    if (lmfs_epoll_fd == -1) {
        ALOGE("epoll create failed (errno=%d)\n",errno);
        return 0;
    }
    return 1;
}

static int lmfs_init(){
    int ret = 1;
    ret = lmfs_init_epoll();
    ret &= lmfs_init_netlink_socket();
    ret &= lmfs_init_unix_socket();
    return ret;
}

static void lmfs_loop(){
    while(1){
        struct epoll_event events[2];
        int nr_events;
        int i;
        nr_events = epoll_wait(lmfs_epoll_fd,events,2,-1);

        if(nr_events == -1) continue;

        for(i=0;i<nr_events;i++){
            if(events[i].events & EPOLLERR){
                ALOGE("epoll error on event #%d",i);
            }

            if(events[i].data.ptr){
                (*(void(*)())events[i].data.ptr)();
            }
        }
        
    }
}

int main(){
    mlockall(MCL_FUTURE);
    if(lmfs_init()){
        lmfs_loop();
    }
    ALOGE("quit");
    return 0;
}
