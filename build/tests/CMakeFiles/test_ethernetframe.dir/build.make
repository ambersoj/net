# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.28

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /usr/local/cmd

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /usr/local/cmd/build

# Include any dependencies generated for this target.
include tests/CMakeFiles/test_ethernetframe.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include tests/CMakeFiles/test_ethernetframe.dir/compiler_depend.make

# Include the progress variables for this target.
include tests/CMakeFiles/test_ethernetframe.dir/progress.make

# Include the compile flags for this target's objects.
include tests/CMakeFiles/test_ethernetframe.dir/flags.make

tests/CMakeFiles/test_ethernetframe.dir/test_ethernetframe.cpp.o: tests/CMakeFiles/test_ethernetframe.dir/flags.make
tests/CMakeFiles/test_ethernetframe.dir/test_ethernetframe.cpp.o: /usr/local/cmd/tests/test_ethernetframe.cpp
tests/CMakeFiles/test_ethernetframe.dir/test_ethernetframe.cpp.o: tests/CMakeFiles/test_ethernetframe.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/usr/local/cmd/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object tests/CMakeFiles/test_ethernetframe.dir/test_ethernetframe.cpp.o"
	cd /usr/local/cmd/build/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT tests/CMakeFiles/test_ethernetframe.dir/test_ethernetframe.cpp.o -MF CMakeFiles/test_ethernetframe.dir/test_ethernetframe.cpp.o.d -o CMakeFiles/test_ethernetframe.dir/test_ethernetframe.cpp.o -c /usr/local/cmd/tests/test_ethernetframe.cpp

tests/CMakeFiles/test_ethernetframe.dir/test_ethernetframe.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/test_ethernetframe.dir/test_ethernetframe.cpp.i"
	cd /usr/local/cmd/build/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /usr/local/cmd/tests/test_ethernetframe.cpp > CMakeFiles/test_ethernetframe.dir/test_ethernetframe.cpp.i

tests/CMakeFiles/test_ethernetframe.dir/test_ethernetframe.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/test_ethernetframe.dir/test_ethernetframe.cpp.s"
	cd /usr/local/cmd/build/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /usr/local/cmd/tests/test_ethernetframe.cpp -o CMakeFiles/test_ethernetframe.dir/test_ethernetframe.cpp.s

# Object files for target test_ethernetframe
test_ethernetframe_OBJECTS = \
"CMakeFiles/test_ethernetframe.dir/test_ethernetframe.cpp.o"

# External object files for target test_ethernetframe
test_ethernetframe_EXTERNAL_OBJECTS =

bin/test_ethernetframe: tests/CMakeFiles/test_ethernetframe.dir/test_ethernetframe.cpp.o
bin/test_ethernetframe: tests/CMakeFiles/test_ethernetframe.dir/build.make
bin/test_ethernetframe: src/libcmd_lib.a
bin/test_ethernetframe: tests/CMakeFiles/test_ethernetframe.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/usr/local/cmd/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../bin/test_ethernetframe"
	cd /usr/local/cmd/build/tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_ethernetframe.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tests/CMakeFiles/test_ethernetframe.dir/build: bin/test_ethernetframe
.PHONY : tests/CMakeFiles/test_ethernetframe.dir/build

tests/CMakeFiles/test_ethernetframe.dir/clean:
	cd /usr/local/cmd/build/tests && $(CMAKE_COMMAND) -P CMakeFiles/test_ethernetframe.dir/cmake_clean.cmake
.PHONY : tests/CMakeFiles/test_ethernetframe.dir/clean

tests/CMakeFiles/test_ethernetframe.dir/depend:
	cd /usr/local/cmd/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /usr/local/cmd /usr/local/cmd/tests /usr/local/cmd/build /usr/local/cmd/build/tests /usr/local/cmd/build/tests/CMakeFiles/test_ethernetframe.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : tests/CMakeFiles/test_ethernetframe.dir/depend

