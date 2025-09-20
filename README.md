# SONiC POC with Official SONiC Image

This repository contains a comprehensive Proof of Concept (POC) for SONiC (Software for Open Networking in the Cloud) Network Operating System using the official SONiC Docker image with full Redis orchestration and C++ testing framework.

## 🎯 Overview

This POC provides **real SONiC integration** with authentic orchestration flow through Redis databases, OrchAgent, syncd, and other SONiC daemons, featuring:

- **C++ Test Framework**: 17 comprehensive tests across HAL, SAI, and Interrupt systems
- **Real SONiC Components**: ConfigMgr, OrchAgent, syncd, SAI interface
- **Redis Integration**: Full database orchestration (CONFIG_DB, APPL_DB, STATE_DB)
- **Hardware Abstraction**: Fan, temperature, PSU, LED control simulation
- **Switch Abstraction**: VLAN management, port configuration, status control
- **Event System**: Cable insertion/removal, link flap detection, SFP hot swap

### ✅ What's Real vs Simulated
- **100% Real**: Redis databases, OrchAgent, syncd, ConfigMgr, SAI interface, SONiC CLI, database schemas
- **Simulated**: Only the physical switch hardware (replaced with virtual switch)

## 🚀 Quick Start

### Prerequisites
- Docker installed and running
- Make build tools
- C++ compiler (g++ with C++17 support)
- 8GB RAM (16GB recommended)

### Setup and Installation

1. **Clone the repository**:
```bash
git clone https://github.com/alokpandey/sonic_poc_official_image.git
cd sonic_poc_official_image
```

2. **Start SONiC Container**:
```bash
./scripts/sonic/start_sonic.sh
```

3. **Build C++ Test Framework**:
```bash
make -f Makefile.cpp all
```

4. **Run All Tests**:
```bash
./build/sonic_functional_tests --verbose
```

## 🧪 Test Framework

### Test Categories (17 Total Tests)

#### HAL Tests (6 Tests)
- Fan Speed Control
- Temperature Monitoring  
- Power Supply Control
- LED Control
- Interface HAL Control
- System Information

#### SAI Tests (6 Tests)
- VLAN Creation and Deletion
- VLAN Member Management
- Port Configuration
- Port Status Control
- Multiple VLAN Operations
- VLAN Port Interaction

#### Interrupt Tests (5 Tests)
- Cable Insertion/Removal
- Link Flap Detection
- SFP Hot Swap
- Multi-Port Cable Events
- SONiC CLI Response

### Running Tests

```bash
# Run all tests
./build/sonic_functional_tests --verbose

# Run specific test suites
./build/sonic_functional_tests --hal-only --verbose
./build/sonic_functional_tests --sai-only --verbose
./build/sonic_functional_tests --interrupt-only --verbose

# Quick smoke test
./build/sonic_functional_tests --quick --verbose
```

## 🔧 Build System

### C++ Build Commands
```bash
# Full build
make -f Makefile.cpp all

# Clean build
make -f Makefile.cpp clean
make -f Makefile.cpp all

# Individual components
make -f Makefile.cpp hal
make -f Makefile.cpp sai
make -f Makefile.cpp interrupts
make -f Makefile.cpp tests
```

### Docker Build
```bash
# Build Docker image
./run_cpp_tests.sh build

# Run tests in Docker
./run_cpp_tests.sh all --verbose
```

## 📁 Project Structure

```
sonic_poc_official_image/
├── src/
│   ├── cpp/                    # C++ implementation
│   │   ├── hal/               # Hardware Abstraction Layer
│   │   ├── sai/               # Switch Abstraction Interface
│   │   ├── interrupts/        # Interrupt and Event Handling
│   │   └── tests/             # Test framework
│   └── python/                # Python utilities
├── scripts/                   # Setup and utility scripts
├── docs/                      # Architecture diagrams (XML)
├── Makefile.cpp              # C++ build configuration
├── docker-compose*.yml       # Docker configurations
├── Dockerfile.*              # Docker build files
└── README.md                 # This file
```

## 🗄️ Redis Database Integration

### Database Usage
- **CONFIG_DB (db 4)**: Configuration data (ports, VLANs, etc.)
- **APPL_DB (db 0)**: Application/operational data
- **STATE_DB (db 6)**: State information (SFP, transceivers)

### Example Redis Operations
```bash
# Check port configuration
docker exec sonic-vs-official redis-cli -n 4 HGET "PORT|Ethernet0" "admin_status"

# Check operational status
docker exec sonic-vs-official redis-cli -n 0 HGET "PORT_TABLE:Ethernet0" "oper_status"

# Check VLAN configuration
docker exec sonic-vs-official redis-cli -n 4 KEYS "VLAN|*"
```

## 🔍 Features

### Hardware Abstraction Layer (HAL)
```cpp
// Fan control with real SONiC integration
hal_controller->setFanSpeed(1, 75);  // Set to 75%
auto fan_info = hal_controller->getFanInfo(1);

// Temperature monitoring
auto temp_info = hal_controller->getTempSensorInfo(1);
```

### Switch Abstraction Interface (SAI)
```cpp
// VLAN management with Redis orchestration
sai_controller->createVLAN(100, "Production_VLAN");
sai_controller->addPortToVLAN(100, "Ethernet0", true);  // Tagged

// Port configuration
sai_controller->setPortSpeed("Ethernet0", 10000);  // 10G
```

### Interrupt and Event Handling
```cpp
// Cable event simulation with Redis updates
interrupt_controller->simulateCableInsertion("Ethernet0");
interrupt_controller->simulateCableRemoval("Ethernet0");
```

## 🐛 Troubleshooting

### Common Issues

1. **SONiC container not running**:
```bash
# Check container status
docker ps | grep sonic

# Start container
./scripts/sonic/start_sonic.sh
```

2. **Build failures**:
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install build-essential g++ make
```

3. **Redis connection issues**:
```bash
# Test Redis connectivity
docker exec sonic-vs-official redis-cli ping
```

## 📄 Documentation

For comprehensive documentation, see `SONIC_POC_Documentation.md` which includes:
- Detailed architecture explanations
- Complete test descriptions
- Performance metrics
- Troubleshooting guides
- Implementation details

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🙏 Acknowledgments

- SONiC community for the open-source network operating system
- Docker for containerization platform
- Redis for the database backend
