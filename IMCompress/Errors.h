/*========================================================================

    Name:     Errors.h

    Date:     June 2017

    Author:   Curtis Buckoll

    Overview:

        To report run-time errors and return the error code.

========================================================================*/

#pragma once

//--------------------------------------------------------------

#include <iostream>

//--------------------------------------------------------------
//
enum MsgNum
{
    STATUS_OKAY          = 0,
    INVALID_FILE_PATH    = 1,
    FAILURE_READING_FILE = 2,
    INVALID_FILE         = 3,
    WAV_DATA_OUT_RANGE   = 4,
    BMP_DATA_OUT_RANGE   = 5,
    BAD_DATA             = 6,
    BAD_WAV_BIT_DEPTH    = 7,
    HUFFMAN_ERROR        = 8
};

//--------------------------------------------------------------
//
inline MsgNum printMsg( MsgNum err )
{
    switch( err )
    {
    case INVALID_FILE_PATH :
        std::cout << "Error: Invalid file path detected. Exiting." << std::endl;
        break;

    case FAILURE_READING_FILE :
        std::cout << "Error: Failure to read file. Exiting" << std::endl;
        break;

    case INVALID_FILE:
        std::cout << "Error: Invalid file - file must have extension .bmp or .wav. Exiting." << std::endl;
        break;

    case WAV_DATA_OUT_RANGE:
        std::cout << "Error: Attempted to decode .wav data out of range." << std::endl;
        break;

    case BMP_DATA_OUT_RANGE:
        std::cout << "Error: Attempted to draw .bmp pixel out of image buffer range." << std::endl;
        break;

    case BAD_DATA:
        std::cout << "Error: The provided file is poorly formed or is not the correct file type. Exiting" << std::endl;
        break;

    case BAD_WAV_BIT_DEPTH:
        std::cout << "Error: Cannot decode the audio file: The bit depth must be a multiple of 8." << std::endl;
        break;

    case HUFFMAN_ERROR:
        std::cout << "Error: Encountered a problem while performing Huffman encoding/decoding. Exiting. " << std::endl;
        break;

    default:
        break;
    }

    return err;
}
