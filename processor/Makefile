# PROGRAM CONFIG
BUILD_DIR := build
SRC_DIR := src
TEST_DIR := test
INCLUDE_DIRS := include
LOG_DIR := log
EXECUTABLE := processor.out

-include $(SRC_DIR)/sources.make
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(SOURCES)))
DEPS := $(patsubst %.o,%.d,$(OBJS))

# LIBRARIES
LIBCUTILS_INCLUDE_PATH := /home/n1njaeng1neer/source/cutils/include
LIBCUTILS              := /home/n1njaeng1neer/source/cutils/build/src/libcutils.a

# COMPILER CONFIG
CC := g++
CPPFLAGS := -MMD -MP $(addprefix -I,$(INCLUDE_DIRS)) $(addprefix -I,$(LIBCUTILS_INCLUDE_PATH)) -D _DEBUG -ggdb3 -std=c++17 -O0 -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations -Wc++14-compat -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts -Wconditionally-supported -Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op -Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods -Wsuggest-final-types -Wsuggest-override -Wswitch-default -Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code -Wunused -Wuseless-cast -Wvariadic-macros -Wno-literal-suffix -Wno-missing-field-initializers -Wno-narrowing -Wno-old-style-cast -Wno-varargs -Wstack-protector -fcheck-new -fsized-deallocation -fstack-protector -fstrict-overflow -flto-odr-type-merging -fno-omit-frame-pointer -Wlarger-than=8192 -Wstack-usage=8192 -pie -fPIE -Werror=vla -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr

# PROGRAM
$(BUILD_DIR)/$(EXECUTABLE): $(OBJS)
	@echo -n Linking $@...
	@$(CC) $(CPPFLAGS) $(OBJS) $(LIBCUTILS) -o $@
	@echo done

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo Building $@...
	@mkdir -p $(BUILD_DIR)
	@$(CC) $(CPPFLAGS) -c $< -o $@

.PHONY: run
run: $(BUILD_DIR)/$(EXECUTABLE)
	@./$(BUILD_DIR)/$(EXECUTABLE)

-include $(TEST_DIR)/test_sources.make
TEST_EXECS := $(patsubst %.c,$(BUILD_DIR)/%.test,$(TEST_SOURCES))

# TESTS
prefix := "\#\#\#\#\#\# [TEST]"
test: $(TEST_EXECS)
	@$(foreach exec,$(TEST_EXECS),echo "$(prefix) Running $(notdir $(exec))..."; ./$(exec); echo -e "$(prefix) Finished $(notdir $(exec))\n";)

$(BUILD_DIR)/%.test: $(TEST_DIR)/%.c $(OBJS)
	@echo -n Building test $@...
	@$(CC) $(CPPFLAGS) $(OBJS) -o $@ $<
	@echo done

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(LOG_DIR)

-include $(DEPS)
