#pragma once

#include "Util.h"
#include "BmpDecoder.h"

#include <vector>

//--------------------------------------------------------------
//
class BitExtractor
{
public:

    //--------------------------------------------------------------
    //
    BitExtractor( std::vector<UByte>& stream )
        : bit_index_( 7 )
        , byte_index( 0 )
        , stream_( stream )
    { };

    //--------------------------------------------------------------
    //
    UByte get_bit();


private:

    //--------------------------------------------------------------
    //
    int8_t   bit_index_;
    uint64_t byte_index;
    const std::vector<UByte>& stream_;
};


class BitEncoder
{
public:

    //--------------------------------------------------------------
    //
    BitEncoder()
        : bit_index_( 7 )
        , curr_byte_( 0 )
    { };

    //--------------------------------------------------------------
    //
    void append_bit( std::vector<UByte>& arr, UByte bit );

    //--------------------------------------------------------------
    //
    void getLastByte( std::vector<UByte>& arr );

private:

    //--------------------------------------------------------------
    //
    int8_t bit_index_;
    UByte curr_byte_;
};

class IN3Coder
{
public:

    //--------------------------------------------------------------
    //
    IN3Coder();

    //--------------------------------------------------------------
    //
    ~IN3Coder();

    //--------------------------------------------------------------
    //
    MsgNum encode( const BmpData& inData,
                   std::vector<UByte>& outData );

    //--------------------------------------------------------------
    //
    MsgNum decode( const std::vector<UByte>& inData,
                   BmpData& outData );
};

