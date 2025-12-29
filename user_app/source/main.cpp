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
#include <fstream>

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
    std::ifstream file("../modbusconfig.json");
    if (!file.is_open())
    {
        std::cerr << "Cannot open file to config modbus!" << std::endl;
        return -1;
    }
    Json::Value root;
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    JSONCPP_STRING errs;
    bool ok = Json::parseFromStream(builder, file, &root, &errs);
    if(!ok || !root.isObject())
    {
        std::cerr << "An error occurred in while reading the json file!" << std::endl;
        return -1;
    }
    if(!root["port"].isString() || !root["baudrate"].isInt() 
    || !root["databit"].isInt() || !root["parity"].isInt() 
    || !root["stopbit"].isInt() || !root["timeout"].isInt()
    || !root["num_of_try"].isInt() || !root["input_regs"].isArray()
    || !root["period_inquiry"].isInt())
    {
        std::cerr << "Key of json is invalid!" << std::endl;
        return -1;
    }
    uint16_t period_inquiry = root["period_inquiry"].asInt(); //ms
    
    for (const Json::Value& item : root["input_regs"])
    {
        /* code */
        if(item["slave_id"].isInt() && item["start_address"].isInt() && item["num_of_reg"].isInt())
        {
            if(item["slave_id"].asInt() > 0 && item["slave_id"].asInt() < 128)
            {
                addInputRegister(item["slave_id"].asInt(),item["start_address"].asInt(),item["num_of_reg"].asInt());
            }
        }
    }
    

    ModbusRTUConfig mb_cgf;
    mb_cgf.serial_port = root["port"].asString();
    mb_cgf.baud_rare = root["baudrate"].asInt();
    mb_cgf.data_bit = root["databit"].asInt();
    mb_cgf.parity = root["parity"].asInt();
    mb_cgf.stop_bit = root["stopbit"].asInt();
    mb_cgf.num_try = root["num_of_try"].asInt();
    mb_cgf.timeout = root["timeout"].asInt();

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

    signal(SIGINT, terminal_sig_callback);
    signal(SIGTERM, terminal_sig_callback);

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
            std::this_thread::sleep_for(std::chrono::milliseconds(period_inquiry));
        }
    }
    http_server.stop(); 
    http_server_thread.join(); 
    return 0;
}


