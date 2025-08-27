#include "main.hpp"

CheckSystem::CheckSystem() : gpio_(GPIOControl::getInstance())
{
    gpio_.setTestMode(false);
    initialize();
}

void CheckSystem::check()
{
    _components["serial1"].running = checkSerial(port1_);
    _components["serial2"].running = checkSerial(port2_);
    _components["realsense"].running = isRealsenseConnected();

    if (!gpio_.isTestMode() && gpio_.isInitialized())
    {
        _components["red"].running = checkLed("red");
        _components["green"].running = checkLed("green");
        _components["blue"].running = checkLed("blue");
        _components["mission"].running = checkButton("mission");
    }

    info();
}

bool CheckSystem::checkSerial(const std::unique_ptr<Serial> &port)
{
    logInfo("Listening for incoming data on /dev/ttyAMA1 (baudrate: 115200)...", Color::TEXT_GRN);

    if (!port->isOpen())
    {
        logError("Port is not open");
        return false;
    }

    auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start);

    while (elapsed.count() <= 10)
    {
        uint8_t buffer;
        int bytes_read = port->readByte(&buffer);
        if (bytes_read > 0)
        {
            return true;
        }

        now = std::chrono::steady_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return false;
}

bool CheckSystem::checkButton(std::string button_name)
{
    logInfo("Toggle button: turn it ON and then OFF within 10 seconds.", Color::TEXT_GRN);
    bool initial_state = gpio_.getSwitchState(button_name);

    auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start);

    while (elapsed.count() <= 7)
    {
        bool current_state = gpio_.getSwitchState(button_name);
        if (current_state != initial_state)
        {
            return true;
        }

        now = std::chrono::steady_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return false;
}

bool CheckSystem::checkLed(std::string led_name)
{
    logInfo("Flashing LED", Color::TEXT_GRN);
    for (int i = 0; i < 5; i++)
    {
        gpio_.setLED(led_name, (i % 2 == 0 ? IOState::IO_HIGH : IOState::IO_LOW));
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    std::cout << "[QUESTION] Is the " << led_name << " LED currently flashing? (y/n): ";
    char response;
    std::cin >> response;
    gpio_.setLED(led_name, IOState::IO_LOW);

    return response == 'y' || response == 'Y' ? true : false;
}

void CheckSystem::initialize()
{
    checkSpecifiedServices();

    _components.at("serial1").initialized = isSerialPortAvailable(_components.at("serial1").name);
    _components.at("serial2").initialized = isSerialPortAvailable(_components.at("serial2").name);

    _components.at("red").initialized = gpio_.isInitialized();
    _components.at("blue").initialized = gpio_.isInitialized();
    _components.at("green").initialized = gpio_.isInitialized();
    _components.at("mission").initialized = gpio_.isInitialized();

    if (gpio_.isInitialized())
    {
        gpio_.addSwitch("mission", 17);
        gpio_.addSwitch("kamikaze", 27);
        gpio_.addLED("red", 22);
        gpio_.addLED("green", 23);
        gpio_.addLED("blue", 18);

        gpio_.setLED("red", IOState::IO_LOW);
        gpio_.setLED("green", IOState::IO_LOW);
        gpio_.setLED("blue", IOState::IO_LOW);
    }

    if (_components.at("serial1").initialized)
    {
        port1_ = std::make_unique<Serial>(_components.at("serial1").name);
        port1_->connect();
    }

    if (_components.at("serial2").initialized)
    {
        port2_ = std::make_unique<Serial>(_components.at("serial2").name);
        port2_->connect();
    }
}

bool CheckSystem::isSerialPortAvailable(const std::string &path)
{
    if (!std::filesystem::exists(path))
    {
        logError("Serial port %s does not exist.", path.c_str());
        return false;
    }

    return true;
}

bool CheckSystem::isRealsenseConnected()
{
    std::array<char, 128> buffer;
    std::string result;

    FILE *pipe = popen(command_.c_str(), "r");
    if (!pipe)
    {
        std::cerr << "\"lsusb\" command failed to execute!" << std::endl;
        return false;
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    {
        result += buffer.data();
    }

    pclose(pipe);

    return result.find(_components.at("realsense").name) != std::string::npos;
}

bool CheckSystem::isServiceRunning(const std::string &serviceName)
{
    std::string cmd = "systemctl show " + serviceName + " --property=SubState --value 2>/dev/null";
    std::array<char, 128> buffer;
    std::string result;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe)
    {
        logError("Command Not Opened: %s", cmd.c_str());
        return false;
    }

    if (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result = buffer.data();
        result.erase(std::remove_if(result.begin(), result.end(),
                                    [](unsigned char c)
                                    { return std::isspace(c); }),
                     result.end());
    }

    return (result == "running");
}

void CheckSystem::checkSpecifiedServices()
{
    std::vector<std::string> services = {
        "microxrceagent.service"
        , "commander.service"
        , "cam_recorder.service"
        // , "realsense.service"
        // , "openvins.service"
    };

    for (const auto &service : services)
    {
        if (isServiceRunning(service))
        {
            logError("Service %s is running. Please stop it before proceeding.", service.c_str());
            exit(EXIT_FAILURE);
        }
    }

    logInfo("All specified services are confirmed to be stopped.");
}

void CheckSystem::info() const
{
    std::cout << " ===================================" << std::endl;
    std::cout << std::left;
    std::cout << std::setw(32) << "| Microxrc (ttyAMA0)" << (_components.at("serial1").running ? "✅" : "❌") << "  |" << std::endl
              << std::setw(32) << "| UKB (ttyAMA1)" << (_components.at("serial2").running ? "✅" : "❌") << "  |" << std::endl
              << std::setw(32) << "| Mission Button" << (_components.at("mission").running ? "✅" : "❌") << "  |" << std::endl
              << std::setw(32) << "| Red Led" << (_components.at("red").running ? "✅" : "❌") << "  |" << std::endl
              << std::setw(32) << "| Green Led" << (_components.at("green").running ? "✅" : "❌") << "  |" << std::endl
              << std::setw(32) << "| Blue Led" << (_components.at("blue").running ? "✅" : "❌") << "  |" << std::endl
              << std::setw(32) << "| Realsense" << (_components.at("realsense").running ? "✅" : "❌") << "  |" << std::endl;
    std::cout << " ===================================" << std::endl;
}
int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    CheckSystem check_system;
    check_system.check();
    return 0;
}
