#include "poe.h"
#include <linux/io.h>
#include <linux/errno.h>

#define BUS_ADDR 	0xF040
#define DEV_ADDR 	0x40

static int smb_write_byte(uint16_t addr, uint8_t i2c_addr, uint8_t cmd, uint8_t data, uint32_t wait)
{
	uint8_t  b;
	uint32_t i;

	if ((b = inb(addr)) & 0x90) outb(inb(addr + 0x02) | (b & 0x80) | 0x02, addr + 0x02);
    outb(0xFF, addr);
	for (i = 0; i < wait * 100; i++){
		if ((b = inb(addr)) & 0x90) outb(inb(addr + 0x02) | (b & 0x80) | 0x02, addr + 0x02);
		if (b & 0x94) outb(b & 0x94, addr);
		if (b & 0x04 || b == 0x40) break;
	}
	if (b & 0x04 || i >= wait * 100) 
        return -EBUSY;
	outb(i2c_addr, addr + 0x04);
	outb(cmd, addr + 0x03);
	outb(data, addr + 0x05);
	outb(0x48, addr + 0x02);
	for (i = 0; i < wait * 100; i++){
		if ((b = inb(addr)) & 0x04) outb(0x04, addr);
		if (b & 0x04 || b == 0x42) break;
	}

	if (b & 0x04 || i >= wait * 100) return -EBUSY;
	return 0;
}

static int smb_read_byte(uint16_t addr, uint8_t i2c_addr, uint8_t cmd, uint8_t *data, uint32_t wait)
{
	uint8_t  b;
	uint32_t i;

	if (!data) return -EINVAL;
	if ((b = inb(addr)) & 0x90) outb(inb(addr + 0x02) | (b & 0x80) | 0x02, addr + 0x02);
	outb(0xFF, addr);
	outb(0x00, addr + 0x05);
	for (i = 0; i < wait * 100; i++){
		if ((b = inb(addr)) & 0x90) outb(inb(addr + 0x02) | (b & 0x80) | 0x02, addr + 0x02);
		if (b & 0x94) outb(b & 0x94, addr);
		if (b & 0x04 || b == 0x40) break;
	}
	if (b & 0x04 || i >= wait * 100) 
        return -EBUSY;
	outb(i2c_addr + 1, addr + 0x04);
	outb(cmd, addr + 0x03);
	outb(0x48, addr + 0x02);
	for (i = 0; i < wait * 100; i++){
		if ((b = inb(addr)) & 0x04) outb(0x04, addr);
		if (b & 0x04 || b == 0x42) break;
	}
	if (b & 0x04 || i >= wait * 100) 
        return -EBUSY;
	*data = inb(addr + 0x05);

	return 0;
}

int setPortSensing(uint8_t port, bool sense)
{
	int error;
	uint8_t sensing;

	error = smb_read_byte(BUS_ADDR, DEV_ADDR, 0x13, &sensing, 30);
	if (error < 0) return error;

	sensing &= 0x0F;
	if (sense) sensing |= 1 << port;
	else sensing &= ~(1 << port);

	return smb_write_byte(BUS_ADDR, DEV_ADDR, 0x13, sensing, 30);
}

int setPortDetection(uint8_t port, bool detect)
{
	int error;
	uint8_t detection;

	error = smb_read_byte(BUS_ADDR, DEV_ADDR, 0x14, &detection, 30);
	if (error < 0) return error;

	if (detect)	detection |= 1 << port;
	else detection &= ~(1 << port);

	return smb_write_byte(BUS_ADDR, DEV_ADDR, 0x14, detection, 30);
}

int setPortClassification(uint8_t port, bool classify)
{
	int error;
	uint8_t classification;

	error = smb_read_byte(BUS_ADDR, DEV_ADDR, 0x14, &classification, 30);
	if (error < 0) return error;

	if (classify) classification |= 1 << (port + 4);
	else classification &= ~(1 << (port + 4));

	return smb_write_byte(BUS_ADDR, DEV_ADDR, 0x14, classification, 30);
}

int getDeviceId(void)
{
	uint8_t devId;
	int error = smb_read_byte(BUS_ADDR, DEV_ADDR, 0x43, &devId, 30);
	if (error != 0) return error;

	return (int)devId;
}

int getPortState(uint8_t port)
{
    uint8_t currentState = 0;
    int error = smb_read_byte(BUS_ADDR, DEV_ADDR, 0x10, &currentState, 30);
    if (error != 0) return error;

    return (BIT(port) & currentState) >> port;
}

int setPortState(uint8_t port, int state)
{
    int error;
    uint8_t data = 0;
    uint8_t currentState = 0;
    
    error = smb_read_byte(BUS_ADDR, DEV_ADDR, 0x10, &currentState, 30);
    if (error != 0) return error;

	data = 1 << port;
	if (!state) data = ~data;
	data |= ~data << 4;

    return smb_write_byte(BUS_ADDR, DEV_ADDR, 0x19, data, 30);
}

int getPortMode(uint8_t port)
{
	int error;
	uint8_t modes, mode;
	error = smb_read_byte(BUS_ADDR, DEV_ADDR, 0x12, &modes, 30);
	if (error < 0) return error;

	mode = modes & (0b11 << (port * 2));
	
	return mode >> (port * 2);
}

int setPortMode(uint8_t port, int mode)
{
	bool state;
	int i, error;
	uint8_t modes = 0;

	if (mode == POE_AUTO) state = true;
	else if (mode == POE_MANUAL) state = false;
	else return -EINVAL;
	
	for (i = 0; i < 4; ++i)
	{
		if (i == port) modes |= mode << (i * 2);
		else
		{
			error = getPortMode(i);
			if (error < 0) return error;
			modes |= (uint8_t)error << (i * 2);
		}
	}

	error = smb_write_byte(BUS_ADDR, DEV_ADDR, 0x12, modes, 30);
	if (error < 0) return error;

	error = setPortDetection(port, state);
	if (error < 0) return error;

	error = setPortClassification(port, state);
	if (error < 0) return error;

	error = setPortSensing(port, state);
	if (error < 0) return error;
	
	return 0;
}

