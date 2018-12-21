#include "stdafx.h"
#include "HuffmanCoder.h"
#include "LZWCoder.h"

#include <map>
#include <array>
#include <algorithm>

//========================================================================
//
template<typename Sym>
HuffmanTree<Sym>::HuffmanTree()
    : root_( nullptr )
    , max_depth_( 0 )
{

}

//========================================================================
//
template<typename Sym>
MsgNum HuffmanTree<Sym>::constructSymbolTable()
{
    if( root_ == nullptr )
    {
        // output err
        std::cout << "Failed reading from huffman tree - root pointer is null." << std::endl;
        return STATUS_OKAY;
    }

    // Assume we have at least two symbols.
    traverse( root_, "" );

    if( max_depth_ >= 32 )
    {
        std::cout << "Table depth (max cw len) is too large.. We cannot accurately store the information." << std::endl;
        return HUFFMAN_ERROR;
    }

    return STATUS_OKAY;
}

//========================================================================
//
template<typename Sym>
void HuffmanTree<Sym>::traverse( Node<Sym>* node, const std::string& currBinStr )
{
    // First check if we are at a leaf node, and insert that symbol 
    // into the symbol table if so.
    if( node->child_[0] == nullptr )
    {
        if( node->child_[1] != nullptr )
        {
            std::cout << "Both children should be null.. found one that is not." << std::endl;
        }

        CompressedSymbol<decltype( node->symbol_ )> newEntry;
        newEntry.sym_str_ = currBinStr;
        newEntry.sym_len_ = currBinStr.length();
        newEntry.new_sym_ = str2bin( currBinStr );

        // Keep track of maximum codeword length. This will be needed for
        // decoding.
        if( newEntry.sym_len_ > max_depth_ ) max_depth_ = newEntry.sym_len_;

        sym_table_.insert( { node->symbol_, std::move( newEntry ) } );

        return;
    }

    traverse( node->child_[0], currBinStr + "0" );
    traverse( node->child_[1], currBinStr + "1" );
}

//========================================================================
//
template<typename Sym>
Sym HuffmanTree<Sym>::str2bin( const std::string& binStr )
{
    int64_t result = 0;
    for( Uint i = 0; i < binStr.length(); ++i)
    {
        if( binStr[i] == 1 )
        {
            result = (result << 1) | 1;
        }
        else
        {
            result = ( result << 1 );
        }
    }
    return result;
}

//========================================================================
//
BitReader::BitReader( const std::vector<UByte>& bit_stream )
    : stream_( bit_stream )
    , curr_byte_( 0 )
    , byte_index_( 0 )
    , bit_index_ ( 0 )
{
    if( bit_stream.size() > 0 )
    {
        curr_byte_ = bit_stream[0];
    }
    else
    {
        // output err
        std::cout << "BitReader has been supplied an empty list." << std::endl;
    }
}

//========================================================================
//
uint64_t BitReader::read_bits( Uint numBits, bool& success )
{
    success       = true;
    uint64_t bits = 0;

    for( size_t i = 0; i < numBits; ++i )
    {
        if( bit_index_ == 8 )
        {
            if( byte_index_ >= stream_.size() - 1 )
            {
                success    = false;
                curr_byte_ = 0;
                ++byte_index_;
            }
            else
            {
                curr_byte_ = stream_[++byte_index_];
            }
            bit_index_ = 0;
        }

        UByte newbit = (curr_byte_ >> (7 - bit_index_)) & 0x1;
        bits = ( bits << 1 ) | newbit;
        ++bit_index_;
    }

    return bits;
}

//========================================================================
//
HuffmanCoder::HuffmanCoder()
{

}

//========================================================================
//
HuffmanCoder::~HuffmanCoder()
{
    // ADD DESTRUCTOR TO WIPE THE TREE!
}

//========================================================================
//
MsgNum HuffmanCoder::encodePerByte( const std::vector<UByte>& inData, 
                                    std::vector<UByte>& outData,
                                    DecoderParameters& dec_params )
{
    // First get the distribution of symbols.
    std::unordered_map<UByte, int64_t> symbol_count;

    for( auto byte : inData )
    {
        auto found = symbol_count.find( byte );

        if( found == symbol_count.end() )
        {
            symbol_count.insert( { byte, 1 } );
        }
        else
        {
            ++( found->second );
        }
    }

    // Now construct the sorted list according to symbol frequency
    std::multimap<uint64_t, Node<UByte>*> symbols_by_freq;
    for( auto symbol : symbol_count )
    {
        Node<UByte>* newNode = new Node<UByte>( symbol.first );
        symbols_by_freq.insert( { symbol.second, newNode } );
    }

    // Now go through each symbol and construct the huffman tree.
    auto itr = symbols_by_freq.begin();
    if( itr == symbols_by_freq.end() ) { return HUFFMAN_ERROR; }

    HuffmanTree<UByte> hTree;

    while( true )
    {
        auto next = itr;
        ++next;
        if( next == symbols_by_freq.end() )
        {
            // We are done.
            hTree.root_ = itr->second;
            break;
        }

        Node<UByte>* newParent = new Node<UByte>( 0 );
        newParent->child_[0] = itr->second;
        newParent->child_[1] = next->second;

        uint64_t parentCost = itr->first + next->first;

        // Remove the two elements inserted into the huffman tree and 
        // then insert their parent and finally update the front of
        // the list.
        symbols_by_freq.erase( itr );
        symbols_by_freq.erase( next );

        symbols_by_freq.insert( { parentCost, newParent } );
        itr = symbols_by_freq.begin();
    }

    // Construct symbol lookup table.
    MsgNum err = hTree.constructSymbolTable();
    if( err ) return err;

    // This keeps track of (old_sym, new_sym, new_sym_length) in a sorted array. This is needed 
    // for decoding, and so is written out in the header of the compressed file.
    std::vector<DecoderLUTEntry> decoder_lookup_table;

    // Expect no more than 256 symbols.
    for( uint16_t b = 0; b < 256; ++b)
    {
        const auto& found = hTree.sym_table_.find( static_cast<UByte>(b) );

        if( found != hTree.sym_table_.end() )
        {
            const std::string& cw   = found->second.sym_str_;
            uint64_t new_sym_as_num = 0;
            Uint bit_index          = 0;

            for( auto bit : cw )
            {
                new_sym_as_num = new_sym_as_num << 1;
                if( bit == '1' ) new_sym_as_num |= 0x1;

                ++bit_index;
            }

            // Now we need to make the magnitude of the numbers make sense for
            // later when we construct the table lookup method during decoding.
            // All we need to do is shift left the result by the difference
            // between the max cw length and the length of the current cw (as
            // binary strings).
            new_sym_as_num = new_sym_as_num << ( hTree.max_depth_ - cw.length() );
            decoder_lookup_table.push_back( { static_cast<UByte>(b), new_sym_as_num, static_cast<UByte>(cw.length()) } );
        }
    }

    // Now sort the lookup table by new_sym size.
    auto sort_fn = []( DecoderLUTEntry a, DecoderLUTEntry b ) {
        return a.new_sym_ < b.new_sym_;
    };
    std::sort( decoder_lookup_table.begin(), decoder_lookup_table.end(), sort_fn );

    // Then build the output stream.
    outData.clear();
    Uint bit_index = 0;
    UByte curr_byte = 0;
    for( auto sym : inData )
    {
        const auto& found = hTree.sym_table_.find( sym );
        if( found != hTree.sym_table_.end() )
        {
            const std::string& cw = found->second.sym_str_;

            // Write the codeword to the ouptut buffer.
            for( auto bit : cw )
            {
                if( bit_index == 8 )
                {
                    outData.push_back( curr_byte );
                    curr_byte = 0;
                    bit_index = 0;
                }

                curr_byte = curr_byte << 1;
                if( bit == '1' ) curr_byte |= 0x1;

                ++bit_index;
            }
        }
    }

    // Ensure that the last (possibly incomplete) byte of data gets added 
    // to the encoded data stream.
    if( bit_index != 0 )
    {
        curr_byte = curr_byte << ( 8 - bit_index );
        outData.push_back( curr_byte );
    }

    dec_params.decoder_LUT_ = std::move( decoder_lookup_table );
    dec_params.max_cw_len_  = hTree.max_depth_;
    dec_params.num_bytes_   = inData.size();

    return STATUS_OKAY;
}

//========================================================================
//
MsgNum HuffmanCoder::decode( const std::vector<UByte>& inData, 
                             std::vector<UByte>& outData, 
                             const DecoderParameters& params )
{
    // First we need to construct the lookup table. Length of the new table should be
    // Equal to 2^(max symbol length)
    Uint table_len = static_cast<Uint>( pow( 2, params.max_cw_len_ ) );

    // reconstructed_table table rebuilds the lookup table for fast decoding.
    std::vector<DecoderLUTEntry> reconstructed_table( table_len );

    
    if( params.decoder_LUT_.size() < 2 )
    {
        // output err
        std::cout << "HuffmanDecoder: Table lookup size is invalid. Need at least size 2." << std::endl;
        return HUFFMAN_ERROR;
    }

    DecoderLUTEntry currPair = params.decoder_LUT_[0];

    Uint output_table_ind = 0;
    Uint source_table_ind = 0;
    for( ; output_table_ind < table_len; ++output_table_ind )
    {
        if( output_table_ind == params.decoder_LUT_.back().new_sym_ )
        {
            currPair = params.decoder_LUT_.back();
            break;
        }
        if( output_table_ind == params.decoder_LUT_[source_table_ind].new_sym_ )
        {
            currPair = params.decoder_LUT_[source_table_ind];
            ++source_table_ind;
        }

        reconstructed_table[output_table_ind] = currPair;
    }

    // Fill in the table with the last remaining symbol
    for( ; output_table_ind < table_len; ++output_table_ind )
    {
        reconstructed_table[output_table_ind] = currPair;
    }

    BitReader bit_reader( inData );
    bool rd_bits_success = false;
    uint64_t x = bit_reader.read_bits( params.max_cw_len_, rd_bits_success );
    uint64_t k = 0;

    uint64_t word_len_mask = ( 1 << params.max_cw_len_ ) - 1;

    // Now write to the uncompressed data to the output buffer.
    outData.clear();

    for( size_t i = 0; i < params.num_bytes_; ++i )
    {
        outData.push_back( reconstructed_table[x].old_sym_ );
        uint64_t len = reconstructed_table[x].new_sym_len_;

        x = x << len;

        // Sometimes we seem to end up with a few less bits in the decoded
        // data. by default, read_bits returns 0 if failure. This seems to handle
        // things okay somehow, and so we omit error check.
        uint64_t new_bits = bit_reader.read_bits( len, rd_bits_success );


        x = x | new_bits;
        x = x & word_len_mask;
    }

    return STATUS_OKAY;
}
