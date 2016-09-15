/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks. Threading Building Blocks is free software;
    you can redistribute it and/or modify it under the terms of the GNU General Public License
    version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
    distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See  the GNU General Public License for more details.   You should have received a copy of
    the  GNU General Public License along with Threading Building Blocks; if not, write to the
    Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA

    As a special exception,  you may use this file  as part of a free software library without
    restriction.  Specifically,  if other files instantiate templates  or use macros or inline
    functions from this file, or you compile this file and link it with other files to produce
    an executable,  this file does not by itself cause the resulting executable to be covered
    by the GNU General Public License. This exception does not however invalidate any other
    reasons why the executable file might be covered by the GNU General Public License.
*/

#include "tbb/tbb_config.h"
#include "../../common/utility/utility.h"

#if __TBB_PREVIEW_ASYNC_NODE && __TBB_CPP11_LAMBDAS_PRESENT

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include "tbb/compat/thread"
#include <queue>

#include "bzlib.h"

#include "tbb/flow_graph.h"
#include "tbb/tick_count.h"
#include "tbb/concurrent_queue.h"

bool quiet = false;
bool verbose = false;
bool asyncIO = false;


bool endsWith( const std::string& str, const std::string& suffix ) {
    return (str.length() > suffix.length()) && (str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0);
}

struct buffer_t {
    size_t seq_id;
    unsigned long len;
    char *b;

    bool operator==( const buffer_t& other ) {
        return (this == &other) || (seq_id == other.seq_id && len == other.len && b == other.b);
    }
};

buffer_t dummyBuffer = {0};

struct buffer_comp {
    bool operator()( const buffer_t& a, const buffer_t& b ) {
        return a.seq_id > b.seq_id;
    }
};

class bufferCompressor {
public:
    bufferCompressor( int block_size_in_100kb ) : my_blockSize(block_size_in_100kb) {}

    buffer_t operator()( buffer_t inputBuffer ) const;
private:
    int my_blockSize;
};

buffer_t bufferCompressor::operator()( buffer_t inputBuffer ) const {
    buffer_t compressedBuffer;
    compressedBuffer.seq_id = inputBuffer.seq_id;
    unsigned int outSize = (unsigned int) (((inputBuffer.len)*1.01)+600);
    compressedBuffer.b = new char[outSize];

    BZ2_bzBuffToBuffCompress( compressedBuffer.b, &outSize, inputBuffer.b, inputBuffer.len, my_blockSize, 0, 30 );
    delete[] inputBuffer.b;

    compressedBuffer.len = outSize;

    return compressedBuffer;
}

buffer_t readChunk(std::istream& inputStream, size_t& chunksRead, size_t chunkSize ) {
    buffer_t buffer;

    buffer.seq_id = chunksRead++;
    buffer.b = new char[chunkSize];
    inputStream.read( buffer.b, chunkSize );
    buffer.len = static_cast<unsigned long>( inputStream.gcount() );

    return buffer;
}

typedef tbb::flow::async_node<tbb::flow::continue_msg, buffer_t> async_file_reader_node;
typedef tbb::flow::async_node< buffer_t, tbb::flow::continue_msg > async_file_writer_node;

class IOActivity {
public:
    IOActivity( std::ifstream& inputStream, std::ofstream& output, size_t chunkSize ) :
        my_inputStream(inputStream), my_outputStream(output), my_chunkSize(chunkSize),
        my_fileWriter( [this]() {
            std::priority_queue< buffer_t, std::vector<buffer_t>, buffer_comp > my_outputQueue;
            size_t my_nextOutputId = 0;

            while( true ) {
                buffer_t buffer;
                my_writeQueue.pop( buffer );
                if( buffer == dummyBuffer ) break;
                if( buffer.seq_id == my_nextOutputId ) {
                    my_outputStream.write( buffer.b, buffer.len );
                    delete[] buffer.b;
                    ++my_nextOutputId;
                    while( !my_outputQueue.empty() && my_outputQueue.top().seq_id == my_nextOutputId ) {
                        buffer = my_outputQueue.top();
                        my_outputQueue.pop();
                        my_outputStream.write( buffer.b, buffer.len );
                        delete[] buffer.b;
                        ++my_nextOutputId;
                    }
                } else {
                    my_outputQueue.push( buffer );
                }
            }
        }) 
    {}

    ~IOActivity() {
        if( my_fileReader.joinable() ) my_fileReader.join();
        if( my_fileWriter.joinable() ) {
            my_writeQueue.push( dummyBuffer );
            my_fileWriter.join();
        }
    }

    void start_read( async_file_reader_node::async_gateway_type& asyncGateway ) {
        if( my_inputStream.is_open() && !my_fileReader.joinable() ) {
            asyncGateway.async_reserve();

            std::thread( ([this, &asyncGateway]() {
                size_t chunksRead = 0;
                while( !my_inputStream.eof() ) {
                    buffer_t buffer = readChunk( my_inputStream, chunksRead, my_chunkSize );
                    asyncGateway.async_try_put( buffer );
                }
                asyncGateway.async_commit();
            }) ).swap( my_fileReader );

        }

    }

    void write( const buffer_t& buffer ) {
        my_writeQueue.push(buffer);
    }

private:
    std::ifstream& my_inputStream;
    std::thread my_fileReader;

    std::ofstream& my_outputStream;
    tbb::concurrent_bounded_queue< buffer_t > my_writeQueue;

    size_t my_chunkSize;

    std::thread my_fileWriter;


};

void fgCompressionAsyncIO( std::ifstream& inputStream, std::ofstream& outputStream, int block_size_in_100kb ) {
    size_t chunkSize = block_size_in_100kb*100*1024;

    IOActivity ioActivity( inputStream, outputStream, chunkSize );

    tbb::flow::graph g;

    async_file_reader_node file_reader( g, tbb::flow::unlimited, [&ioActivity]( const tbb::flow::continue_msg& msg, async_file_reader_node::async_gateway_type& asyncGateway ) {
            ioActivity.start_read( asyncGateway );
        } );

    tbb::flow::function_node< buffer_t, buffer_t > compressor( g, tbb::flow::unlimited, bufferCompressor( block_size_in_100kb ) );

    tbb::flow::sequencer_node< buffer_t > ordering( g, []( const buffer_t& buffer )->size_t {
            return buffer.seq_id;
        });

    async_file_writer_node output_writer( g, tbb::flow::unlimited, [&ioActivity](const buffer_t& buffer, async_file_writer_node::async_gateway_type& asyncGateway ) {
            ioActivity.write( buffer );
        });

    make_edge( file_reader, compressor );
    make_edge( compressor, ordering );
    make_edge( ordering, output_writer );

    file_reader.try_put( tbb::flow::continue_msg() );

    g.wait_for_all();
}

void fgCompression( std::ifstream& inputStream, std::ofstream& outputStream, int block_size_in_100kb ) {
    size_t chunkSize = block_size_in_100kb*100*1024;
    tbb::flow::graph g;

    size_t chunksRead = 0;
    tbb::flow::source_node< buffer_t > file_reader( g, [&inputStream, chunkSize, &chunksRead]( buffer_t& buffer )->bool {
            if( inputStream.eof() ) return false;
            buffer = readChunk( inputStream, chunksRead, chunkSize );
            return true;
        } );

    tbb::flow::function_node< buffer_t, buffer_t > compressor( g, tbb::flow::unlimited, bufferCompressor( block_size_in_100kb ) );

    tbb::flow::sequencer_node< buffer_t > ordering(g, []( const buffer_t& buffer )->size_t {
            return buffer.seq_id;
        });

    tbb::flow::function_node< buffer_t > output_writer(g, tbb::flow::serial, [&outputStream]( const buffer_t& buffer ) {
            outputStream.write( buffer.b, buffer.len );
            delete[] buffer.b;
        });

    make_edge( file_reader, compressor );
    make_edge( compressor, ordering );
    make_edge( ordering, output_writer );

    g.wait_for_all();
}

int main( int argc, char* argv[] ) {
    try {
        tbb::tick_count mainStartTime = tbb::tick_count::now();

        const std::string archiveExtension = ".bz2";
        std::string inputFileName = "";
        int block_size_in_100kb = 1; // block size in 100 Kb chunks


        utility::parse_cli_arguments(argc, argv,
                utility::cli_argument_pack()
                //"-h" option for displaying help is present implicitly
                .arg(block_size_in_100kb, "-b", "\t block size in 100 Kb chunks, [1 .. 9]")
                .arg(quiet, "-q", "quiet mode")
                .arg(verbose, "-v", "verbose mode")
                .arg(asyncIO, "-a", "asynchronous I/O")
                .positional_arg(inputFileName,"filename","input file name")
                );

        if( quiet ) verbose = false;

        if( inputFileName.empty() ) {
            if(!quiet) std::cerr << "Please specify input file name." << std::endl;
            return -1;
        }

        if( block_size_in_100kb < 1 || block_size_in_100kb > 9) {
            if(!quiet) std::cerr << "Incorrect block size." << std::endl;
            return -1;
        }

        if( verbose ) std::cout << "Input file name: " << inputFileName << std::endl;
        if( endsWith(inputFileName, archiveExtension) ) {
            if(!quiet) std::cerr << "Input file already have " << archiveExtension << " extension." << std::endl;
            return 0;
        }

        std::ifstream inputStream( inputFileName.c_str(), std::ios::in | std::ios::binary );
        if( !inputStream.is_open() ) {
            if(!quiet) std::cerr << "Cannot open " << inputFileName << " file." << std::endl;
            return -1;
        }

        std::string outputFileName(inputFileName + archiveExtension);

        std::ofstream outputStream( outputFileName.c_str(), std::ios::out | std::ios::binary | std::ios::trunc );
        if( !outputStream.is_open() ) {
            if(!quiet) std::cerr << "Cannot open " << outputFileName << " file." << std::endl;
            return -1;
        }

        if( asyncIO ) {
            if( verbose ) std::cout << "Running compression with async_node." << std::endl;
            fgCompressionAsyncIO( inputStream, outputStream, block_size_in_100kb );
        } else {
            if( verbose ) std::cout << "Running compression without async_node." << std::endl;
            fgCompression( inputStream, outputStream, block_size_in_100kb );
        }

        inputStream.close();
        outputStream.close();

        utility::report_elapsed_time((tbb::tick_count::now() - mainStartTime).seconds());

        return 0;
    } catch( std::exception& e ) {
        std::cerr<<"error occurred. error text is :\"" <<e.what()<<"\"\n";
        return 1;
    }
}
#else
int main() {
    utility::report_skipped();
    return 0;
}
#endif /* __TBB_PREVIEW_ASYNC_NODE && __TBB_CPP11_LAMBDAS_PRESENT */
