#ifndef __SERVER_H
#define __SERVER_H

#include "esp.h"

void server_thread(void const* arg);

espr_t      server_serve(esp_netconn_p client);

#endif /* __SERVER_H */
