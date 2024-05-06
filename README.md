**ITCH5.0 Parser**

This tool reads an ITCH5.0 tick data file (of a trading day) and prints out the VWAP of each stock every hour, including market close.

**How to build**

Make sure you have `cmake` installed and with version >= 3.0.0. and `g++` that supports C++20. 

Clone the repo and run `cmake .` right under the repo. Then run `make`. The binary is a file named `parser` and it should appear right under the repo directory.

**How to run**

Copy/move the tick data under `data/`. Or choose a path of your own and specify it in the command below.

Run `./parser [<'csv'or 'log'> [<data_file_path> [<output_directory>]]]`

- If `data_file_path` not specified, by default the program will look for data file named `01302019.NASDAQ_ITCH50` under `data/`.

The output will be a directory with multiple csv files or human-readable log files each representing the beginning of each market hour (including the hour when market closes). 

- If `'csv' or 'log'` not specified, by default the output will be csv. If `output_directory` not specified, by default output will go to `output/vwap`.

So the simplest command would be `./parser`. The program will find data file named `01302019.NASDAQ_ITCH50` under `data/`, and output a csv for each hour under `output/vwap`.
