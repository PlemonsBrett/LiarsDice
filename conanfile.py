from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.build import check_min_cppstd
from conan.tools.scm import Version
from conan.errors import ConanInvalidConfiguration


class LiarsDiceConan(ConanFile):
    name = "liarsdice"
    version = "1.0.0"
    description = "A modern C++23 implementation of Liar's Dice"
    author = "LiarsDice Team"
    license = "MIT"
    url = "https://github.com/yourusername/liarsdice"
    homepage = "https://github.com/yourusername/liarsdice"
    topics = ("game", "dice", "cpp23", "cmake")

    # Package configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "build_tests": [True, False],
        "build_examples": [True, False],
        "build_benchmarks": [True, False],
        "enable_logging": [True, False],
        "log_level": ["trace", "debug", "info", "warn", "error", "critical", "off"],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "build_tests": True,
        "build_examples": True,
        "build_benchmarks": False,
        "enable_logging": True,
        "log_level": "info",
    }

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "src/*", "include/*", "apps/*", "tests/*", "examples/*", "cmake/*", "assets/*"

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        
        # Ensure C++23 standard is set if not provided by profile
        if not self.settings.compiler.get_safe("cppstd"):
            self.settings.compiler.cppstd = "23"

    def layout(self):
        cmake_layout(self)

    def validate(self):
        check_min_cppstd(self, "23")
        
        # Check for minimum compiler versions that support C++23
        min_versions = {
            "gcc": "12",
            "clang": "15", 
            "apple-clang": "14",
            "msvc": "193"  # Visual Studio 2022
        }
        
        compiler = str(self.settings.compiler)
        version = Version(self.settings.compiler.version)
        
        if compiler in min_versions:
            if version < min_versions[compiler]:
                raise ConanInvalidConfiguration(
                    f"This package requires {compiler} >= {min_versions[compiler]} "
                    f"for C++23 support, but found {version}"
                )

    def requirements(self):
        # Core dependencies
        if self.options.enable_logging:
            self.requires("spdlog/1.12.0", override=True)
            # Force a newer fmt that's compatible with C++23
            self.requires("fmt/10.2.1", override=True)
        self.requires("nlohmann_json/3.11.2")  # For structured logging and configuration
        
        # Test dependencies
        if self.options.build_tests:
            self.requires("catch2/3.4.0")

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.28]")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        
        tc = CMakeToolchain(self)
        
        # Pass options to CMake
        tc.variables["LIARSDICE_BUILD_TESTS"] = self.options.build_tests
        tc.variables["LIARSDICE_BUILD_EXAMPLES"] = self.options.build_examples
        tc.variables["LIARSDICE_BUILD_BENCHMARKS"] = self.options.build_benchmarks
        tc.variables["LIARSDICE_INSTALL"] = True
        tc.variables["LIARSDICE_USE_SANITIZERS"] = self.settings.build_type == "Debug"
        tc.variables["LIARSDICE_ENABLE_LOGGING"] = self.options.enable_logging
        tc.variables["LIARSDICE_LOG_LEVEL"] = str(self.options.log_level).upper()
        
        # Ensure C++23 standard
        tc.variables["CMAKE_CXX_STANDARD"] = "23"
        tc.variables["CMAKE_CXX_STANDARD_REQUIRED"] = "ON"
        
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["liarsdice_core"]
        self.cpp_info.requires = ["liarsdice::core"]
        
        # Set include directories
        self.cpp_info.includedirs = ["include"]
        
        # Ensure C++23 standard for consumers
        self.cpp_info.cppstd = "23"
        
        # Add compiler features
        if self.settings.compiler == "gcc" or self.settings.compiler == "clang":
            self.cpp_info.cxxflags = ["-std=c++23"]
        
        # Add component information
        self.cpp_info.components["core"].libs = ["liarsdice_core"]
        self.cpp_info.components["core"].includedirs = ["include"]
        self.cpp_info.components["core"].cppstd = "23"
        self.cpp_info.components["core"].requires = ["spdlog::spdlog", "nlohmann_json::nlohmann_json"]
        
        # Add logging configuration defines
        if self.options.enable_logging:
            self.cpp_info.defines.append("LIARSDICE_ENABLE_LOGGING")
            log_level_map = {
                "trace": "SPDLOG_LEVEL_TRACE",
                "debug": "SPDLOG_LEVEL_DEBUG", 
                "info": "SPDLOG_LEVEL_INFO",
                "warn": "SPDLOG_LEVEL_WARN",
                "error": "SPDLOG_LEVEL_ERROR",
                "critical": "SPDLOG_LEVEL_CRITICAL",
                "off": "SPDLOG_LEVEL_OFF"
            }
            log_level = str(self.options.log_level).lower()
            if log_level in log_level_map:
                self.cpp_info.defines.append(f"SPDLOG_ACTIVE_LEVEL={log_level_map[log_level]}")
        else:
            self.cpp_info.defines.append("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF")