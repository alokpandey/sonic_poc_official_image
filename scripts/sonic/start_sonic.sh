#!/bin/bash

# SONiC Container Startup Script
# This script starts the official SONiC virtual switch container

set -e

CONTAINER_NAME="sonic-vs-official"
SONIC_IMAGE="docker-sonic-vs:latest"

echo "Starting SONiC Virtual Switch Container..."

# Check if container already exists
if docker ps -a --format 'table {{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Container ${CONTAINER_NAME} already exists."
    
    # Check if it's running
    if docker ps --format 'table {{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
        echo "Container ${CONTAINER_NAME} is already running."
        exit 0
    else
        echo "Starting existing container ${CONTAINER_NAME}..."
        docker start ${CONTAINER_NAME}
        sleep 5
    fi
else
    echo "Creating and starting new container ${CONTAINER_NAME}..."
    
    # Try to pull the image first
    echo "Pulling SONiC image..."
    docker pull ${SONIC_IMAGE} || {
        echo "Warning: Could not pull ${SONIC_IMAGE}. Using local image if available."
    }
    
    # Run the container
    docker run -d \
        --name ${CONTAINER_NAME} \
        --privileged \
        --network bridge \
        -p 2222:22 \
        -p 8080:8080 \
        -p 9200:9200 \
        -p 6379:6379 \
        ${SONIC_IMAGE} || {
        echo "Error: Failed to start SONiC container."
        echo "Please ensure the SONiC image is available locally or can be pulled."
        exit 1
    }
    
    echo "Waiting for SONiC to initialize..."
    sleep 10
fi

# Verify container is running
if docker ps --format 'table {{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo " SONiC container ${CONTAINER_NAME} is running successfully!"
    
    # Test Redis connectivity
    echo "Testing Redis connectivity..."
    if docker exec ${CONTAINER_NAME} redis-cli ping >/dev/null 2>&1; then
        echo " Redis is responding"
    else
        echo "  Redis is not responding yet, may need more time to initialize"
    fi
    
    # Show container info
    echo ""
    echo "Container Information:"
    echo "====================="
    docker ps --filter "name=${CONTAINER_NAME}" --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
    
    echo ""
    echo "To connect to SONiC CLI:"
    echo "docker exec -it ${CONTAINER_NAME} bash"
    echo ""
    echo "To test Redis:"
    echo "docker exec ${CONTAINER_NAME} redis-cli ping"
    echo ""
    echo "To run tests:"
    echo "make -f Makefile.cpp all && ./build/sonic_functional_tests --verbose"
    
else
    echo " Failed to start SONiC container"
    exit 1
fi
