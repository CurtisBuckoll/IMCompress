#include "stdafx.h"
#include "LZWCoder.h"

#include <iostream>
#include <vector>

//========================================================================
//
LZWCoder::LZWCoder()
    : do_shift_( false )
{
}
//========================================================================
//

LZWCoder::~LZWCoder()
{
}

//========================================================================
//
void LZWCoder::insert12Bits( uint16_t bits, std::vector<UByte>& arr )
{
    // Here we are assuming 'bits' contains at most 12 bits worth of information.
    if( do_shift_ )
    { 
        UByte b1 = ( bits >> 8 ) & 0xf;
        UByte b2 = bits & 0xff;
        arr.back() = arr.back() | b1;
        arr.push_back( b2 );
        do_shift_ = false;
    }
    else
    {
        UByte b1 = ( bits >> 4 ) & 0xff;
        UByte b2 = ( bits & 0xf ) << 4;
        arr.push_back( b1 );
        arr.push_back( b2 );
        do_shift_ = true;
    }

    return;
}

//========================================================================
//
MsgNum LZWCoder::encode( const std::vector<UByte>& inData, std::vector<UByte>& outData )
{
    // Size is 4096 - 256 = 3840. This will ensure we always have space for
    // The single byte oriented codewords.
    static const size_t MAX_NUM_CWS = 3840;

    // Initialize the string table with the first possible bytes.
    std::unordered_map<std::string, uint16_t> str_table;

    //std::wstring s = std::wstring( 1, inDataResize[0] );

    size_t code = 0;
    std::string s = std::string( 1, inData[0] );

    std::vector<uint16_t> codewords;

    Uint max_cw_len = 0;

    for( size_t i = 1; i < inData.size(); ++i )
    {
        UByte c = inData[i];

        std::string s_plus_c = s + (char)c;

        auto found = str_table.find( s_plus_c );

        if( found == str_table.end() )
        {
            // First output the code for "s":
            // If the codeword "s" is not found, then insert into 
            // table. This will happen sometimes with completely new 
            // symbols, as the dictionary is initialized empty.
            auto cw = str_table.find( s );

            if( cw == str_table.end() )
            {
                auto pos = str_table.insert( { s, code++ } );
                cw = pos.first;
            }

            insert12Bits( cw->second, outData );

            // Insert the current string into the dictionary if less than
            // max codewords.
            if( str_table.size() < MAX_NUM_CWS )
            {
                str_table.insert( { s_plus_c, code++ } );
                if( s_plus_c.length() > max_cw_len ) max_cw_len = s_plus_c.length();
            }

            // Finally, update "s".
            s = std::string( 1, c );
        }
        else
        {
            s = s_plus_c;
        }
    }

    auto cw = str_table.find( s );
    if( cw == str_table.end() )
    {
        auto pos = str_table.insert( { s, code } );
        cw = pos.first;
    }
    insert12Bits( cw->second, outData );

    return STATUS_OKAY;
}
