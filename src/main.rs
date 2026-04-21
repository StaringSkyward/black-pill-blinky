//! Blinks the STATUS LED (PC13) on a Black Pill (STM32F401CEU6).
//!
//! Port of the CubeMX-generated C blinky to Embassy. Functionally identical:
//! toggle PC13 every 333ms. Uses the HSI by default — matches the C version's
//! SystemClock_Config which also selected HSI with PLL off.

#![no_std]
#![no_main]

use embassy_executor::Spawner;
use embassy_stm32::gpio::{Level, Output, Speed};
use embassy_time::Timer;
use panic_halt as _;

#[embassy_executor::main]
async fn main(_spawner: Spawner) {
    let p = embassy_stm32::init(Default::default());
    let mut led = Output::new(p.PC13, Level::High, Speed::Low);

    loop {
        led.toggle();
        Timer::after_millis(333).await;
    }
}
