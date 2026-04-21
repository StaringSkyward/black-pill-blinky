/* STM32F401CEU6 memory regions.
 * Matches the C build's stm32f401.ld. cortex-m-rt's link.x script reads this
 * and places the vector table, .text, .rodata, .data, and .bss for us — we
 * don't need to spell out the section layout like the C linker script did. */
MEMORY
{
  FLASH : ORIGIN = 0x08000000, LENGTH = 512K
  RAM   : ORIGIN = 0x20000000, LENGTH = 96K
}
