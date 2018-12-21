#include "stdafx.h"
#include "IM3Coder.h"

#include "HuffmanCoder.h"

#include <array>

//========================================================================
//
DCT::DCT( size_t block_size, double quality_scale )
    : sz_( block_size )
    , T( sz_ )
    , T_t( sz_ )
    , quantization_table_( 8 )
{
    // Construct the size sz_ DCT Matrices
    const size_t N = sz_;
    for( Uint i = 0; i < N; ++i )
    {
        for( Uint j = 0; j < N; ++j )
        {
            const double a = i == 0 ? sqrt( 1.0 / N ) : sqrt( 2.0 / N );
            double entry = a * std::cos( ( ( 2.0 * j + 1.0 ) * i * PI ) / ( 2.0 * N ) );
            T.matrix_[i][j] = entry;
        }
    }

    T_t = T;
    T_t.transpose();

    // Build the quantization table: this is fixed.
    quantization_table_.matrix_[0] = {  1,  1,  2,  4,  8, 16, 32, 64 };
    quantization_table_.matrix_[1] = {  1,  1,  2,  4,  8, 16, 32, 64 };
    quantization_table_.matrix_[2] = {  2,  2,  2,  4,  8, 16, 32, 64 };
    quantization_table_.matrix_[3] = {  4,  4,  4,  4,  8, 16, 32, 64 };
    quantization_table_.matrix_[4] = {  8,  8,  8,  8,  8, 16, 32, 64 };
    quantization_table_.matrix_[5] = { 16, 16, 16, 16, 16, 16, 32, 64 };
    quantization_table_.matrix_[6] = { 32, 32, 32, 32, 32, 32, 32, 64 };
    quantization_table_.matrix_[7] = { 64, 64, 64, 64, 64, 64, 64, 64 };

    quantization_table_.scaleBy( quality_scale );
};

//========================================================================
//
Matrix DCT::transform( Matrix block )
{
    return T * block * T_t;
}

//========================================================================
//
Matrix DCT::inverse_transform( Matrix block )
{
    return T_t * block * T;
}

//========================================================================
//
void DCT::quantize( Matrix& block )
{
    if( block.sz_ != this->sz_ )
    {
        // output err;
        std::cout << "Issue quantizing: quantization block and input block are of different size." << std::endl;
        return;
    }
    
    for( Uint i = 0; i < this->sz_; ++i )
    {
        for( Uint j = 0; j < this->sz_; ++j )
        {
            block.matrix_[i][j] = std::round( block.matrix_[i][j] / quantization_table_.matrix_[i][j] );
        }
    }
}

//========================================================================
//
void DCT::dequantize( Matrix& block )
{
    if( block.sz_ != this->sz_ )
    {
        // output err;
        std::cout << "Issue quantizing: quantization block and input block are of different size." << std::endl;
        return;
    }

    for( Uint i = 0; i < this->sz_; ++i )
    {
        for( Uint j = 0; j < this->sz_; ++j )
        {
            block.matrix_[i][j] = std::round( block.matrix_[i][j] * quantization_table_.matrix_[i][j] );
        }
    }
}

//========================================================================
//
IM3Coder::IM3Coder( double compressionRatio )
    : compression_factor_( compressionRatio )
{

}

//========================================================================
//
IM3Coder::~IM3Coder()
{

}

//========================================================================
//
MsgNum IM3Coder::encode( const BmpData& inData,
                         std::vector<UByte>& outData )
{
    // We can write width and height now.
    IM3CoderParameters coder_params;
    coder_params.imgW = inData.width_;
    coder_params.imgH = inData.height_;

    std::vector<UByte> encoded_data;
    lossyEncode( coder_params, inData.body_, encoded_data );

    // Now assemble the compressed output file:
    // First we encode the 8-byte header: | 0 I M 3 | imgw | imgH |
    // The "0 I M 3" is 4 bytes, and the width/height are 2-byte unsigned 
    // integers.
    outData.clear();
    outData.push_back( '0' );
    outData.push_back( 'I' );
    outData.push_back( 'M' );
    outData.push_back( '3' );
    
    outData.push_back( ( coder_params.imgW >> 8 ) & 0xff );
    outData.push_back( coder_params.imgW & 0xff );
    outData.push_back( ( coder_params.imgH >> 8 ) & 0xff );
    outData.push_back( coder_params.imgH & 0xff );


    // -------------------------------------------------------------
    // Perform lossless Huffman encoding on the image body.
    HuffmanCoder       huffCoder;
    DecoderParameters  dec_params;

    std::vector<UByte> huffman_encoded;

    MsgNum err = huffCoder.encodePerByte( encoded_data, huffman_encoded, dec_params );
    if( err ) return err;

    // Assemble the output file.

    // Store the Huffman coding related information needed for
    // decoding.
    // 2 bytes : Max cw length
    outData.push_back( ( dec_params.max_cw_len_ >> 8 ) & 0xff );
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

    // -------------------------------------------------------------
    // Now write the actual encoded data to the binary output.
    for( auto b : huffman_encoded )
    {
        outData.push_back( b );
    }

    return STATUS_OKAY;
}

//========================================================================
//
MsgNum IM3Coder::decode( const std::vector<UByte>& inData,
                         BmpData& outData )
{
    // First we must construct the decoder parameters to pass to the
    // decoder. This only includes the width and height of the image.
    IM3CoderParameters coder_params;

    coder_params.imgW = ( ( inData[4] << 8 ) | inData[5] ) & 0xffff;
    coder_params.imgH = ( ( inData[6] << 8 ) | inData[7] ) & 0xffff;

    std::vector<UByte> decoded_data;

    // Copy the remaining data to a new buffer that we can directly 
    // pass to lossyDecode().
    std::vector<UByte> data_to_decode;
    for( Uint i = 8; i < inData.size(); ++i )
    {
        data_to_decode.push_back( inData[i] );
    }

    // -------------------------------------------------------------
    // Decode the Huffman-encoded body.
    // Extract and rebuild the decoder parameters struct.
    // Max cw length.
    Uint pos = 0;
    DecoderParameters dec_params = { 0 };
    dec_params.max_cw_len_ |= data_to_decode[pos++] << 8;
    dec_params.max_cw_len_ |= data_to_decode[pos++];

    // Number of bytes in compressed data portion.
    for( Uint i = 0; i < 8; ++i )
    {
        dec_params.num_bytes_ = dec_params.num_bytes_ << 8 | data_to_decode[pos++];
    }

    // Decoder lookup table size.
    uint16_t table_size = ( data_to_decode[pos++] << 8 );
    table_size |= data_to_decode[pos++];

    //std::cout << table_size;

    // The lookup table data.
    for( Uint i = 0; i < table_size; ++i )
    {
        UByte old_sym = data_to_decode[pos++];
        UByte new_sym_len = data_to_decode[pos++];

        uint64_t new_sym = 0;

        for( Uint i = 0; i < 4; ++i )
        {
            new_sym = ( new_sym << 8 ) | data_to_decode[pos++];
        }

        dec_params.decoder_LUT_.push_back( DecoderLUTEntry( old_sym, new_sym, new_sym_len ) );
    }

    std::vector<UByte> compressed_data_block;

    for( ; pos < data_to_decode.size(); ++pos )
    {
        compressed_data_block.push_back( data_to_decode[pos] );
    }

    // Decompress the data portion.
    HuffmanCoder huffCoder;
    std::vector<UByte> huffman_decoded;
    MsgNum err = huffCoder.decode( compressed_data_block, huffman_decoded, dec_params );


    // -------------------------------------------------------------
    // Decode the pixel data.
    lossyDecode( coder_params, huffman_decoded, decoded_data );
    
    // Allocate the data into the struct.

    outData.height_ = coder_params.imgH;
    outData.width_  = coder_params.imgW;
    outData.pixels_ = std::move( util::RGB2Color256( decoded_data ) );

    return STATUS_OKAY;
}

//========================================================================
//
MsgNum IM3Coder::lossyEncode( IM3CoderParameters& params, 
                              const std::vector<UByte>& inData,
                              std::vector<UByte>& outData )
{
    // Pass in the quality scale as the second argument. Higher 
    // means lower quality. Must be strictly greater than 0.
    DCT dct( 8, compression_factor_ );

    // First convert to YUV space. In the raw bytestream, the 
    // componenets are encoded as BGR, so rgb2yuv will account for 
    // this and flip it back to RGB.
    std::vector<int16_t> as_yuv = util::rgb2yuv( inData );

    // Split into corresponding YUV components.
    std::array<std::vector<int16_t>, 3> yuv_components;

    // Ordering in the raw data is BGR, so pass in the indices of 
    // rgb_componenets in reverse.
    util::extractYUVComponents( as_yuv, yuv_components[0], yuv_components[1], yuv_components[2] );


    bool downsample_yuv = false;
    if( params.imgW % 16 == 0 && params.imgH % 16 == 0 )
    {
        downsample_yuv = true;
    }
    else
    {
        std::cout << "Skipping downsampling of U,V channels." << std::endl;
    }

    if( downsample_yuv )
    {
        // Downsample the U and the V channels.
        yuv_components[1] = util::downsampleChannel( yuv_components[1], params.imgW, params.imgH );
        yuv_components[2] = util::downsampleChannel( yuv_components[2], params.imgW, params.imgH );
    }
    
    // Integer division.. We should assume the height and width are 
    // multiples of 8, however.
    Uint num_horiz_blocks = params.imgW / 8;
    Uint num_vert_blocks  = params.imgH / 8;
    
    // This has to change when we encode downsampled U, V channels.
    Uint curr_img_width   = params.imgW;

    Uint channel_index = 0;
    std::array<std::vector<std::vector<UByte>>, 3> RLE_blocks;

    // Perform the transform on each block of the image and quantize.
    for( auto channel : yuv_components )
    {
        Uint X = 0;
        Uint Y = 0;

        // This is for when we encode the U, V channels, we must 
        // halve the number of blocks in both x, y directions
        if( channel_index == 1 && downsample_yuv )
        {
            num_vert_blocks  = num_vert_blocks / 2;
            num_horiz_blocks = num_horiz_blocks / 2;
            curr_img_width   = curr_img_width / 2;
        }

        for( Uint y = 0; y < num_vert_blocks; ++y )
        {
            Y = y * 8;

            for( Uint x = 0; x < num_horiz_blocks; ++x )
            {
                X = x * 8;

                Matrix block( 8 );
                for( Uint j = 0; j < 8; ++j )
                {
                    for( Uint i = 0; i < 8; ++i )
                    {
                        block.matrix_[j][i] = static_cast<double>(channel[( X + i ) + ( (Y + j) * curr_img_width )]);
                    }
                }

                block = dct.transform( block );
                dct.quantize( block );

                std::vector<double> vectorized_block = zigZagRead( block );
                std::vector<UByte> rle_encoded_block;
                compressVectorBlock( vectorized_block, rle_encoded_block );

                RLE_blocks[channel_index].push_back( std::move( rle_encoded_block ) );
            }
        }

        ++channel_index;
    }

    // The last thing to do is to copy the RLE blocks per channel to the 
    // outdata vector.
    outData.clear();
    for( auto chan : RLE_blocks )
    {
        for( auto rle_block : chan )
        {
            for( auto b : rle_block )
            {
                outData.push_back( b );
            }
        }
    }

    return STATUS_OKAY;
}

//========================================================================
//
MsgNum IM3Coder::lossyDecode( const IM3CoderParameters& params,
                              const std::vector<UByte>& inData,
                              std::vector<UByte>& outData )
{
    DCT dct( 8, compression_factor_ );

    Uint num_horiz_blocks = params.imgW / 8;
    Uint num_vert_blocks = params.imgH / 8;

    bool upsample_yuv = false;
    if( params.imgW % 16 == 0 && params.imgH % 16 == 0 )
    {
        upsample_yuv = true;
    }

    // Now try to decode the data and reconstruct the image.
    std::array<std::vector<int16_t>, 3> yuv;
    yuv[0].resize( params.imgW * params.imgH );
    upsample_yuv ? yuv[1].resize( params.imgW * params.imgH / 4 ) : yuv[1].resize( params.imgW * params.imgH );
    upsample_yuv ? yuv[2].resize( params.imgW * params.imgH / 4 ) : yuv[2].resize( params.imgW * params.imgH );

    // This has to change when we encode downsampled U, V channels...
    uint16_t curr_img_width = params.imgW;

    // Get the RLE blocks seperated for further processing.
    std::array<std::vector<std::vector<UByte>>, 3> RLE_blocks  =  extractRLEBlocks( inData, num_horiz_blocks  * num_vert_blocks, upsample_yuv );
    Uint channel_index = 0;
    for( auto channel : RLE_blocks )
    {
        Uint X = 0;
        Uint Y = 0;

        // This is for when we decode the U, V channels, must halve the 
        // number of blocks in both x, y directions
        if( channel_index == 1 && upsample_yuv )
        {
            num_vert_blocks = num_vert_blocks / 2;
            num_horiz_blocks = num_horiz_blocks / 2;
            curr_img_width = curr_img_width / 2;
        }

        for( Uint y = 0; y < num_vert_blocks; ++y )
        {
            Y = y * 8;

            for( Uint x = 0; x < num_horiz_blocks; ++x )
            {
                // First decode the linearly compressed zig-zag data
                // and write it into a block.
                std::vector<double> zig_zag_decompressed;
                decompressVectorBlock( channel[x + y * num_horiz_blocks], zig_zag_decompressed );
                Matrix block = zigZagWrite( zig_zag_decompressed, 8 );

                dct.dequantize( block );
                block = dct.inverse_transform( block );

                X = x * 8;

                for( Uint j = 0; j < 8; ++j )
                {
                    for( Uint i = 0; i < 8; ++i )
                    {
                        int16_t pix = static_cast<int16_t>( block.matrix_[j][i] );
                        yuv[channel_index][( X + i ) + ( ( Y + j ) * curr_img_width )] = pix;
                    }
                }
            }
        }

        ++channel_index;
    }

    if( upsample_yuv )
    {
        // Upsample the channels back to their full size before converting back to RGB.
        yuv[1] = util::upsampleChannel( yuv[1], params.imgW, params.imgH );
        yuv[2] = util::upsampleChannel( yuv[2], params.imgW, params.imgH );
    }

    outData = std::move( util::yuvArray2RGB( yuv ) );

    return STATUS_OKAY;
}

//========================================================================
//
std::vector<double> IM3Coder::zigZagRead( const Matrix& m )
{
    std::vector<double> asVec;

    if( m.matrix_.size() < 1 )
    {
        // output err
        std::cout << "Must need non-zero matrix dimension for zig-zag scan." << std::endl;
        return asVec;
    }

    bool left_to_right = true;
    double entry       = 0;

    for( size_t elems_in_diag = 1; elems_in_diag <= m.sz_; ++elems_in_diag )
    {
        if( left_to_right )
        {
            for( size_t e = 0; e < elems_in_diag; ++e )
            {
                entry = m.matrix_[elems_in_diag - e - 1][e];
                asVec.push_back( entry );
            }
        }
        else
        {
            for( size_t e = 0; e < elems_in_diag; ++e )
            {
                entry = m.matrix_[e][elems_in_diag - e - 1];
                asVec.push_back( entry );
            }
        }

        left_to_right = !left_to_right;
    }

    size_t start_ind = 1;
    for( size_t elems_in_diag = m.sz_ - 1; elems_in_diag < m.sz_; --elems_in_diag )
    {
        if( left_to_right )
        {
            for( size_t e = 0; e < elems_in_diag; ++e )
            {
                entry = m.matrix_[m.sz_ - e - 1][start_ind + e];
                asVec.push_back( entry );
            }
        }
        else
        {
            for( size_t e = 0; e < elems_in_diag; ++e )
            {
                entry = m.matrix_[start_ind + e][m.sz_ - e - 1];
                asVec.push_back( entry );
            }
        }
        start_ind += 1;
        left_to_right = !left_to_right;
    }

    return asVec;
}

//========================================================================
//
Matrix IM3Coder::zigZagWrite( const std::vector<double>& vec, size_t matSize )
{
    Matrix asMatrix( matSize );

    bool left_to_right = true;
    double entry       = 0;
    size_t vec_index   = 0;

    for( size_t elems_in_diag = 1; elems_in_diag <= asMatrix.sz_; ++elems_in_diag )
    {
        if( left_to_right )
        {
            for( size_t e = 0; e < elems_in_diag; ++e )
            {
                asMatrix.matrix_[elems_in_diag - e - 1][e] = vec[vec_index];
                ++vec_index;
            }
        }
        else
        {
            for( size_t e = 0; e < elems_in_diag; ++e )
            {
                asMatrix.matrix_[e][elems_in_diag - e - 1] = vec[vec_index];
                ++vec_index;
            }
        }
        left_to_right = !left_to_right;
    }

    size_t start_ind = 1;
    for( size_t elems_in_diag = asMatrix.sz_ - 1; elems_in_diag < asMatrix.sz_; --elems_in_diag )
    {
        if( left_to_right )
        {
            for( size_t e = 0; e < elems_in_diag; ++e )
            {
                asMatrix.matrix_[asMatrix.sz_ - e - 1][start_ind + e] = vec[vec_index];
                ++vec_index;
            }
        }
        else
        {
            for( size_t e = 0; e < elems_in_diag; ++e )
            {
                asMatrix.matrix_[start_ind + e][asMatrix.sz_ - e - 1] = vec[vec_index];
                ++vec_index;
            }
        }

        start_ind += 1;
        left_to_right = !left_to_right;
    }

    return asMatrix;
}

//========================================================================
//
void IM3Coder::compressVectorBlock( const std::vector<double>& src, std::vector<UByte>& target )
{
    // Encode in the format: (# zeros to skip, next non-zero val).
    // # zeros to skip is a single byte, next non-zero val is a signed 16-bit
    // integer occupying 2 bytes.
    UByte num_zeros = 0;
    for( auto b : src )
    {
        if( b != 0.0 )
        {
            int16_t val = static_cast<int16_t>( b );

            // Encode the value as a 16-bit signed integer.
            target.push_back( num_zeros );
            target.push_back( val >> 8 & 0xff );
            target.push_back( val & 0xff );
            num_zeros = 0;
        }
        else
        {
            ++num_zeros;
        }
    }

    // Input the deliminator pair (0, 0).
    for( Uint i = 0; i < 3; ++i )
    {
        target.push_back( 0 );
    }

    return;
}

//========================================================================
//
void IM3Coder::decompressVectorBlock( const std::vector<UByte>& src, std::vector<double>& target )
{
    if( src.size() < 3 )
    {
        // output err.
        std::cout << "Problem decompressing a vector block: Minimum required size is 3." << std::endl;
        return;
    }

    for( size_t i = 0; i + 2 < src.size(); i += 3 )
    {
        UByte numZeros = src[i];
        int16_t val    = 0;

        for( size_t b = 0; b < 2; ++b )
        {
            val |= src[i + 1 +  b] << ( 8 * ( 1 - b ) );
        }

        if( numZeros == 0 && val == 0 )
        {
            // (0,0) is the delimiter, so quit if we're here.
            break;
        }

        for( size_t k = 0; k < numZeros; ++k )
        {
            target.push_back( 0.0 );
        }

        target.push_back( static_cast<double>( val ) );
    }

    // Fill the block with the remaining zeros.
    for( size_t i = target.size(); i < 64; ++i )
    {
        target.push_back( 0.0 );
    }

    return;
}

//========================================================================
//
std::array<std::vector<std::vector<UByte>>, 3> IM3Coder::extractRLEBlocks( const std::vector<UByte> raw_data, Uint blocksPerChannel, bool upsample )
{
    std::array<std::vector<std::vector<UByte>>, 3> encoded_blocks;


    Uint raw_data_ind = 0;
    Uint chan = 0;

    while( chan < 3 )
    {
        if( chan == 1 && upsample )
        {
            blocksPerChannel = blocksPerChannel / 4;
        }

        Uint b = 0;

        while( b < blocksPerChannel )
        {
            encoded_blocks[chan].emplace_back();

            while( raw_data[raw_data_ind] != 0 || raw_data[raw_data_ind + 1] != 0 || raw_data[raw_data_ind + 2] != 0 )
            {
                encoded_blocks[chan][b].push_back( raw_data[raw_data_ind] );
                encoded_blocks[chan][b].push_back( raw_data[raw_data_ind + 1] );
                encoded_blocks[chan][b].push_back( raw_data[raw_data_ind + 2] );
                raw_data_ind += 3;
            }

            encoded_blocks[chan][b].push_back( 0 );
            encoded_blocks[chan][b].push_back( 0 );
            encoded_blocks[chan][b].push_back( 0 );

            raw_data_ind += 3;
            ++b;
        }

        ++chan;
    }

    return encoded_blocks;
}
