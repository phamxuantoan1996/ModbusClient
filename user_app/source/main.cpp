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

struct Reg_Info_Structure
{
    /* data */
    uint8_t slave_id;
    uint16_t start_addr;
    uint16_t num_of_reg;
    uint16_t *values;
};


std::atomic<bool> running{true};
httplib::Server http_server;
std::vector<Reg_Info_Structure> list_input_regs;
std::vector<Reg_Info_Structure> list_hold_regs;
std::mutex lock_list_input_regs;
std::mutex lock_list_holding_regs;


void terminal_sig_callback(int) 
{ 
    running = false; 
}

void http_server_init()
{
    http_server.Get("/input_register", [](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("slave_id") ||
            !req.has_param("start_address") ||
            !req.has_param("num_of_reg")) {
            res.status = 400;
            res.set_content("Missing parameters", "text/plain");
            return;
        }

        Json::Value root_response;
        Json::Value array_values(Json::arrayValue);


        try 
        {
            int slave_id;
            int start_address;
            int num_of_reg;
            uint16_t *values = nullptr;
            slave_id = std::stoi(req.get_param_value("slave_id"));
            start_address  = std::stoi(req.get_param_value("start_address"));
            num_of_reg = std::stoi(req.get_param_value("num_of_reg"));
            {
                std::lock_guard<std::mutex> lock(lock_list_input_regs);
                for (Reg_Info_Structure item : list_input_regs)
                {
                    /* code */
                    if(item.slave_id == slave_id && item.start_addr == start_address && item.num_of_reg == num_of_reg)
                    {
                        values = item.values;
                    }
                }
            }

            if(values != nullptr)
            {
                for(int16_t i = 0; i < num_of_reg; i++)
                {
                    array_values.append(values[i]);
                }
            }
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "Invalid number: " << e.what() << std::endl;
        }

        root_response["response"] = array_values;

        Json::StreamWriterBuilder writer;
        res.set_content(Json::writeString(writer, root_response), "application/json");
    });
    http_server.Post("/holding_register", [](const httplib::Request& req, httplib::Response& res) {
        std::istringstream ss(req.body);
        Json::CharReaderBuilder rbuilder;
        Json::Value root_recv;
        std::string errs;

        bool check = Json::parseFromStream(rbuilder, ss, &root_recv, &errs);

        if (!check || !root_recv.isObject()) 
        {
            res.status = 400;
            res.set_content("{\"error\":\"Invalid JSON\"}", "application/json");
            return;
        }

        check = false;

        if(root_recv.isMember("slave_id") && root_recv.isMember("start_address") && root_recv.isMember("num_of_reg") && root_recv.isMember("values"))
        {
            if(root_recv["slave_id"].isInt() && root_recv["start_address"].isInt() && root_recv["num_of_reg"].isInt() && root_recv["values"].isArray())
            {
                if(root_recv["values"].size() == root_recv["num_of_reg"].asInt())
                {
                    Reg_Info_Structure holding_req;
                    holding_req.slave_id = root_recv["slave_id"].asInt();
                    holding_req.start_addr = root_recv["start_id"].asInt();
                    holding_req.num_of_reg = root_recv["num_of_reg"].asInt();
                    holding_req.values = new uint16_t[holding_req.num_of_reg]; //free this memory when sending request is completed;
                    uint16_t count = 0;
                    for (const auto& value : root_recv["values"])
                    {
                        holding_req.values[count] = value.asInt();
                        count++;
                    }
                    {
                        std::lock_guard<std::mutex> lock(lock_list_holding_regs);
                        list_hold_regs.push_back(holding_req);
                    }
                    check = true;
                }
            }
        }
        if(check)
        {
            Json::Value root_response;
            root_response["response"] = 0;
            Json::StreamWriterBuilder writer;
            res.set_content(Json::writeString(writer, root_response), "application/json");
        }
        else
        {
            Json::Value root_response;
            root_response["response"] = 1;
            root_response["message"] = "key of json is invalid";
            Json::StreamWriterBuilder writer;
            res.set_content(Json::writeString(writer, root_response), "application/json");
        }
    });
}

void addInputRegister(uint8_t slave_id,uint16_t start_addr,uint16_t num_of_reg)
{
    Reg_Info_Structure input_reg;
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

    uint16_t count_input_reg = 0;
    uint16_t size_list_input_regs = list_input_regs.size();

    while (running)
    {
        /* code */
        {
            // std::lock_guard<std::mutex> lock(lock_list_holding_regs);
            // if(!list_hold_regs.empty())
            // {
            //     auto item = list_hold_regs.begin();
            //     int ret = mb_client.writeHoldingRegisters(item->slave_id,item->start_addr,item->num_of_reg,item->values);
            //     delete[] item->values;
            //     list_hold_regs.erase(item);
            //     if(ret == EBADF || ret == EIO || ret == ECONNRESET)
            //     {
            //         while(!mb_client.reconnect());
            //     }
            //     std::this_thread::sleep_for(std::chrono::milliseconds(25));
            //     continue;
            // }
            Reg_Info_Structure item = {0,0,0,nullptr};
            {
                std::lock_guard<std::mutex> lock(lock_list_holding_regs);
                if(!list_hold_regs.empty())
                {
                    item = list_hold_regs.back();
                    list_hold_regs.pop_back();
                }
            }
            if(item.values != nullptr)
            {
                int r = 0;
                int ret = mb_client.writeHoldingRegisters(item.slave_id,item.start_addr,item.num_of_reg,item.values);
                delete[] item.values;
                if(ret == EBADF || ret == EIO || ret == ECONNRESET)
                {
                    while(!mb_client.reconnect());
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }
        }    

        {
            Reg_Info_Structure item;
            {
                std::lock_guard<std::mutex> lock(lock_list_input_regs);
                item = list_input_regs.at(count_input_reg);
            }

            uint16_t *buffer = new uint16_t[item.num_of_reg];
            int ret = 0;
            ret = mb_client.readInputRegisters(item.slave_id,item.start_addr,item.num_of_reg,buffer);

            {
                std::lock_guard<std::mutex> lock(lock_list_input_regs);
                memcpy(item.values,buffer,sizeof(uint16_t)*item.num_of_reg);
            }
            delete[] buffer;
            count_input_reg++;
            if(count_input_reg == size_list_input_regs)
            {
                count_input_reg = 0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
    }
    http_server.stop(); 
    http_server_thread.join(); 
    return 0;
}


