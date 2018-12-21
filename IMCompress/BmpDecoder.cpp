/*========================================================================

    Name:     BmpDecoder.cpp

    Date:     June 2017

    Author:   Curtis Buckoll

    Overview:

========================================================================*/

#include "stdafx.h"
#include "BmpDecoder.h"

//========================================================================
//
BmpDecoder::BmpDecoder( const std::vector<Byte>& raw_data )
    : raw_data_( raw_data )
{
    bmp_data_ = { 0 };
}

//========================================================================
//
BmpDecoder::~BmpDecoder()
{

}

//========================================================================
//
MsgNum BmpDecoder::decode()
{
    // pos is a locator to let us keep track of where we are in the file,
    // and this gets updated when passed to the methods in here.
    Uint pos = 0;

    MsgNum err = storeMetaData( pos );
    if( err ) return printMsg( err );

    err = storePixelData( pos );
    if( err ) return printMsg( err );

    return STATUS_OKAY;
}

//========================================================================
//
const BmpData& BmpDecoder::getData() const
{
    return bmp_data_;
}

//========================================================================
//
BmpData BmpDecoder::releaseData()
{
    return std::move( bmp_data_ );
}

//========================================================================
//
uint32_t BmpDecoder::bytesToUInt32LE( Uint& pos,
                                      Uint numBytes ) const
{
    uint32_t res    = 0;
    Uint multiplier = 0;

    for( Uint i = pos; i < pos + numBytes; ++i )
    {
        UByte currByte = static_cast<UByte>( raw_data_[i] );
        Uint currVal = currByte << ( 8 * multiplier );
        res += currVal;
        ++multiplier;
    }

    pos += numBytes;
    return res;
}

//========================================================================
//
MsgNum BmpDecoder::storeMetaData( Uint& pos )
{
    // We should see at least 54 bytes within the header.
    if( raw_data_.size() < 54 )
    {
        return printMsg( BAD_DATA );
    }

    pos = 2;

    bmp_data_.file_size_      = bytesToUInt32LE( pos, 4 ) & 0xffffffff;

    pos += 4;

    bmp_data_.offset_to_data_ = bytesToUInt32LE( pos, 4 ) & 0xffffffff;

    pos += 4;

    bmp_data_.width_          = bytesToUInt32LE( pos, 4 ) & 0xffffffff;
    bmp_data_.height_         = bytesToUInt32LE( pos, 4 ) & 0xffffffff;

    pos += 2;

    bmp_data_.bits_per_pixel_ = bytesToUInt32LE( pos, 2 ) & 0x0000ffff;

    pos = bmp_data_.offset_to_data_;

    // Save the header and data blocks if we wish to compress the file
    // later.
    for( Uint i = 0; i < bmp_data_.offset_to_data_; ++i )
    {
        bmp_data_.header_.push_back( raw_data_[i] );
    }
    for( Uint i = bmp_data_.offset_to_data_; i < raw_data_.size(); ++i )
    {
        bmp_data_.body_.push_back( raw_data_[i] );
    }

    // Add zero byte padding in the case that the file size is slightly 
    // less than what we are given..? Not sure how this happens but this
    // seems to provide a solution..
    for( Uint i = raw_data_.size(); i < bmp_data_.file_size_; ++i )
    {
        bmp_data_.body_.push_back( 0 );
    }

    return STATUS_OKAY;
}

//========================================================================
//
MsgNum BmpDecoder::storePixelData( Uint& pos )
{
    // Keep track of position in row in case we need to seek over
    // padding bytes present at the end of the row. This can happen
    // if the image width is not a multiple of four.
    Uint curr_row_size = 0;

    // Copy the data and store and to an internal buffer.
    for( Uint i = pos; i + 2 < raw_data_.size(); i += 3 )
    {

        // Seek past padding bytes if at end of row.
        if( curr_row_size == bmp_data_.width_ * 3 )
        {
            while( curr_row_size % 4 != 0 )
            {
                ++i;
                ++curr_row_size;
            }

            curr_row_size = 0;
        }

        // We have to check bounds again as we might seek past the 
        // end of the data after the padding adjustment.
        if( i + 2 < raw_data_.size() )
        {
            bmp_data_.pixels_.push_back( Color256( raw_data_[i + 2],
                                                   raw_data_[i + 1],
                                                   raw_data_[i] ) );
        }

        curr_row_size += 3;
    }

    return STATUS_OKAY;
}
