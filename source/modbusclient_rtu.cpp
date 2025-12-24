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
    uint16_t count = num_try;
    while (count--)
    {
        /* code */
        if(modbus_connect(ctx) == 0)
        {
            std::cerr << "[BUS] reconnect success\n";
            return true;
        }
        std::cerr << "[BUS] reconnect failed: " << modbus_strerror(errno) << " → retry\n";
        std::this_thread::sleep_for(std::chrono::seconds(timeout));
    }
    return false;
}

int ModbusClientRTU::readHoldingRegisters(uint8_t slave_id,uint16_t start_address,uint16_t num_of_reg,uint16_t *values)
{
    if(modbus_set_slave(ctx,slave_id) < 0)
    {
        std::cerr << "Invalid slave id\n";
        return errno;
    }

    int rc = modbus_read_registers(ctx,start_address,num_of_reg,values);

    if (rc == num_of_reg)
    {
        /* code */
        return 0;
    }
    
    if (errno == EBADF || errno == EIO || errno == ECONNRESET)
    {
        std::cerr << "[BUS ERROR] " << modbus_strerror(errno) << std::endl;
    }
    else if (errno == ETIMEDOUT) 
    {
            std::cerr << "[SLAVE " << slave_id << "] timeout → skip\n";
    }
    else
    {
        std::cerr << "[OTHER ERROR] " << modbus_strerror(errno) << " → retry\n";
    }

    return errno;
}

int ModbusClientRTU::writeHoldingRegisters(uint8_t slave_id,uint16_t start_address,uint16_t num_of_reg,uint16_t *values)
{

    modbus_set_slave(ctx, slave_id);

    int rc = modbus_write_registers(ctx,start_address,num_of_reg,values);
    if (rc == num_of_reg) 
    {
        return 0;
    }

    // ===== BUS ERROR → BLOCK & RECONNECT =====
    if (errno == EBADF || errno == EIO || errno == ECONNRESET) 
    {
        std::cerr << "[BUS ERROR] " << modbus_strerror(errno) << "\n";
    }
    // ===== SLAVE ERROR → SKIP =====
    else if (errno == ETIMEDOUT) 
    {
        std::cerr << "[SLAVE " << slave_id << "] timeout → skip write\n";
    }
    else
    {
        // ===== OTHER ERROR → RETRY =====
        std::cerr << "[ERROR] " << modbus_strerror(errno) << " → retry\n";
    }
    return errno;
}

int ModbusClientRTU::readInputRegisters(uint8_t slave_id,uint16_t start_address,uint16_t num_of_reg,uint16_t *values)
{
    if(modbus_set_slave(ctx,slave_id) < 0)
    {
        std::cerr << "Invalid slave id\n";
        return errno;
    }

    int rc = modbus_read_input_registers(ctx,start_address,num_of_reg,values);

    if (rc == num_of_reg)
    {
        /* code */
        return 0;
    }
    
    if (errno == EBADF || errno == EIO || errno == ECONNRESET)
    {
        std::cerr << "[BUS ERROR] " << modbus_strerror(errno) << std::endl;
    }
    else if (errno == ETIMEDOUT) 
    {
            std::cerr << "[SLAVE " << slave_id << "] timeout → skip\n";
    }
    else
    {
        std::cerr << "[OTHER ERROR] " << modbus_strerror(errno) << " → retry\n";
    }
    return errno;
}