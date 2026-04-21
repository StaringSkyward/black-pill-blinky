# Build + flash recipes for the STM32F401 blinky (Rust + Embassy).
#
# Prerequisite: probe-rs for flashing/debugging via ST-Link.
#   cargo install probe-rs-tools --locked
#
# The thumbv7em-none-eabihf target is auto-installed by rust-toolchain.toml.

chip := "STM32F401CEUx"
elf  := "target/thumbv7em-none-eabihf/release/blinky"

# default: build
# --unsorted so that we list them in the order they're in the file
default:
    just --list --unsorted --list-submodules

# Debug build (fast compile, useful when iterating on logic).
check:
    cargo check

# Release build — the one you actually want on the MCU.
build:
    cargo build --release

# Flash + run on the target, leaving probe-rs attached for RTT output.
# Equivalent to the old `make flash`, but driven by probe-rs over ST-Link —
# no BOOT0/NRST dance needed.
run:
    cargo run --release

# One-shot flash (no RTT attach).
flash: build
    probe-rs download --chip {{chip}} {{elf}}
    probe-rs reset --chip {{chip}}

# Wipe the chip's flash.
erase:
    probe-rs erase --chip {{chip}}

# Show text/data/bss sizes — same `arm-none-eabi-size` the C Makefile used.
size: build
    arm-none-eabi-size {{elf}}

clean:
    cargo clean
