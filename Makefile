# Disable echoing of commands
#MAKEFLAGS += --silent

# Optimize the code and show all warnings (except unused parameters)
BUILD_FLAGS := -O3 -Wall -Wextra -pedantic -Wno-unused-parameter

# Don't optimize, provide all warnings and build with clang's memory checks and support for GDB debugging
DEBUG_FLAGS :=-Wall -Wextra -pedantic -Wno-unused-parameter -fsanitize=address -fno-omit-frame-pointer -g

# Link towards the math library and thread library as well as libraries for TLS
LINKER_FLAGS :=-lm -L/usr/local/opt/openssl@1.1/lib -lssl -lcrypto

# Include generated and third-party code
INCLUDES := -Ibuild -Iincludes -I/usr/local/opt/openssl@1.1/include

# The name of the target binary
TARGET_NAME := "irc-watchlist-bot"
DEBUG_TARGET_NAME := "irc-watchlist-bot.debug"

# Source code
source := $(shell find src -type f -name "*.c" -not -path "src/resources/*") src/resources/resources.c
headers := $(shell find src -type f -name "*.h" -not -path "src/resources/*") src/resources/resources.h
objects := $(subst src,build,$(source:.c=.o))

# Resources as defined in their source form (be it html, toml etc.)
resources := $(shell find src/resources -type f -not -name "*.c" -not -name "*.h")
# Generated files for resources
resourceSources := $(subst src,build,$(resources:=.c))
resourceHeaders := $(subst src,build,$(resources:=.h))
resourceObjects := $(subst src,build,$(resources:=.o))

filesToFormat := $(source) $(headers)

.PHONY: build clean debug

# Build wsic, default action
build: build/$(TARGET_NAME)

# Build wsic with extra debugging enabled
debug: BUILD_FLAGS := $(DEBUG_FLAGS)
debug: TARGET_NAME := $(DEBUG_TARGET_NAME)
debug: CC = clang
debug: build/$(TARGET_NAME)

# Executable linking
build/$(TARGET_NAME): $(resourceObjects) $(objects)
	$(CC) $(INCLUDES) $(BUILD_FLAGS) -o build/$(TARGET_NAME) $(resourceObjects) $(objects) $(LINKER_FLAGS)

# Source compilation
$(objects): build/%.o: src/%.c src/%.h
	mkdir -p $(dir $@)
	$(CC) $(INCLUDES) $(BUILD_FLAGS) -c $< -o $@

# Turn resources into c files
$(resourceSources): build/%.c: src/%
	mkdir -p $(dir $@)

	echo '#include "$(addsuffix .h, $(basename $(notdir $@)))"' > $@
	$(eval resourceName := $(shell echo "$@" | sed -e 's/build\/resources\/data\///g' -e 's/.csv.c//' -e 's/[^0-9a-zA-Z]/_/g' | tr '[:lower:]' '[:upper:]'))
	echo "char *RESOURCES_$(resourceName)[] = {" >> $@
	sed -e 's/\(.*\)$$/  "&",/g' $< >> $@
	echo "  0" >> $@
	echo "};" >> $@

# Turn resources into h files
$(resourceHeaders): build/%.h: build/%.c
	mkdir -p $(dir $@)

	$(eval name := $(shell echo "$@" | sed 's/[^0-9a-zA-Z]//g'))
	$(eval resourceName := $(shell echo "$@" | sed -e 's/build\/resources\/data\///g' -e 's/.csv.h//' -e 's/[^0-9a-zA-Z]/_/g' | tr '[:lower:]' '[:upper:]'))
	echo "#ifndef $(name)\n#define $(name)" > $@
	echo "#include <stdint.h>" >> $@
	echo "extern char *RESOURCES_$(resourceName)[];" >> $@
	echo "#endif" >> $@

# Turn resources into objects
$(resourceObjects): build/%.o: build/%.c build/%.h
	$(CC) $(INCLUDES) $(BUILD_FLAGS) -c $< -o $@

# Create the compilation database for llvm tools
compile_commands.json: Makefile
	# compiledb is installed using: pip install compiledb
	compiledb -n make

# Format code according to .clang-format
format: compile_commands.json
	clang-format -i -style=file $(filesToFormat)

# Build and tag the docker image
docker: $(resources) $(source) $(headers)
	docker build -t axgn/irc-watchlist-bot

clean:
	rm -rf build/*
