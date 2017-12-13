#ifndef XACT_SOUNDBANK_HPP
#define XACT_SOUNDBANK_HPP

#include <istream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "xact/Sound.hpp"

namespace xact
{
    class SoundBank
    {
        public:
            bool loadFromFile( const std::string& path );
            bool loadFromStream( std::istream& in );

        private:
            std::map< std::string, std::vector< std::unique_ptr< Sound > > > sounds;
            std::map< std::string, std::vector< float > > probabilities;
    };
}

#endif // XACT_SOUNDBANK_HPP
