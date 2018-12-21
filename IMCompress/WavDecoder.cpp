/*========================================================================

    Name:     WavDecoder.cpp

    Date:     June 2017

    Author:   Curtis Buckoll

    Overview:

========================================================================*/

#include "stdafx.h"
#include "WavDecoder.h"

#include <iostream>
#include <algorithm>

//========================================================================
//
WavDecoder::WavDecoder( const std::vector<Byte>& raw_data )
    : raw_data_( raw_data )
    , bit_depth_mask_( 0 )
{
    wav_data_ = { 0 };
}

//========================================================================
//
WavDecoder::~WavDecoder()
{

}

//========================================================================
//
MsgNum WavDecoder::decode()
{
    // pos is a locator to let us keep track of where we are in the file,
    // and this gets updated when passed to the methods in here.
    Uint pos = 0;

    MsgNum err = storeMetaData( pos );
    if( err ) return printMsg( err );

    // Now that we know the bit depth we can store the number of bits
    // used to then help us decode the audio data.
    for( Uint i = 0; i < wav_data_.bits_per_sample_; ++i )
    {
        bit_depth_mask_ = bit_depth_mask_ | ( static_cast<uint64_t>(0x1) << i );
    }

    err = storeAudioData( pos );
    if( err ) return printMsg( err );

    //printInfo();

    return STATUS_OKAY;
}

//========================================================================
//
const WavData& WavDecoder::getData() const
{
    return wav_data_;
}

//========================================================================
//
WavData WavDecoder::releaseData()
{
    return std::move( wav_data_ );
}

//========================================================================
//
uint32_t WavDecoder::bytesToUInt32LE( Uint& pos, 
                                      Uint numBytes ) const
{
    if( pos + numBytes > raw_data_.size() )
    {
        // output err
        std::cout << "Attempted to decode byte to uint64_t that was out of range." << std::endl;
        return 0;
    }

    uint32_t res    = 0;
    Uint multiplier = 0;

    for( Uint i = pos; i < pos + numBytes; ++i )
    {
        UByte currByte = static_cast<UByte>( raw_data_[i] );
        Uint currVal   = currByte << ( 8 * multiplier );
        res += currVal;
        ++multiplier;
    }

    pos += numBytes;
    return res;
}

//========================================================================
//
inline int64_t WavDecoder::bytesToInt64LE( Uint& pos,
                                           Uint numBytes ) const
{
    if( pos + numBytes > raw_data_.size() )
    {
        // output err
        std::cout << "Attempted to decode byte to int64_t that was out of range." << std::endl;
        return 0;
    }

    int64_t res = 0;
    Uint multiplier = 0;

    for( Uint i = pos; i < pos + numBytes; ++i )
    {
        UByte currByte  = static_cast<UByte>( raw_data_[i] );

        if( numBytes == 1 )
        {
            // According to https://en.wikipedia.org/wiki/WAV, 8-bit .wav
            // audio is offset binary, and that this is the only bit depth
            // that stores UNSIGNED integers insted of signed. Subtracting
            // 127 should get us out of this mess..
            currByte = currByte - 127;
        }

        int64_t currVal = currByte << ( 8 * multiplier );
        res |= currVal;
        ++multiplier;
    }

    // If -ve, find the 2's complement of 64-bit.
    if( res & ( static_cast<uint64_t>(0x1) << ( wav_data_.bits_per_sample_ - 1 ) ) )
    {
        res = ( 0xffffffffffffffff ^ bit_depth_mask_ ) | res;
    }

    pos += numBytes;
    return res;
}

//========================================================================
//
MsgNum WavDecoder::storeMetaData( Uint& pos )
{
    // We need access to at least 44 bytes here.
    if( raw_data_.size() < 44 )
    {
        return printMsg( BAD_DATA );
    }

    // RIFF
    pos = 4;

    wav_data_.chunk_size_       = bytesToUInt32LE( pos, 4 ) & 0xffffffff;

    // WAVE, fmt []
    pos += 8;

    wav_data_.sub_chunk1_size_  = bytesToUInt32LE( pos, 4 ) & 0xffffffff;
    wav_data_.audio_format_     = bytesToUInt32LE( pos, 2 ) & 0x0000ffff;
    wav_data_.num_channels_     = bytesToUInt32LE( pos, 2 ) & 0x0000ffff;
    wav_data_.sample_rate_      = bytesToUInt32LE( pos, 4 ) & 0xffffffff;
    wav_data_.byte_rate_        = bytesToUInt32LE( pos, 4 ) & 0xffffffff;
    wav_data_.block_align_      = bytesToUInt32LE( pos, 2 ) & 0x0000ffff;
    wav_data_.bits_per_sample_  = bytesToUInt32LE( pos, 2 ) & 0x0000ffff;

    // data
    pos += 4;

    wav_data_.sub_chunk2_size_  = bytesToUInt32LE( pos, 4 ) & 0xffffffff;

    return STATUS_OKAY;
}

//========================================================================
//
MsgNum WavDecoder::storeAudioData( Uint& pos )
{
    // Here we interpret the data and store to an internal buffer. The 
    // buffer is always a 64-bit signed integer, as this is the largest
    // expected bit-depth.

    // Assume the bit depth is a multiple of 8 and <= 64 bits, that
    // is, we are storing at least 1 byte and no more than 8 bytes per 
    // sample. Expect the samples to be stored little endian.
    if( wav_data_.bits_per_sample_ % 8 != 0 )
    {
        return printMsg( BAD_WAV_BIT_DEPTH );
    }

    Uint numBytes = wav_data_.bits_per_sample_ / 8;

    for( Uint i = pos; i < raw_data_.size(); i += numBytes )
    {
        wav_data_.samples_as_bytes_.push_back( (UByte)raw_data_[i] );

        // Throw this guard in here as a sanity check
        if( i + numBytes - 1 < raw_data_.size() )
        {
            wav_data_.samples_.push_back( bytesToInt64LE( pos, numBytes ) );
        }
        else
        {
            // Just print a warning. This is not necessarily a fatal
            // error.
            printMsg( WAV_DATA_OUT_RANGE );
        }
    }

    return STATUS_OKAY;
}

//========================================================================
//
void WavDecoder::printInfo()
{
    std::cout << "Decoded .wav data with no found errors." << std::endl;
    std::cout << "Number of samples: " << wav_data_.samples_.size() << std::endl;

    int64_t max_val = *std::max_element( wav_data_.samples_.begin(),
                                         wav_data_.samples_.end() );

    auto absval_comparator = []( int64_t a, int64_t b ) -> bool
    {
        return std::abs( a ) < std::abs( b );
    };

    int64_t max_abs_val = std::abs( *std::max_element( wav_data_.samples_.begin(),
                                                       wav_data_.samples_.end(),
                                                       absval_comparator ) );

    std::cout << "Maximum sample value: " << max_val << std::endl;
    std::cout << "Maximum (absolute) sample value: " << max_abs_val << std::endl;
}
