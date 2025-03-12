typedef struct TagConnListenerNode {
    ListNode node;
    ConnModule moduleId;
    ConnectCallback callback;
} ConnListenerNode;