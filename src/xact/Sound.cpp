#include "xact/Sound.hpp"

#include <cmath>
#include <SFML/Config.hpp>
#include <vector>

#include "util/Logger.hpp"
#include "xact/PrivateUtil.hpp"

using namespace xact::priv;


// From the MonoGame source code:
namespace
{
    float ParseVolumeFromDecibels(float decibles)
    {
        // Convert from decibles to linear volume.
        return (float)std::pow(10.0, decibles / 20.0);
    }

    float ParseVolumeFromDecibels(sf::Uint8 decibles)
    {
        //lazy 4-param fitting:
        //0xff 6.0
        //0xca 2.0
        //0xbf 1.0
        //0xb4 0.0
        //0x8f -4.0
        //0x5a -12.0
        //0x14 -38.0
        //0x00 -96.0
        const double a = -96.0;
        const double b = 0.432254984608615;
        const double c = 80.1748600297963;
        const double d = 67.7385212334047;
        auto dB = (float)(((a - d) / (1 + (std::pow(decibles / c, b)))) + d);

        return ParseVolumeFromDecibels(dB);
    }
}


namespace xact
{
    Sound::Sound( SoundBank& theSoundBank )
    :   soundBank( theSoundBank )
    {
    }

    void Sound::readFromStream( std::istream& in )
    {
        auto flags = read< sf::Uint8 >( in );
        bool complex = flag( flags, 0x01 );
        bool rpcs = flag( flags, 0x0E );
        bool dsps = flag( flags, 0x10 );

        auto category = read< sf::Uint16 >( in );
        auto volume = ParseVolumeFromDecibels( read< sf::Uint8 >( in ) );
        auto pitch = read< sf::Int16 >( in ) / 1000.f;
        auto priority = read< sf::Uint8 >( in );
        auto filter = read< sf::Uint16 >( in );

        int clipCount = 0, trackIndex = 0, wavebankIndex = 0;
        if ( complex )
        {
            clipCount = read< sf::Uint8 >( in );
        }
        else
        {
            trackIndex = read< sf::Uint16 >( in );
            wavebankIndex = read< sf::Uint8 >( in );
        }

        std::vector< int > rpcCurves;
        if ( rpcs )
        {
            auto currPos = in.tellg();

            auto dataLen = read< sf::Uint16 >( in );

            auto presetCount = read< sf::Uint8 >( in );
            for ( std::size_t i = 0; i < presetCount; ++i )
            {
                rpcCurves.push_back( -1 );//getrpcindex( read< sf::Uint32 >( in ) ) );
                util::log("TODO");
            }

            in.seekg( currPos + dataLen, std::ios::beg );
        }

        bool reverb = false;
        if ( dsps )
        {
            reverb = true;
            in.seekg( 7, std::ios::cur );
        }

        this->trackIndex = trackIndex;
        this->wavebankIndex = wavebankIndex;
    }
}
