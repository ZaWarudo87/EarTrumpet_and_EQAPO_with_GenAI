# =========================================================================
# Makefile with separate Release and Debug configurations
# =========================================================================

# --- Variables ---
CXX = cl.exe
TARGET_BASE_NAME = gemini_app
SOURCES = main.cpp

# --- vcpkg Configuration ---
VCPKG_ROOT = ..\vcpkg
VCPKG_TRIPLET = x64-windows
VCPKG_INCLUDE_DIR = $(VCPKG_ROOT)\installed\$(VCPKG_TRIPLET)\include
VCPKG_LIB_DIR     = $(VCPKG_ROOT)\installed\$(VCPKG_TRIPLET)\lib
VCPKG_DEBUG_LIB_DIR = $(VCPKG_ROOT)\installed\$(VCPKG_TRIPLET)\debug\lib

# --- Build Configurations ---

# Common flags
COMMON_CXXFLAGS = /std:c++17 /utf-8 /EHsc /I"$(VCPKG_INCLUDE_DIR)"

# Release-specific flags
# /MD       - Multithreaded dynamic runtime (Release)
# /O2       - Optimize for speed
# /DNDEBUG  - Define NDEBUG macro (disables asserts, etc.)
RELEASE_CXXFLAGS = $(COMMON_CXXFLAGS) /MD /O2 /DNDEBUG
RELEASE_LDFLAGS = /link /LIBPATH:"$(VCPKG_LIB_DIR)" cpr.lib Ws2_32.lib Crypt32.lib

# Debug-specific flags
# /MDd      - Multithreaded dynamic runtime (Debug)
# /Zi       - Generate full debug information
# /Od       - Disable optimizations
DEBUG_CXXFLAGS = $(COMMON_CXXFLAGS) /MDd /Zi /Od
# Note: Linking against debug libraries from vcpkg
DEBUG_LDFLAGS = /link /LIBPATH:"$(VCPKG_DEBUG_LIB_DIR)" cpr.lib Ws2_32.lib Crypt32.lib

# --- Targets ---

# Default target will be the release build
all: release

# Release build target
release:
	@echo Building RELEASE...
	$(CXX) $(RELEASE_CXXFLAGS) $(SOURCES) /Fo:release\ /Fe:$(TARGET_BASE_NAME)_release.exe $(RELEASE_LDFLAGS)

# Debug build target
debug:
	@echo Building DEBUG...
	$(CXX) $(DEBUG_CXXFLAGS) $(SOURCES) /Fo:debug\ /Fe:$(TARGET_BASE_NAME)_debug.exe $(DEBUG_LDFLAGS)

# To use the debug build, you need to install the debug libraries from vcpkg first:
# > vcpkg install cpr:x64-windows --triplet x64-windows-static
# Or ensure debug libs are installed for your dynamic triplet.

# Run the release version
run: release
	@echo Running RELEASE version...
	.\$(TARGET_BASE_NAME)_release.exe

# Clean all build outputs
clean:
	@echo Cleaning up...
	del /Q /F *.obj *.ilk *.pdb 2>nul
	del /Q /F *_release.exe *_debug.exe 2>nul
	if exist release rmdir /s /q release
	if exist debug rmdir /s /q debug

.PHONY: all release debug run clean