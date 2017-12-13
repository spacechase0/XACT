#ifndef XACT_WAVEBANK_HPP
#define XACT_WAVEBANK_HPP

#include <istream>
#include <SFML/Config.hpp>
#include <string>
#include <vector>

namespace xact
{
    class WaveBank
    {
        public:
            struct StreamInfo
            {
                int format;
                int fileOffset;
                int fileLength;
                int loopStart;
                int loopLength;
            };

            bool loadFromFile( const std::string& path );
            bool loadFromStream( std::istream& in );

        //private:
            struct Sound
            {
                int sampleRate;
                int channels;
                std::vector< sf::Int16 > samples;
            };

            std::vector<Sound> sounds;
            std::vector<StreamInfo> streams;
    };
}

#endif // XACT_WAVEBANK_HPP
