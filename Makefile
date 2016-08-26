# Programs
rm    = rm -rf
mkdir = mkdir -p

# Compiler
CXX         ?= clang++
compiler     = $(CXX)
base_flags   = $(CXXFLAGS) -std=c++11 -fmessage-length=0 -Wall -Wextra -pedantic -D_GLIBCXX_USE_C99
base_libs    =
base_ldflags =

# Folders
output_dir = dist
source_dir = lib
test_dir   = tests

# Tests
test_files   = all.cpp
test_flags   = $(base_flags) -O0 -g -DDEBUG -Ilib -Ideps/catch/include
test_libs    = $(base_libs)
test_ldflags = $(base_ldflags)
test_exename = test

# Artifacts
test_objs = $(test_files:%.cpp=$(output_dir)/$(test_dir)/%.o)
test_deps = $(test_files:%.cpp=$(output_dir)/$(test_dir)/%.d)
test_output = $(output_dir)/$(test_exename)

# Default targets
all:

clean:
	$(rm) $(test_objs) $(test_deps) $(test_output)
	$(rm) $(output_dir)

# Test targets
-include $(test_deps)

test: $(test_output)
	./$(test_output)

$(test_output): $(test_objs)
	@$(mkdir) $(dir $@)
	$(compiler) $(test_ldflags) -o$@ $(test_objs) $(test_libs)

$(output_dir)/$(test_dir)/%.o: $(test_dir)/%.cpp
	@$(mkdir) $(dir $@)
	$(compiler) -c $(test_flags) -o$@ $< -MMD -MF$(@:%.o=%.d)
