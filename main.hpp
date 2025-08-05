#ifndef SYSTEM_TESTS_MAIN_HPP
#define SYSTEM_TESTS_MAIN_HPP

#include <array>
#include <string>
#include <chrono>
#include <thread>
#include <iomanip>
#include <unistd.h>
#include <iostream>
#include <termios.h>
#include <sys/stat.h>
#include <filesystem>
#include <map>

#include "lectron/serial/serial.hpp"
#include "lectron/logger/logger.hpp"
#include "lectron/gpio/gpio_control.hpp"

enum class Category
{
    LED,
    BUZZER,
    SWITCH,
    SERVO,
    SERIAL,
    CAMERA
};

struct PortInfo
{
    Category category;
    std::string name;
    bool initialized;    
    bool running;        
};

class CheckSystem
{
public:
    CheckSystem();
    void check();
    bool checkSerial(const std::unique_ptr<Serial>& port);
    bool checkButton(std::string button_name);
    bool checkLed(std::string led_name);
    bool isRealsenseConnected();

private:
    void initialize();
    bool isSerialPortAvailable(const std::string &path);
    bool isServiceRunning(const std::string& serviceName);
    void checkSpecifiedServices();
    void info() const;

private:
    GPIOControl &gpio_;
    const std::string command_ = "lsusb";

    std::unique_ptr<Serial> port1_;
    std::unique_ptr<Serial> port2_;

    std::map<std::string, PortInfo> _components = {
        {"red", {Category::LED, "red", false, false}},
        {"blue", {Category::LED, "blue", false, false}},
        {"green", {Category::LED, "green", false, false}},
        {"mission", {Category::SWITCH, "mission", false, false}},
        {"serial1", {Category::SERIAL, "/dev/ttyAMA0", false, false}},
        {"serial2", {Category::SERIAL, "/dev/ttyAMA1", false, false}},
        {"realsense", {Category::CAMERA, "RealSense(TM) Depth Module", false, false}}
    };
};

#endif // SYSTEM_TESTS_MAIN_HPP