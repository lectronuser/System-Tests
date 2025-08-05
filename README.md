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

