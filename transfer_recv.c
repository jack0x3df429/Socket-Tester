#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <netinet/in.h>
#include <linux/net_tstamp.h>
#include <time.h>
#include <pthread.h>
#define MAX_BUFFER_SIZE 1000
#define AVG_SIZE 400
long long tstoll(struct timespec ts) {
    return ts.tv_sec*(int)1e9+ts.tv_nsec;
}
struct pipedata {
    long bytesRead;
    int delay;
    int latency;
    long long t;
};
long avgNpass(long *arr,long new){
    long long sum=new;
    //printf("%lld\n",sum);
    for(int i=AVG_SIZE-1;i>0;i--){
      arr[i]=arr[i-1];
      sum+=arr[i];
      
    }
    arr[0]=new;
    return sum/AVG_SIZE;
}
void* Data_Proc(void* pipefd) {
    int *fd = (int*) pipefd; // 取得輸入資料
    char file_name_delay[7]="u_dly";
    char file_name_bitrate[7]="u_brt";
    char file_name_sec[7]="u_sec";
    sprintf(file_name_delay+5,"%d",fd[2]%10);
    sprintf(file_name_bitrate+5,"%d",fd[2]%10);
    sprintf(file_name_sec+5,"%d",fd[2]%10);
    FILE *file_delay = fopen(file_name_delay,"wb");
    FILE *file_bitrate = fopen(file_name_bitrate,"wb");
    FILE *file_sec = fopen(file_name_sec,"wb");
    long delays[AVG_SIZE];
    long latencys[AVG_SIZE];
    //long bytesReads[AVG_SIZE];
    long bitrates[AVG_SIZE];
    //int bytesReads_total=0;
    //int sum[5];
    long avg_delay;
    long avg_latency;
    int avg_bytesRead;
    long avg_bitrate;
    struct pipedata received_data;
    do{
      read(fd[0], &received_data, sizeof(received_data));
      //if(received_data.delay>=0)
      //  bytesReads_total+=received_data.bytesRead;
      //if(received_data.delay>0&&received_data.bytesRead>0){
      avg_delay=avgNpass(&delays,received_data.delay);
      //avg_latency=avgNpass(&latencys,received_data.latency);
      //avg_bytesRead=avgNpass(&bytesReads,received_data.bytesRead);
      //avg_bitrate=avgNpass(&bitrates,(long)(received_data.bytesRead*1e3/received_data.delay));
      //printf("%d\n",avg_bitrate);
      //printf("%d\n",avg_delay);
      //printf("%d\n",avg_latency);
      avg_bitrate=(received_data.bytesRead*1e3/avg_delay);
      //printf("%d %d\n",received_data.bytesRead,avg_bytesRead);
      //printf("%d %d\n",received_data.delay,avg_delay);
      //printf("%.3f %.3f\n",(double)( (received_data.bytesRead)*1e3/received_data.delay ),avg_bitrate);
      if(received_data.delay >0){
        fprintf(file_sec,"%lld\n",received_data.t);
        fprintf(file_bitrate,"%.3f\n",(double)(avg_bitrate));
        //fprintf(file_delay,"%d\n",avg_latency);
        fprintf(file_delay,"%d\n",received_data.latency);
        printf("%d %10lld %10d %10d\n", fd[2]%10,  received_data.t,  received_data.latency,  avg_bitrate);
        //printf("%d %10lld %10d %10d /\n", fd[2]%10,  received_data.t,  avg_latency,  avg_bitrate);
      //}
      }
    }while(received_data.bytesRead>0);
    //for(int a=0;a<AVG_SIZE;a++)
    //  printf("%d ",delays[a]);
    close(fd[1]);
    //printf("%d\t%lld\t%10d\t%.3f\n", fd[2]%10,  received_data.t,  (long)received_data.delay,  (double)(avg_bitrate));
    pthread_exit(NULL); // 離開子執行緒
}
void recv_sock(void* pipefd) {
    int *fd = (int*) pipefd; 

}
void receive(const int port) {
    //PIPE
    int fd[4];  // 管道文件描述符数
    if (pipe(fd) == -1) {
        perror("pipe");
        return;
    }
    fd[2]=port;
    
    //pthread
    pthread_t t; 
    pthread_create(&t, NULL, Data_Proc, fd); // 建立子執行緒
    
    //socket
    //int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("無法創建套接字");
        return;
    }

    struct sockaddr_in serverAddr, clientAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port); // 設定接收端的端口號
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    int opt= 0;
    opt |= SOF_TIMESTAMPING_RX_HARDWARE;
    opt |= SOF_TIMESTAMPING_RAW_HARDWARE;
    if (setsockopt(sockfd, SOL_SOCKET, SO_TIMESTAMPNS, (char *)&opt, sizeof(opt))) {
        perror("setsockopt SO_TIMESTAMPNS");
        return;
    }
    //int nRecvBuf=512*1024; // 设置为32K
    //if (setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int))){
    //    perror("setsockopt SO_RCVBUF");
    //    return;
    //}
    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("綁定端口失敗");
        return;
    }

    //listen(sockfd, 1);

    socklen_t clientLen = sizeof(clientAddr);
    
    struct pipedata pdata;
    char buffer[MAX_BUFFER_SIZE];
    int bytesRead;
    long long bytesRead_total=0;
    long delay_time=0;
    long long send_time=0;
    pid_t tid=syscall(SYS_gettid);
    struct msghdr msg;
    struct iovec iov;
    struct cmsghdr *cmsg=(struct cmsghdr *)malloc(sizeof(struct cmsghdr));;
    memset(&msg, 0, sizeof(msg));
    struct timespec *timestamp=(struct timespec *)malloc(sizeof(struct timespec));
    char control_buffer[CMSG_SPACE(sizeof(struct timespec))];
    

    iov.iov_base = buffer;
    iov.iov_len = sizeof(buffer);

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control_buffer;
    msg.msg_controllen = sizeof(control_buffer);
    struct timespec start, end, now, tmp;
    struct timespec recvd;
    //clock_gettime(CLOCK_REALTIME, &start);
    //tmp=start;
    //while ((bytesRead = recvmsg(clientfd, &msg, 0)) > 1) {
    start.tv_sec=0;
    while ((bytesRead = recvmsg(sockfd, &msg, 0)) > 1) {
        for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMPNS) {
                timestamp = (struct timespec *)CMSG_DATA(cmsg);
                break;
            }
        }
        if(start.tv_sec==0){
          memcpy(&start,timestamp,sizeof(struct timespec));
          continue;
        }
        pdata.bytesRead+=bytesRead;
        while(pdata.bytesRead<MAX_BUFFER_SIZE){
          bytesRead=recv(sockfd,buffer+pdata.bytesRead,MAX_BUFFER_SIZE-pdata.bytesRead,0);
          if(bytesRead<0)
            continue;
          pdata.bytesRead+=bytesRead;
        }
        if(pdata.bytesRead<MAX_BUFFER_SIZE)
            continue;
        bytesRead_total+=bytesRead;
        memcpy(&send_time,&buffer,sizeof(long long));
        pdata.delay=tstoll(*timestamp)-tstoll(tmp);

        pdata.latency=tstoll(*timestamp)-send_time;
        pdata.t=tstoll(*timestamp)-tstoll(start);
        write(fd[1], &pdata, sizeof(pdata));
        pdata.bytesRead=0;
        memset(buffer,0,sizeof(buffer));
        memcpy(&tmp,timestamp,sizeof(struct timespec));
        pdata.delay=0;
    }
    clock_gettime(CLOCK_REALTIME, &end);//gettimeofday(&end,NULL);
    pdata.bytesRead=-1;
    write(fd[1], &pdata, sizeof(pdata));
    sleep(2);
    double elapsed = (timestamp->tv_sec - start.tv_sec)+1e-9 * (timestamp->tv_nsec - start.tv_nsec);
    printf("%d %.3f %lld %.3f\n",fd[2],elapsed, bytesRead_total, (bytesRead_total/1000000)/(elapsed));
    //printf("%d %.3f %lld %.3f\n",fd[2],elapsed, bytesRead_total, (bytesRead_total/1000000)/(elapsed));
    //close(clientfd);
    close(fd[1]);
    close(sockfd);
    pthread_join(t, NULL); // 等待子執行緒執行完成
}

int main(int argc, char *argv[]) {
    int port = atoi(argv[1]);
    receive(port);
    return 0;
}
