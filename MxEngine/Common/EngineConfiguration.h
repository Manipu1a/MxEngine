#pragma once
#include <cstdint>
#include <ostream>

namespace MxEngine
{
    struct EngineConfiguration
    {
        EngineConfiguration(uint32_t r = 8, uint32_t g = 8, uint32_t b = 8,
            uint32_t a = 8, uint32_t d = 24, uint32_t s = 0, uint32_t msaa = 0,
            uint32_t width = 1920, uint32_t height = 1080) :
            redBits(r), greenBits(g), blueBits(b), alphaBits(a), depthBits(d),
        stencilBits(s), msaaSamples(msaa), screenWidth(width), screenHeight(height)
        {}
        
        uint32_t redBits;
        uint32_t greenBits;
        uint32_t blueBits;
        uint32_t alphaBits;
        uint32_t depthBits;
        uint32_t stencilBits;
        uint32_t msaaSamples;
        uint32_t screenWidth;
        uint32_t screenHeight;


        std::ostream& operator<<(std::ostream& out, const EngineConfiguration& conf)
        {
            out << "EngineConfiguration:" <<
                "R:" << conf.redBits <<
                    "G:" << conf.greenBits <<
                        "B:" << conf.blueBits <<
                            "A:" << conf.alphaBits <<
                                "D:" << conf.depthBits <<
                                    "S:" << conf.stencilBits <<
                                        "M:" << conf.msaaSamples <<
                                            "W:" << conf.screenWidth <<
                                                "H" << conf.screenHeight << std::endl;
            return out;
        }
        
    };

    
}
