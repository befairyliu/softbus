typedef struct CoapRequest {
    const char *remoteUrl;
    char *data;
    size_t dataLength;
    const char *remoteIp;
} CoapRequest;

// 创建线程
int CreateThread(Runnable run, void *argv, const ThreadAttr *attr, unsigned int *threadId)
{
    // 创建线程（封装）
    int errCode = pthread_create((pthread_t *)threadId, &threadAttr, run, argv);
    // ...
}

// 发送coap请求报文
static int CoapSendRequest(const CoapRequest *coapRequest)
{
    // ...
    // 组织socket地址信息(IP、端口、IPV4类型）
    sockAddr.sin_addr.s_addr = inet_addr(coapRequest->remoteIp);
    sockAddr.sin_port = htons(COAP_DEFAULT_PORT);
    sockAddr.sin_family = AF_INET;
    // 创建一个客户端socket并连接对应的地址：端口
    int ret = CoapCreateUdpClient(&sockAddr);

    // 获取创建好的scoket套接字
    SocketInfo socket = {0};
    socket.cliendFd = GetCoapClientSocket();
    socket.dstAddr = sockAddr;
    // 发送数据
    CoapSocketSend(&socket, (uint8_t *)coapRequest->data, coapRequest->dataLength);
    // ...
}

// 发送响应报文，整合CoapSendRequest
static int CoapResponseService(const COAP_Packet *pkt, const char* remoteUrl, const char* remoteIp)
{
    // 声明一个request及初始化
    CoapRequest coapRequest;
    (void)memset_s(&coapRequest, sizeof(coapRequest), 0, sizeof(coapRequest));
    coapRequest.remoteUrl = remoteUrl;
    coapRequest.remoteIp = remoteIp;

    // 声明一个发送报文的缓冲区
    COAP_ReadWriteBuffer sndPktBuff = {0};
    sndPktBuff.readWriteBuf = calloc(1, COAP_MAX_PDU_SIZE);
    sndPktBuff.size = COAP_MAX_PDU_SIZE;
    sndPktBuff.len = 0;

    // 构建发送报文，并把报文包放入缓冲区
    ret = BuildSendPkt(pkt, remoteIp, payload, &sndPktBuff);

    // 将缓冲区数据发送出去
    coapRequest.data = sndPktBuff.readWriteBuf;
    coapRequest.dataLength = sndPktBuff.len;
    ret = CoapSendRequest(&coapRequest);
    // ...
}

// 获取服务发现信息
int GetServiceDiscoverInfo(const uint8_t *buf, size_t size, DeviceInfo *deviceInfo, char **remoteUrlPtr)
{
    // 解析出设备发现信息
    ParseServiceDiscover(buf, deviceInfo, remoteUrlPtr);
}

// 对接收到的消息进行服务发现响应
void PostServiceDiscover(const COAP_Packet *pkt)
{
    char *remoteUrl = NULL; // 保存对端设备url
    DeviceInfo deviceInfo; // 保存对端设备信息
    // 为deiveInfo分配空间
    (void)memset_s(&deviceInfo, sizeof(deviceInfo), 0, sizeof(deviceInfo));
    // 解析出对端设备信息
    GetServiceDiscoverInfo(pkt->payload.buffer, pkt->payload.len, &deviceInfo, &remoteUrl);
    // 用字符数组保存对端IP和网络信息
    char wifiIpAddr[NSTACKX_MAX_IP_STRING_LEN];
    (void)memset_s(wifiIpAddr, sizeof(wifiIpAddr), 0, sizeof(wifiIpAddr));
    (void)inet_ntop(AF_INET, &deviceInfo.netChannelInfo.wifiApInfo.ip, wifiIpAddr, sizeof(wifiIpAddr));

    // 发送响应报文
    CoapResponseService(pkt, remoteUrl, wifiIpAddr);
    // ...
}

// 处理读事件
static void HandleReadEvent(int fd)
{
    int socketFd = fd; // 可读就绪的套接字
    // 分配一个接收缓存区
    unsigned char *recvBuffer = calloc(1, COAP_MAX_PDU_SIZE + 1);

    // 从缓冲区接收数据
    nRead = CoapSocketRecv(socketFd, recvBuffer, COAP_MAX_PDU_SIZE);

    // 解析收到的报文，构建COAP_Packet用于保存从接收缓冲区解析出来的coap数据包
    COAP_Packet decodePacket;
    (void)memset_s(&decodePacket, sizeof(COAP_Packet), 0, sizeof(COAP_Packet));
    decodePacket.protocol = COAP_UDP;
    COAP_SoftBusDecode(&decodePacket, recvBuffer, nRead);
    PostServiceDiscover(&decodePacket);
    // ...
}

// 接收并处理收到的数据
static void CoapReadHandle(unsigned int uwParam1, unsigned int uwParam2, unsigned int uwParam3, unsigned int uwParam4)
{
    // ...
    //获取服务端socket
    int serverFd = GetCoapServerSocket();
    HandleReadEvent(serverFd);
    // ...
}

// 注册wifi回调函数
void RegisterWifiCallback(WIFI_PROC_FUNC callback)
{
    g_wifiCallback = callback;
}

// 处理wifi事件
void CoapHandleWifiEvent(unsigned int para)
{
    g_wifiCallback(para);
}

// 将coap wifi事件加入到消息队列中
void CoapWriteMsgQueue(int state)
{
    AddressEventHandler handler; // 声明一个地址事件处理结构
    // // 注册wifi回调函数和状态
    handler.handler = CoapHandleWifiEvent; 
    handler.state = state; 
    // 发送到对应id的消息队列中
    (void)WriteMsgQue(g_wifiQueueId, &handler, sizeof(AddressEventHandler));
}

// 从消息队列中读取消息并处理
void CoapWifiEventThread(unsigned int uwParam1, unsigned int uwParam2, unsigned int uwParam3, unsigned int uwParam4)
{
    // 声明一个事件处理结构，用来保存从消息队列中读取的消息
    AddressEventHandler handle;
    unsigned int readSize;
    unsigned int ret;
    readSize = sizeof(AddressEventHandler);
    g_wifiTaskStart = 1; // 开始从消息队列中读取消息
    while (g_wifiTaskStart) {
        // 从消息队列中读取消息
        ret = ReadMsgQue(g_wifiQueueId, &handle, &readSize);
        // 处理消息
        handle.handler(handle.state);
    }
}

// 创建消息队列处理线程
int CreateMsgQueThread(void)
{
    // ...
    ThreadAttr attr = {"wifi_event", 0x800, 20, 0, 0};
    ret = CreateThread((Runnable)CoapWifiEventThread, NULL, &attr, (unsigned int*)&g_msgQueTaskId);
}

// 初始化wifi事件
int CoapInitWifiEvent(void)
{
    // 如果wifi队列ID ==-1 ，表示之前没有创建消息队列，就创建消息队列
    if (g_wifiQueueId == -1) {
        ret = CreateMsgQue("/wifiQue",
            WIFI_QUEUE_SIZE, (unsigned int*)&g_wifiQueueId,
            0, sizeof(AddressEventHandler));
    }
}
// 接口状态
typedef enum {
    STATUS_UP,
    STATUS_DOWN
} InterfaceStatus;

// 接口信息
typedef struct {
    char ip[NSTACKX_MAX_IP_STRING_LEN];
    InterfaceStatus status;
    int flag;
} InterfaceInfo;

// 获取网络接口信息 
void GetInterfaceInfo(int fd, const char* interfaceName, int length, InterfaceInfo *info)
{
    // 用来配置ip地址，激活接口，配置MTU等接口信息的结构体
    struct ifreq ifr;
    memset_s(&ifr, sizeof(struct ifreq), 0, sizeof(struct ifreq));
    // 将接口名拷贝到ifreq结构体中
    int ret = strncpy_s(ifr.ifr_name, sizeof(ifr.ifr_name), interfaceName, length);

    // ioctl 获取本地网络接口信息
    ioctl(fd, SIOCGIFFLAGS, &ifr);
    // 接口是否处于活动状态且准备好传输数据，如果是，接口状态就是上传
    if ((unsigned short int)ifr.ifr_flags & IFF_UP) {
        info->status = STATUS_UP;
    } else {
        info->status = STATUS_DOWN;
        info->ip[0] = '\0';
        info->flag = 1;
        return;
    }

    memset_s(&ifr, sizeof(struct ifreq), 0, sizeof(struct ifreq));
    ret = strncpy_s(ifr.ifr_name, sizeof(ifr.ifr_name), interfaceName, length);
    // 获取本地网络接口的IP地址
    ioctl(fd, SIOCGIFADDR, &ifr);
    // 将获取到的IP地址拷贝到info结构体中
    ret = strcpy_s(info->ip, sizeof(info->ip),
        inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr));
    info->flag = 1;
    // ...
}

// 获取wifi IP
void CoapGetWifiIp(char *ip, int length)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    // 调用GetInterfaceInfo获取网络接口信息
    InterfaceInfo info = {0};
    GetInterfaceInfo(fd, ETH, strlen(ETH) + 1, &info);
    // 获取以太网网络接口信息失败，就获取wifi网络接口信息
    if (!info.flag) {
        memset_s(&info, sizeof(info), 0, sizeof(info));
        GetInterfaceInfo(fd, WLAN, strlen(WLAN) + 1, &info);
    }

    // 将获取到的IP地址拷贝到ip参数中
    strcpy_s(ip, length, info.ip);
    close(fd);
}