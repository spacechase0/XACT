#include "xact/SoundBank.hpp"

#include <fstream>
#include <util/Logger.hpp>
#include <vector>

#include "xact/PrivateUtil.hpp"
#include "xact/Sound.hpp"

using namespace xact::priv;

namespace xact
{
    bool SoundBank::loadFromFile( const std::string& path )
    {
        std::ifstream file( path, std::ifstream::in | std::ifstream::binary );
        if ( !file )
        {
            util::log( "[ERROR] $: Failed to open file.\n", path );
            return false;
        }

        util::log( "[INFO] Reading $...\n", path );
        return loadFromStream( file );
    }

    bool SoundBank::loadFromStream( std::istream& in )
    {
        in.exceptions( std::ifstream::failbit );
        try
        {
            if ( in.get() != 'S' || in.get() != 'D' || in.get() != 'B' || in.get() != 'K' )
            {
                util::log( "[ERROR] File is not an XSB file.\n" );
                return false;
            }

            auto toolVer = read< sf::Uint16 >( in );
            auto formatVer = read< sf::Uint16 >( in );
            if ( formatVer != 43 )
            {
                util::log( "[ERROR] SoundBank version number $ not supported.\n", formatVer );
                return false;
            }

            auto crc = read< sf::Uint16 >( in );
            auto lastModLow = read< sf::Uint32 >( in );
            auto lastModHigh = read< sf::Uint32 >( in );
            auto platform = read< sf::Uint8 >( in );

            auto simpleCueCount = read< sf::Uint16 >( in );
            auto complexCueCount = read< sf::Uint16 >( in );
            auto unknown1 = read< sf::Uint16 >( in );
            auto totalCueCount = read< sf::Uint16 >( in );
            auto totalWavebanks = read< sf::Uint8 >( in );
            auto totalSoundCount = read< sf::Uint16 >( in );
            auto cueNamesLength = read< sf::Uint16 >( in );
            auto unknown2 = read< sf::Uint16 >( in );

            auto simpleCueOffset = read< sf::Uint32 >( in );
            auto complexCueOffset = read< sf::Uint32 >( in );
            auto cueNameOffset = read< sf::Uint32 >( in );
            auto unknown3 = read< sf::Uint32 >( in );
            auto variationsOffset = read< sf::Uint32 >( in );
            auto unknown4 = read< sf::Uint32 >( in );
            auto wavebankNameOffset = read< sf::Uint32 >( in );
            auto cueNameHashOffset1 = read< sf::Uint32 >( in );
            auto cueNameHashOffset2 = read< sf::Uint32 >( in );
            auto soundOffset = read< sf::Uint32 >( in );

            std::string name = readString< 64 >( in );

            in.seekg( wavebankNameOffset, std::istream::beg );
            std::vector< std::string > wavebankNames;
            wavebankNames.reserve( totalWavebanks );
            for ( std::size_t i = 0; i < totalWavebanks; ++i )
                wavebankNames.push_back( readString< 64 >( in ) );

            in.seekg( cueNameOffset, std::istream::beg );
            std::string cueNamesStr( cueNamesLength, '\0' );
            in.read( &cueNamesStr[ 0 ], cueNamesLength );
            std::vector< std::string > cueNames = util::tokenize( cueNamesStr, std::string( 1, '\0' ) );

            if ( simpleCueCount > 0 )
            {
                in.seekg( simpleCueOffset, std::istream::beg );
                for ( std::size_t i = 0; i < simpleCueCount; ++i )
                {
                    auto flags = read< sf::Uint8 >( in );
                    auto offset = read< sf::Uint32 >( in );

                    auto oldPos = in.tellg();
                    in.seekg( offset, std::istream::beg );

                    sounds[ cueNames[ i ] ].push_back( std::unique_ptr< Sound >( new Sound( * this ) ) );
                    sounds[ cueNames[ i ] ][ 0 ]->readFromStream( in );
                    probabilities[ cueNames[ i ] ] = std::vector< float >( 1, 1 );

                    in.seekg( oldPos, std::istream::beg );

                }
            }

            if ( complexCueCount > 0 )
            {
                in.seekg( complexCueOffset, std::istream::beg );
                for ( std::size_t i = 0; i < complexCueCount; ++i )
                {
                    auto flags = read< sf::Uint8 >( in );
                    if ( flags & 0x4 )
                    {
                        auto offset = read< sf::Uint32 >( in );
                        auto unknown5 = read< sf::Uint32 >( in );

                        auto oldPos = in.tellg();
                        in.seekg( offset, std::istream::beg );

                        sounds[ cueNames[ simpleCueCount + i ] ].push_back( std::unique_ptr< Sound >( new Sound( * this ) ) );
                        sounds[ cueNames[ simpleCueCount + i ] ][ 0 ]->readFromStream( in );
                        probabilities[ cueNames[ simpleCueCount + i ] ] = std::vector< float >( 1, 1 );

                        in.seekg( oldPos, std::istream::beg );
                    }
                    else
                    {
                        auto varTableOffset = read< sf::Uint32 >( in );
                        auto transTableOffset = read< sf::Uint32 >( in );

                        auto oldPos = in.tellg();
                        in.seekg( varTableOffset, std::istream::beg );

                        auto entryCount = read< sf::Uint16 >( in );
                        auto varFlags = read< sf::Uint16 >( in );
                        auto unknown5 = read< sf::Uint8 >( in );
                        auto unknown6 = read< sf::Uint16 >( in );
                        auto unknown7 = read< sf::Uint8 >( in );

                        std::vector< std::unique_ptr< Sound > > sounds;
                        sounds.reserve( entryCount );
                        std::vector< float > probabilities;
                        probabilities.reserve( entryCount );

                        auto tableType = ( varFlags >> 4 ) & 0x7;
                        for ( std::size_t j = 0; j < entryCount; ++j )
                        {
                            switch ( tableType )
                            {
                                case 0:
                                case 4:
                                    {
                                        int trackIndex = read< sf::Uint16 >( in );
                                        int wavebankIndex = read< sf::Uint8 >( in );
                                        if ( tableType == 0 )
                                        {
                                            auto unknown8 = read< sf::Uint8 >( in );
                                            auto unknown9 = read< sf::Uint8 >( in );
                                        }

                                        sounds.emplace_back( std::unique_ptr< Sound >( new Sound( * this ) ) );
                                        sounds[ sounds.size() - 1 ]->trackIndex = trackIndex;
                                        sounds[ sounds.size() - 1 ]->wavebankIndex = wavebankIndex;
                                    }
                                    break;
                                case 1:
                                case 3:
                                    throw std::runtime_error( util::format( "Complex table type $ not implemented", tableType ) );
                                default:
                                    throw std::runtime_error( "Unknown complex table type" );
                            }
                        }

                        in.seekg( oldPos, std::istream::beg );

                        this->sounds[ cueNames[ simpleCueCount + i ] ] = std::move( sounds );
                        this->probabilities[ cueNames[ simpleCueCount + i ] ] = std::move( probabilities );
                    }

                    auto instanceLimit = read< sf::Uint8 >( in );
                    auto fadeIn = read< sf::Uint16 >( in ) / 1000.f;
                    auto fadeOut = read< sf::Uint16 >( in ) / 1000.f;
                    auto instanceFlags = read< sf::Uint8 >( in );
                }
            }

            return true;
        }
        catch ( std::exception& e )
        {
            util::log( "[ERROR] Exception reading: $\n", e.what() );
            return false;
        }
    }
}
