#ifndef PRIVATEUTIL_HPP
#define PRIVATEUTIL_HPP

#include <array>
#include <istream>
#include <string>

namespace xact
{
    namespace priv
    {
        template< typename T >
        T read( std::istream& in )
        {
            T t;
            in.read( reinterpret_cast< char* >( &t ), sizeof( T ) );
            return t;
        }

        template<>
        std::string read< std::string >( std::istream& in );

        template< int N >
        std::string readString( std::istream& in )
        {
            std::array< char, N > str;
            in.read( &str[ 0 ], N );
            return &str[ 0 ];
        }

        template< typename T >
        bool flag( T t, int f )
        {
            return ( t & f ) != 0;
        }

        std::string ConvertMsAdpcmToPcm(const std::string& buffer, int offset, int count, int channels, int blockAlignment);
    }
}

#endif // PRIVATEUTIL_HPP
