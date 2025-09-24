#include "main.hpp"

namespace {

// Basit argüman -> TestKind eşlemesi
TestKind parse_arg(int argc, char* argv[]) {
    if (argc < 2) return TestKind::All;

    std::string a = argv[1];
    std::transform(a.begin(), a.end(), a.begin(), ::tolower);

    if (a == "--all")              return TestKind::All;
    if (a == "--serial1")          return TestKind::Serial1;
    if (a == "--serial2")          return TestKind::Serial2;
    if (a == "--realsense")        return TestKind::Realsense;
    if (a == "--led=red")          return TestKind::LedRed;
    if (a == "--led=green")        return TestKind::LedGreen;
    if (a == "--led=blue")         return TestKind::LedBlue;
    if (a == "--button=mission")   return TestKind::ButtonMission;
    if (a == "--services")         return TestKind::Services;

    std::cout <<
        "Kullanim:\n"
        "  " << argv[0] << " [--all | --serial1 | --serial2 | --realsense |\n"
        "                    --led=red | --led=green | --led=blue |\n"
        "                    --button=mission | --services]\n"
        "Varsayilan: --all\n";
    // Taninmazsa tüm testler
    return TestKind::All;
}

} // namespace

CheckSystem::CheckSystem() : gpio_(GPIOControl::getInstance()) {
    gpio_.setTestMode(false);
    initialize();
}

void CheckSystem::check() {
    // Eski davranış: hepsini çalıştır
    check(TestKind::All);
}

void CheckSystem::check(TestKind which) {
    // Önce, bazı testler için gerekli önkoşullar zaten initialize'da yapıldı.

    // İstenirse sadece servis kontrolü
    if (which == TestKind::Services) {
        checkSpecifiedServices();
        logInfo("Service kontrolü bitti.");
        return;
    }

    // Sadece istenen testi koştur
    auto run_serial1 = [&](){
        _components["serial1"].running = (_components["serial1"].initialized && port1_) ? checkSerial(port1_) : false;
    };
    auto run_serial2 = [&](){
        _components["serial2"].running = (_components["serial2"].initialized && port2_) ? checkSerial(port2_) : false;
    };
    auto run_realsense = [&](){
        _components["realsense"].running = isRealsenseConnected();
    };
    auto run_led = [&](const std::string& name){
        if (!gpio_.isTestMode() && gpio_.isInitialized())
            _components[name].running = checkLed(name);
        else
            _components[name].running = false;
    };
    auto run_button_mission = [&](){
        if (!gpio_.isTestMode() && gpio_.isInitialized())
            _components["mission"].running = checkButton("mission");
        else
            _components["mission"].running = false;
    };

    switch (which) {
        case TestKind::All:
            // Eski toplu akış
            _components["serial1"].running = (_components["serial1"].initialized && port1_) ? checkSerial(port1_) : false;
            _components["serial2"].running = (_components["serial2"].initialized && port2_) ? checkSerial(port2_) : false;
            _components["realsense"].running = isRealsenseConnected();

            if (!gpio_.isTestMode() && gpio_.isInitialized()) {
                _components["red"].running    = checkLed("red");
                _components["green"].running  = checkLed("green");
                _components["blue"].running   = checkLed("blue");
                _components["mission"].running= checkButton("mission");
            }
            break;

        case TestKind::Serial1:        run_serial1(); break;
        case TestKind::Serial2:        run_serial2(); break;
        case TestKind::Realsense:      run_realsense(); break;
        case TestKind::LedRed:         run_led("red"); break;
        case TestKind::LedGreen:       run_led("green"); break;
        case TestKind::LedBlue:        run_led("blue"); break;
        case TestKind::ButtonMission:  run_button_mission(); break;
        case TestKind::Services:       /* üstte handle edildi */ break;
    }

    info();
}

bool CheckSystem::checkSerial(const std::unique_ptr<Serial> &port) {
    logInfo("Listening for incoming data on /dev/ttyAMA1 (baudrate: 115200)...", Color::TEXT_GRN);

    if (!port || !port->isOpen()) {
        logError("Port is not open");
        return false;
    }

    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count() <= 10) {
        uint8_t buffer;
        int bytes_read = port->readByte(&buffer);
        if (bytes_read > 0) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}

bool CheckSystem::checkButton(std::string button_name) {
    logInfo("Toggle button: turn it ON and then OFF within 10 seconds.", Color::TEXT_GRN);
    bool initial_state = gpio_.getSwitchState(button_name);

    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count() <= 7) {
        bool current_state = gpio_.getSwitchState(button_name);
        logInfo("Button: %s", initial_state ? "ON" : "OFF");
        if (current_state != initial_state) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}

bool CheckSystem::checkLed(std::string led_name) {
    logInfo("Flashing LED", Color::TEXT_GRN);
    for (int i = 0; i < 5; i++) {
        gpio_.setLED(led_name, (i % 2 == 0 ? IOState::IO_HIGH : IOState::IO_LOW));
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    std::cout << "[QUESTION] Is the " << led_name << " LED currently flashing? (y/n): ";
    char response;
    std::cin >> response;
    gpio_.setLED(led_name, IOState::IO_LOW);
    return response == 'y' || response == 'Y';
}

void CheckSystem::initialize() {
    // Çakışma olmasın diye önce servis kontrolü (tüm modlarda mantıklı)
    checkSpecifiedServices();

    _components.at("serial1").initialized = isSerialPortAvailable(_components.at("serial1").name);
    _components.at("serial2").initialized = isSerialPortAvailable(_components.at("serial2").name);

    _components.at("red").initialized    = gpio_.isInitialized();
    _components.at("blue").initialized   = gpio_.isInitialized();
    _components.at("green").initialized  = gpio_.isInitialized();
    _components.at("mission").initialized= gpio_.isInitialized();

    if (gpio_.isInitialized()) {
        gpio_.addSwitch("mission", 17);
        gpio_.addSwitch("kamikaze", 27);
        gpio_.addLED("red", 22);
        gpio_.addLED("green", 23);
        gpio_.addLED("blue", 18);
        gpio_.setLED("red", IOState::IO_LOW);
        gpio_.setLED("green", IOState::IO_LOW);
        gpio_.setLED("blue", IOState::IO_LOW);
    }

    if (_components.at("serial1").initialized) {
        port1_ = std::make_unique<Serial>(_components.at("serial1").name);
        port1_->connect();
    }
    if (_components.at("serial2").initialized) {
        port2_ = std::make_unique<Serial>(_components.at("serial2").name);
        port2_->connect();
    }
}

bool CheckSystem::isSerialPortAvailable(const std::string &path) {
    if (!std::filesystem::exists(path)) {
        logError("Serial port %s does not exist.", path.c_str());
        return false;
    }
    return true;
}

bool CheckSystem::isRealsenseConnected() {
    std::array<char, 128> buffer{};
    std::string result;

    FILE *pipe = popen(command_.c_str(), "r");
    if (!pipe) {
        std::cerr << "\"lsusb\" command failed to execute!" << std::endl;
        return false;
    }
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);

    return result.find(_components.at("realsense").name) != std::string::npos;
}

bool CheckSystem::isServiceRunning(const std::string &serviceName) {
    std::string cmd = "systemctl show " + serviceName + " --property=SubState --value 2>/dev/null";
    std::array<char, 128> buffer{};
    std::string result;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        logError("Command Not Opened: %s", cmd.c_str());
        return false;
    }
    if (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result = buffer.data();
        result.erase(std::remove_if(result.begin(), result.end(),
                                    [](unsigned char c){ return std::isspace(c); }),
                     result.end());
    }
    return (result == "running");
}

void CheckSystem::checkSpecifiedServices() {
    std::vector<std::string> services = {
        "microxrceagent.service",
        "commander.service",
        // "cam_recorder.service"
        // , "realsense.service"
        // , "openvins.service"
    };

    for (const auto &service : services) {
        if (isServiceRunning(service)) {
            logError("Service %s is running. Please stop it before proceeding.", service.c_str());
            exit(EXIT_FAILURE);
        }
    }
    logInfo("All specified services are confirmed to be stopped.");
}

void CheckSystem::info() const {
    std::cout << " ===================================\n";
    std::cout << std::left;
    std::cout << std::setw(32) << "| Microxrc (ttyAMA0)" << (_components.at("serial1").running ? "✅" : "❌") << "  |\n"
              << std::setw(32) << "| UKB (ttyAMA1)"      << (_components.at("serial2").running ? "✅" : "❌") << "  |\n"
              << std::setw(32) << "| Mission Button"     << (_components.at("mission").running ? "✅" : "❌") << "  |\n"
              << std::setw(32) << "| Red Led"            << (_components.at("red").running ? "✅" : "❌") << "  |\n"
              << std::setw(32) << "| Green Led"          << (_components.at("green").running ? "✅" : "❌") << "  |\n"
              << std::setw(32) << "| Blue Led"           << (_components.at("blue").running ? "✅" : "❌") << "  |\n"
              << std::setw(32) << "| Realsense"          << (_components.at("realsense").running ? "✅" : "❌") << "  |\n";
    std::cout << " ===================================\n";
}

int main(int argc, char *argv[]) {
    CheckSystem check_system;
    TestKind which = parse_arg(argc, argv);
    check_system.check(which);
    return 0;
}
