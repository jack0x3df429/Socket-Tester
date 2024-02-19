#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/net_tstamp.h>
#include <time.h>
#include <pthread.h>
#define MAX_BUFFER_SIZE 8192
#define AVG_SIZE 50
struct pipedata {
    int bytesRead;
    int delay;
    long t;
};

int avgNpass(int *arr,int new){
  int sum=new;
  for(int i=AVG_SIZE-1;i>0;i--){
    arr[i]=arr[i-1];
    sum+=arr[i];
  }
  arr[0]=new;
  return sum/AVG_SIZE;
}

void* Data_Proc(void* pipefd) {
    int *fd = (int*) pipefd; // 取得輸入資料
    int delays[AVG_SIZE];
    int bytesReads[AVG_SIZE];
    int sum[5];
    double avg_delay;
    struct pipedata received_data;
    do{
      read(fd[0], &received_data, sizeof(received_data));
      if(received_data.delay>0){
        avg_delay=(double)((avgNpass(&bytesReads,received_data.bytesRead)*1e3)/avgNpass(&delays,received_data.delay));
        sum[((received_data.t)%(5000))/1000]++;
        printf("%d %.2f\n",fd[2],avg_delay);
      }
    }while(received_data.bytesRead>0);
    for(int j=0;j<5;j++)
    {
      printf("%d %d\n",j,sum[j]);
    }
    close(fd[1]);
    pthread_exit(NULL); // 離開子執行緒
}    
void receiveFile(const char* filename,const int port) {
    //file
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        printf("無法創建檔案：%s\n", filename);
        return;
    }
    //PIPE
    int fd[2];  // 管道文件描述符数
    if (pipe(fd) == -1) {
        perror("pipe");
        return;
    }
    struct pipedata pdata;
    //pthread
    pthread_t t; 
    pthread_create(&t, NULL, Data_Proc, fd); // 建立子執行緒
    
    //socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
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
    if (setsockopt(sockfd, SOL_SOCKET, SO_TIMESTAMP,
               (char *)&opt, sizeof(opt))) {
        perror("setsockopt timestamping");
        return;
    }
    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("綁定端口失敗");
        return;
    }

    listen(sockfd, 1);

    socklen_t clientLen = sizeof(clientAddr);
    int clientfd = accept(sockfd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientfd < 0) {
        perror("接受連接失敗");
        return;
    }else{printf("接受連接");}
    
    char buffer[MAX_BUFFER_SIZE];
    int bytesRead;
    int bytesRead_total=0;
    struct msghdr msg;
    struct iovec iov;
    struct cmsghdr *cmsg;
    struct timeval *timestamp;
    char control_buffer[CMSG_SPACE(sizeof(struct timeval))];
    memset(&msg, 0, sizeof(msg));

    iov.iov_base = buffer;
    iov.iov_len = MAX_BUFFER_SIZE;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control_buffer;
    msg.msg_controllen = sizeof(control_buffer);
    
    struct timeval start, end, now, tmp;
    gettimeofday(&start,NULL);
    tmp=start;
    while ((bytesRead = recvmsg(clientfd, &msg, 0)) > 0) {
        for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMP) {
                timestamp = (struct timeval *)CMSG_DATA(cmsg);
                break;
            }
            //printf("\n%d\n",SO_TIMESTAMP);
        }
        pdata.bytesRead=bytesRead;
        pdata.delay=timestamp->tv_usec - tmp.tv_usec;
        pdata.t=timestamp->tv_usec;
        bytesRead_total+=bytesRead;
        write(fd[1], &pdata, sizeof(pdata));
        fwrite(buffer, 1, bytesRead, file);
        tmp = now;
    }
    gettimeofday(&end,NULL);
    pdata.bytesRead=-1;
    write(fd[1], &pdata, sizeof(pdata));
    double elapsed = (end.tv_sec - start.tv_sec)+1e-6 * (end.tv_usec - start.tv_usec);
    printf("%d -1\n%d %.3f %.3f\n",port,port,elapsed, (bytesRead_total/1000000)/(elapsed));
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    printf("在%.3f秒內完成傳輸 %.3f bytes/sec\n",elapsed, file_size/elapsed);
    
    fclose(file);
    close(clientfd);
    close(sockfd);
    close(fd[1]);
    pthread_join(t, NULL); // 等待子執行緒執行完成
}

int main(int argc, char *argv[]) {
    int port = atoi(argv[1]);
    const char* filename = argv[2];
    
    receiveFile(filename,port);

    return 0;
}
