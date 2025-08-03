"""
Robot Framework library for testing LiarsDice CLI using Pexpect
"""

import pexpect
import signal
import time
import os
import psutil
from robot.api.deco import keyword
from robot.libraries.BuiltIn import BuiltIn


class LiarsDiceLibrary:
    """Library for testing LiarsDice CLI application."""
    
    ROBOT_LIBRARY_SCOPE = 'TEST SUITE'
    
    def __init__(self):
        self.process = None
        self.timeout = 30
        self.cli_path = None
        self._start_time = None
        
    @keyword
    def set_cli_path(self, path):
        """Set the path to the LiarsDice CLI executable."""
        if not os.path.exists(path):
            raise ValueError(f"CLI executable not found at: {path}")
        self.cli_path = path
        
    @keyword
    def start_liarsdice(self, timeout=30, args=None):
        """Start the LiarsDice CLI application."""
        if not self.cli_path:
            raise ValueError("CLI path not set. Use 'Set CLI Path' keyword first.")
        
        self.timeout = int(timeout)

        # Build command with optional arguments
        if args:
            cmd = f"{self.cli_path} {args}"
            self.process = pexpect.spawn(cmd, encoding='utf-8', timeout=self.timeout)
        else:
            self.process = pexpect.spawn(self.cli_path, encoding='utf-8', timeout=self.timeout)
            
        self._start_time = time.time()
        return self.process

    @keyword
    def start_liarsdice_with_args(self, args, timeout=30):
        """Start the LiarsDice CLI with command line arguments."""
        return self.start_liarsdice(timeout=timeout, args=args)
        
    @keyword
    def expect_prompt(self, prompt, timeout=None):
        """Wait for a specific prompt."""
        if not self.process:
            raise ValueError("Process not started")

        # Convert timeout to float if it's a string
        if timeout is not None:
            timeout = float(timeout)
        else:
            timeout = self.timeout
            
        try:
            self.process.expect(prompt, timeout=timeout)
            return True
        except pexpect.TIMEOUT:
            self._log_process_output()
            raise AssertionError(f"Timeout waiting for prompt: {prompt}")
        except pexpect.EOF:
            self._log_process_output()
            raise AssertionError("Process terminated unexpectedly")
            
    @keyword
    def send_input(self, text):
        """Send input to the CLI."""
        if not self.process:
            raise ValueError("Process not started")

        self.process.sendline(str(text))
        
    @keyword
    def send_control_c(self):
        """Send Ctrl+C signal to the process."""
        if not self.process:
            raise ValueError("Process not started")
        
        self.process.sendcontrol('c')
        
    @keyword
    def send_signal(self, signal_name):
        """Send a specific signal to the process."""
        if not self.process:
            raise ValueError("Process not started")
        
        sig = getattr(signal, signal_name.upper())
        self.process.kill(sig)
        
    @keyword
    def process_should_be_running(self):
        """Verify that the process is still running."""
        if not self.process or not self.process.isalive():
            raise AssertionError("Process is not running")
            
    @keyword
    def process_should_be_terminated(self, timeout=5):
        """Verify that the process has terminated."""
        if not self.process:
            raise ValueError("Process not started")
        
        start = time.time()
        while self.process.isalive() and (time.time() - start) < timeout:
            time.sleep(0.1)
            
        if self.process.isalive():
            raise AssertionError("Process is still running")
            
    @keyword
    def get_output(self):
        """Get all output from the process."""
        if not self.process:
            return ""
        
        return self.process.before or ""
        
    @keyword
    def output_should_contain(self, text):
        """Verify that the output contains specific text."""
        output = self.get_output()
        if text not in output:
            raise AssertionError(f"Output does not contain '{text}'. Output: {output}")
            
    @keyword
    def output_should_not_contain(self, text):
        """Verify that the output does not contain specific text."""
        output = self.get_output()
        if text in output:
            raise AssertionError(f"Output contains '{text}'. Output: {output}")
            
    @keyword
    def measure_response_time(self):
        """Measure the response time of the last operation."""
        if not self._start_time:
            return 0
        
        return time.time() - self._start_time
        
    @keyword
    def response_time_should_be_less_than(self, max_seconds):
        """Verify that response time is within acceptable limits."""
        response_time = self.measure_response_time()
        max_seconds = float(max_seconds)
        
        if response_time > max_seconds:
            raise AssertionError(f"Response time {response_time}s exceeds maximum {max_seconds}s")
            
    @keyword
    def get_process_memory_usage(self):
        """Get current memory usage of the process in MB."""
        if not self.process or not self.process.isalive():
            return 0
        
        try:
            proc = psutil.Process(self.process.pid)
            return proc.memory_info().rss / 1024 / 1024  # Convert to MB
        except:
            return 0
            
    @keyword
    def memory_usage_should_be_less_than(self, max_mb):
        """Verify that memory usage is within acceptable limits."""
        usage = self.get_process_memory_usage()
        max_mb = float(max_mb)
        
        if usage > max_mb:
            raise AssertionError(f"Memory usage {usage:.2f}MB exceeds maximum {max_mb}MB")
            
    @keyword
    def cleanup_process(self):
        """Clean up the process."""
        if self.process:
            if self.process.isalive():
                self.process.terminate(force=True)
            self.process = None
            
    def _log_process_output(self):
        """Log process output for debugging."""
        if self.process:
            output = self.process.before or ""
            BuiltIn().log(f"Process output: {output}")
            
    @keyword
    def wait_for_pattern(self, pattern, timeout=None):
        """Wait for a regex pattern in the output."""
        if not self.process:
            raise ValueError("Process not started")

        # Convert timeout to float if it's a string
        if timeout is not None:
            timeout = float(timeout)
        else:
            timeout = self.timeout
            
        try:
            self.process.expect(pattern, timeout=timeout)
            return self.process.match.group(0) if self.process.match else ""
        except pexpect.TIMEOUT:
            self._log_process_output()
            raise AssertionError(f"Timeout waiting for pattern: {pattern}")
            
    @keyword
    def simulate_ai_game(self, num_ai_players=1):
        """Start a game with AI players."""
        # Ensure num_ai_players is an integer
        num_ai_players = int(num_ai_players)
        total_players = num_ai_players + 1

        # Wait for the prompt first
        self.expect_prompt("Enter the number of players")
        self.send_input(str(total_players))  # Total players
        
        if total_players == 1:
            # Single player mode - goes to difficulty selection
            self.expect_prompt("Single Player Mode!")
            self.expect_prompt("Enter your choice (1-4)")
            self.send_input("1")  # Choose Easy difficulty
        else:
            # Multi-player mode - asks for AI count
            self.expect_prompt("How many AI players")
            self.send_input(str(num_ai_players))
            # Wait for game to start
            try:
                self.process.expect("Game starting", timeout=5)
            except:
                pass  # Continue even if we don't see this exact message
        
    @keyword
    def make_guess(self, dice_count, face_value):
        """Make a guess in the game."""
        self.send_input("1")  # Choose to make a guess
        self.expect_prompt("dice count")
        self.send_input(str(dice_count))
        self.expect_prompt("face value")
        self.send_input(str(face_value))