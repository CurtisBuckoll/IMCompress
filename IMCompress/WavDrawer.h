/*========================================================================

    Name:     WavDrawer.h

    Date:     June 2017

    Author:   Curtis Buckoll

    Overview:

        Implements the IDrawer interface. Draws the waveform data of a
    .wav file to the screen and outputs relevant information to the
    terminal.

========================================================================*/

#pragma once

#include "IDrawer.h"
#include "Util.h"
#include "WavDecoder.h"
#include "Vertex.h"

// Forward declarations
class IWindow;

//========================================================================
// WavDrawer
class WavDrawer : public IDrawer
{
public:

    //--------------------------------------------------------------
    //
    WavDrawer( WavData data );

    //--------------------------------------------------------------
    //
    ~WavDrawer();

    //--------------------------------------------------------------
    // Draw the visual representation of the .wav file to the
    // supplied pixel buffer contained within the frame argument.
    void writeToWindowBuffer( IWindow& frame ) override;

    //--------------------------------------------------------------
    // This method from the superclass is not needed here.
    void advance() override { /* Do nothing. */ };


private:

    //--------------------------------------------------------------
    //
    void drawVerticalLine( IWindow& frame, 
                           int64_t sample, 
                           Uint x, 
                           double scale );
    
    //==============================================================
    // Private Data Members
    //==============================================================

    Color256            wav_colour_;
    uint64_t            max_val_;
    WavData             data_;

    Uint                half_screen_height_;
    Uint                max_waveform_screen_height_;
};

