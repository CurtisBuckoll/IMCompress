/*========================================================================

    Name:     Util.h

    Date:     June 2017

    Author:   Curtis Buckoll

    Overview:

        Contains useful program-wide definitions and functions.

========================================================================*/

#pragma once

//--------------------------------------------------------------

#include "Errors.h"
#include "Vertex.h"

#include <vector>
#include <string>
#include <stdint.h>
#include <array>

#include <numeric>

//--------------------------------------------------------------
//
using Uint  = unsigned int;
using UByte = unsigned char;
using Byte  = char;

namespace util
{
//--------------------------------------------------------------
//
enum class FILE_TYPE
{
    WAV = 0,
    BMP = 1,
    IM3 = 2,
    IN3 = 3
};

//========================================================================
//
inline MsgNum determineFileType( std::string file_path, FILE_TYPE& type )
{
    if( file_path.length() > 3 )
    {
        std::string file_ext = file_path.substr( file_path.length() - 3, 3 );

        if( file_ext == "wav" )
        {
            type = FILE_TYPE::WAV;
            return STATUS_OKAY;
        }
        else if( file_ext == "bmp" )
        {
            type = FILE_TYPE::BMP;
            return STATUS_OKAY;
        }
        else if( file_ext == "im3" )
        {
            type = FILE_TYPE::IM3;
            return STATUS_OKAY;
        }
        else if( file_ext == "in3" )
        {
            type = FILE_TYPE::IN3;
            return STATUS_OKAY;
        }
    }

    return INVALID_FILE_PATH;
}


//========================================================================
//
inline std::string getFileName( std::string file_path )
{
    std::string file_name;
    char c = file_path[file_path.length() - 1];

    Uint i = file_path.length() - 1;
    while( c != '\\' ) { c = file_path[--i]; }


    c = file_path[++i];

    while( c != '.' )
    {
        file_name += c;
        c = file_path[++i];
    }

    return file_name;
}


//========================================================================
//
inline std::vector<int16_t> rgb2yuv( const std::vector<UByte>& rgb )
{
    std::vector<int16_t> yuv;

    for( Uint i = 0; i + 2 < rgb.size(); i += 3 )
    {
        // In the raw bytestreams that we should be working with here,
        // the RGB componennts are backwards, so account for this here.
        const UByte r = rgb[i + 2];
        const UByte g = rgb[i + 1];
        const UByte b = rgb[i];

        yuv.push_back( static_cast<int32_t>( std::round( ( 0.299 * r ) + ( 0.587 * g ) + ( 0.114 * b ) ) ) );
        yuv.push_back( static_cast<int32_t>( std::round( (-0.299 * r ) + (-0.587 * g ) + ( 0.886 * b ) ) ) );
        yuv.push_back( static_cast<int32_t>( std::round( ( 0.701 * r ) + (-0.587 * g ) + (-0.114 * b ) ) ) );
    }

    return yuv;
}

//========================================================================
//
inline std::vector<UByte> yuv2rgb( const std::vector<int16_t>& yuv )
{
    std::vector<UByte> rgb;

    for( Uint i = 0; i + 2 < yuv.size(); i += 3 )
    {
        const int16_t Y = yuv[i];
        const int16_t U = yuv[i + 1];
        const int16_t V = yuv[i + 2];

        rgb.push_back( static_cast<UByte>( std::round( ( 1.000 * Y ) + ( 0.000    * U ) + ( 1.000   * V ) ) ) );
        rgb.push_back( static_cast<UByte>( std::round( ( 1.000 * Y ) + (-0.194208 * U ) + (-0.50937 * V ) ) ) );
        rgb.push_back( static_cast<UByte>( std::round( ( 1.000 * Y ) + ( 1.000    * U ) + ( 0.000   * V ) ) ) );
    }

    return rgb;
}

//========================================================================
//
inline std::vector<Color256> yuvArray2Color256( const std::array<std::vector<int16_t>, 3>& yuv )
{
    std::vector<Color256> rgb;

    if( yuv[0].size() != yuv[1].size() || yuv[0].size() != yuv[2].size() )
    {
        // output err
        std::cout << "Error in yuvArray2rgb() - sizes do not match." << std::endl;
    }

    for( Uint i = 0; i < yuv[0].size(); ++i )
    {
        const int16_t Y = yuv[0][i];
        const int16_t U = yuv[1][i];
        const int16_t V = yuv[2][i];

        double r = std::round( ( 1.000 * Y ) + (  0.000    * U ) + (  1.000   * V ) );
        double g = std::round( ( 1.000 * Y ) + ( -0.194208 * U ) + ( -0.50937 * V ) );
        double b = std::round( ( 1.000 * Y ) + (  1.000    * U ) + (  0.000   * V ) );

        // Make sure we end up within range [0,255]
        if( r < 0 ) r = 0; else if( r > 255 ) r = 255;
        if( g < 0 ) g = 0; else if( g > 255 ) g = 255;
        if( b < 0 ) b = 0; else if( b > 255 ) b = 255;

        rgb.emplace_back( static_cast<UByte>( r ), static_cast<UByte>( g ), static_cast<UByte>( b ) );

        //rgb.push_back( static_cast<UByte>( std::round( ( 1.000 * Y ) + ( 0.000    * U ) + ( 1.000   * V ) ) ) );
        //rgb.push_back( static_cast<UByte>( std::round( ( 1.000 * Y ) + ( -0.194208 * U ) + ( -0.50937 * V ) ) ) );
        //rgb.push_back( static_cast<UByte>( std::round( ( 1.000 * Y ) + ( 1.000    * U ) + ( 0.000   * V ) ) ) );
    }

    return rgb;
}

//========================================================================
//
inline std::vector<UByte> yuvArray2RGB( const std::array<std::vector<int16_t>, 3>& yuv )
{
    std::vector<UByte> rgb;

    if( yuv[0].size() != yuv[1].size() || yuv[0].size() != yuv[2].size() )
    {
        // output err
        std::cout << "Error in yuvArray2rgb() - sizes do not match." << std::endl;
    }

    for( Uint i = 0; i < yuv[0].size(); ++i )
    {
        const int16_t Y = yuv[0][i];
        const int16_t U = yuv[1][i];
        const int16_t V = yuv[2][i];

        double r = std::round( ( 1.000 * Y ) + ( 0.000    * U ) + ( 1.000   * V ) );
        double g = std::round( ( 1.000 * Y ) + ( -0.194208 * U ) + ( -0.50937 * V ) );
        double b = std::round( ( 1.000 * Y ) + ( 1.000    * U ) + ( 0.000   * V ) );

        // Make sure we end up within range [0,255]
        if( r < 0 ) r = 0; else if( r > 255 ) r = 255;
        if( g < 0 ) g = 0; else if( g > 255 ) g = 255;
        if( b < 0 ) b = 0; else if( b > 255 ) b = 255;

        rgb.emplace_back( static_cast<UByte>( r ) );
        rgb.emplace_back( static_cast<UByte>( g ) );
        rgb.emplace_back( static_cast<UByte>( b ) );
    }

    return rgb;
}

//========================================================================
//
inline void extractRGBComponents( const std::vector<UByte>& RGB, 
                                  std::vector<UByte>& r, 
                                  std::vector<UByte>& g, 
                                  std::vector<UByte>& b )
{
    r.clear();
    g.clear();
    b.clear();

    for( Uint i = 0; i + 2 < RGB.size(); i += 3 )
    {
        r.push_back( RGB[i] );
        g.push_back( RGB[i + 1] );
        b.push_back( RGB[i + 2] );
    }
}

//========================================================================
//
inline void combineRGBComponenets( std::vector<UByte>& RGB,
                                   const std::vector<UByte>& r,
                                   const std::vector<UByte>& g,
                                   const std::vector<UByte>& b )
{
    if( r.size() != g.size() || g.size() != b.size() )
    {
        // output err
        std::cout << "Error combining RGB componenets - sizes do not match." << std::endl;
    }

    RGB.clear();

    for( Uint i = 0; i < r.size(); ++i )
    {
        RGB.push_back( r[i] );
        RGB.push_back( g[i] );
        RGB.push_back( b[i] );
    }
}

//========================================================================
//
inline std::vector<Color256> RGB2Color256( const std::vector<UByte>& RGB )
{
    std::vector<Color256> col256arr;

    for( Uint i = 0; i + 2 < RGB.size(); i += 3 )
    {
        col256arr.emplace_back( RGB[i], RGB[i + 1], RGB[i + 2] );
    }

    return col256arr;
}

//========================================================================
//
inline void combineRGBComponenets( std::vector<Color256>& RGB,
                                   const std::vector<UByte>& r,
                                   const std::vector<UByte>& g,
                                   const std::vector<UByte>& b )
{
    if( r.size() != g.size() || g.size() != b.size() )
    {
        // output err
        std::cout << "Error combining RGB componenets - sizes do not match." << std::endl;
    }

    RGB.clear();

    for( Uint i = 0; i < r.size(); ++i )
    {
        RGB.push_back( Color256( r[i], g[i], b[i] ) );
    }
}

//========================================================================
//
inline void extractYUVComponents( const std::vector<int16_t>& YUV,
                                  std::vector<int16_t>& y,
                                  std::vector<int16_t>& u,
                                  std::vector<int16_t>& v )
{
    y.clear();
    u.clear();
    v.clear();

    for( Uint i = 0; i + 2 < YUV.size(); i += 3 )
    {
        y.push_back( YUV[i] );
        u.push_back( YUV[i + 1] );
        v.push_back( YUV[i + 2] );
    }
}

//========================================================================
//
inline std::vector<int16_t> downsampleChannel( const std::vector<int16_t>& channel, const Uint w, const Uint h )
{
    std::vector<int16_t> downsampled;

    if( w % 16 != 0 || h % 16 != 0 )
    {
        // output err
        std::cout << "Error in downsampleChannel(): The downsampled channel is not a multiple of 8, and will not encode correctly. This is fatal error and the program is terminating.. Image dimensions must be a multiple of 16." << std::endl;
        system( "Pause" );
        exit( -1 );
        return downsampled;
    }

    //downsampled.resize( channel.size() / 4 );

    std::array<int16_t, 4> to_average;

    for( size_t y = 0; y + 1 < h; y += 2 )
    {
        for( size_t x = 0; x + 1 < w; x += 2 )
        {
            to_average[0] = channel[x + y * w];
            to_average[1] = channel[( x + 1 ) + y * w];
            to_average[2] = channel[x + ( y + 1 ) * w];
            to_average[3] = channel[( x + 1 ) + ( y + 1 ) * w];

            int16_t avg = static_cast<int16_t>( std::round( static_cast<double>( std::accumulate( to_average.begin(), to_average.end(), 0 ) ) / to_average.size() ) );

            downsampled.push_back( avg );
        }
    }

    return downsampled;
}

//========================================================================
//
inline std::vector<int16_t> upsampleChannel( const std::vector<int16_t>& channel, const Uint w, const Uint h )
{
    std::vector<int16_t> upsampled;

    upsampled.resize( channel.size() * 4 );

    Uint channel_ind = 0;

    for( size_t y = 0; y < (h / 2); ++y )
    {
        for( size_t x = 0; x < (w / 2); ++x )
        {
            upsampled[2 * x + 2 * y * w]               = channel[channel_ind];
            upsampled[( 2 * x + 1 ) + 2 * y * w]       = channel[channel_ind];
            upsampled[2 * x + ( 2 * y + 1 ) * w]       = channel[channel_ind];
            upsampled[(2 * x + 1) + ( 2 * y + 1 ) * w] = channel[channel_ind];

            ++channel_ind;
        }
    }

    return upsampled;
}

};
