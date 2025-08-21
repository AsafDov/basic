# 💡 ESP-IDF Console-Controlled LED Blinker

This project is a simple yet powerful demonstration of **real-time operating system (RTOS)** concepts using the **ESP-IDF framework** and a FreeRTOS-based application. It implements an interactive command-line interface (CLI) over a serial port, allowing a user to dynamically control the blink rate of an on-board LED.

The core of this application highlights **thread-safe communication** between two FreeRTOS tasks by using a **FreeRTOS queue**, which is a fundamental concept for reliable shared resource management in embedded systems.

***

### ✨ Features

* **Interactive Serial Console:** A robust and responsive command-line interface built with the ESP-IDF REPL (Read-Eval-Print Loop) component.
* **Dynamic Control:** Change the LED blink delay in real-time by entering a simple command.
* **FreeRTOS Multithreading:** Utilizes two separate FreeRTOS tasks (`blink_task` and the console's REPL task) that run concurrently.
* **Thread-Safe Communication:** A FreeRTOS queue is used to safely pass new delay values from the console task to the blink task, preventing race conditions.

***

### ⚙️ How It Works (Core Concepts)

This application is built around two primary FreeRTOS tasks and a queue that connects them.

#### The `blink_task`

This is a dedicated task responsible for blinking the LED. It enters an infinite loop, where it reads a new delay value from a FreeRTOS queue. If a new value is present, it updates its internal delay variable. If the queue is empty, it continues using the most recent value. After updating, it toggles the LED's state and uses `vTaskDelay()` to pause for the specified amount of time.

#### The `delay_command`

This function is registered as a custom command with the ESP-IDF console. When a user types `delay <value>` in the serial monitor, this function is executed. It parses the integer value from the command line, and then, crucially, it **sends this integer value to the queue** using `xQueueSend()`.

#### The FreeRTOS Queue

The queue (`queue_handle`) serves as the central point of **thread-safe communication**. It is initialized in `app_main()` and has a defined size and item size (`sizeof(int)`). It acts as a buffer that decouples the two tasks. The `delay_command` (on the console's task) writes to the queue, while the `blink_task` reads from it. This ensures that data is passed safely and predictably between the two tasks without the risk of one task overwriting data while the other is using it.

***

### 🛠️ Prerequisites

* **ESP-IDF v5.0 or later:** The official Espressif IoT Development Framework.
* **Supported ESP32 Board:** A development board with an on-board LED (GPIO 2 in this example).
* **Standard C/C++ Build Tools:** A common toolchain for embedded development.

***

### ⬇️ Building and Flashing

1.  Clone the repository:
    ```sh
    git clone https://github.com/AsafDov/ESP_IDF_Console_Controlled_LED_Blinker
    cd ESP_IDF_Console_Controlled_LED_Blinker
    ```
2.  Set the target chip:
    ```sh
    idf.py set-target esp32
    ```
3.  Build the project:
    ```sh
    idf.py build
    ```
4.  Flash the project to your board and monitor the serial output:
    ```sh
    idf.py -p <PORT> flash monitor
    ```
    (Replace `<PORT>` with your board's serial port, e.g., `/dev/ttyUSB0` or `COM3`).

***

### ⌨️ Usage

Once the application is running, open the serial monitor and you will see the console prompt.

* To set the LED blink delay, type:
    ```sh
    delay <milliseconds>
    ```
    For example, to set the delay to 500 milliseconds (0.5 seconds), enter:
    ```sh
    delay 500
    ```
* To see a list of available commands, type:
    ```sh
    help
    ```
