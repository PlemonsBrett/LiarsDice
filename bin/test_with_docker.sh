#!/bin/bash

# Script to test GitHub Actions workflows using Docker containers

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "========================================================"
echo "Testing GitHub Actions Workflows with Docker Containers"
echo "========================================================"

# Function to test a workflow
test_workflow() {
    local name=$1
    local dockerfile=$2
    
    echo -e "\n${BLUE}Testing $name workflow...${NC}"
    
    # Build the Docker image
    echo "Building Docker image for $name..."
    if docker build -f "$dockerfile" -t "liarsdice-$name" . > /tmp/docker-$name.log 2>&1; then
        echo -e "${GREEN}✓${NC} Docker image built successfully"
        
        # Run the container
        echo "Running $name workflow test..."
        if docker run --rm "liarsdice-$name" > /tmp/docker-$name-run.log 2>&1; then
            echo -e "${GREEN}✓${NC} $name workflow passed!"
        else
            echo -e "${RED}✗${NC} $name workflow failed!"
            echo "Last 20 lines of output:"
            tail -20 /tmp/docker-$name-run.log
        fi
    else
        echo -e "${RED}✗${NC} Failed to build Docker image for $name"
        echo "Last 20 lines of build log:"
        tail -20 /tmp/docker-$name.log
    fi
}

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    echo -e "${RED}Docker is not installed. Please install Docker first.${NC}"
    exit 1
fi

# Create docker directory if it doesn't exist
mkdir -p docker

# Test each workflow
workflows=(
    "ubuntu:docker/ubuntu.Dockerfile"
    "install:docker/install.Dockerfile"
    "standalone:docker/standalone.Dockerfile"
    "documentation:docker/documentation.Dockerfile"
)

for workflow in "${workflows[@]}"; do
    IFS=':' read -r name dockerfile <<< "$workflow"
    if [ -f "$dockerfile" ]; then
        test_workflow "$name" "$dockerfile"
    else
        echo -e "${YELLOW}Skipping $name: $dockerfile not found${NC}"
    fi
done

# Note about Windows
echo -e "\n${YELLOW}Note:${NC} Windows workflow requires actual Windows containers or VMs for accurate testing."
echo "The windows.Dockerfile uses MinGW cross-compilation as an approximation."

echo -e "\n========================================================"
echo "Docker testing complete!"
echo "========================================================"
echo ""
echo "To view detailed logs:"
echo "  cat /tmp/docker-<workflow>.log      # Build logs"
echo "  cat /tmp/docker-<workflow>-run.log  # Run logs"
echo ""
echo "To run a specific workflow interactively:"
echo "  docker run -it --rm -v \$(pwd):/workspace liarsdice-<workflow> bash"