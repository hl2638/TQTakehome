#ifndef MESSAGE_READER_H
#define MESSAGE_READER_H
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <unordered_map>
#include "message_types.h"

class MessageReader {
public:
    MessageReader(const std::string& file_path): file_path_{file_path} {}
    void start_reading() {
        ifs_.open(file_path_);
        if (!ifs_.is_open()) {
            std::cerr << "Error opening file " << file_path_ << std::endl;
            return;
        }
        reader_thread_ = std::thread(&MessageReader::read_from_stream, this);
    }

    void stop_reading() {
        if (reader_thread_.joinable()) {
            reader_thread_.join();
        }
    }

    bool ifs_finished() const {
        return finished_reading_;
    }

    bool get_next_message(std::unique_ptr<BaseMessage>& msg) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [&]{ return !msg_queue_.empty() || finished_reading_; });
        if (finished_reading_ && msg_queue_.empty()) {
            return false;
        }
        msg = std::move(msg_queue_.front());
        msg_queue_.pop();
        return true;
    }

private:
    std::string file_path_;
    std::ifstream ifs_;
    std::queue<std::unique_ptr<BaseMessage>> msg_queue_;
    int msg_count = 0;
    std::thread reader_thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool finished_reading_ = false;

    void read_from_stream() {
        while (!ifs_.eof()) {
            // Read message length
            uint16_t msg_len = read_big_endian<2>(ifs_);
            if (msg_len == 0) {
                continue;
            }

            // Read first byte: message type
            char msg_type;
            ifs_.read(&msg_type, 1);

            msg_count++;
            // [DEBUG]
            if (msg_count % 10'000'000 == 0) {
                std::cout << "Read " << msg_count << " messages." << std::endl;
            }
            
            std::unique_ptr<BaseMessage> new_msg;
            switch(msg_type) {
                case 'S': {
                    // System Event
                    new_msg = std::make_unique<SystemEventMessage>();
                    break;
                }
                case 'R': {
                    // Stock Directory
                    new_msg = std::make_unique<StockDirectoryMessage>();
                    break;
                }
                case 'A': {
                    // Add Order
                    new_msg = std::make_unique<AddOrderMessage>();
                    break;
                }
                case 'F': {
                    // Add Order with MPID Attribution
                    new_msg = std::make_unique<AddOrderMPIDAttributionMessage>();
                    break;
                }
                case 'E': {
                    // Order Executed
                    new_msg = std::make_unique<OrderExecutedMessage>();
                    break;
                }
                case 'C': {
                    // Order Executed With Price
                    new_msg = std::make_unique<OrderExecutedWithPriceMessage>();
                    break;
                }
                case 'U': {
                    // Order Replace
                    new_msg = std::make_unique<OrderReplaceMessage>();
                    break;
                }
                case 'P': {
                    // Trade Message
                    new_msg = std::make_unique<TradeMessage>();
                    break;
                }
                case 'Q': {
                    // Cross Trade Message
                    new_msg = std::make_unique<CrossTradeMessage>();
                    break;
                }
                case 'B': {
                    // Broken Trade Message
                    new_msg = std::make_unique<BrokenTradeMessage>();
                    break;
                }
                default: {
                    // std::cout << "Skip message type: " << msg_type << ", len: " << msg_len << std::endl;
                    ifs_.ignore(msg_len - 1);
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
            new_msg->read_from_stream(ifs_);
            {
                std::lock_guard<std::mutex> lock(mutex_);
                msg_queue_.push(std::move(new_msg));
            }
            cv_.notify_one();
            
        }

        // Finished reading
        {
            std::lock_guard<std::mutex> lock(mutex_);
            finished_reading_ = true;
        }
        cv_.notify_one();
    }    
};

#endif // MESSAGE_READER_H