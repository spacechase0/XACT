#include "xact/WaveBank.hpp"

#include <cstdio>
#include <fstream>
#include <util/Logger.hpp>

#include "xact/PrivateUtil.hpp"

using namespace xact::priv;

namespace xact
{
    bool WaveBank::loadFromFile( const std::string& path )
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

    bool WaveBank::loadFromStream( std::istream& in )
    {
        in.exceptions( std::ifstream::failbit );
        try
        {
            read< sf::Uint32 >( in ); // ?

            int ver = read< sf::Int32 >( in );
            int lastSegment = 4;
            if ( ver <= 3 )
                lastSegment = 3;
            else if ( ver >= 42 )
                read< sf::Int32 >( in ); // header version?

            util::log( "V/LS: $/$\n", ver, lastSegment );

            struct Segment { int offset; int length; };
            std::vector< Segment > segments;
            for ( int i = 0; i <= lastSegment; ++i )
            {
                Segment s;
                s.offset = read< sf::Int32 >( in );
                s.length = read< sf::Int32 >( in );
                util::log("S$:$ $\n",i, s.offset, s.length);
                segments.push_back( s );
            }

            in.seekg( segments[ 0 ].offset, std::ios::beg );

            auto flags = read< sf::Int32 >( in );
            auto entryCount = read< sf::Int32 >( in );

            std::string name = "";
            if ( ver == 2 || ver == 3 )
                name = readString< 16 >( in );
            else
                name = readString< 64 >( in );

            int wavebankOffset = 0;
            int metaElemSize = 20;
            int nameElemSize = 0;
            int alignment = 0;
            if ( ver != 1 )
            {
                metaElemSize = read< sf::Int32 >( in );
                nameElemSize = read< sf::Int32 >( in );
                alignment = read< sf::Int32 >( in );
                wavebankOffset = segments[ 1 ].offset;
            }

            util::log("$, $ $ $ $\n",entryCount, wavebankOffset, metaElemSize, nameElemSize, alignment );

            if ( ( flags & 0x00020000 ) != 0 )
                throw std::runtime_error( "Compact not supported" );

            auto playOffset = segments[ lastSegment ].offset;
            if ( playOffset == 0 )
                playOffset = wavebankOffset + ( entryCount * metaElemSize );

            // Some stuff that doesn't seem to do anything

            sounds.reserve( entryCount );
            streams.reserve( entryCount );

            in.seekg( wavebankOffset, std::ios::beg );

            // compact
            // else
            {
                for ( std::size_t i = 0; i < entryCount; ++i )
                {
                    int format = 0, fileOffset = 0, fileLength = 0, loopStart = 0, loopLength = 0;
                    if (ver == 1)
                    {
                        format = read< sf::Int32 >( in );
                        fileOffset = read< sf::Int32 >( in );
                        fileLength = read< sf::Int32 >( in );
                        loopStart = read< sf::Int32 >( in );
                        loopLength = read< sf::Int32 >( in );
                    }
                    else
                    {
                        int flagsAndDuration = read< sf::Int32 >( in ); // Unused

                        if (metaElemSize >= 8)
                            format = read< sf::Int32 >( in );
                        if (metaElemSize >= 12)
                            fileOffset = read< sf::Int32 >( in );
                        if (metaElemSize >= 16)
                            fileLength = read< sf::Int32 >( in );
                        if (metaElemSize >= 20)
                            loopStart = read< sf::Int32 >( in );
                        if (metaElemSize >= 24)
                            loopLength = read< sf::Int32 >( in );
                    }

                    if (metaElemSize < 24)
                    {
                        if (fileLength != 0)
                            fileLength = segments[ lastSegment ].length;
                    }

                    streams.push_back( StreamInfo{ format, fileOffset, fileLength, loopStart, loopLength } );
                }
            }

            // Since we don't support streaming...
            {
                for ( int i = 0; i < entryCount/25; ++i )
                {
                    auto info = streams[ i ];
                    auto format = info.format;

                    in.seekg( info.fileOffset + playOffset, std::ios::beg );
                    std::string str;
                    str.resize( info.fileLength );
                    in.read( &str[ 0 ], info.fileLength );

                    int codec, channels, rate, alignment;
                    if (ver == 1)
                    {
                        codec = ((format) & ((1 << 1) - 1));
                        channels = (format >> (1)) & ((1 << 3) - 1);
                        rate = (format >> (1 + 3 + 1)) & ((1 << 18) - 1);
                        alignment = (format >> (1 + 3 + 1 + 18)) & ((1 << 8) - 1);
                    }
                    else
                    {
                        codec = ((format) & ((1 << 2) - 1));
                        channels = (format >> (2)) & ((1 << 3) - 1);
                        rate = (format >> (2 + 3)) & ((1 << 18) - 1);
                        alignment = (format >> (2 + 3 + 18)) & ((1 << 8) - 1);
                    }
                    if ( codec != 2 )
                        throw std::runtime_error( "Unsupported codec" );

                    // Hmm... I need to get the samples out from this...
                    int indexTable[16] = {
                      -1, -1, -1, -1, 2, 4, 6, 8,
                      -1, -1, -1, -1, 2, 4, 6, 8
                    };
                    int stepsizeTable[89] = {
                      7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
                      19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
                      50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
                      130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
                      337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
                      876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
                      2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
                      5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
                      15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
                    };

                    alignment = ( alignment + 16 ) * channels+12;//?

                    #define clamp(i,n,a) (std::min((a),std::max((i),(n))))

                    std::vector< sf::Int16 > samples;
                    if(i==0xb)util::log("testing:$ $ $\n",str.length(), alignment, in.tellg());
                    for ( int b = 0; b < str.length() / alignment; ++b )
                    {
                        int pred = * reinterpret_cast< sf::Int16* >( &str[ 0 ] );
                        int stepI = (int) str[2];
                        int step = stepsizeTable[ stepI ];
                        int diff = 0;
                        for ( int i = b * alignment + 12; i < ( b + 1 ) * alignment; ++i )
                        {
                            int nibble1 = str[ i ] & 0x0F;
                            int nibble2 = ( str[ i ] >> 4 ) & 0x0F;

                            ///////////////
                            diff=0;
                            if(nibble1&4)diff+=step;
                            if(nibble1&2)diff+=step>>1;
                            if(nibble1&1)diff+=step>>2;
                            diff+=step>>3;
                            if(nibble1&0x8)diff=-diff;

                            pred = clamp( -32768, pred + diff, 32767 );
                            samples.push_back(pred);

                            stepI = clamp( 0, stepI + indexTable[ (unsigned int) nibble1 ], 88 );
                            step = stepsizeTable[ stepI ];
                            ///////////////
                            diff=0;
                            if(nibble2&4)diff+=step;
                            if(nibble2&2)diff+=step>>1;
                            if(nibble2&1)diff+=step>>2;
                            diff+=step>>3;
                            if(nibble2&0x8)diff=-diff;

                            pred = clamp( -32768, pred + diff, 32767 );
                            samples.push_back(pred);

                            stepI = clamp( 0, stepI + indexTable[ (unsigned int) nibble2 ], 88 );
                            step = stepsizeTable[ stepI ];
                            //////////////
                        }
                    }

                    #undef clamp

                    sounds.push_back( Sound{ rate, channels, samples } );
                }
            }
        }
        catch ( std::exception& e )
        {
            util::log( "[ERROR] Exception reading: $\n", e.what() );
            return false;
        }

        return true;
    }
}
