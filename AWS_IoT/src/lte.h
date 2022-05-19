
extern struct k_sem lte_connected;

void lte_handler(const struct lte_lc_evt *const evt);
void modem_configure(void);