#!/bin/bash
# Build script for SONiC POC with Official SONiC Components

set -e

echo "=== Building SONiC POC with Official SONiC Components ==="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check prerequisites
print_status "Checking prerequisites..."

if ! command -v docker &> /dev/null; then
    print_error "Docker is not installed"
    exit 1
fi

if ! command -v docker-compose &> /dev/null; then
    print_error "Docker Compose is not installed"
    exit 1
fi

# Create necessary directories
print_status "Creating directories..."
mkdir -p data/sonic-vs
mkdir -p data/redis
mkdir -p data/telemetry
mkdir -p logs/sonic
mkdir -p configs/sonic
mkdir -p configs/redis

# Download official SONiC images
print_status "Pulling official SONiC Docker images..."

# Note: These are example image names - actual SONiC images may have different names
# Check https://hub.docker.com/u/sonicdev for official images

print_warning "Attempting to pull SONiC images..."
print_warning "If images are not available, we'll build mock versions"

# Try to pull official images, fallback to building if not available
SONIC_IMAGES=(
    "docker-sonic-vs:latest"
    "docker-orchagent:latest" 
    "docker-syncd-vs:latest"
    "docker-sonic-config:latest"
    "docker-sonic-restapi:latest"
    "docker-sonic-telemetry:latest"
)

for image in "${SONIC_IMAGES[@]}"; do
    if docker pull "$image" 2>/dev/null; then
        print_status "Successfully pulled $image"
    else
        print_warning "Could not pull $image - will build alternative"
        
        # Create alternative Dockerfile for missing images
        case "$image" in
            "docker-sonic-vs:latest")
                print_status "Creating alternative SONiC VS container..."
                cat > Dockerfile.sonic-vs-alt << 'EOF'
FROM ubuntu:20.04

# Install SONiC dependencies
RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    redis-server \
    openssh-server \
    net-tools \
    iproute2 \
    bridge-utils \
    vlan \
    tcpdump \
    curl \
    jq \
    supervisor \
    && rm -rf /var/lib/apt/lists/*

# Install Python packages
RUN pip3 install redis pyyaml jsonschema

# Create SONiC user
RUN useradd -m -s /bin/bash admin && \
    echo 'admin:YourPaSsWoRd' | chpasswd && \
    usermod -aG sudo admin

# Create SONiC directories
RUN mkdir -p /etc/sonic /var/log/sonic /usr/local/bin

# Copy mock SONiC CLI scripts
COPY scripts/sonic/mock-sonic-cli.py /usr/local/bin/
RUN chmod +x /usr/local/bin/mock-sonic-cli.py

# Create symbolic links for SONiC commands
RUN ln -s /usr/local/bin/mock-sonic-cli.py /usr/local/bin/sonic-cfggen && \
    ln -s /usr/local/bin/mock-sonic-cli.py /usr/local/bin/show && \
    ln -s /usr/local/bin/mock-sonic-cli.py /usr/local/bin/config

# Configure SSH
RUN mkdir /var/run/sshd && \
    echo 'PermitRootLogin yes' >> /etc/ssh/sshd_config && \
    sed -i 's/#PasswordAuthentication yes/PasswordAuthentication yes/' /etc/ssh/sshd_config

# Supervisor configuration
COPY supervisord-sonic-vs.conf /etc/supervisor/conf.d/supervisord.conf

EXPOSE 22 8080 9090 6379

CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/conf.d/supervisord.conf"]
EOF
                docker build -f Dockerfile.sonic-vs-alt -t docker-sonic-vs:latest .
                ;;
                
            "docker-orchagent:latest")
                print_status "Creating alternative orchagent container..."
                cat > Dockerfile.orchagent-alt << 'EOF'
FROM ubuntu:20.04

RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    redis-tools \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install redis

# Mock orchagent
COPY scripts/sonic/mock-orchagent.py /usr/bin/orchagent
RUN chmod +x /usr/bin/orchagent

CMD ["/usr/bin/orchagent"]
EOF
                docker build -f Dockerfile.orchagent-alt -t docker-orchagent:latest .
                ;;
                
            "docker-syncd-vs:latest")
                print_status "Creating alternative syncd container..."
                cat > Dockerfile.syncd-alt << 'EOF'
FROM ubuntu:20.04

RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    redis-tools \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install redis

# Mock syncd
COPY scripts/sonic/mock-syncd.py /usr/bin/syncd
RUN chmod +x /usr/bin/syncd

CMD ["/usr/bin/syncd"]
EOF
                docker build -f Dockerfile.syncd-alt -t docker-syncd-vs:latest .
                ;;
        esac
    fi
done

# Build custom demo applications
print_status "Building SAI demo application..."
docker build -f Dockerfile.sai-demo -t sonic-sai-demo:latest .

print_status "Building BSP demo application..."
docker build -f Dockerfile.bsp-demo -t sonic-bsp-demo:latest .

print_status "Building management interface..."
docker build -f Dockerfile.sonic-mgmt -t sonic-mgmt:latest .

# Create Redis configuration for SONiC
print_status "Creating SONiC Redis configuration..."
cat > configs/redis/sonic-redis.conf << 'EOF'
# SONiC Redis Configuration
port 6379
bind 0.0.0.0
protected-mode no
save 900 1
save 300 10
save 60 10000
rdbcompression yes
dbfilename sonic.rdb
dir /data
maxmemory-policy allkeys-lru
EOF

# Create basic SONiC configuration
print_status "Creating basic SONiC configuration..."
cat > configs/sonic/config_db.json << 'EOF'
{
    "DEVICE_METADATA": {
        "localhost": {
            "hostname": "sonic-vs",
            "platform": "x86_64-kvm_x86_64-r0",
            "mac": "52:54:00:12:34:56",
            "bgp_asn": "65100",
            "deployment_id": "1",
            "type": "ToRRouter"
        }
    },
    "PORT": {
        "Ethernet0": {
            "lanes": "0",
            "speed": "25000",
            "index": "0"
        },
        "Ethernet4": {
            "lanes": "1", 
            "speed": "25000",
            "index": "1"
        },
        "Ethernet8": {
            "lanes": "2",
            "speed": "25000", 
            "index": "2"
        },
        "Ethernet12": {
            "lanes": "3",
            "speed": "25000",
            "index": "3"
        },
        "Ethernet16": {
            "lanes": "4",
            "speed": "25000",
            "index": "4"
        },
        "Ethernet20": {
            "lanes": "5",
            "speed": "25000",
            "index": "5"
        }
    }
}
EOF

# Create supervisor configuration for SONiC VS
print_status "Creating supervisor configuration..."
cat > supervisord-sonic-vs.conf << 'EOF'
[supervisord]
nodaemon=true
user=root
logfile=/var/log/supervisord.log
pidfile=/var/run/supervisord.pid

[program:redis]
command=redis-server /etc/redis/redis.conf
autostart=true
autorestart=true
stdout_logfile=/var/log/redis.log
stderr_logfile=/var/log/redis.log

[program:sshd]
command=/usr/sbin/sshd -D
autostart=true
autorestart=true
stdout_logfile=/var/log/sshd.log
stderr_logfile=/var/log/sshd.log
EOF

print_status "Build completed successfully!"
print_status "You can now start the SONiC POC with:"
print_status "  docker-compose -f docker-compose-sonic-official.yml up -d"

echo ""
print_warning "Note: This setup uses alternative containers where official SONiC images are not available."
print_warning "For production use, replace with actual SONiC images from the SONiC project."
print_warning "Official SONiC images: https://github.com/sonic-net/sonic-buildimage"
