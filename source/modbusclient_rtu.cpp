#include <modbusclient_rtu.h>

ModbusClientRTU::ModbusClientRTU(ModbusRTUConfig config)
{
    num_try = config.num_try;
    timeout = config.timeout;
    ctx = modbus_new_rtu(config.serial_port.c_str(),config.baud_rare,config.parity,config.data_bit,config.stop_bit);
    if (!ctx) {
        std::cerr << "Unable to create modbus context\n";
    }
    modbus_set_response_timeout(ctx, config.timeout, 0);
}

bool ModbusClientRTU::connect()
{
    bool ret = true;
    if(modbus_connect(ctx) == -1)
    {
        std::cerr << "Connection failed: " << modbus_strerror(errno) << "\n";
        modbus_free(ctx);
        ret = false;
    }
    return ret;
}

void ModbusClientRTU::disconnect()
{
    modbus_close(ctx);
}

bool ModbusClientRTU::reconnect()
{
    bool ret = false;

    return ret;
}

std::list<uint16_t> ModbusClientRTU::readHoldingRegisters(uint8_t slave_id,uint16_t start_address,uint16_t num_of_reg)
{
    std::list<uint16_t> value;

    return value;
}

bool ModbusClientRTU::writeHoldingRegisters(uint8_t slave_id,uint16_t start_address,std::list<uint16_t> values)
{
    bool ret = false;

    return ret;
}

int ModbusClientRTU::readInputRegisters(uint8_t slave_id,uint16_t start_address,uint16_t num_of_reg,uint16_t *values)
{
    
    return 0;
}