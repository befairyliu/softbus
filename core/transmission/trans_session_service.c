int32_t TransServerInit(void)
{
    if (atomic_load_explicit(&g_transSessionInitFlag, memory_order_acquire)) {
        return SOFTBUS_OK;
    }
    int32_t ret = TransPermissionInit();
    if (ret != SOFTBUS_OK) {
        TRANS_LOGE(TRANS_INIT, "Init trans permission failed");
        return ret;
    }
    ret = TransSessionMgrInit();
    if (ret != SOFTBUS_OK) {
        TRANS_LOGE(TRANS_INIT, "TransSessionMgrInit failed");
        return ret;
    }
    ret = TransChannelInit();
    if (ret != SOFTBUS_OK) {
        TRANS_LOGE(TRANS_INIT, "TransChannelInit failed");
        return ret;
    }
    ret = InitQos();
    if (ret != SOFTBUS_OK) {
        TRANS_LOGE(TRANS_INIT, "QosInit Failed");
        return ret;
    }
    ret = ScenarioManagerGetInstance();
    if (ret != SOFTBUS_OK) {
        TRANS_LOGE(TRANS_INIT, "ScenarioManager init Failed");
        return ret;
    }
    RegisterPermissionChangeCallback();
    atomic_store_explicit(&g_transSessionInitFlag, true, memory_order_release);
    TRANS_LOGI(TRANS_INIT, "trans session server list init succ");
    return SOFTBUS_OK;
}