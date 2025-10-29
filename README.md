This is a fork of [matthijskooijman/arduino-dsmr](https://github.com/matthijskooijman/arduino-dsmr).
The primary goal is to make the parser independent of the Arduino framework and usable on ESP32 with the ESP-IDF framework or any other platform.

# Features
* Combines all fixes from [matthijskooijman/arduino-dsmr](https://github.com/matthijskooijman/arduino-dsmr) and [glmnet/arduino-dsmr](https://github.com/glmnet/arduino-dsmr) including unmerged pull requests.
* Added an extensive unit test suite
* Small refactoring and code optimizations
* Supported compilers: MSVC, GCC, Clang
* Header-only library, no dependencies
* Code can be used on any platform, not only embedded.

# Differences from the original arduino-dsmr
* Requires a C++20 compatible compiler.
* [P1Reader](https://github.com/matthijskooijman/arduino-dsmr/blob/master/src/dsmr/reader.h) class is replaced with the [PacketAccumulator](https://github.com/esphome-libs/arduino-dsmr/blob/master/src/dsmr/packet_accumulator.h) class with a different interface to allow usage on any platform.
* Added [EncryptedPacketAccumulator](https://github.com/esphome-libs/arduino-dsmr/blob/master/src/dsmr_parser/encrypted_packet_accumulator.h) class to receive encrypted DSMR messages (like from "Luxembourg Smarty").

# How to use
## General usage
The library is header-only. Add the `src/dsmr_parser` folder to your project.<br>
Note: `encrypted_packet_accumulator.h` header depends on [Mbed TLS](https://www.trustedfirmware.org/projects/mbed-tls/) library. It is already included in the `ESP-IDF` framework and can be easily added to any other platforms.

## Usage from PlatformIO
The library is available on the PlatformIO registry:<br>
**TODO: configure deployment to PlatformIO **

# Examples
* How to use the parser
  * [minimal_parse.ino](https://github.com/matthijskooijman/arduino-dsmr/blob/master/examples/minimal_parse/minimal_parse.ino)
  * [parse.ino](https://github.com/matthijskooijman/arduino-dsmr/blob/master/examples/parse/parse.ino)
* Complete example using PacketAccumulator
  * [packet_accumulator_example_test.cpp](https://github.com/esphome-libs/arduino-dsmr/blob/master/src/test/packet_accumulator_example_test.cpp)
* Example using EncryptedPacketAccumulator
  * [encrypted_packet_accumulator_example_test.cpp](https://github.com/esphome-libs/arduino-dsmr/blob/master/src/test/encrypted_packet_accumulator_example_test.cpp)

# History behind arduino-dsmr
[matthijskooijman](https://github.com/matthijskooijman) is the original creator of this DSMR parser.
[glmnet](https://github.com/glmnet) and [zuidwijk](https://github.com/zuidwijk) continued work on this parser in the fork [glmnet/arduino-dsmr](https://github.com/glmnet/arduino-dsmr). They used the parser to create [ESPHome DSMR](https://esphome.io/components/sensor/dsmr/) component.
After that, the work on the `arduino-dsmr` parser stopped.
Since then, some issues and unmerged pull requests have accumulated. Additionally, the dependency on the Arduino framework causes various issues for some ESP32 boards.
This fork addresses the existing issues and makes the parser usable on any platform.

## The reasons `dsmr_parser` fork was created
* Dependency on the Arduino framework limits the applicability of this parser. For example, it is not possible to use it on Linux.
* The Arduino framework on ESP32 inflates the FW size and doesn't allow usage of the latest version of ESP-IDF.
* Many pull requests and bug fixes needed to be integrated into the parser.
* Lack of support for encrypted DSMR messages.

# How to work with the code
* You can open the code using any IDE that supports CMake.
  * `build-windows.ps1` script needs `Visual Studio 2022` to be installed.
  * `build-linux.sh` script needs `clang` to be installed.

# References
* [DSMR parser in Python](https://github.com/ndokter/dsmr_parser/tree/master) - alternative DSMR parser implementation in Python.
* [SmartyReader](https://www.weigu.lu/microcontroller/smartyReader_P1/index.html) - open source hardware to communicate with P1 port.
* [SmartyReader. Chapter "Encryption"](https://www.weigu.lu/microcontroller/smartyReader/index.html) - how the encrypted DSMR protocol works.
