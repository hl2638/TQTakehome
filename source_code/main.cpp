#include <iostream>
#include <fstream>
#include <memory>
#include <cassert>
#include "message_reader.h"
#include "message_parser.h"
#include "system_data.h"
#include "utils.h"

int main(int argc, char** argv)
{
    
    if (argc <= 3) {
        std::cout << "HINT: Usage: " << argv[0] << " [<'csv' or 'log'> [<data_file_path> [<output_dir_path>]]]" << std::endl;
    }

    const std::string csv_or_log_str = (argc > 1) ? argv[1] : "csv";
    SystemData::PrintFormat print_format;
    if (csv_or_log_str == "csv") {
        print_format = SystemData::PrintFormat::csv;
    } else if (csv_or_log_str == "log") {
        print_format = SystemData::PrintFormat::log;
    } else {
        std::cerr << "format must be 'csv' or 'log'" << std::endl
        << "HINT: Usage: " << argv[0] << " [<'csv' or 'log'> [<data_file_path> [<output_dir_path>]]]" << std::endl;
        return 1;
    }
    const std::string data_file_path = (argc > 2) ? argv[2] : "./data/01302019.NASDAQ_ITCH50";
    const std::string output_dir_path = (argc> 3) ? argv[3] : "./output/vwap/";

    std::cout << "Data file is: " << data_file_path << std::endl;
    std::cout << "Output log directory is: " << output_dir_path << std::endl;
    std::cout << "Output format is: " << csv_or_log_str << std::endl;

    SystemData sys_data{output_dir_path, print_format};

    MessageReader msg_reader(data_file_path);
    msg_reader.start_reading();

    MessageParser msg_parser(msg_reader, sys_data);
    msg_parser.start_parsing();


    msg_reader.stop_reading();
    msg_parser.stop_parsing();
    
    return 0;
}