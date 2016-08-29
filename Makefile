# Programs
rm    = rm -rf
mkdir = mkdir -p
ar    = ar rcs

# Compiler
CXX         ?= clang++
compiler     = $(CXX)
base_flags   = $(CXXFLAGS) -std=c++11 -fmessage-length=0 -Wall -Wextra -pedantic -D_GLIBCXX_USE_C99
base_libs    =
base_ldflags =

# Folders
output_dir = dist
lib_dir    = lib
test_dir   = tests

# Library
lib_name    = knxproto
lib_sover   = 0
lib_files   =
lib_headers = knxproto/common.hpp knxproto/utils/natseq.hpp knxproto/utils/buffer.hpp
lib_flags   = $(base_flags) -fPIC
lib_libs    = $(base_libs)
lib_ldflags = $(base_ldflags) -shared -Wl,-soname,lib$(lib_name).so

# Tests
test_exename = test
test_files   = all.cpp utils/natseq.cpp utils/buffer.cpp
test_flags   = $(base_flags) -O0 -g -DDEBUG -Ilib -Ideps/catch/include
test_libs    = $(base_libs)
test_ldflags = $(base_ldflags)

# Artifacts
lib_objs     = $(lib_files:%.cpp=$(output_dir)/$(lib_dir)/%.o)
lib_deps     = $(lib_files:%.cpp=$(output_dir)/$(lib_dir)/%.d)
lib_sooutput = $(output_dir)/lib$(lib_name).so
lib_aoutput  = $(output_dir)/lib$(lib_name).a

test_objs    = $(test_files:%.cpp=$(output_dir)/$(test_dir)/%.o)
test_deps    = $(test_files:%.cpp=$(output_dir)/$(test_dir)/%.d)
test_output  = $(output_dir)/$(test_exename)

# Default targets
all: $(lib_sooutput) $(lib_aoutput)

shared: $(lib_sooutput)

static: $(lib_aoutput)

test: $(test_output)
	./$(test_output)

clean:
	$(rm) $(test_objs) $(test_deps) $(test_output)
	$(rm) $(lib_objs) $(lib_deps) $(lib_output)
	$(rm) $(output_dir)

# Library targets
-include $(lib_deps)

$(lib_sooutput): $(lib_objs)
	@$(mkdir) $(dir $@)
	$(compiler) $(lib_ldflags) -o$@ $(lib_objs) $(lib_libs)

$(lib_aoutput): $(lib_objs)
	@$(mkdir) $(dir $@)
	$(ar) $@ $(lib_objs)

$(output_dir)/$(lib_dir)/%.o: $(lib_dir)/%.cpp
	@$(mkdir) $(dir $@)
	$(compiler) -c $(lib_flags) -o$@ $< -MMD -MF$(@:%.o=%.d)

# Test targets
-include $(test_deps)

$(test_output): $(test_objs) $(lib_aoutput)
	@$(mkdir) $(dir $@)
	$(compiler) $(test_ldflags) -o$@ $(test_objs) $(lib_aoutput) $(test_libs)

$(output_dir)/$(test_dir)/%.o: $(test_dir)/%.cpp
	@$(mkdir) $(dir $@)
	$(compiler) -c $(test_flags) -o$@ $< -MMD -MF$(@:%.o=%.d)

# Extra
.PHONY: all shared static clean test
