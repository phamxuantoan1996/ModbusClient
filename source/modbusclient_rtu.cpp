#include <modbusclient_rtu.h>

ModbusClientRTU::ModbusClientRTU(ModbusConfig modbus_config)
{

}

bool ModbusClientRTU::connect()
{
    bool ret = false;
    
    return ret;
}

void ModbusClientRTU::disconnect()
{

}

bool ModbusClientRTU::reconnect()
{
    bool ret = false;

    return ret;
}

std::list<uint16_t> ModbusClientRTU::readHoldingRegisters(uint8_t slave_id,uint16_t start_address,uint16_t num_of_reg)
{
    std::list<uint16_t> values;

    return values;
}