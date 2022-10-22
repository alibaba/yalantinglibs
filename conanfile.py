from conans import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class CoroRpcConan(ConanFile):
    name = "coro_rpc"
    version = "0.1.0"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "BUILD_WITH_LIBCXX": [True, False],
        "BUILD_EXAMPLES": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "BUILD_WITH_LIBCXX": True,
        "BUILD_EXAMPLES": False,
    }

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", \
                      "asio_util/*", "examples/*", "logging/*", "coro_rpc/*", "struct_pack/*", "util/*", "tests/*"

    def requirements(self):
        self.requires("asio/1.23.0")
        self.requires("spdlog/1.10.0")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        variables = {
            "USE_CONAN": "ON"
        }
        cmake.configure(variables)
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["hello"]
