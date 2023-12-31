/* Read gameground/LICENSE.md file for copyright and licensing information. */

#include <yaal/hcore/macro.hxx>
#include <yaal/hcore/hexception.hxx>
#include <yaal/hcore/safe_int.hxx>
#include <yaal/tools/streamtools.hxx>
#include <yaal/tools/util.hxx>

M_VCSID( "$Id: " __ID__ " $" )
#include "client.hxx"
#include "setup.hxx"

using namespace yaal;
using namespace yaal::hcore;
using namespace yaal::tools;
using namespace yaal::tools::util;

namespace gameground {

WebSockFragmentHeader::WebSockFragmentHeader( u16_t raw_ )
	: _final( false )
	, _opcode( OPCODE::FRAGMENT )
	, _masked( false )
	, _len( 0 )
	, _valid( false ) {
	raw_ = util::hton( raw_ );
	_final = ( ( raw_ >> 15 ) & 1 ) != 0;
	int reserved = ( raw_ >> 12 ) & 7;
	_opcode = static_cast<OPCODE>( ( raw_ >> 8 ) & 15 );
	_masked = ( ( raw_ >> 7 ) & 1 ) != 0;
	_len = raw_ & 127;
	_valid = ( reserved == 0 )
		&& _masked
		&& (
			( _opcode == OPCODE::FRAGMENT )
			|| ( _opcode == OPCODE::TEXT )
			|| ( _opcode == OPCODE::BINARY )
			|| ( _opcode == OPCODE::CLOSE )
			|| ( _opcode == OPCODE::PING )
			|| ( _opcode == OPCODE::PONG )
		);
}

WebSockFragmentHeader::WebSockFragmentHeader( yaal::hcore::HUTF8String const& str_, OPCODE opcode_ )
	: _final( false )
	, _opcode( opcode_ )
	, _masked( false )
	, _len( 0 )
	, _valid( false ) {
	_final = true;
	_len = static_cast<int>( str_.byte_count() );
	if ( _len > 65535 ) {
		_len = 127;
	} else if ( _len > 125 ) {
		_len = 126;
	}
	_valid = true;
}

u16_t WebSockFragmentHeader::raw( void ) const {
	u16_t r( 0 );
	r = static_cast<u16_t>( _len ) & 0xff;
	r |= static_cast<u16_t>( _masked ? ( 1 << 7 ) : 0 );
	r |= static_cast<u16_t>( static_cast<u16_t>( _opcode ) << 8 );
	r |= static_cast<u16_t>( _final ? ( 1 << 15 ) : 0 );
	return ( util::hton( r ) );
}

char const* WebSockFragmentHeader::opcode( void ) const {
	char const* oc( "reserved" );
	switch ( _opcode ) {
		case ( OPCODE::FRAGMENT ): oc = "fragment"; break;
		case ( OPCODE::TEXT ):     oc = "text";     break;
		case ( OPCODE::BINARY ):   oc = "binary";   break;
		case ( OPCODE::CLOSE ):    oc = "close";    break;
		case ( OPCODE::PING ):     oc = "ping";     break;
		case ( OPCODE::PONG ):     oc = "pong";     break;
	}
	return oc;
}

inline void apply_mask( void* data_, int size_, u32_t mask_ ) {
	u32_t* d( static_cast<u32_t*>( data_ ) );
	int const maskSize( static_cast<int>( sizeof ( mask_ ) ) );
	for ( int i( 0 ); i < ( size_ / maskSize ); ++ i ) {
		d[i] ^= mask_;
	}
	int rest( size_ % maskSize );
	u8_t* o( static_cast<u8_t*>( data_ ) + size_ - rest );
	u8_t* m( reinterpret_cast<u8_t*>( &mask_ ) );
	for ( int i( 0 ); i < rest; ++ i ) {
		o[i] ^= m[i];
	}
	return;
}

inline HStreamInterface& operator << ( HStreamInterface& os, WebSockFragmentHeader const& wsfh ) {
	os << "final = " << wsfh._final << "\n"
		<< "opcode = " << wsfh.opcode() << "\n"
		<< "masked = " << wsfh._masked << "\n"
		<< "len = " << wsfh._len << endl;
	return os;
}

HClient::HClient( yaal::hcore::HStreamInterface::ptr_t const& sock_ )
	: _webSocket( false )
	, _valid( true )
	, _authenticated( false )
	, _login()
	, _socket( sock_ )
	, _buffer()
	, _utf8()
	, _logics() {
	M_ASSERT( !! _socket );
}

HClient::~HClient( void ) {
	M_PROLOG
	M_ASSERT( _logics.is_empty() );
	return;
	M_EPILOG
}

void HClient::upgrade( void ) {
	_webSocket = true;
}

void HClient::send( yaal::hcore::HString const& message_ ) {
	M_PROLOG
	if ( ! _valid ) {
		return;
	}
	if ( ! _webSocket ) {
		*_socket << message_;
	} else {
		send_web_socket( message_ );
	}
	_socket->flush();
	return;
	M_EPILOG
}

void HClient::send_web_socket( yaal::hcore::HString const& message_, WebSockFragmentHeader::OPCODE opcode_ ) {
	M_PROLOG
	do {
		_utf8.assign( message_ );
		WebSockFragmentHeader wsfh( _utf8, opcode_ );
		u16_t header( wsfh.raw() );
		int toWrite( sizeof ( header ) );
		if ( _socket->write( &header, toWrite ) != toWrite ) {
			OUT << "failed to write header" << endl;
			break;
		}
		if ( wsfh._len == 127 ) {
			u64_t len( static_cast<u64_t>( _utf8.byte_count() ) );
			len = util::hton( len );
			toWrite = sizeof ( len );
			if ( _socket->write( &len, toWrite ) != toWrite ) {
				OUT << "failed to write 64-bit len" << endl;
				break;
			}
		} else if ( wsfh._len == 126 ) {
			u16_t len( static_cast<u16_t>( _utf8.byte_count() ) );
			len = util::hton( len );
			toWrite = sizeof ( len );
			if ( _socket->write( &len, toWrite ) != toWrite ) {
				OUT << "failed to write 16-bit len" << endl;
				break;
			}
		}
		_socket->write( _utf8.c_str(), _utf8.byte_count() );
	} while ( false );
	return;
	M_EPILOG
}

int long HClient::read( yaal::hcore::HString& line_ ) {
	M_PROLOG
	if ( ! _valid ) {
		return ( 0 );
	}
	int long nRead( -1 );
	if ( ! _webSocket ) {
		nRead = _socket->read_until( line_, "\n" );
	} else {
		nRead = read_web_socket( line_ );
	}
	if ( nRead > 1 ) {
		int long len( line_.get_length() );
		line_.trim_right( "\r\n" );
		nRead -= ( len - line_.get_length() );
	}
	return nRead;
	M_EPILOG
}

int long HClient::read_web_socket( yaal::hcore::HString& line_ ) {
	M_PROLOG
	if ( ! _valid ) {
		return ( 0 );
	}
	int long nRead( 0 );
	do {
		u16_t header( 0 );
		int toRead( sizeof ( header ) );
		if ( _socket->read( &header, toRead ) != toRead ) {
			break;
		}
		WebSockFragmentHeader wsfh( header );
		if ( ! wsfh.is_valid() ) {
			OUT << "invalid header" << endl;
			break;
		}
		if ( setup._debug ) {
			OUT << "wsfh:\n" << wsfh << endl;
		}
		int payloadLen( wsfh._len );
		if ( payloadLen == 127 ) {
			u64_t len( 0 );
			toRead = sizeof ( len );
			if ( _socket->read( &len, toRead ) != toRead ) {
				OUT << "failed to read 64-bit len" << endl;
				break;
			}
			payloadLen = safe_int::cast<int>( util::hton( len ) );
		} else if ( payloadLen == 126 ) {
			u16_t len( 0 );
			toRead = sizeof ( len );
			if ( _socket->read( &len, toRead ) != toRead ) {
				OUT << "failed to read 16-bit len" << endl;
				break;
			}
			payloadLen = util::hton( len );
		}
		u32_t mask( 0 );
		toRead = sizeof ( mask );
		if ( _socket->read( &mask, toRead ) != toRead ) {
			OUT << "failed to read mask" << endl;
			break;
		}
		if ( payloadLen > 0 ) {
			_buffer.realloc( payloadLen );
			if ( _socket->read( _buffer.raw(), payloadLen ) != payloadLen ) {
				OUT << "failed to read payload" << endl;
				break;
			}
			apply_mask( _buffer.raw(), payloadLen, mask );
			line_.assign( _buffer.get<char>(), payloadLen );
		}
		nRead = payloadLen;
		if ( wsfh._opcode == WebSockFragmentHeader::OPCODE::PING ) {
			pong( line_ );
			line_.clear();
			nRead = -1;
		}
	} while ( false );
	return nRead;
	M_EPILOG
}

void HClient::pong( yaal::hcore::HString const& message_ ) {
	M_PROLOG
	send_web_socket( message_, WebSockFragmentHeader::OPCODE::PONG );
	OUT << "ping-pong!" << endl;
	return;
	M_EPILOG
}

void HClient::set_login( yaal::hcore::HString const& login_ ) {
	M_PROLOG
	M_ASSERT( _login.is_empty() );
	_login = login_;
	return;
	M_EPILOG
}

void HClient::enter( HLogic::id_t id_ ) {
	M_PROLOG
	_logics.insert( id_ );
	return;
	M_EPILOG
}

void HClient::leave( HLogic::id_t id_ ) {
	M_PROLOG
	_logics.erase( id_ );
	return;
	M_EPILOG
}

void HClient::invalidate( void ) {
	_valid = false;
}

void HClient::authenticate( void ) {
	_authenticated = true;
}

}

