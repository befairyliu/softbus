
int32_t NSTACKX_Init(const NSTACKX_Parameter *parameter)
{
    return NstackxInitEx(parameter, false);
}

static int32_t NstackxInitEx(const NSTACKX_Parameter *parameter, bool isNotifyPerDevice)
{
    Coverity_Tainted_Set((void *)parameter);

    int32_t ret;

    if (g_nstackInitState != NSTACKX_INIT_STATE_START) {
        return NSTACKX_EOK;
    }

    g_nstackInitState = NSTACKX_INIT_STATE_ONGOING;
    cJSON_InitHooks(NULL);

    InitLogLevel();

#ifdef NSTACKX_WITH_LITEOS
    EpollEventPtrInit(); /* init g_epollEventPtrMutex g_epollEventPtrArray */
#endif

    g_epollfd = CreateEpollDesc();
    if (!IsEpollDescValid(g_epollfd)) {
        DFINDER_LOGE(TAG, "epoll creat fail! errno: %d", errno);
        g_nstackInitState = NSTACKX_INIT_STATE_START;
        return NSTACKX_EFAILED;
    }

    DFINDER_LOGD(TAG, "nstack ctrl creat epollfd %d", REPRESENT_EPOLL_DESC(g_epollfd));
#ifdef DFINDER_SAVE_DEVICE_LIST
    ret = InternalInit(g_epollfd, parameter != NULL ? parameter->maxDeviceNum : NSTACKX_DEFAULT_DEVICE_NUM);
#else
    ret = InternalInit(g_epollfd, parameter != NULL ? parameter->maxDeviceNum : NSTACKX_MAX_DEVICE_NUM);
#endif
    if (ret != NSTACKX_EOK) {
        DFINDER_LOGE(TAG, "internal init failed, ret=%d", ret);
        goto L_ERR_INIT;
    }
    (void)memset_s(&g_parameter, sizeof(g_parameter), 0, sizeof(g_parameter));
    if (parameter != NULL) {
        (void)memcpy_s(&g_parameter, sizeof(g_parameter), parameter, sizeof(NSTACKX_Parameter));
    }

#ifndef DFINDER_USE_MINI_NSTACKX
    CoapInitSubscribeModuleInner(); /* initialize subscribe module number */
#endif /* END OF DFINDER_USE_MINI_NSTACKX */
    g_isNotifyPerDevice = isNotifyPerDevice;
    g_nstackInitState = NSTACKX_INIT_STATE_DONE;
    DFINDER_LOGI(TAG, "DFinder init successfully");
    return NSTACKX_EOK;

L_ERR_INIT:
    NSTACKX_Deinit();
    return ret;
}