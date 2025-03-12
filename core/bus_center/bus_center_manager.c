typedef enum {
    INIT_LOCAL_LEDGER_DELAY_TYPE = 0,
    INIT_EVENT_MONITER_DELAY_TYPE,
    INIT_NETWORK_MANAGER_DELAY_TYPE,
    INIT_NETBUILDER_DELAY_TYPE,
    INIT_LANEHUB_DELAY_TYPE,
    INIT_DELAY_MAX_TYPE,
} InitDelayType;

typedef struct {
    LnnInitDelayImpl implInit;
    bool isInit;
} InitDelayImpl;

typedef struct {
    InitDelayImpl initDelayImpl[INIT_DELAY_MAX_TYPE];
    int32_t delayLen;
} LnnLocalConfigInit;
