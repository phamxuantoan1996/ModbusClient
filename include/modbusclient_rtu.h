#ifndef MODBUSCLIENT_H
#include <modbusclient.h>

#define MODBUS_RTU_DATABIT_8 8
#define MODBUS_RTU_DATABIT_7 7

#define MODBUS_RTU_PARITY_NONE 'N'
#define MODBUS_RTU_PARITY_EVEN 'E'
#define MODBUS_RTU_PARITY_ODD 'O'

#define MODBUS_RTU_BAUDRATE_9600 9600
#define MODBUS_RTU_BAUDRATE_38400 38400
#define MODBUS_RTU_BAUDRATE_115200 115200

#define MODBUS_RTU_STOPBIT_1 1
#define MODBUS_RTU_STOPBIT_1 2

struct ModbusRTUConfig : public ModbusConfig
{
    /* data */
    std::string serial_port;
    uint32_t baud_rare;
    uint8_t data_bit;
    uint8_t parity;
    uint8_t stop_bit;
    uint32_t timeout;
    uint8_t num_try;
};

class ModbusClientRTU : public ModbusClient
{
    private:
        modbus_t *ctx;
        uint32_t timeout;
        uint8_t num_try;
    public:
    ModbusClientRTU(ModbusConfig modbus_config);
    bool connect() override;
    void disconnect() override;
    bool reconnect() override;
    std::list<uint16_t> readHoldingRegisters(uint8_t slave_id,uint16_t start_address,uint16_t num_of_reg) override;
};
#endif
