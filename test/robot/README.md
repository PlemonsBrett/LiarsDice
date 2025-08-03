# LiarsDice Robot Framework Tests

This directory contains automated tests for the LiarsDice CLI using Robot Framework and Pexpect.

## Overview

The test suite provides comprehensive coverage of:

- **Input Validation**: Tests for invalid inputs, edge cases, and boundary conditions
- **Game Logic**: Win/lose conditions, game flow, and AI behavior
- **Performance**: Response times, memory usage, and stress testing
- **Signal Handling**: Graceful shutdown with Ctrl+C and other signals
- **Timeouts**: Proper handling of user inactivity

## Test Structure

```sh
tests/robot/
├── LiarsDiceLibrary.py      # Custom Robot Framework library using Pexpect
├── liarsdice_tests.robot    # Main test suite
├── edge_cases.robot         # Edge case and boundary tests
├── performance.robot        # Performance and stress tests
├── requirements.txt         # Python dependencies
├── run_tests.sh            # Test execution script
├── ci_tests.yml            # GitHub Actions workflow template
└── README.md               # This file
```

## Prerequisites

- Python 3.8 or higher
- Built LiarsDice CLI executable
- Unix-like environment (Linux, macOS)

## Installation

1. Build the LiarsDice project:

   ```bash
   ./scripts/build.sh
   ```

2. Install test dependencies:

   ```bash
   cd tests/robot
   python3 -m venv venv
   source venv/bin/activate
   pip install -r requirements.txt
   ```

## Running Tests

### Using the Test Runner Script

The easiest way to run tests is using the provided script:

```bash
# Run all tests
./run_tests.sh

# Run specific test suite
./run_tests.sh --suite main
./run_tests.sh --suite edge
./run_tests.sh --suite performance

# Run tests with specific tags
./run_tests.sh --tag smoke
./run_tests.sh --tag input-validation
./run_tests.sh --exclude long-running

# Enable verbose output
./run_tests.sh --verbose

# Enable debug logging
./run_tests.sh --debug
```

### Manual Execution

You can also run Robot Framework directly:

```bash
# Activate virtual environment
source venv/bin/activate

# Set Python path
export PYTHONPATH=$PWD:$PYTHONPATH

# Run tests
robot --variable CLI_PATH:../../build/bin/liarsdice-cli liarsdice_tests.robot
```

## Test Tags

Tests are organized with tags for selective execution:

- `smoke`: Quick tests for basic functionality
- `input-validation`: Input validation tests
- `gameplay`: Game logic tests
- `win-condition`: Tests for winning scenarios
- `ai`: AI-specific tests
- `performance`: Performance measurement tests
- `stress`: Stress and load tests
- `signal-handling`: Signal handling tests
- `timeout`: Timeout behavior tests
- `edge-case`: Edge case tests
- `security`: Security-related tests
- `long-running`: Tests that take longer to execute

## Test Reports

After running tests, Robot Framework generates:

- `results/report.html`: Detailed test report
- `results/log.html`: Execution log with debug information
- `results/xunit.xml`: JUnit-compatible XML for CI integration

## Custom Library (LiarsDiceLibrary)

The custom library provides keywords for:

- Starting and controlling the CLI process
- Sending input and signals
- Expecting specific prompts or patterns
- Measuring performance metrics
- Validating output

Key keywords:

- `Start LiarsDice`: Launch the CLI application
- `Send Input`: Send text input to the CLI
- `Expect Prompt`: Wait for specific text
- `Send Control C`: Send Ctrl+C signal
- `Process Should Be Running/Terminated`: Verify process state
- `Get Process Memory Usage`: Monitor memory consumption
- `Measure Response Time`: Track performance

## Writing New Tests

Example test case:

```robotframework
*** Test Cases ***
Test New Feature
    [Documentation]    Test description
    [Tags]    feature-x
    
    # Start the CLI
    Start LiarsDice
    
    # Wait for prompt
    Expect Prompt    Enter number of players
    
    # Send input
    Send Input    2
    
    # Verify output
    Output Should Contain    Expected text
    
    # Clean up is automatic via Test Teardown
```

## CI/CD Integration

Copy `ci_tests.yml` to `.github/workflows/robot-tests.yml` to run tests automatically on:

- Push to main/develop branches
- Pull requests

The CI workflow:

1. Builds the project
2. Installs test dependencies
3. Runs smoke and validation tests
4. Uploads test results as artifacts
5. Publishes test results in PR comments

## Troubleshooting

### Tests fail to start

- Ensure the CLI is built: `./scripts/build.sh`
- Check the CLI path in test variables
- Verify Python environment is activated

### Timeout errors

- Increase timeout in test variables
- Check if CLI is hanging (use `--debug` flag)
- Review process output in logs

### Memory/Performance issues

- Adjust thresholds in test variables
- Run performance tests in isolation
- Check for memory leaks in the application

## Best Practices

1. **Keep tests independent**: Each test should be self-contained
2. **Use appropriate timeouts**: Balance between reliability and speed
3. **Tag tests properly**: Helps with test organization and CI
4. **Document test purpose**: Use `[Documentation]` tags
5. **Handle cleanup**: Ensure processes are terminated properly
6. **Validate both positive and negative cases**: Test both success and failure paths

## Contributing

When adding new tests:

1. Follow the existing test structure
2. Add appropriate tags
3. Document the test purpose
4. Consider edge cases
5. Update this README if needed
