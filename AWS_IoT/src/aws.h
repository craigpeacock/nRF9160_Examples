
#define APP_TOPICS_COUNT CONFIG_AWS_IOT_APP_SUBSCRIPTION_LIST_COUNT

int app_topics_subscribe(void);
int shadow_update(bool version_number_include);
void aws_iot_event_handler(const struct aws_iot_evt *const evt);
void print_received_data(const char *buf, const char *topic, size_t topic_len);
