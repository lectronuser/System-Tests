# System Test

**In order for the system to work, the systemd files specified in [this](https://github.com/lectronuser/System-Tests/blob/main/main.cpp#L197) section must not be running.**

### Install 

```bash
git clone git@github.com:lectronuser/System-Tests.git
```

## Build & Run

```bash
cd System-Tests
```

```bash
cmake -B build
cmake --build build

./build/system_checker
```

```bash
# Hepsi (varsayılan)
./system_tests

# Sadece ttyAMA0 dinle
./system_tests --serial1

# Sadece ttyAMA1 dinle
./system_tests --serial2

# Sadece RealSense kontrolü
./system_tests --realsense

# LED testleri (kullanıcı onaylı)
./system_tests --led=red
./system_tests --led=green
./system_tests --led=blue

# Mission buton testi
./system_tests --button=mission

# Sadece servis çakışma kontrolü
./system_tests --services
```