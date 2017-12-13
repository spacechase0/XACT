#include "xact/PrivateUtil.hpp"

#include <iostream>

namespace
{
    // Code here was taken from MonoGame
    struct MsAdpcmState
    {
        int delta;
        int sample1;
        int sample2;
        int coeff1;
        int coeff2;
    };

    int adaptationTable[] =
    {
        230, 230, 230, 230, 307, 409, 512, 614,
        768, 614, 512, 409, 307, 230, 230, 230
    };

    int adaptationCoeff1[] =
    {
        256, 512, 0, 192, 240, 460, 392
    };

    int adaptationCoeff2[] =
    {
        0, -256, 0, 64, 0, -208, -232
    };

    int AdpcmMsExpandNibble(MsAdpcmState& channel, int nibble)
    {
        int nibbleSign = nibble - (((nibble & 0x08) != 0) ? 0x10 : 0);
        int predictor = ((channel.sample1 * channel.coeff1) + (channel.sample2 * channel.coeff2)) / 256 + (nibbleSign * channel.delta);

        if (predictor < -32768)
            predictor = -32768;
        else if (predictor > 32767)
            predictor = 32767;

        channel.sample2 = channel.sample1;
        channel.sample1 = predictor;

        channel.delta = (adaptationTable[nibble] * channel.delta) / 256;
        if (channel.delta < 16)
            channel.delta = 16;

        return predictor;
    }
}

namespace xact
{
    namespace priv
    {
        template<>
        std::string read< std::string >( std::istream& in )
        {
            std::string str;
            for ( char c = in.get(); c != '\0'; c = in.get() )
                str += c;
            return str;
        }

        // Code below was taken from MonoGame
        std::string ConvertMsAdpcmToPcm(const std::string& buffer, int offset, int count, int channels, int blockAlignment)
        {
            MsAdpcmState channel0;
            MsAdpcmState channel1;
            int blockPredictor;

            int sampleCountFullBlock = ((blockAlignment / channels) - 7) * 2 + 2;
            int sampleCountLastBlock = 0;
            if ((count % blockAlignment) > 0)
                sampleCountLastBlock = (((count % blockAlignment) / channels) - 7) * 2 + 2;
            int sampleCount = ((count / blockAlignment) * sampleCountFullBlock) + sampleCountLastBlock;
            auto samples = std::string(sampleCount * sizeof(short) * channels, '\0' );
            int sampleOffset = 0;

            bool stereo = channels == 2;

            while (count > 0)
            {
                int blockSize = blockAlignment;
                if (count < blockSize)
                    blockSize = count;
                count -= blockAlignment;

                int totalSamples = ((blockSize / channels) - 7) * 2 + 2;
                if (totalSamples < 2)
                    break;

                int offsetStart = offset;
                blockPredictor = buffer[offset];
                ++offset;
                if (blockPredictor > 6)
                    blockPredictor = 6;
                channel0.coeff1 = adaptationCoeff1[blockPredictor];
                channel0.coeff2 = adaptationCoeff2[blockPredictor];
                if (stereo)
                {
                    blockPredictor = buffer[offset];
                    ++offset;
                    if (blockPredictor > 6)
                        blockPredictor = 6;
                    channel1.coeff1 = adaptationCoeff1[blockPredictor];
                    channel1.coeff2 = adaptationCoeff2[blockPredictor];
                }

                channel0.delta = buffer[offset];
                channel0.delta |= buffer[offset + 1] << 8;
                if ((channel0.delta & 0x8000) != 0)
                    channel0.delta -= 0x10000;
                offset += 2;
                if (stereo)
                {
                    channel1.delta = buffer[offset];
                    channel1.delta |= buffer[offset + 1] << 8;
                    if ((channel1.delta & 0x8000) != 0)
                        channel1.delta -= 0x10000;
                    offset += 2;
                }

                channel0.sample1 = buffer[offset];
                channel0.sample1 |= buffer[offset + 1] << 8;
                if ((channel0.sample1 & 0x8000) != 0)
                    channel0.sample1 -= 0x10000;
                offset += 2;
                if (stereo)
                {
                    channel1.sample1 = buffer[offset];
                    channel1.sample1 |= buffer[offset + 1] << 8;
                    if ((channel1.sample1 & 0x8000) != 0)
                        channel1.sample1 -= 0x10000;
                    offset += 2;
                }

                channel0.sample2 = buffer[offset];
                channel0.sample2 |= buffer[offset + 1] << 8;
                if ((channel0.sample2 & 0x8000) != 0)
                    channel0.sample2 -= 0x10000;
                offset += 2;
                if (stereo)
                {
                    channel1.sample2 = buffer[offset];
                    channel1.sample2 |= buffer[offset + 1] << 8;
                    if ((channel1.sample2 & 0x8000) != 0)
                        channel1.sample2 -= 0x10000;
                    offset += 2;
                }

                if (stereo)
                {
                    samples[sampleOffset] = (char)channel0.sample2;
                    samples[sampleOffset + 1] = (char)(channel0.sample2 >> 8);
                    samples[sampleOffset + 2] = (char)channel1.sample2;
                    samples[sampleOffset + 3] = (char)(channel1.sample2 >> 8);
                    samples[sampleOffset + 4] = (char)channel0.sample1;
                    samples[sampleOffset + 5] = (char)(channel0.sample1 >> 8);
                    samples[sampleOffset + 6] = (char)channel1.sample1;
                    samples[sampleOffset + 7] = (char)(channel1.sample1 >> 8);
                    if ( sampleOffset+7>=samples.size())std::cout<<"problem!A\n";
                    sampleOffset += 8;
                }
                else
                {
                    samples[sampleOffset] = (char)channel0.sample2;
                    samples[sampleOffset + 1] = (char)(channel0.sample2 >> 8);
                    samples[sampleOffset + 2] = (char)channel0.sample1;
                    samples[sampleOffset + 3] = (char)(channel0.sample1 >> 8);
                    if ( sampleOffset+3>=samples.size())std::cout<<"problem!B\n";
                    sampleOffset += 4;
                }

                blockSize -= (offset - offsetStart);
                if (stereo)
                {
                    for (int i = 0; i < blockSize; ++i)
                    {
                        int nibbles = buffer[offset];

                        int sample = AdpcmMsExpandNibble(channel0, nibbles >> 4);
                        samples[sampleOffset] = (char)sample;
                        samples[sampleOffset + 1] = (char)(sample >> 8);

                        sample = AdpcmMsExpandNibble(channel1, nibbles & 0x0f);
                        samples[sampleOffset + 2] = (char)sample;
                        samples[sampleOffset + 3] = (char)(sample >> 8);
                    if ( sampleOffset+3>=samples.size())std::cout<<"problem!C "<<sampleOffset<<"/"<<samples.size()<<"\n";

                        sampleOffset += 4;
                        ++offset;
                    }
                }
                else
                {
                    for (int i = 0; i < blockSize; ++i)
                    {
                        int nibbles = buffer[offset];

                        int sample = AdpcmMsExpandNibble(channel0, nibbles >> 4);
                        samples[sampleOffset] = (char)sample;
                        samples[sampleOffset + 1] = (char)(sample >> 8);

                        sample = AdpcmMsExpandNibble(channel0, nibbles & 0x0f);
                        samples[sampleOffset + 2] = (char)sample;
                        samples[sampleOffset + 3] = (char)(sample >> 8);
                    if ( sampleOffset+3>=samples.size())std::cout<<"problem!D\n";

                        sampleOffset += 4;
                        ++offset;
                    }
                }
            }

            return samples;
        }
    }
}
