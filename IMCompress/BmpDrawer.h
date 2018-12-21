/*========================================================================

    Name:     BmpDrawer.h

    Date:     June 2017

    Author:   Curtis Buckoll

    Overview:

        Implements the IDrawer interface. Handles the writing of image
    array data to the frame buffer to be drawn to the screen.

========================================================================*/

#pragma once

//--------------------------------------------------------------

#include "IDrawer.h"
#include "Util.h"
#include "BmpDecoder.h"
#include "Vertex.h"

//--------------------------------------------------------------
// Forward declarations
class IWindow;

//--------------------------------------------------------------
//
struct DitherMatrix
{
    //--------------------------------------------------------------
    // Fix the size in the default ctor: 8 x 8.
    DitherMatrix()
        : size_( 8 )
        , mat_( size_, std::vector<UByte>(size_, 0))
    {
        mat_[0] = { 42, 10, 34,  2, 40,  8, 32,  0 };
        mat_[1] = { 26, 58, 18, 50, 24, 56, 16, 48 };
        mat_[2] = { 38,  6, 46, 14, 36,  4, 44, 12 };
        mat_[3] = { 22, 54, 30, 62, 20, 52, 28, 60 };
        mat_[4] = { 41,  9, 33,  1, 43, 11, 35,  3 };
        mat_[5] = { 25, 57, 17, 49, 27, 59, 19, 51 };
        mat_[6] = { 37,  5, 45, 13, 39,  7, 47, 15 };
        mat_[7] = { 21, 53, 29, 61, 23, 55, 31, 63 };
    }

    //--------------------------------------------------------------
    //
    Uint                            size_;
    std::vector<std::vector<UByte>> mat_;
};

//========================================================================
// WavDrawer
class BmpDrawer : public IDrawer
{
public:

    //--------------------------------------------------------------
    //
    BmpDrawer( BmpData data );

    //--------------------------------------------------------------
    //
    ~BmpDrawer();

    //--------------------------------------------------------------
    //
    void writeToWindowBuffer( IWindow& frame ) override;

    //--------------------------------------------------------------
    //
    void writeTwoImagesToBuffer( const BmpData& im1,
                                 const BmpData& im2,
                                 IWindow& frame );


    //--------------------------------------------------------------
    //
    void advance() override;


private:

    //--------------------------------------------------------------
    //
    void writeImgToBuffer( const BmpData& data, 
                           IWindow& frame );

    //--------------------------------------------------------------
    //
    std::vector<YUV> rgb2yuv( const std::vector<Color256>& pixels ) const;

    //--------------------------------------------------------------
    //
    std::vector<Color256> yuv2rgb( const std::vector<YUV>& pixels ) const;


    //==============================================================
    // Private Data Members
    //==============================================================

    BmpData               data_;
    std::vector<YUV>      yuv_;
    bool                  generate_yuv_;
    BmpData               grayscale_data_;
    bool                  generate_grayscale_;

    //--------------------------------------------------------------
    // To store the information needed for the histogram displays.
    struct RGBDistribution
    {
        //--------------------------------------------------------------
        //
        RGBDistribution()
            : R_( 256, 0 )
            , G_( 256, 0 )
            , B_( 256, 0 )
            , max_val_R_( 0 )
            , max_val_G_( 0 )
            , max_val_B_( 0 )
            , max_( 0 )
        { }

        //--------------------------------------------------------------
        //
        std::vector<Uint> R_;
        std::vector<Uint> G_;
        std::vector<Uint> B_;

        Uint max_val_R_;
        Uint max_val_G_;
        Uint max_val_B_;
        Uint max_;
    };

    RGBDistribution       rgb_dist_;
    IWindow*              histogram_window_;
    bool                  generate_histogram_;
    Uint                  histogram_channel_;

};
