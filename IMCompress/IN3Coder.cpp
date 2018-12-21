#include "stdafx.h"
#include "IN3Coder.h"

#include <array>

#include "HuffmanCoder.h"
#include "BmpDecoder.h"

//========================================================================
//
UByte BitExtractor::get_bit()
{
    if( bit_index_ < 0 )
    {
        ++byte_index;
        bit_index_ = 7;
    }

    UByte b = ( stream_[byte_index] >> bit_index_ ) & 0x1;
    --bit_index_;

    return b;
}

//========================================================================
//
void BitEncoder::append_bit( std::vector<UByte>& arr, UByte bit )
{
    if( bit_index_ == 0 )
    {
        UByte b = curr_byte_ | bit;
        arr.push_back( b );
        bit_index_ = 7;
        curr_byte_ = 0;
    }
    else
    {
        curr_byte_ = curr_byte_ | ( bit << bit_index_ );
        --bit_index_;
    }
}

//========================================================================
//
void BitEncoder::getLastByte( std::vector<UByte>& arr )
{
    arr.push_back( curr_byte_ );
    curr_byte_ = 0;
}

//========================================================================
//
IN3Coder::IN3Coder()
{

}

//========================================================================
//
IN3Coder::~IN3Coder()
{

}

//========================================================================
//
MsgNum IN3Coder::encode( const BmpData& inData,
                         std::vector<UByte>& outData )
{
    // First perform delta encoding on the body (pixel data) of the file.
    // Here we assume there is no padding in the source file, ie. the 
    // width/height is always some multiple of 4.
    std::vector<UByte> delta_encoded;
    std::vector<UByte> symbol_sign;

    delta_encoded.push_back( inData.body_[0] );
    delta_encoded.push_back( inData.body_[1] );
    delta_encoded.push_back( inData.body_[2] );

    std::array<int16_t, 3> next_val;
    BitEncoder bit_encoder;

    for( Uint i = 3; i + 2 < inData.body_.size(); i += 3 )
    {
        // Need to keep track of +ve and -ve. see your paper.
        next_val[0] = (int16_t)inData.body_[i] - (int16_t)inData.body_[i - 3];
        next_val[1] = (int16_t)inData.body_[i + 1] - (int16_t)inData.body_[i - 2];
        next_val[2] = (int16_t)inData.body_[i + 2] - (int16_t)inData.body_[i - 1];

        delta_encoded.push_back( std::abs( next_val[0] ) );
        delta_encoded.push_back( std::abs( next_val[1] ) );
        delta_encoded.push_back( std::abs( next_val[2] ) );

        // Encode the sign of the value: 0 is +ve, 1 is -ve
        next_val[0] < 0 ? bit_encoder.append_bit( symbol_sign, 1 ) : bit_encoder.append_bit( symbol_sign, 0 );
        next_val[1] < 0 ? bit_encoder.append_bit( symbol_sign, 1 ) : bit_encoder.append_bit( symbol_sign, 0 );
        next_val[2] < 0 ? bit_encoder.append_bit( symbol_sign, 1 ) : bit_encoder.append_bit( symbol_sign, 0 );
    }

    bit_encoder.getLastByte( symbol_sign );


    HuffmanCoder       huffCoder;
    DecoderParameters  dec_params;
    std::vector<UByte> delta_encoded_body;

    // Insert the delta encoded information in this pattern: 
    // | num_bytes (uint64_t) | byte_data ... | num_sign_bytes (uint64_t) | sign_data ... |
    uint64_t sz_bytes = delta_encoded.size();
    uint64_t sz_signs = symbol_sign.size();

    for( size_t i = 7; i < 8; --i )
    {
        delta_encoded_body.push_back( ( sz_bytes >> ( i * 8 ) ) & 0xff );
    }
    for( auto b : delta_encoded )
    {
        delta_encoded_body.push_back( b );
    }
    for( size_t i = 7; i < 8; --i )
    {
        delta_encoded_body.push_back( ( sz_signs >> ( i * 8 ) ) & 0xff );
    }
    for( auto b : symbol_sign )
    {
        delta_encoded_body.push_back( b );
    }

    // Perform Huffman coding on the 
    // result
    std::vector<UByte> encoded_body;

    // comment out to test delta encode vs non delta encode performance.
    //delta_encoded_body = inData.body_;

    MsgNum err = huffCoder.encodePerByte( delta_encoded_body, encoded_body, dec_params );
    if( err ) return err;


    // Now assemble the output file.
    outData.clear();

    // Copy the header. We will not compress the header.
    for( auto byte : inData.header_ )
    {
        outData.push_back( byte );
    }

    // Store the Huffman coding related information needed for
    // decoding.
    // 2 bytes : Max cw length
    outData.push_back( ( dec_params.max_cw_len_ >> 8 ) & 0xff);
    outData.push_back( dec_params.max_cw_len_ & 0xff );

    // 8 bytes : Num bytes of pixel data in input file.
    for( Uint i = 7; i < 8; --i )
    {
        outData.push_back( ( dec_params.num_bytes_ >> ( i * 8 ) ) & 0xff );
    }

    // Now we need the decoder symbol LUT created by the Huffman encoder.
    // Put the size in first - 2 bytes.
    uint16_t tableSize = dec_params.decoder_LUT_.size();
    outData.push_back( ( tableSize >> 8 ) & 0xff );
    outData.push_back( tableSize & 0xff );

    
    // Encoding format per entry is
    // | old_sym | new_sym | new_sym_length |
    for( auto entry : dec_params.decoder_LUT_ )
    {
        outData.push_back( entry.old_sym_ );
        outData.push_back( entry.new_sym_len_ );
           
        for( Uint i = 3; i < 4; --i )
        {
            outData.push_back( ( entry.new_sym_ >> ( i * 8 ) ) & 0xff );
        }
    }
    

    // Finally, append the compressed pixel data.
    for( auto byte : encoded_body )
    {
        outData.push_back( byte );
    }

    return STATUS_OKAY;
}

//========================================================================
//
MsgNum IN3Coder::decode( const std::vector<UByte>& inData,
                         BmpData& outData )
{
    Uint pos = 0;

    std::vector<Byte> decodeFromBmp;

    // Extract the header. assume 54 bytes ( for now )
    for(; pos < 54; ++pos)
    {
        decodeFromBmp.push_back( inData[pos] );
        outData.header_.push_back( inData[pos] );
    }

    // Extract and rebuild the decoder parameters struct.
    // Max cw length.
    DecoderParameters dec_params = { 0 };
    dec_params.max_cw_len_ |= inData[pos++] << 8;
    dec_params.max_cw_len_ |= inData[pos++];

    // Number of bytes in compressed data portion.
    for( Uint i = 0; i < 8; ++i )
    {
        dec_params.num_bytes_ = dec_params.num_bytes_ << 8 | inData[pos++];
    }

    // Decoder lookup table size.
    uint16_t table_size = ( inData[pos++] << 8 );
    table_size |= inData[pos++];

    // The lookup table data.
    for( Uint i = 0; i < table_size; ++i )
    {
        UByte old_sym     = inData[pos++];
        UByte new_sym_len = inData[pos++];

        uint64_t new_sym  = 0;

        for( Uint i = 0; i < 4; ++i )
        {
            new_sym = ( new_sym << 8 ) | inData[pos++];
        }

        dec_params.decoder_LUT_.push_back( DecoderLUTEntry( old_sym, new_sym, new_sym_len ) );
    }

    std::vector<UByte> compressed_data_block;

    for( ; pos < inData.size(); ++pos )
    {
        compressed_data_block.push_back( inData[pos] );
    }

    // Finally, decompress the data portion.
    HuffmanCoder huffCoder;
    std::vector<UByte> delta_data;
    MsgNum err = huffCoder.decode( compressed_data_block, delta_data, dec_params );

    if( err ) return err;


    // Perform delta decoding.
    uint64_t sz_bytes = 0;
    uint64_t sz_signs = 0;

    for( size_t i = 0; i < 8; ++i )
    {
        sz_bytes  = sz_bytes << 8;
        sz_bytes |= delta_data[i] & 0xff;
    }
    for( size_t i = 0; i < 8; ++i )
    {
        sz_signs  = sz_signs << 8;
        sz_signs |= delta_data[i + sz_bytes + 8] & 0xff;
    }

    std::vector<UByte> delta_encoded;
    std::vector<UByte> symbol_sign;

    for( size_t i = 8; i < sz_bytes + 8; ++i )
    {
        delta_encoded.push_back( delta_data[i] );
    }
    for( size_t i = sz_bytes + 16; i < delta_data.size(); ++i )
    {
        symbol_sign.push_back( delta_data[i] );
    }

    // And then recover the RGB data fromt the difference data.
    BitExtractor bit_extractor( symbol_sign );

    std::array<UByte, 3> next_val;
    next_val[0] = delta_encoded[0];
    next_val[1] = delta_encoded[1];
    next_val[2] = delta_encoded[2];

    // Write in the first RGB bytes.
    for( Uint i = 0; i < 3; ++i )
    {
        decodeFromBmp.push_back( delta_encoded[i] );
        outData.body_.push_back( delta_encoded[i] );
    }

    // Extract the rest from their differences.
    for( Uint i = 3; i + 2 < delta_encoded.size(); i += 3 )
    {
        // Need to keep track of +ve and -ve. see your paper.
        next_val[0] = static_cast<UByte>( static_cast<int16_t>( outData.body_[i - 3] ) +
                                          static_cast<int16_t>( delta_encoded[i] ) * ( bit_extractor.get_bit() == 0 ? 1 : -1 ) );
        next_val[1] = static_cast<UByte>( static_cast<int16_t>( outData.body_[i - 2] ) +
                                          static_cast<int16_t>( delta_encoded[i + 1] ) * ( bit_extractor.get_bit() == 0 ? 1 : -1 ) );
        next_val[2] = static_cast<UByte>( static_cast<int16_t>( outData.body_[i - 1] ) +
                                          static_cast<int16_t>( delta_encoded[i + 2] ) * ( bit_extractor.get_bit() == 0 ? 1 : -1 ) );

        outData.body_.push_back( next_val[0] );
        outData.body_.push_back( next_val[1] );
        outData.body_.push_back( next_val[2] );

        decodeFromBmp.push_back( next_val[0] );
        decodeFromBmp.push_back( next_val[1] );
        decodeFromBmp.push_back( next_val[2] );
    }

    // Decode the stored data and release/return it.
    BmpDecoder bmp_decoder( decodeFromBmp );
    bmp_decoder.decode();
    outData = bmp_decoder.releaseData();

    return STATUS_OKAY;
}
