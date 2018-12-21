/*========================================================================

    Name:     BmpDecoder.h

    Date:     June 2017

    Author:   Curtis Buckoll

    Overview:

========================================================================*/

#pragma once

//--------------------------------------------------------------

#include "Util.h"
#include "Errors.h"
#include "Vertex.h"

#include <windows.h>
#include <vector>

//========================================================================
// Container for extracted image data.
struct BmpData
{
    uint32_t width_;
    uint32_t height_;

    uint32_t file_size_;
    uint16_t bits_per_pixel_;
    uint32_t offset_to_data_;

    std::vector<Color256> pixels_;

    // For compressing, save the unmodified header and data blocks seperately.
    std::vector<UByte> header_;
    std::vector<UByte> body_;
};

//========================================================================
//
class BmpDecoder
{
public:

    //--------------------------------------------------------------
    //
    BmpDecoder( const std::vector<Byte>& raw_data );

    //--------------------------------------------------------------
    //
    ~BmpDecoder();

    //--------------------------------------------------------------
    // Decode and internally store the supplied data.
    MsgNum decode();

    //--------------------------------------------------------------
    // Returns a constant reference to the data. This object must
    // persist for the duration that we access it externally.
    const BmpData& getData() const;

    //--------------------------------------------------------------
    // Moves the .nbmp data block to the caller, leaving the current
    // state of the decoded data undefineded. This should not be
    // called more than once.
    BmpData releaseData();


private:

    //--------------------------------------------------------------
    // We must construct the decoder with raw data, so hide the
    // default constructor
    BmpDecoder() = delete;

    //--------------------------------------------------------------
    // Converts numBytes bytes in a byte array to a 32-bit signed 
    // int starting at index pos. Little endian intepretation.
    // Updates pos to next position after the final read byte.
    uint32_t bytesToUInt32LE( Uint& pos,
                              Uint numBytes ) const;

    //--------------------------------------------------------------
    // Extracts the header data from the byte array. Updates pos to 
    // next position in the raw data.
    MsgNum storeMetaData( Uint& pos );

    //--------------------------------------------------------------
    // Extracts the image data from the byte array. Updates pos to 
    // next position in the raw data.
    MsgNum storePixelData( Uint& pos );


    //==============================================================
    //  Private Data Members
    //==============================================================

    BmpData                     bmp_data_;
    const std::vector<Byte>&    raw_data_;
};
