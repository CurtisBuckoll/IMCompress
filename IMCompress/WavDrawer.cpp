/*========================================================================

    Name:     WavDrawer.cpp

    Date:     June 2017

    Author:   Curtis Buckoll

    Overview:

========================================================================*/

#include "stdafx.h"
#include "WavDrawer.h"

#include "IWindow.h"

#include <algorithm>
#include <iostream>


//========================================================================
//
WavDrawer::WavDrawer( WavData data )
    : wav_colour_( 10, 200, 100 )
    , data_( std::move(data) )
    , half_screen_height_( 0 )
    , max_waveform_screen_height_( 200 )
{
    // Let's get the maximum absolute value of the data here. This is 
    // used later for scaling the display.
    auto absval_comparator = []( int64_t a, int64_t b ) -> bool
    {
        return std::abs( a ) < std::abs( b );
    };

    max_val_ = std::abs( *std::max_element( data_.samples_.begin(),
                                            data_.samples_.end(),
                                            absval_comparator ) );
}

//========================================================================
//
WavDrawer::~WavDrawer()
{

}

//========================================================================
//
inline void WavDrawer::drawVerticalLine( IWindow& frame, 
                                         int64_t sample,
                                         Uint x, 
                                         double scale )
{
    // Compute the height of the line based on the maximum pixel height 
    // we want to fill and the supplied scale argument. The scale 
    // variable depends on the maximum absolute value found in the audio 
    // data.
    double max_height = max_waveform_screen_height_ * scale;

    // Performs ( abs( sample ) / 2^(k-1) ) where k is the bit depth.
    double ratio = static_cast<double>( abs( sample ) ) / ( 0x1 << ( data_.bits_per_sample_ - 1 ) );

    // The number of vertical pixels to write.
    Uint numPix = static_cast<Uint>( round( ratio * max_height ) );

    // Start at the vertical center of the window. If the sample is -ve 
    // then we will write towards the bottom of the screen. If the sample 
    // is +ve then write towards to the top.
    int direction = sample < 0 ? -1 : 1;

    Uint y = half_screen_height_;

    // Write the pixels.
    for( Uint i = 0; i < numPix; ++i )
    {
        frame.setPixel( x, y, wav_colour_ );
        y += direction;
    }
}

//========================================================================
//
void WavDrawer::writeToWindowBuffer( IWindow& frame )
{
    // Performs: scale = 2^(bit_depth - 1) / maxVal
    double scale = ( 0x1 << ( data_.bits_per_sample_ - 1 ) ) / static_cast<double>( max_val_ );

    Uint x = 0;
    half_screen_height_ = frame.getHeight() / 2;

    // For a bit of optimization..
    Uint      prev_x = 0;
    int64_t max_samp = 0;
    int64_t min_samp = 0;

    for( auto sample : data_.samples_ )
    {
        // Compute the current x coordinate based on window width and the number of samples in the audio file.
        Uint xCoord = static_cast<Uint>( round( ( x / static_cast<double>( data_.samples_.size() ) ) * frame.getWidth() ) );

        // If we are trying to draw a sample less than or greater than the 
        // max/min resp. for which we have already drawn at the same computed 
        // x-coordinate, then don't draw it (there's no point)
        if( prev_x == xCoord )
        {
            if( sample >= max_samp )
            {
                max_samp = sample;
            }
            else if( sample < min_samp )
            {
                min_samp = sample;
            }
            else
            {
                ++x;
                continue;
            }
        }
        else
        {
            max_samp = 0;
            min_samp = 0;
            prev_x = xCoord;
        }

        drawVerticalLine( frame, sample, xCoord, scale );
        ++x;
    }

    return;
}
