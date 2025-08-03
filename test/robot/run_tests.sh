#!/bin/bash

# Robot Framework test runner for LiarsDice CLI
# This script sets up the environment and runs the automated tests

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
CLI_PATH="${BUILD_DIR}/standalone/liarsdice"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

echo "LiarsDice CLI Test Runner"
echo "========================"

# Check if CLI is built
if [ ! -f "${CLI_PATH}" ]; then
    echo -e "${RED}Error: LiarsDice CLI not found at ${CLI_PATH}${NC}"
    echo "Please build the project first with: ./scripts/build.sh"
    exit 1
fi

# Check Python and pip
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}Error: Python 3 is required but not installed.${NC}"
    exit 1
fi

# Create virtual environment if it doesn't exist
VENV_DIR="${SCRIPT_DIR}/venv"
if [ ! -d "${VENV_DIR}" ]; then
    echo -e "${YELLOW}Creating Python virtual environment...${NC}"
    python3 -m venv "${VENV_DIR}"
fi

# Activate virtual environment
source "${VENV_DIR}/bin/activate"

# Install dependencies
echo -e "${YELLOW}Installing test dependencies...${NC}"
pip install -q -r "${SCRIPT_DIR}/requirements.txt"

# Set PYTHONPATH to include our library
export PYTHONPATH="${SCRIPT_DIR}:${PYTHONPATH}"

# Default test options
OUTPUT_DIR="${SCRIPT_DIR}/results"
mkdir -p "${OUTPUT_DIR}"

# Parse command line arguments
TEST_SUITE="all"
TAGS=""
VERBOSE=""
DEBUG=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --suite)
            TEST_SUITE="$2"
            shift 2
            ;;
        --tag)
            TAGS="--include $2"
            shift 2
            ;;
        --exclude)
            TAGS="${TAGS} --exclude $2"
            shift 2
            ;;
        --verbose)
            VERBOSE="--loglevel DEBUG"
            shift
            ;;
        --debug)
            DEBUG="--debugfile ${OUTPUT_DIR}/debug.log"
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --suite SUITE     Run specific test suite (all, main, edge, performance)"
            echo "  --tag TAG         Include tests with specific tag"
            echo "  --exclude TAG     Exclude tests with specific tag"
            echo "  --verbose         Enable verbose logging"
            echo "  --debug           Enable debug logging to file"
            echo "  --help            Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                           # Run all tests"
            echo "  $0 --suite main              # Run main test suite only"
            echo "  $0 --tag smoke               # Run smoke tests only"
            echo "  $0 --exclude long-running    # Skip long-running tests"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Determine which test files to run
case ${TEST_SUITE} in
    all)
        TEST_FILES="${SCRIPT_DIR}/*.robot"
        ;;
    main)
        TEST_FILES="${SCRIPT_DIR}/liarsdice_tests.robot"
        ;;
    edge)
        TEST_FILES="${SCRIPT_DIR}/edge_cases.robot"
        ;;
    performance)
        TEST_FILES="${SCRIPT_DIR}/performance.robot"
        ;;
    *)
        TEST_FILES="${TEST_SUITE}"
        ;;
esac

# Run the tests
echo -e "${GREEN}Running Robot Framework tests...${NC}"
echo "Test suite: ${TEST_SUITE}"
echo "Output directory: ${OUTPUT_DIR}"
echo ""

# Change to project root so assets can be found
cd "${PROJECT_ROOT}"

robot \
    ${VERBOSE} \
    ${DEBUG} \
    ${TAGS} \
    --outputdir "${OUTPUT_DIR}" \
    --report report.html \
    --log log.html \
    --xunit xunit.xml \
    --variable CLI_PATH:"${CLI_PATH}" \
    ${TEST_FILES}

# Check test results
if [ $? -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    echo "View the report at: ${OUTPUT_DIR}/report.html"
else
    echo -e "${RED}Some tests failed.${NC}"
    echo "View the report at: ${OUTPUT_DIR}/report.html"
    exit 1
fi

# Deactivate virtual environment
deactivate