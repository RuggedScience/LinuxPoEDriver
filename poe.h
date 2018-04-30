#include <linux/types.h>

#define POE_AUTO    0b11
#define POE_MANUAL  0b01

#define POE_DISABLE 0
#define POE_ENABLE  1

int getDeviceId(void);

int getPortState(uint8_t port);
int setPortState(uint8_t port, int state);

int getPortMode(uint8_t port);
int setPortMode(uint8_t port, int mode);