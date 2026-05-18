# VibeCoach: TinyML-Powered Wearable Gym Form Detector

VibeCoach is an intelligent Edge AI wearable device designed to act as a real-time personal training assistant. It detects lifting form execution errors with 100% offline, on-device neural network inference, providing instantaneous corrective feedback via a haptic vibration motor and an interactive visual interface.

The project is developed using C/C++ within the STM32CubeIDE ecosystem, integrating a trained deep learning model deployed through the Edge Impulse SDK via CMSIS-Pack components.

---

## 🛠️ Key Technical Features

* **Adaptive Orientation Alignment:** Computes a quaternion inverse (invQ) algorithm to dynamically align the MPU-9250 IMU orientation with the user's initial setup posture. This removes the rigid requirement for precise mechanical mounting positions on the user's body.
* **Non-Blocking Score Accumulator:** An intelligent signal conditioning filter that converts AI classification confidence probabilities into an accumulated penalty score. This acts as a robust software debouncer to eliminate false alarms triggered by transient motion artifacts or short-lived inference anomalies.
* **Asynchronous Haptic State Machine:** Manages vibration pulse frequencies and durations using millisecond tick evaluations (`HAL_GetTick()`) rather than blocking delay loops. This asynchronous layout guarantees that background sensor sampling and inference cycles remain entirely unimpeded every 10 ms.
* **Low-Overhead Power Telemetry:** Monitors LiPo battery levels via a hardware voltage divider network, smoothed using an Exponential Moving Average (EMA) filter and a dual-layer hysteresis logic to display steady, bounce-free capacity graphics on the LCD panel.

---

## 🔌 Hardware Architecture & Pin Mapping

The system utilizes a modular hardware topology centered around the **STM32F411CEU6** microcontroller (clocked at 50 MHz via an external crystal oscillator), mapped with the following peripheral assignments:

* **Kinematic Ingestion:** MPU-9250 IMU -> Connected via serial bus (I2C/SPI) with continuous raw telemetry acquired at a stable 78 Hz sampling rate.
* **Visual Interface:** ST7735 LCD & Onboard RGB LED -> Renders menu frames, real-time battery readouts, and state indicators (Solid Green = Perfect Form, Blue = Early Discrepancy Alert, Red = Critical Form Error).
* **Haptic Actuation:** Coreless Vibration Motor -> Driven via Pulse Width Modulation (PWM) on pin PA8 (TIM1 Channel 1) with an Auto-Reload Register (ARR) configuration of 999.
* **Power & Charging:** 3.7V 250mAh LiPo Battery + TP4056 USB Type-C Charging Module -> Battery potential is securely monitored via analog pin PA0 (ADC1 Channel 0).
* **Navigation Hardware:** Blue Button Bar (4-Button Array) -> Tied to digital GPIO Input pins utilizing internal resistor pull configurations for system menu switching.

---

## 🧠 TinyML Model Specification

The embedded neural network classification pipeline is optimized specifically for constrained ARM Cortex-M4 hardware architectures:

* **Signal Processing (DSP Block):** Spectral Analysis framework that extracts frequency and power characteristics from the 4-axis quaternion time-series data, filtering out low-frequency static noise.
* **Neural Network Architecture (Learning Block):** A multi-layer Perceptron (MLP) configuration featuring dense fully-connected layers, optimized with dropout regularization to prevent overfitting during small-dataset gym form training.
* **Classification Outputs:** The model evaluates incoming kinematic frames and classifies them into distinct operational states: `idle`, `correct_move`, or `wrong_move`.
* **Quantization & Footprint:** Fully quantized to INT8 precision using TensorFlow Lite Micro. This optimization path achieves a **90.1% validation accuracy** while maintaining an incredibly low SRAM and Flash memory overhead on the STM32 chip.

---

## ⚙️ Software & Logic Breakdown

### 1. Score Accumulator Execution
The Edge Impulse model continuously processes windows of data. When the classifier returns a `wrong_move` label with a confidence threshold equal to or greater than 70 percent, the system escalates the `s_wrong_score` variable by 2 points. Conversely, flawless execution (`correct_move` or an intentional transition to `idle`) forces a decay step of minus 1 point down to a baseline floor of 0.

Physical PWM intensities map directly to these accumulated penalty tiers:
* **Score 4 to 7:** Low PWM Duty Cycle (1 short pulse) -> Acts as a gentle reminder to adjust form.
* **Score 8 to 11:** Medium PWM Duty Cycle (2 pulses) -> Alerts that form error is compounding (guarded by a 1.5-second cooldown).
* **Score 12 or higher:** Strong PWM Duty Cycle (3 continuous bursts) -> Urgent signal to halt execution due to injury risk (guarded by a strict 3-second absolute cooldown).

### 2. Anti-Flicker LCD Power Management
Acquiring a noise-reduced ADC average across 32 successive samples requires roughly a 160 ms execution window. Running this synchronously inside a rendering sequence stalls the SPI transmission bus, which drops display refresh rates and creates massive visual display flickering.

VibeCoach resolves this by introducing a Cache Pre-Warming routine via `bsp_battery_force_init()` inside the initialization stage before entering the main loop. The render pipeline subsequent queries this local cache instantly in a non-blocking fashion. The background cache engine re-reads the hardware status asynchronously every 5000 ms using an EMA filter (alpha value 0.15) to maintain accurate metrics despite high-current voltage ripples generated by the haptic motor.

---

## 🚀 How to Open and Build

1. Clone this repository to your local directory setup.
2. Launch **STM32CubeIDE** (Latest stable version recommended).
3. Navigate to *File* -> *Import* -> *Existing Projects into Workspace*, and select the root directory of this cloned repository.
4. Ensure the *Edge Impulse CMSIS-Pack* components are properly registered within your local IDE paths to link signal processing structures.
5. Execute the *Build Project* command. Since the entry configuration utilizes `main.cpp`, verify that your compiler toolchain routes via the G++ compiler.
6. Connect an ST-Link hardware debugger to your target STM32F411 development platform, and run the *Flash/Debug* routine.
