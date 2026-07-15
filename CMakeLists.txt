cmake_minimum_required(VERSION 3.20.0)

set(BOARD esp32_devkitc/esp32/procpu)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(curing_station)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE components/state_machine.c)
target_sources(app PRIVATE components/curing_time.c)

target_include_directories(app PRIVATE ./)
target_include_directories(app PRIVATE components)
