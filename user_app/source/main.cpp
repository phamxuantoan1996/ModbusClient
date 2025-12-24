#include <iostream>
#include <modbusclient_rtu.h>
#include <thread>
#include <chrono>

int main(int argc,char *argv[])
{
    ModbusRTUConfig mb_cgf;
    mb_cgf.serial_port = "/dev/CH340";
    mb_cgf.baud_rare = MODBUS_RTU_BAUDRATE_115200;
    mb_cgf.data_bit = MODBUS_RTU_DATABIT_8;
    mb_cgf.parity = MODBUS_RTU_PARITY_NONE;
    mb_cgf.stop_bit = MODBUS_RTU_STOPBIT_1;
    mb_cgf.num_try = 5;
    mb_cgf.timeout = 5;

    ModbusClientRTU mb_client(mb_cgf);
    if(mb_client.connect())
    {
        std::cout << "connection success\n";
    }
    else
    {
        std::cout << "connection failure\n";
        return -1;
    }

    while (1)
    {
        /* code */
        uint16_t input[10];
        mb_client.readHoldingRegisters(1,0,10,input);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}