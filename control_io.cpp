#include "control_io.h"
#include <cstdlib>

namespace wte
{
static float _out_an0;
static float _out_an1;
//static float _in_an0;
//static float _in_an1;
static union
{
    struct
    {
        uint8_t in0 : 1;
        uint8_t in1 : 1;
        uint8_t in2 : 1;
        uint8_t in3 : 1;
        uint8_t rsv : 4;
    };
    uint8_t raw;
}_digital_in;
static union
{
    struct
    {
        uint8_t io_in0 : 1;
        uint8_t io_in1 : 1;
        uint8_t io_in2 : 1;
        uint8_t io_in3 : 1;
        uint8_t rsv    : 4;
    };
    uint8_t raw;
}_digital_io_in;
static union
{
    struct
    {
        uint8_t out0 : 1;
        uint8_t out1 : 1;
        uint8_t out2 : 1;
        uint8_t out3 : 1;
        uint8_t out4 : 1;
        uint8_t rsv  : 3;
    };
    uint8_t raw;
}_digital_out;
static union
{
    struct
    {
        uint8_t io_out0 : 1;
        uint8_t io_out1 : 1;
        uint8_t io_out2 : 1;
        uint8_t io_out3 : 1;
        uint8_t rsv     : 4;
    };
    uint8_t raw;
}_digital_io_out;

bool read_digital_in(digital_in channel)
{
    return rand()%1000 > 500;
/*    _digital_in.raw = rand()%255;
    _digital_io_in.raw = rand()%255;
    switch( channel )
    {
    case digital_in::in0:
        return _digital_in.in0;
    case digital_in::in1:
        return _digital_in.in1;
    case digital_in::in2:
        return _digital_in.in2;
    case digital_in::in3:
        return _digital_in.in3;
    case digital_in::io_in0:
        return _digital_io_in.io_in0;
    case digital_in::io_in1:
        return _digital_io_in.io_in1;
    case digital_in::io_in2:
        return _digital_io_in.io_in2;
    case digital_in::io_in3:
        return _digital_io_in.io_in3;
    }*/
}
void write_digital_out(digital_out channel,bool state)
{
    switch( channel )
    {
    case digital_out::out0:
        _digital_out.out0 = state;
        break;
    case digital_out::out1:
        _digital_out.out1 = state;
        break;
    case digital_out::out2:
        _digital_out.out2 = state;
        break;
    case digital_out::out3:
        _digital_out.out3 = state;
        break;
    case digital_out::out4:
        _digital_out.out4 = state;
        break;
    case digital_out::io_out0:
        _digital_io_out.io_out0 = state;
        break;
    case digital_out::io_out1:
        _digital_io_out.io_out1 = state;
        break;
    case digital_out::io_out2:
        _digital_io_out.io_out2 = state;
        break;
    case digital_out::io_out3:
        _digital_io_out.io_out3 = state;
        break;
    }
}
bool read_digital_out(digital_out channel)
{
    switch( channel )
    {
    case digital_out::out0:
        return _digital_out.out0;
    case digital_out::out1:
        return _digital_out.out1;
    case digital_out::out2:
        return _digital_out.out2;
    case digital_out::out3:
        return _digital_out.out3;
    case digital_out::out4:
        return _digital_out.out4;
    case digital_out::io_out0:
        return _digital_io_out.io_out0;
    case digital_out::io_out1:
        return _digital_io_out.io_out1;
    case digital_out::io_out2:
        return _digital_io_out.io_out2;
    case digital_out::io_out3:
        return _digital_io_out.io_out3;
    }
}
float read_analog_in(analog_in channel)
{
    return float(rand()%9999)/100.0f;
}
void write_analog_out(analog_out channel,float value)
{
    if( channel == analog_out::out0 )
        _out_an0 = value;
    if( channel == analog_out::out1 )
        _out_an1 = value;
}
float read_analog_out (analog_out channel)
{
    if( channel == analog_out::out0 )
        return _out_an0;
    return _out_an1;
}
}//namespace wte
