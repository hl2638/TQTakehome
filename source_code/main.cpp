#include <iostream>
#include <fstream>
#include <memory>
#include <cassert>
#include "message_types.h"
#include "system_data.h"
#include "utils.h"

int main(int argc, char** argv)
{
    const std::string data_file_path = (argc > 1) ? argv[1] : "./data/01302019.NASDAQ_ITCH50";
    std::ifstream ifs{data_file_path};
    assert(ifs.good() && "Failed to open data file.");

    std::shared_ptr<SystemData> sys_data = std::make_shared<SystemData>();

    int count = 0;
    while (!ifs.eof()) {
        // Read message length
        uint16_t msg_len = read_big_endian<2>(ifs);
        // std::cout << "Message length: " << msg_len << std::endl;

        // Read first byte: message type
        char msg_type;
        ifs.read(&msg_type, 1);

        count++;
        if (count % 10'000'000 == 0) {
            std::cout << "[DEBUG]Read " << count << " messages." << std::endl;
        }
        std::unique_ptr<BaseMessage> new_msg;
        switch(msg_type) {
            case 'S': {
                // System Event
                new_msg = std::make_unique<SystemEventMessage>(sys_data);
                break;
            }
            case 'R': {
                // Stock Directory
                new_msg = std::make_unique<StockDirectoryMessage>(sys_data);
                break;
            }
            case 'A': {
                // Add Order
                new_msg = std::make_unique<AddOrderMessage>(sys_data);
                break;
            }
            case 'F': {
                // Add Order with MPID Attribution
                new_msg = std::make_unique<AddOrderMPIDAttributionMessage>(sys_data);
                break;
            }
            case 'E': {
                // Order Executed
                new_msg = std::make_unique<OrderExecutedMessage>(sys_data);
                break;
            }
            case 'C': {
                // Order Executed With Price
                new_msg = std::make_unique<OrderExecutedWithPriceMessage>(sys_data);
                break;
            }
            case 'U': {
                // Order Replace
                new_msg = std::make_unique<OrderReplaceMessage>(sys_data);
                break;
            }
            case 'P': {
                // Trade Message
                new_msg = std::make_unique<TradeMessage>(sys_data);
                break;
            }
            case 'Q': {
                // Cross Trade Message
                new_msg = std::make_unique<CrossTradeMessage>(sys_data);
                break;
            }
            case 'B': {
                // Broken Trade Message
                new_msg = std::make_unique<BrokenTradeMessage>(sys_data);
                break;
            }
            default: {
                ifs.ignore(msg_len - 1);
                continue; // read and discard, skip message
                break;
            }
            /* 
                For the purpose of VWAP, not interested in order cancel and delete 
                    since they don't modify price
                Also not checking if an order is still valid
                    assuming data is correct 
                    (trades that are erratic will be announced in trade break messages)
            */
        }
        // assert(new_msg && "new_msg not initialized");
        new_msg->read_from_stream(ifs);
        new_msg->process();
        
    }
    
    std::cout << "[DEBUG]Read " << count << " messages in total" << std::endl;
    return 0;
}