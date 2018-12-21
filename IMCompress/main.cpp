#include "stdafx.h"
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
#include "PSNRMeasure.h"

#include <memory>
#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>

//========================================================================
//
enum PART_TO_EXECUTE {
    Q1 = 0,
    Q2 = 1,
    Q3 = 2
};

//========================================================================
//
static const PART_TO_EXECUTE assignment_part = Q2;

//========================================================================
//
int main( int argc, char** argv )
{
    // Get the file.
    std::vector<Byte> data;
    OpenFileDialog file_explorer( "Select A File" );

    MsgNum err = file_explorer.invoke( data );
    if( err ) return printMsg( err );

    // Check file extension in case something different is opened.
    util::FILE_TYPE file_type;
    err = util::determineFileType( file_explorer.getFilePath(), file_type );
    if( err ) return printMsg( err );

    // For part 1 - decoding .wav data.
    if( assignment_part == Q1 )
    {
        if( file_type != util::FILE_TYPE::WAV  )
        {
            std::cout << "File must be .wav. Exiting" << std::endl;
            return 0;
        }

        WavDecoder decoder( data );
        decoder.decode();

        HuffmanCoder huffCoder;
        DecoderParameters huffman_params;

        std::vector<UByte> huffCodedData;
        huffCoder.encodePerByte( decoder.getData().samples_as_bytes_, huffCodedData, huffman_params );
        
        std::cout << "Huffman compression ratio: " << (double)( decoder.getData().samples_as_bytes_.size() ) / huffCodedData.size() << std::endl;

        LZWCoder lzwCoder;

        std::vector<UByte> lzwCodedData;
        lzwCoder.encode( huffCodedData, lzwCodedData );
        std::cout << "LZW compression ratio: " << (double)( huffCodedData.size() ) / lzwCodedData.size() << std::endl;

        system( "Pause" );

        return 0;
    }

    // For part 2 - Lossless encode/decode
    if( assignment_part == Q2 )
    {
        if( file_type != util::FILE_TYPE::IM3 && file_type != util::FILE_TYPE:: BMP )
        {
            std::cout << "File must be either .bmp or .im3. Exiting." << std::endl;
            return 0;
        }

        IM3Coder im3coder( 2.0 );
        std::vector<UByte> encoded_data;
        std::unique_ptr<IWindow> window( nullptr );
        BmpData uncompressedImg;

        // If it is .bmp, we need to decode it first.
        if( file_type == util::FILE_TYPE::BMP )
        {
            std::cout << "Compressing to .im3 format.." << std::endl;

            BmpDecoder bmp_decoder( data );
            bmp_decoder.decode();

            uint32_t time_before = SDL_GetTicks();

            im3coder.encode( bmp_decoder.getData(), encoded_data );

            std::cout << "Encode Run Time: " << (double)( SDL_GetTicks() - time_before ) / 1000 << std::endl;

            std::cout << "Compression Ratio: " << (double)( bmp_decoder.getData().body_.size() + bmp_decoder.getData().header_.size() ) / encoded_data.size() << std::endl;

            // Write the file to output.
            std::string file_name = util::getFileName( file_explorer.getFilePath() );

            std::ofstream outFile;
            outFile.open( file_name + ".im3", std::ios::out | std::ios::binary );
            if( !encoded_data.empty() )
            {
                outFile.write( (char*)&encoded_data[0], encoded_data.size() * sizeof( UByte ) );
            }

            uncompressedImg = bmp_decoder.releaseData();
        }
        else
        {
            for( auto b : data )
            {
                encoded_data.push_back( static_cast<UByte>( b ) );
            }
        }

        BmpData finalData;

        uint32_t time_before = SDL_GetTicks();

        im3coder.decode( encoded_data, finalData );

        std::cout << "Decode Run Time: " << (double)( SDL_GetTicks() - time_before ) / 1000 << std::endl;

        std::unique_ptr<BmpDrawer> object_drawer( nullptr );
        object_drawer.reset( new BmpDrawer( finalData ) );

        // Render the first frame prior to the control loop.
        if( file_type == util::FILE_TYPE::BMP )
        {
            std::cout << "Displaying the uncompressed image (left) and the compressed image (right)." << std::endl;
            window.reset( new Window( uncompressedImg.width_ * 2, uncompressedImg.height_, "CMPT365 Assignment 2 - Part 2: Lossy Compression" ) );
            window->init();
            object_drawer->writeTwoImagesToBuffer( uncompressedImg, finalData, *window );
        }
        else
        {
            std::cout << "Displaying the image." << std::endl;
            window.reset( new Window( finalData.width_, finalData.height_, "CMPT365 Assignment 2 - Part 2: Lossy Compression" ) );
            window->init();
            object_drawer->writeToWindowBuffer( *window );
        }

        window->RenderFrame();

        bool running = true;
        SDL_Event evnt;

        // loop through and refresh the screen if we see a keyboard event.
        while ( running )
        {
            while( SDL_PollEvent( &evnt ) )
            {
                if( evnt.type == SDL_QUIT )
                {
                    running = false;
                }
                else if ( evnt.type == SDL_KEYDOWN )
                {
                    running = false;
                }
            }
        }

        // All done. Window destructor will manage that object,
        SDL_Quit();
        return 0;
    }

    // For part 3 - encode/decode .bmp data.
    if( assignment_part == Q3 )
    {
        if( file_type != util::FILE_TYPE::IN3 && file_type != util::FILE_TYPE::BMP )
        {
            std::cout << "File must be either .bmp or .in3. Exiting." << std::endl;
            return 0;
        }

        IN3Coder in3coder;
        std::vector<UByte> encoded_data;
        std::unique_ptr<IWindow> window( nullptr );
        BmpData uncompressedImg;

        // If it is .bmp, we need to decode it first.
        if( file_type == util::FILE_TYPE::BMP )
        {
            std::cout << "Compressing to .in3 format.." << std::endl;

            BmpDecoder bmp_decoder( data );
            bmp_decoder.decode();

            uint32_t time_before = SDL_GetTicks();

            in3coder.encode( bmp_decoder.getData(), encoded_data );

            std::cout << "Encode Run Time: " << (double)( SDL_GetTicks() - time_before ) / 1000 << std::endl;

            std::cout << "Compression Ratio: " << (double)( bmp_decoder.getData().body_.size() + bmp_decoder.getData().header_.size() ) / encoded_data.size() << std::endl;

            // Write the file to output.
            std::string file_name = util::getFileName( file_explorer.getFilePath() );

            std::ofstream outFile;
            outFile.open( file_name + ".in3", std::ios::out | std::ios::binary );
            if( !encoded_data.empty() )
            {
                outFile.write( (char*)&encoded_data[0], encoded_data.size() * sizeof( UByte ) );
            }

            uncompressedImg = bmp_decoder.releaseData();
        }
        else
        {
            for( auto b : data )
            {
                encoded_data.push_back( static_cast<UByte>( b ) );
            }
        }

        BmpData finalData;

        uint32_t time_before = SDL_GetTicks();

        in3coder.decode( encoded_data, finalData );

        std::cout << "Decode Run Time: " << (double)( SDL_GetTicks() - time_before ) / 1000 << std::endl;

        std::unique_ptr<BmpDrawer> object_drawer( nullptr );
        object_drawer.reset( new BmpDrawer( finalData ) );

        // Render the first frame prior to the control loop.
        if( file_type == util::FILE_TYPE::BMP )
        {
            std::cout << "Displaying the uncompressed image (left) and the compressed image (right)." << std::endl;
            window.reset( new Window( uncompressedImg.width_ * 2, uncompressedImg.height_, "CMPT365 Assignment 2 - Part 3: Lossless Compression" ) );
            window->init();
            object_drawer->writeTwoImagesToBuffer( uncompressedImg, finalData, *window );
        }
        else
        {
            std::cout << "Displaying the image." << std::endl;
            window.reset( new Window( finalData.width_, finalData.height_, "CMPT365 Assignment 2 - Part 3: Lossless Compression" ) );
            window->init();
            object_drawer->writeToWindowBuffer( *window );
        }

        window->RenderFrame();

        bool running = true;
        SDL_Event evnt;

        // loop through and refresh the screen if we see a keyboard event.
        while( running )
        {
            while( SDL_PollEvent( &evnt ) )
            {
                if( evnt.type == SDL_QUIT )
                {
                    running = false;
                }
                else if( evnt.type == SDL_KEYDOWN )
                {
                    running = false;
                }
            }
        }

        // All done. Window destructor will manage that object.
        SDL_Quit();
        return 0;
    }
}
