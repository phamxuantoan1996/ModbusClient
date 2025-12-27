#include <iostream>
#include <modbusclient_rtu.h>
#include <thread>
#include <chrono>
#include <httplib.h>

#include <unistd.h>
#include <signal.h>
#include <cstring>
#include <mutex>
#include <atomic>
#include <cerrno>

#include <jsoncpp/json/json.h>
#include <sstream>

struct Input_Reg_Structure
{
    /* data */
    uint8_t slave_id;
    uint16_t start_addr;
    uint16_t num_of_reg;
    uint16_t *values;
};

std::atomic<bool> running{true};
httplib::Server http_server;
std::list<Input_Reg_Structure> list_input_regs;


void terminal_sig_callback(int) 
{ 
    running = false; 
}

void http_server_init()
{
    http_server.Get("/status", [](const httplib::Request& req, httplib::Response& res) {
        Json::Value root;

        root["commodity"] = 1;
        root["robot_state"] = 1;

        Json::StreamWriterBuilder writer;
        res.set_content(Json::writeString(writer, root), "application/json");
    });
}

void addInputRegister(uint8_t slave_id,uint16_t start_addr,uint16_t num_of_reg)
{
    Input_Reg_Structure input_reg;
    input_reg.slave_id = slave_id;
    input_reg.start_addr = start_addr;
    input_reg.num_of_reg = num_of_reg;
    input_reg.values = new uint16_t[num_of_reg];
    list_input_regs.push_back(input_reg);
}


int main(int argc,char *argv[])
{
    signal(SIGINT, terminal_sig_callback);
    signal(SIGTERM, terminal_sig_callback);

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

    addInputRegister(1,0,10);
    addInputRegister(2,0,10);

    http_server_init();
    http_server.new_task_queue = [] { return new httplib::ThreadPool(2); };
    std::thread http_server_thread([&]() {
        std::cout << "HTTP server starting on port 9000...\n";
        http_server.listen("0.0.0.0", 9000);  // BLOCKING
        std::cout << "HTTP server stopped.\n";
    });

    while (running)
    {
        /* code */
        uint16_t input[10];
        int ret = mb_client.readHoldingRegisters(1,0,10,input);
        if(ret == EBADF || ret == EIO || ret == ECONNRESET)
        {
            while(!mb_client.reconnect());
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    http_server.stop(); 
    http_server_thread.join(); 
    return 0;
}


