#pragma once

#include <cstdint>

namespace wte
{

enum class digital_in : uint8_t
{
    in0,
    in1,
    in2,
    in3,
    io_in0,
    io_in1,
    io_in2,
    io_in3,
};

enum class digital_out : uint8_t
{
    out0,
    out1,
    out2,
    out3,
    out4,
    io_out0,
    io_out1,
    io_out2,
    io_out3,
};

enum class analog_in : uint8_t
{
    in0,
    in1
};

enum class analog_out : uint8_t
{
    out0,
    out1
};

/**
 * @brief Reads a digital input channel
 */
bool  read_digital_in  (digital_in  channel);

/**
 * @brief Writes a digital output channel
 */
void  write_digital_out(digital_out channel,bool state);

/**
 * @brief Read a digital output channel
 */
bool  read_digital_out(digital_out channel);

/**
 * @brief reads the adc analog channel
 *
 * @param channel identify the adc channel to read
 * @returns value between 0 to 100 in float format
 */
float read_analog_in   (analog_in channel);

/**
 * @brief writes the dac analog channel
 *
 * @param channel identify the dac channel to write
 * @param value to write in the range [0:100]
 */
void  write_analog_out (analog_out channel,float value);

/**
 * @brief reads the dac analog channel
 *
 * @param channel identify the dac channel to write
 * @returns value between 0 to 100 in float format
 */
float read_analog_out (analog_out channel);

}	//namespace wte
