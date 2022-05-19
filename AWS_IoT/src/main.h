
extern struct k_work_delayable shadow_update_work;
extern struct k_work_delayable connect_work;
extern struct k_work shadow_update_version_work;

extern bool cloud_connected;

void work_init(void);
void connect_work_fn(struct k_work *work);
void shadow_update_work_fn(struct k_work *work);
void shadow_update_version_work_fn(struct k_work *work);

