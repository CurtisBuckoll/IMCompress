/*========================================================================

    Name:     WavDecoder.h

    Date:     June 2017

    Author:   Curtis Buckoll

    Overview:

========================================================================*/

#pragma once

//--------------------------------------------------------------

#include "Util.h"
#include "Errors.h"

#include <windows.h>
#include <vector>

//========================================================================
// Container for extracted audio data.
struct WavData
{
    uint32_t chunk_size_;
    uint32_t sub_chunk1_size_;
    uint16_t audio_format_;
    uint16_t num_channels_;
    uint32_t sample_rate_;
    uint32_t byte_rate_;
    uint32_t block_align_;
    uint16_t bits_per_sample_;
    uint32_t sub_chunk2_size_;

    std::vector<int64_t> samples_;
    std::vector<UByte>   samples_as_bytes_;
};

//========================================================================
//
class WavDecoder
{
public:

    //--------------------------------------------------------------
    //
    WavDecoder( const std::vector<Byte>& raw_data );

    //--------------------------------------------------------------
    //
    ~WavDecoder();

    //--------------------------------------------------------------
    // Decode and internally store the supplied data.
    MsgNum decode();

    //--------------------------------------------------------------
    // Returns a constant reference to the data. This object must
    // persist for the duration that we access it externally.
    const WavData& getData() const;

    //--------------------------------------------------------------
    // Moves the .wav data block to the caller, leaving the current
    // state of the decoded data undefineded. This should not be
    // called more than once.
    WavData releaseData();


private:

    //--------------------------------------------------------------
    // We must construct the decoder with raw data, so hide the
    // default constructor
    WavDecoder() = delete;
    
    //--------------------------------------------------------------
    // Converts numBytes bytes in a byte array to a 32-bit signed 
    // int starting at index pos. Little endian intepretation.
    // Updates pos to next position after the final read byte.
    uint32_t bytesToUInt32LE( Uint& pos,
                              Uint numBytes ) const;

    //--------------------------------------------------------------
    // Converts numBytes bytes in a byte array to a 64-bit unsigned 
    // int starting at index pos. Little endian intepretation.
    // Updates pos to next position after the final read byte.
    int64_t bytesToInt64LE( Uint& pos,
                            Uint numBytes ) const;

    //--------------------------------------------------------------
    // Extracts the header data from the byte array. Updates pos to 
    // next position in the raw data.
    MsgNum storeMetaData( Uint& pos );

    //--------------------------------------------------------------
    // Extracts the audio data from the byte array. Updates pos to 
    // next position in the raw data.
    MsgNum storeAudioData( Uint& pos );
    
    //--------------------------------------------------------------
    //
    void printInfo();

    //==============================================================
    //  Private Data Members
    //==============================================================

    WavData                     wav_data_;
    const std::vector<Byte>&    raw_data_;
    uint64_t                    bit_depth_mask_;
};
