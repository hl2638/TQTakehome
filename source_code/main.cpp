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
    const std::string data_file_path = (argc > 1) ? argv[1] : "./data/01302019.NASDAQ_ITCH50";
    const std::string output_file_path = (argc> 2) ? argv[2] : "./output/vwap.log";

    if (argc <= 2) {
        std::cout << "Usage: " << argv[0] << " [<data_file_path> [<output_file_path>]]" << std::endl;
    }

    std::cout << "Assuming data file is: " << data_file_path << std::endl;
    std::cout << "Assuming output log file is: " << output_file_path << std::endl;

    std::ifstream ifs{data_file_path};
    assert(ifs.good() && "Failed to open data file.");
    std::ofstream ofs{output_file_path};
    assert(ofs.good() && "Failed to open output log file.");

    SystemData sys_data{ofs};

    MessageReader msg_reader(ifs);
    msg_reader.start_reading();

    MessageParser msg_parser(msg_reader, sys_data);
    msg_parser.start_parsing();


    msg_reader.stop_reading();
    msg_parser.stop_parsing();
    
    return 0;
}