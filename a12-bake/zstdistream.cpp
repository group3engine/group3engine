#include "zstdistream.hpp"

#include <cstdlib>
#include <streambuf>
#include <fstream>

#include <zstd.h>


namespace
{
	class ZStdStreambuf_ : public std::streambuf
	{
		public:
			ZStdStreambuf_( char const* aPath ), ~ZStdStreambuf_();

		protected:
			int underflow() override;

		private:
			char* mOutBuf;
			std::size_t mOutSize;

			char* mInBuf;
			std::size_t mInSize;
			ZSTD_inBuffer mInState;

			ZSTD_DCtx* mCtx;

			std::ifstream mStream;
	};
}

ZStdIStream::ZStdIStream( char const* aPath )
	: std::istream( nullptr )
	, mInternal( std::make_unique<ZStdStreambuf_>(aPath) )
{
	rdbuf( mInternal.get() );
}

namespace
{
	ZStdStreambuf_::ZStdStreambuf_( char const* aPath )
		: mStream( aPath, std::ios::binary )
	{
		if( !mStream.is_open() ) {
			std::fprintf(stderr,  "Unable to open '%s'", aPath );
			std::exit(EXIT_FAILURE);
		}

		// Init ZStd
		mOutSize = ZSTD_DStreamOutSize();
		mOutBuf = reinterpret_cast<char*>(::operator new( mOutSize ));

		mInSize = ZSTD_DStreamInSize();
		mInBuf = reinterpret_cast<char*>(::operator new( mInSize ));

		mCtx = ZSTD_createDCtx();
		if( !mCtx ) {
			std::fprintf(stderr,  "ZSTD_createDCtx(): returned error" );
			std::exit(EXIT_FAILURE);
		}

		// Fill buffer once
		mStream.read( mInBuf, mInSize );
		if( mStream.bad() ) {
			std::fprintf(stderr,  "Reading: badness happened" ); // :-(
			std::exit(EXIT_FAILURE);
		}

		mInState.src = mInBuf;
		mInState.pos = 0;
		mInState.size = mStream.gcount(); // iostreams are terrible.

		// Decompress once
		ZSTD_outBuffer ob{ mOutBuf, mOutSize, 0 };
		auto const ret = ZSTD_decompressStream( mCtx, &ob, &mInState );
		if( ZSTD_isError(ret) ) {
			std::fprintf(stderr,  "Decompression: %s", ZSTD_getErrorName(ret) );
			std::exit(EXIT_FAILURE);
		}

		// Initialize stream buffer
		setg( mOutBuf, mOutBuf, mOutBuf + ob.pos );
	}

	ZStdStreambuf_::~ZStdStreambuf_()
	{
		::operator delete( mInBuf );
		::operator delete( mOutBuf );

		ZSTD_freeDCtx( mCtx );
	}

	int ZStdStreambuf_::underflow()
	{
		// Decompressed buffer empty?
		if( gptr() == egptr() )
		{
			// Input buffer empty?
			if( mInState.pos == mInSize )
			{
				mStream.read( mInBuf, mInSize );
				if( mStream.bad() ) {
					std::fprintf(stderr,  "Reading: badness happened" ); // :-(
					std::exit(EXIT_FAILURE);
				}

				mInState.pos = 0;
				mInState.size = mStream.gcount();
			}

			ZSTD_outBuffer ob{ mOutBuf, mOutSize, 0 };
			auto const ret = ZSTD_decompressStream( mCtx, &ob, &mInState );
			if( ZSTD_isError(ret) ) {
				std::fprintf(stderr,  "Decompression: %s", ZSTD_getErrorName(ret) );
				std::exit(EXIT_FAILURE);
			}

			setg( mOutBuf, mOutBuf, mOutBuf + ob.pos );
		}

		return gptr() == egptr() 
			? traits_type::eof() 
			: traits_type::to_int_type( *gptr() )
		;
	}
}
