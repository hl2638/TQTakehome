#ifndef MESSAGE_PARSER_H
#define MESSAGE_PARSER_H
#include <mutex>
#include <condition_variable>
#include <thread>
#include "message_types.h"
#include "message_reader.h"

class MessageParser {
public:
    MessageParser() {}

    // Function to start parsing messages
    void start_parsing(MessageReader& reader, SystemData& sd) {
        parser_thread_ = std::thread(&MessageParser::parse_messages, this, std::ref(reader), std::ref(sd));
    }

    // Function to stop parsing messages
    void stopParsing() {
        if (parser_thread_.joinable()) {
            parser_thread_.join();
        }
    }

private:
    // Function to parse messages
    void parse_messages(MessageReader& reader, SystemData& sd) {
        while (true) {
            std::unique_ptr<BaseMessage> msg;
            if (!reader.get_next_message(msg)) {
                break;
            }
            // Process the message
            msg->process(sd);
        }
    }

    std::thread parser_thread_;
};

#endif // MESSAGE_PARSER_H