#ifndef MESSAGE_PARSER_H
#define MESSAGE_PARSER_H
#include <mutex>
#include <condition_variable>
#include <thread>
#include "message_types.h"
#include "message_reader.h"

class MessageParser {
public:
    MessageParser(MessageReader& reader, SystemData& sd)
    : reader_{reader}, sd_{sd} {}

    void start_parsing() {
        parser_thread_ = std::thread(&MessageParser::parse_messages_, this);
    }

    void stop_parsing() {
        if (parser_thread_.joinable()) {
            parser_thread_.join();
        }
    }

private:
    MessageReader& reader_;
    SystemData& sd_;

    void parse_messages_() {
        while (true) {
            std::unique_ptr<BaseMessage> msg;
            if (!reader_.get_next_message(msg)) {
                break;
            }
            msg->process(sd_);
        }
    }

    std::thread parser_thread_;
};

#endif // MESSAGE_PARSER_H