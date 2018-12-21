#pragma once

#include "Util.h"
#include "BmpDecoder.h"
#include "Matrix.h"

#include <vector>
#include <array>
#include <math.h>

#define PI 3.14159265

//--------------------------------------------------------------
//
struct DCT 
{
    DCT( size_t block_size, double quality_scale );

    Matrix transform( Matrix block );

    Matrix inverse_transform( Matrix block );

    void quantize( Matrix& block );

    void dequantize( Matrix& block );

    size_t sz_;
    Matrix T;
    Matrix T_t;
    Matrix quantization_table_;
};

//--------------------------------------------------------------
//
struct IM3CoderParameters 
{
    IM3CoderParameters()
        : imgW( 0 )
        , imgH( 0 )
    { }

    uint16_t imgW;
    uint16_t imgH;
};

//--------------------------------------------------------------
//
class IM3Coder
{
public:

    //--------------------------------------------------------------
    //
    IM3Coder( double compressionRatio );

    //--------------------------------------------------------------
    //
    ~IM3Coder();

    //--------------------------------------------------------------
    //
    MsgNum encode( const BmpData& inData,
                   std::vector<UByte>& outData );

    //--------------------------------------------------------------
    //
    MsgNum decode( const std::vector<UByte>& inData,
                   BmpData& outData );

    //--------------------------------------------------------------
    //
    MsgNum lossyEncode( IM3CoderParameters& params,
                        const std::vector<UByte>& inData,
                        std::vector<UByte>& outData );

private:

    //--------------------------------------------------------------
    //
    MsgNum lossyDecode( const IM3CoderParameters& params, 
                        const std::vector<UByte>& inData,
                        std::vector<UByte>& outData );

    //--------------------------------------------------------------
    //
    std::vector<double> zigZagRead( const Matrix& m );

    //--------------------------------------------------------------
    //
    Matrix zigZagWrite( const std::vector<double>& m, 
                        size_t matSize );

    //--------------------------------------------------------------
    //
    void compressVectorBlock( const std::vector<double>& src, 
                              std::vector<UByte>& target );

    //--------------------------------------------------------------
    //
    void decompressVectorBlock( const std::vector<UByte>& src, 
                                std::vector<double>& target );

    //--------------------------------------------------------------
    //
    std::array<std::vector<std::vector<UByte>>, 3> extractRLEBlocks( const std::vector<UByte> raw_data, 
                                                                     Uint blocksPerChannel,
                                                                     bool upsample );

    //--------------------------------------------------------------
    //
    BmpData compressed_image_;
    double  compression_factor_;
};

