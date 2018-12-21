#include "stdafx.h"
#include "PSNRMeasure.h"

#include <SDL/SDL.h>

#include "Window.h"
#include "OpenFileDialog.h"
#include "WavDecoder.h"
#include "WavDrawer.h"
#include "BmpDecoder.h"
#include "BmpDrawer.h"
#include "HuffmanCoder.h"
#include "IM3Coder.h"
#include "IN3Coder.h"
#include "LZWCoder.h"

#include <memory>
#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>

//========================================================================
//
PSNRMeasure::PSNRMeasure()
{

}

//========================================================================
//
PSNRMeasure::~PSNRMeasure()
{

}

//========================================================================
//
MsgNum PSNRMeasure::run()
{
    bool should_encode = false;

    BmpData im3_decoded;

    // Get the first file
    std::vector<Byte> data1;
    OpenFileDialog file_explorer1( "Select A File For .im3 Encoding" );

    MsgNum err = file_explorer1.invoke( data1 );
    if( err ) return printMsg( err );

    BmpDecoder bmp_decoder1( data1 );
    bmp_decoder1.decode();

    if( should_encode )
    {
        BmpData toim3encode = bmp_decoder1.releaseData();

        IM3Coder im3coder( 2.0 );
        std::vector<UByte> im3_encoded;
        im3coder.encode( toim3encode, im3_encoded );

        im3coder.decode( im3_encoded, im3_decoded );

    }
    else
    {
        im3_decoded = bmp_decoder1.releaseData();
    }

    // Get the next file
    std::vector<Byte> data2;
    OpenFileDialog file_explorer2( "Select A File To Compare To" );

    err = file_explorer2.invoke( data2 );
    if( err ) return printMsg( err );

    BmpDecoder bmp_decoder2( data2 );
    bmp_decoder2.decode();
    BmpData tocompareto = bmp_decoder2.releaseData();

    // Now compare

    const std::vector<Color256>& a = im3_decoded.pixels_;
    const std::vector<Color256>& b = tocompareto.pixels_;

    if( a.size() != b.size() )
    {
        std::cout << "PSNR: Images are not the same size." << std::endl;
        system( "Pause" );
        exit( -1 );
    }

    double MSE = 0;
    double MAX_i = 255;

    for( size_t i = 0; i < a.size(); ++i )
    {
        MSE += ( a[i].r - b[i].r ) * ( a[i].r - b[i].r );
        MSE += ( a[i].g - b[i].g ) * ( a[i].g - b[i].g );
        MSE += ( a[i].b - b[i].b ) * ( a[i].b - b[i].b );
    }
    
    MSE = MSE / ( a.size() * 3.0 );

    double PSNR = 20 * std::log10( MAX_i ) - 10 * std::log10( MSE );

    std::cout << "MSE: " << MSE << " PSNR: " << PSNR << std::endl;

    system( "Pause" );

    return STATUS_OKAY;
}
