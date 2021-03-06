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
static const double kLOSSY_COMPRESSION_FACTOR = 2.0;

//========================================================================
//
int main( int argc, char** argv )
{
    // Get the file.
    std::vector<Byte> data;
    OpenFileDialog file_explorer( "Select A File" );

    MsgNum err = file_explorer.invoke( data );
    if( err ) return printMsg( err );

    // Check file extension.
    util::FILE_TYPE file_type;
    err = util::determineFileType( file_explorer.getFilePath(), file_type );
    if( err ) return printMsg( err );

    // Set up.
    std::vector<UByte> encoded_data;
    std::unique_ptr<IWindow> window( nullptr );
    BmpData uncompressedImg;
    BmpData finalData;

    // Branch on file type.

    switch( file_type )
    {
    // ------------------------------------------------------------------------
    case util::FILE_TYPE::BMP:
    {
        char choice = '\0';

        // Ask for lossy/lossless mode.
        std::cout << "Enter L/l for lossy (.in3) encode or N/n for lossless encode (.im3): ";

        while( choice != 'N' && choice != 'L' )
        {
            std::string ip;
            std::getline( std::cin, ip );
            choice = ip[0];

            // Generalize to upper case.
            if( choice >= 97 ) choice -= 32;

            // Decode the .bmp first.
            BmpDecoder bmp_decoder( data );
            bmp_decoder.decode();

            // Check input and execute accordingly.
            switch( choice )
            {
            case 'N':
            {
                std::cout << "Lossless mode selected.\n" << std::endl;
                std::cout << "Compressing to .in3 format.." << std::endl;

                IN3Coder in3coder;

                uint32_t time_before = SDL_GetTicks();

                in3coder.encode( bmp_decoder.getData(), encoded_data );
                std::cout << "Encode Run Time:\t\t" << (double)( SDL_GetTicks() - time_before ) / 1000 << "\tseconds" << std::endl;
                std::cout << "Compression Ratio:\t\t" << (double)( bmp_decoder.getData().body_.size() + bmp_decoder.getData().header_.size() ) / encoded_data.size() << std::endl;
                std::cout << "Original file data size:\t" << bmp_decoder.getData().body_.size() << "\tbytes" << std::endl;
                std::cout << "Compressed file data size:\t" << encoded_data.size() << "\tbytes" << std::endl;

                // Write the file to output.
                std::string file_name = util::getFileName( file_explorer.getFilePath() );

                std::ofstream outFile;
                outFile.open( file_name + ".in3", std::ios::out | std::ios::binary );
                if( !encoded_data.empty() )
                {
                    outFile.write( (char*)&encoded_data[0], encoded_data.size() * sizeof( UByte ) );
                }

                uncompressedImg = bmp_decoder.releaseData();

                // Decode and time.
                time_before = SDL_GetTicks();
                in3coder.decode( encoded_data, finalData );
                std::cout << "Decode Run Time:\t\t" << (double)( SDL_GetTicks() - time_before ) / 1000 << std::endl;
                break;
            }
            case 'L':
            {
                std::cout << "Lossy mode selected.\n" << std::endl;
                std::cout << "Compressing to .im3 format.." << std::endl;

                IM3Coder im3coder( kLOSSY_COMPRESSION_FACTOR );

                uint32_t time_before = SDL_GetTicks();

                im3coder.encode( bmp_decoder.getData(), encoded_data );
                std::cout << "Encode Run Time:\t\t" << (double)( SDL_GetTicks() - time_before ) / 1000 << "\tseconds" << std::endl;
                std::cout << "Compression Ratio:\t\t" << (double)( bmp_decoder.getData().body_.size() + bmp_decoder.getData().header_.size() ) / encoded_data.size() << std::endl;
                std::cout << "Original file data size:\t" << bmp_decoder.getData().body_.size() << "\tbytes" << std::endl;
                std::cout << "Compressed file data size:\t" << encoded_data.size() << "\tbytes" << std::endl;

                // Write the file to output.
                std::string file_name = util::getFileName( file_explorer.getFilePath() );

                std::ofstream outFile;
                outFile.open( file_name + ".im3", std::ios::out | std::ios::binary );
                if( !encoded_data.empty() )
                {
                    outFile.write( (char*)&encoded_data[0], encoded_data.size() * sizeof( UByte ) );
                }

                uncompressedImg = bmp_decoder.releaseData();

                // Decode and time.
                time_before = SDL_GetTicks();
                im3coder.decode( encoded_data, finalData );
                std::cout << "Decode Run Time:\t\t" << (double)( SDL_GetTicks() - time_before ) / 1000 << std::endl;
                break;
            }
            default:
                std::cout << "Please enter a valid selection: ";
            }
        }

        std::unique_ptr<BmpDrawer> object_drawer( nullptr );
        object_drawer.reset( new BmpDrawer( finalData ) );

        // Render the first frame prior to the control loop.
        std::cout << "Displaying the uncompressed image (left) and the compressed image (right)." << std::endl;
        window.reset( new Window( uncompressedImg.width_ * 2, uncompressedImg.height_, "Uncompressed/Compressed Image" ) );
        window->init();
        object_drawer->writeTwoImagesToBuffer( uncompressedImg, finalData, *window );

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

        // All done. Window destructor will manage that object,
        SDL_Quit();
        return 0;
    }
    // ------------------------------------------------------------------------
    case util::FILE_TYPE::IM3:
    {
        IM3Coder im3coder( kLOSSY_COMPRESSION_FACTOR );

        // Just copy the image buffer for now.
        for( auto b : data )
        {
            encoded_data.push_back( static_cast<UByte>( b ) );
        }
       
        BmpData finalData;

        uint32_t time_before = SDL_GetTicks();

        im3coder.decode( encoded_data, finalData );

        std::cout << "Decode Run Time: " << (double)( SDL_GetTicks() - time_before ) / 1000 << std::endl;

        std::unique_ptr<BmpDrawer> object_drawer( nullptr );
        object_drawer.reset( new BmpDrawer( finalData ) );

        // Render the first frame prior to the control loop.
        std::cout << "Displaying the image." << std::endl;
        window.reset( new Window( finalData.width_, finalData.height_, "CMPT365 Assignment 2 - Part 2: Lossy Compression" ) );
        window->init();
        object_drawer->writeToWindowBuffer( *window );

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

        // All done. Window destructor will manage that object,
        SDL_Quit();
        return 0;
    }
    // ------------------------------------------------------------------------
    case util::FILE_TYPE::IN3:
    {
        IN3Coder in3coder;

        // Just copy the image buffer for now.
        for( auto b : data )
        {
            encoded_data.push_back( static_cast<UByte>( b ) );
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
    // ------------------------------------------------------------------------
    default:
        // Shouldn't ever reach here.
        break;
    }
}
