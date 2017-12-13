#include <iostream>
#include <fstream>

#include <SFML/Audio.hpp>
#include <util/Logger.hpp>

#include "xact/SoundBank.hpp"
#include "xact/WaveBank.hpp"

int main()
{
    util::Logger::setName( "Main Log", "log.txt" );

    xact::SoundBank sounds;
    if ( !sounds.loadFromFile( "C:\\Users\\Chase\\sdv\\workspace\\Content\\XACT\\Sound Bank.xsb" ) )
    {
        std::cout << "Failed to load sound bank!\n";
        return 1;
    }

    xact::WaveBank waves;
    if ( !waves.loadFromFile( "C:\\Users\\Chase\\sdv\\workspace\\Content\\XACT\\Wave Bank.xwb" ) )
    {
        std::cout << "Failed to load wave bank!\n";
        return 1;
    }

    //*
    int i = 0;
    for ( auto sound : waves.sounds )
    {if(i++!=0x0b)continue;
        sf::SoundBuffer sb;
        sb.loadFromFile("C:\\Users\\Chase\\Downloads\\WaveBankExtractPackV2\\New\\0000000b.wav");
        auto samples = sb.getSamples();
        int sc = sb.getSampleCount();
        auto s2=&sound.samples[0];
        std::cout << sc << ' ' << sound.samples.size() << std::endl;

        sb.loadFromSamples( &sound.samples[ 0 ], sound.samples.size(), sound.channels, sound.sampleRate );
        sf::Sound snd;
        snd.setBuffer( sb );
        snd.play();
        while ( snd.getStatus() == sf::Sound::Playing)
            sf::sleep(sf::seconds(0.05f));
        sb.saveToFile("test.wav");
    }
}
