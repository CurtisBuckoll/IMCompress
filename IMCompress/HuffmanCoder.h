#pragma once

#include "Util.h"
#include <vector>
#include <unordered_map>

//--------------------------------------------------------------
//
template<typename Sym>
struct CompressedSymbol
{
    Sym         new_sym_;
    Uint        sym_len_;
    std::string sym_str_;
};

//--------------------------------------------------------------
//
template<typename Sym>
using SymbolTable = std::unordered_map<Sym, CompressedSymbol<Sym>>;

//--------------------------------------------------------------
//
template<typename Sym>
struct Node 
{
    Node( Sym symbol )
        : symbol_( symbol )
    {
        child_[0] = nullptr;
        child_[1] = nullptr;
    }

    Node* child_[2];
    Sym   symbol_;
};

//--------------------------------------------------------------
//
template<typename Sym>
struct HuffmanTree
{
    HuffmanTree();

    MsgNum constructSymbolTable();

    Node<Sym>*       root_;
    SymbolTable<Sym> sym_table_;
    uint16_t         max_depth_;

private:

    void traverse( Node<Sym>* node, 
                   const std::string& currBinStr );

    Sym str2bin( const std::string& binStr );
};

//--------------------------------------------------------------
//
struct DecoderLUTEntry
{
    DecoderLUTEntry()
        : old_sym_( 0 )
        , new_sym_( 0 )
        , new_sym_len_( 0 )
    { }

    DecoderLUTEntry( UByte old_sym, uint64_t new_sym, UByte new_sym_len )
        : old_sym_( old_sym )
        , new_sym_( new_sym )
        , new_sym_len_( new_sym_len )
    { }

    UByte old_sym_;
    uint64_t new_sym_;
    UByte new_sym_len_;
};

//--------------------------------------------------------------
//
class BitReader
{
public:

    //--------------------------------------------------------------
    //
    BitReader( const std::vector<UByte>& bit_stream );

    //--------------------------------------------------------------
    // Returns true on success.
    uint64_t read_bits( Uint numBits, bool& success );

private:

    const std::vector<UByte>& stream_;
    UByte                     curr_byte_;
    Uint                      byte_index_;
    Uint                      bit_index_;
};  

//--------------------------------------------------------------
//
struct DecoderParameters
{
    // Maximum codeword length, which is equal to the height
    // of the Huffman tree.
    uint16_t                     max_cw_len_;

    // Number of bytes (the length) of the supplied input.
    uint64_t                     num_bytes_;

    // The data needed to construct the fast version of the 
    // lookup table during decode.
    std::vector<DecoderLUTEntry> decoder_LUT_;
};

//--------------------------------------------------------------
//
class HuffmanCoder
{
public:

    //--------------------------------------------------------------
    //
    HuffmanCoder();

    //--------------------------------------------------------------
    //
    ~HuffmanCoder();


    //--------------------------------------------------------------
    // Performs huffman encoding, but operates per byte.
    // Decoder params to be populated so we can store with
    // the encoded file.
    MsgNum encodePerByte( const std::vector<UByte>& inData,
                          std::vector<UByte>& outData,
                          DecoderParameters& params );

    //--------------------------------------------------------------
    //
    MsgNum decode( const std::vector<UByte>& inData,
                   std::vector<UByte>& outData,
                   const DecoderParameters& dec_params );
};
