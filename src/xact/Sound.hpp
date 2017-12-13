#ifndef XACT_SOUND_HPP
#define XACT_SOUND_HPP

#include <istream>

namespace xact
{
    class SoundBank;

    class Sound
    {
        private:
            friend class SoundBank;
            SoundBank& soundBank;

            int trackIndex;
            int wavebankIndex;

            Sound( SoundBank& theSoundBank );

            void readFromStream( std::istream& in );
    };
}

#endif // XACT_SOUND_HPP

