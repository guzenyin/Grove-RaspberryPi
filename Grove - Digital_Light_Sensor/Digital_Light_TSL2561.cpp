/*
   TSL2561 library V1.0
   2010 Copyright (c) Seeed Technology Inc.  All right reserved.

   Original Author: zhangkun

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
   */
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>

#include "Digital_Light_TSL2561.h"

Digital_Light_TSL2561::Digital_Light_TSL2561(uint16_t deviceAddress)
{
    device_address = deviceAddress;
    init();
}

Digital_Light_TSL2561::~Digital_Light_TSL2561()
{

}

uint16_t Digital_Light_TSL2561::readRegister(uint16_t deviceAddress, uint16_t address)
{
    uint16_t value;
    int file;
    uint16_t buf[1] = {address};
    uint16_t reBuf[1] = {0};

    if ( (file = open("/dev/i2c-1", O_RDWR)) < 0 ) {
        printf("Failed to open the bus.");
    }

    if(ioctl(file,I2C_SLAVE,deviceAddress) < 0) {
        printf("Failed to acquire bus access and/or talk to slave.\n");
    }

    /* send the register address which want read */
    write(file, buf, 1);
    read(file, reBuf, 1);

    close(file);
    return reBuf[0];
}

void Digital_Light_TSL2561::writeRegister(uint16_t deviceAddress, uint16_t address, uint16_t val) 
{
    int file; // fd
    char buf[2] = {0}; // buffer for write.

    /* open the i2c dev */
    if((file = open("/dev/i2c-1",O_RDWR)) < 0) {
        printf("Failed to open the bus.");
    }

    if(ioctl(file,I2C_SLAVE,deviceAddress) < 0) {
        printf("Failed to acquire bus access and/or talk to slave.\n");
    }

    buf[0] = (char)address;
    buf[1] = (char)val;

    //    printf("Write Register\n");
    //    printf("address %d, val %d \n", &address, &val);

    write(file,buf,2);
    close(file);
}
void Digital_Light_TSL2561::getLux(void)
{
    CH0_LOW=readRegister(device_address,TSL2561_CHANNAL0L);
    CH0_HIGH=readRegister(device_address,TSL2561_CHANNAL0H);
    //read two bytes from registers 0x0E and 0x0F
    CH1_LOW=readRegister(device_address,TSL2561_CHANNAL1L);
    CH1_HIGH=readRegister(device_address,TSL2561_CHANNAL1H);

    channel0=CH0_HIGH*256+CH0_LOW;
    channel1=CH1_HIGH*256+CH1_LOW;
}
void Digital_Light_TSL2561::init()
{
    writeRegister(device_address,TSL2561_CONTROL,0x03);  // POWER UP
    writeRegister(device_address,TSL2561_TIMING,0x11);  //High Gain (16x), integration time of 101ms
    writeRegister(device_address,TSL2561_INTERRUPT,0x00);
}

uint32_t Digital_Light_TSL2561::calculateLux(uint16_t iGain, uint16_t tInt, uint16_t iType)
{
    switch (tInt)
    {
        case 0:  // 13.7 msec
            chScale = CHSCALE_TINT0;
            break;
        case 1: // 101 msec
            chScale = CHSCALE_TINT1;
            break;
        default: // assume no scaling
            chScale = (1 << CH_SCALE);
            break;
    }
    if (!iGain)  chScale = chScale << 4; // scale 1X to 16X
    // scale the channel values
    channel0 = (channel0 * chScale) >> CH_SCALE;
    channel1 = (channel1 * chScale) >> CH_SCALE; 

    ratio1 = 0;
    if (channel0!= 0) ratio1 = (channel1 << (RATIO_SCALE+1))/channel0;
    // round the ratio value
    uint16_t ratio = (ratio1 + 1) >> 1;

    switch (iType)
    {
        case 0: // T package
            if ((ratio >= 0) && (ratio <= K1T))
            {b=B1T; m=M1T;}
            else if (ratio <= K2T)
            {b=B2T; m=M2T;}
            else if (ratio <= K3T)
            {b=B3T; m=M3T;}
            else if (ratio <= K4T)
            {b=B4T; m=M4T;}
            else if (ratio <= K5T)
            {b=B5T; m=M5T;}
            else if (ratio <= K6T)
            {b=B6T; m=M6T;}
            else if (ratio <= K7T)
            {b=B7T; m=M7T;}
            else if (ratio > K8T)
            {b=B8T; m=M8T;}
            break;
        case 1:// CS package
            if ((ratio >= 0) && (ratio <= K1C))
            {b=B1C; m=M1C;}
            else if (ratio <= K2C)
            {b=B2C; m=M2C;}
            else if (ratio <= K3C)
            {b=B3C; m=M3C;}
            else if (ratio <= K4C)
            {b=B4C; m=M4C;}
            else if (ratio <= K5C)
            {b=B5C; m=M5C;}
            else if (ratio <= K6C)
            {b=B6C; m=M6C;}
            else if (ratio <= K7C)
            {b=B7C; m=M7C;}
    } 
    temp=((channel0*b)-(channel1*m));
    if(temp<0) temp=0;
    temp+=(1<<LUX_SCALE-1);
    // strip off fractional portion
    lux=temp>>LUX_SCALE;
    return (lux);
}
