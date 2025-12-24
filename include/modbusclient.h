#ifndef MODBUSCLIENT_H
#define MODBUSCLIENT_H

#include <iostream>
#include <stdint.h>
#include <stdbool.h>
#include <cerrno>
#include <cstring>
#include <modbus/modbus.h>
#include <thread>
#include <chrono>
#include <list>

struct ModbusConfig
{
    /* data */
    virtual ~ModbusConfig() = default;
};


class ModbusClient
{
private:
    /* data */
public:
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool reconnect() = 0;
    virtual int readHoldingRegisters(uint8_t slave_id,uint16_t start_address,uint16_t num_of_reg,uint16_t *values) = 0;
    virtual int writeHoldingRegisters(uint8_t slave_id,uint16_t start_address,uint16_t num_of_reg,uint16_t *values) = 0;
    virtual int readInputRegisters(uint8_t slave_id,uint16_t start_address,uint16_t num_of_reg,uint16_t *values) = 0;
    virtual ~ModbusClient() = default;
};

#endif