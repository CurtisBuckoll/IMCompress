/*========================================================================

    Name:     BmpDrawer.cpp

    Date:     June 2017

    Author:   Curtis Buckoll

    Overview:

========================================================================*/

#include "stdafx.h"
#include "BmpDrawer.h"

#include "Window.h"

#include <algorithm>
#include <iostream>

//========================================================================
//
BmpDrawer::BmpDrawer( BmpData data )
    : data_( std::move( data ) )
    , histogram_window_( nullptr )
    , generate_histogram_( true )
    , histogram_channel_( 0 )
    , generate_yuv_( true )
    , generate_grayscale_( true )
{

}

//========================================================================
//
BmpDrawer::~BmpDrawer()
{

}

//========================================================================
//
void BmpDrawer::writeToWindowBuffer( IWindow& frame )
{
    writeImgToBuffer( data_, frame );
}

//========================================================================
//
void BmpDrawer::advance()
{
}

//========================================================================
//
void BmpDrawer::writeImgToBuffer( const BmpData& data, 
                                  IWindow& frame )
{
    // 'data' contains a buffer of RGB  pixel values. 'frame' contains the
    // image buffer that we want to copy the data to. setPixel() sets one
    // RGB pixel value at location x, y in the output image. We need to
    // check bounds of data.pixels_ in the inside loop in case the data
    // and window buffers are not the same size.

    bool quit_loop = false;

    for( Uint y = 0; y < data.height_; ++y )
    {
        for( Uint x = 0; x < data.width_; ++x )
        {
            // Get the corresponding index of the RGB pixel value according
            // to the x, y values.
            Uint index = x + y * data.width_;

            if( index < data.pixels_.size() )
            {
                // Set the pixel.
                frame.setPixel( x, y, data.pixels_[x + y * data.width_] );
            }
            else
            {
                printMsg( BMP_DATA_OUT_RANGE );
                quit_loop = true;
                break;
            }
        }

        if( quit_loop ) break;
    }
}

//========================================================================
//
void BmpDrawer::writeTwoImagesToBuffer( const BmpData& im1,
                                        const BmpData& im2,
                                        IWindow& frame )
{
    // 'data' contains a buffer of RGB  pixel values. 'frame' contains the
    // image buffer that we want to copy the data to. setPixel() sets one
    // RGB pixel value at location x, y in the output image. We need to
    // check bounds of data.pixels_ in the inside loop in case the data
    // and window buffers are not the same size.

    bool quit_loop = false;

    for( Uint y = 0; y < im1.height_; ++y )
    {
        for( Uint x = 0; x < im1.width_; ++x )
        {
            // Get the corresponding index of the RGB pixel value according
            // to the x, y values.
            Uint index = x + y * im1.width_;

            if( index < im1.pixels_.size() )
            {
                // Set the pixel.
                frame.setPixel( x, y, im1.pixels_[x + y * im1.width_] );
            }
            else
            {
                printMsg( BMP_DATA_OUT_RANGE );
                quit_loop = true;
                break;
            }
        }

        if( quit_loop ) break;
    }

    quit_loop = false;

    for( Uint y = 0; y < im2.height_; ++y )
    {
        for( Uint x = 0; x < im2.width_; ++x )
        {
            // Get the corresponding index of the RGB pixel value according
            // to the x, y values.
            Uint index = x + y * im2.width_;

            if( index < im2.pixels_.size() )
            {
                // Set the pixel.
                frame.setPixel( x + im2.width_, y, im2.pixels_[x + y * im2.width_] );
            }
            else
            {
                printMsg( BMP_DATA_OUT_RANGE );
                quit_loop = true;
                break;
            }
        }

        if( quit_loop ) break;
    }
}

//========================================================================
//
std::vector<YUV> BmpDrawer::rgb2yuv( const std::vector<Color256>& pixels ) const
{
    std::vector<YUV> yuv;

    for( auto p : pixels )
    {
        YUV pix;

        pix.Y = static_cast<int32_t>( std::round( ( 0.299 * p.r ) + ( 0.587 * p.g ) + ( 0.114 * p.b ) ) );
        pix.U = static_cast<int32_t>( std::round( (-0.299 * p.r ) + (-0.587 * p.g ) + ( 0.886 * p.b ) ) );
        pix.V = static_cast<int32_t>( std::round( ( 0.701 * p.r ) + (-0.587 * p.g ) + (-0.114 * p.b ) ) );

        yuv.push_back( pix );
    }

    return yuv;
}

//========================================================================
//
std::vector<Color256> BmpDrawer::yuv2rgb( const std::vector<YUV>& pixels ) const
{
    std::vector<Color256> rgb;

    for( auto p : pixels )
    {
        Color256 pix;

        pix.r = static_cast<UByte>( std::round( ( 1.000 * p.Y ) + ( 0.000    * p.U ) + ( 1.000   * p.V ) ) );
        pix.g = static_cast<UByte>( std::round( ( 1.000 * p.Y ) + (-0.194208 * p.U ) + (-0.50937 * p.V ) ) );
        pix.b = static_cast<UByte>( std::round( ( 1.000 * p.Y ) + ( 1.000    * p.U ) + ( 0.000   * p.V ) ) );

        rgb.push_back( pix );
    }

    return rgb;
}
