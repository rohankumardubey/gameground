/*
---           `gameground' 0.0.0 (c) 1978 by Marcin 'Amok' Konarski            ---

	god.cxx - this file is integral part of `gameground' project.

  i.  You may not make any changes in Copyright information.
  ii. You must attach Copyright information to any part of every copy
      of this software.

Copyright:

 You can use this software free of charge and you can redistribute its binary
 package freely but:
  1. You are not allowed to use any part of sources of this software.
  2. You are not allowed to redistribute any part of sources of this software.
  3. You are not allowed to reverse engineer this software.
  4. If you want to distribute a binary package of this software you cannot
     demand any fees for it. You cannot even demand
     a return of cost of the media or distribution (CD for example).
  5. You cannot involve this software in any commercial activity (for example
     as a free add-on to paid software or newspaper).
 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE. Use it at your own risk.
*/

#include <cstring>

#include <yaal/yaal.hxx>

M_VCSID( "$Id: "__ID__" $" )
#include "god.hxx"

#include "setup.hxx"
#include "clientinfo.hxx"
#include "logicfactory.hxx"
#include "spellchecker.hxx"
#include "security.hxx"

using namespace yaal;
using namespace yaal::hcore;
using namespace yaal::tools;
using namespace yaal::tools::util;
using namespace sgf;

namespace gameground {

namespace go {

HGo::STONE::stone_t const HGo::STONE::BLACK = 'b';
HGo::STONE::stone_t const HGo::STONE::WHITE = 'w';
HGo::STONE::stone_t const HGo::STONE::NONE = ' ';
HGo::STONE::stone_t const HGo::STONE::MARK = 'm';
HGo::STONE::stone_t const HGo::STONE::DEAD_BLACK = 's';
HGo::STONE::stone_t const HGo::STONE::DEAD_WHITE = 't';
HGo::STONE::stone_t const HGo::STONE::TERITORY_BLACK = 'p';
HGo::STONE::stone_t const HGo::STONE::TERITORY_WHITE = 'q';
HGo::STONE::stone_t const HGo::STONE::TERITORY_NONE = 'x';

char const* const HGo::PROTOCOL::SETUP = "setup";
char const* const HGo::PROTOCOL::ADMIN = "admin";
char const* const HGo::PROTOCOL::PLAY = "play";
char const* const HGo::PROTOCOL::NEWGAME = "newgame";
char const* const HGo::PROTOCOL::UNDO = "undo";
char const* const HGo::PROTOCOL::CONTESTANT = "contestant";
char const* const HGo::PROTOCOL::GOBAN = "goban";
char const* const HGo::PROTOCOL::KOMI = "komi";
char const* const HGo::PROTOCOL::HANDICAPS = "handicaps";
char const* const HGo::PROTOCOL::MAINTIME = "maintime";
char const* const HGo::PROTOCOL::BYOYOMIPERIODS = "byoyomiperiods";
char const* const HGo::PROTOCOL::BYOYOMITIME = "byoyomitime";
char const* const HGo::PROTOCOL::STONE = "stone";
char const* const HGo::PROTOCOL::TOMOVE = "to_move";
char const* const HGo::PROTOCOL::PUTSTONE = "put_stone";
char const* const HGo::PROTOCOL::SELECT = "select";
char const* const HGo::PROTOCOL::PASS = "pass";
char const* const HGo::PROTOCOL::SGF = "sgf";
char const* const HGo::PROTOCOL::SIT = "sit";
char const* const HGo::PROTOCOL::GETUP = "get_up";
char const* const HGo::PROTOCOL::DEAD = "dead";
char const* const HGo::PROTOCOL::ACCEPT = "accept";
static int const ACCEPTED = -7;

int const GO_MSG_NOT_YOUR_TURN = 0;
int const GO_MSG_INSUFFICIENT_PRIVILEGES = 1;
int const GO_MSG_MALFORMED = 2;
char const* const GO_MSG[] = {
	"not your turn",
	"insifficient privileges",
	"malformed packet"
};

HGo::HGo( HServer* server_, HLogic::id_t const& id_, HString const& comment_ )
	: HLogic( server_, id_, comment_ ),
	_state( STONE::NONE ), _gobanSize( setup._gobanSize ),
	_komi( setup._komi ), _handicaps( setup._handicaps ), _mainTime( setup._mainTime ),
	_byoYomiPeriods( setup._byoYomiPeriods ), _byoYomiTime( setup._byoYomiTime ),
	_move( 0 ), _pass( 0 ), _start( 0 ),
	_game( GOBAN_SIZE::NORMAL * GOBAN_SIZE::NORMAL + sizeof ( '\0' ) ),
	_koGame( GOBAN_SIZE::NORMAL * GOBAN_SIZE::NORMAL + sizeof ( '\0' ) ),
	_oldGame( GOBAN_SIZE::NORMAL * GOBAN_SIZE::NORMAL + sizeof ( '\0' ) ),
	_sgf( SGF::GAME_TYPE::GO, "gameground" ),
	_players(), _path(), _branch(), _varTmpBuffer() {
	M_PROLOG
	_sgf.set_info( SGF::Player::BLACK, _gobanSize, _komi, _handicaps, _mainTime );
	_contestants[ 0 ] = _contestants[ 1 ] = NULL;
	_handlers[ PROTOCOL::SETUP ] = static_cast<handler_t>( &HGo::handler_setup );
	_handlers[ PROTOCOL::PLAY ] = static_cast<handler_t>( &HGo::handler_play );
	_handlers[ PROTOCOL::SGF ] = static_cast<handler_t>( &HGo::handler_sgf );
	_handlers[ PROTOCOL::SIT ] = static_cast<handler_t>( &HGo::handler_sit );
	_handlers[ PROTOCOL::GETUP ] = static_cast<handler_t>( &HGo::handler_getup );
	_handlers[ PROTOCOL::SELECT ] = static_cast<handler_t>( &HGo::handler_select );
	_handlers[ PROTOCOL::NEWGAME ] = static_cast<handler_t>( &HGo::handler_newgame );
	set_handicaps( _handicaps );
	return;
	M_EPILOG
}

HGo::~HGo ( void ) {
	M_PROLOG
	revoke_scheduled_tasks();
	return;
	M_EPILOG
}

void HGo::broadcast_contestants( yaal::hcore::HString const& message_ ) {
	M_PROLOG
	M_ASSERT( _contestants[ 0 ] && _contestants[ 1 ] );
	_contestants[ 0 ]->_socket->write_until_eos( message_ );
	_contestants[ 1 ]->_socket->write_until_eos( message_ );
	return;
	M_EPILOG
}

void HGo::handler_setup( OClientInfo* clientInfo_, HString const& message_ ) {
	M_PROLOG
	HLock l( _mutex );
	if ( ! can_setup( clientInfo_ ) )
		throw HLogicException( GO_MSG[ GO_MSG_INSUFFICIENT_PRIVILEGES ] );
	HString item = get_token( message_, ",", 0 );
	int value( 0 );
	try {
		value = lexical_cast<int>( get_token( message_, ",", 1 ) );
	} catch ( HLexicalCastException const& ) {
		throw HLogicException( GO_MSG[GO_MSG_MALFORMED] );
	}
	bool regenGoban( false );
	int handicaps( _handicaps );
	if ( item == PROTOCOL::GOBAN ) {
		_sgf.set_board_size( _gobanSize = value );
		regenGoban = true;
	} else if ( item == PROTOCOL::KOMI )
		_sgf.set_komi( _komi = value );
	else if ( item == PROTOCOL::HANDICAPS ) {
		handicaps = value;
		regenGoban = true;
	} else if ( item == PROTOCOL::MAINTIME )
		_sgf.set_time( _mainTime = value );
	else if ( item == PROTOCOL::BYOYOMIPERIODS )
		_byoYomiPeriods = value;
	else if ( item == PROTOCOL::BYOYOMITIME )
		_byoYomiTime = value;
	else
		throw HLogicException( GO_MSG[ GO_MSG_MALFORMED ] );
	broadcast( _out << PROTOCOL::SETUP << PROTOCOL::SEP << message_ << endl << _out );
	if ( regenGoban )
		set_handicaps( handicaps );
	return;
	M_EPILOG
}

void HGo::handler_sgf( OClientInfo* clientInfo_, HString const& message_ ) {
	M_PROLOG
	if ( ! can_setup( clientInfo_ ) )
		throw HLogicException( GO_MSG[ GO_MSG_INSUFFICIENT_PRIVILEGES ] );
	try {
		SGF sgf( SGF::GAME_TYPE::GO, "gameground" );
		out << message_ << endl;
		sgf.load( message_ );
		_sgf.swap( sgf );
		_gobanSize = _sgf.get_board_size();
		_komi = static_cast<int>( _sgf.get_komi() );
		_handicaps = _sgf.get_handicap();
		_mainTime = _sgf.get_time();
		broadcast( _out << PROTOCOL::SGF << PROTOCOL::SEP << message_ << endl << _out );
		broadcast( _out << PROTOCOL::SETUP << PROTOCOL::SEP
			<< PROTOCOL::GOBAN << PROTOCOL::SEPP << _gobanSize << endl
			<< *this << PROTOCOL::SETUP << PROTOCOL::SEP
			<< PROTOCOL::KOMI << PROTOCOL::SEPP << _komi << endl
			<< *this << PROTOCOL::SETUP << PROTOCOL::SEP
			<< PROTOCOL::HANDICAPS << PROTOCOL::SEPP << _handicaps << endl
			<< *this << PROTOCOL::SETUP << PROTOCOL::SEP
			<< PROTOCOL::MAINTIME << PROTOCOL::SEPP << _mainTime << endl
			<< *this << PROTOCOL::SETUP << PROTOCOL::SEP
			<< PROTOCOL::BYOYOMIPERIODS << PROTOCOL::SEPP << _byoYomiPeriods << endl
			<< *this << PROTOCOL::SETUP << PROTOCOL::SEP
			<< PROTOCOL::BYOYOMITIME << PROTOCOL::SEPP << _byoYomiTime << endl << _out );
	} catch ( SGFException const& ) {
		throw HLogicException( "provided SGF has unexpected content" );
	}
	return;
	M_EPILOG
}

void HGo::handler_sit( OClientInfo* clientInfo_, HString const& message_ ) {
	M_PROLOG
	if ( message_.get_length() < 1 )
		throw HLogicException( GO_MSG[ GO_MSG_MALFORMED ] );
	else {
		char stone = message_[ 0 ];
		if ( ( stone != STONE::BLACK ) && ( stone != STONE::WHITE ) )
			throw HLogicException( GO_MSG[ GO_MSG_MALFORMED ] );
		else if ( ( contestant( STONE::BLACK ) == clientInfo_ )
				|| ( contestant( STONE::WHITE ) == clientInfo_ ) )
			throw HLogicException( "you were already sitting" );
		else if ( contestant( stone ) != NULL )
			*clientInfo_->_socket << *this
				<< PROTOCOL::MSG << PROTOCOL::SEP << "Some one was faster." << endl;
		else {
			M_ASSERT( _state == STONE::NONE );
			contestant( stone ) = clientInfo_;
			OPlayerInfo& info = *get_player_info( clientInfo_ );
			info._timeLeft = _mainTime;
			info._byoYomiPeriods = _byoYomiPeriods;
			info._stonesCaptured = 0;
			info._score = ( stone == STONE::WHITE ? _komi : 0 );
			OClientInfo* black( contestant( STONE::BLACK ) );
			OClientInfo* white( contestant( STONE::WHITE ) );
			if ( black && white ) {
				OPlayerInfo& foe = *get_player_info( contestant( opponent( stone ) ) );
				foe._timeLeft = info._timeLeft;
				foe._byoYomiPeriods = info._byoYomiPeriods;
				_state = ( _handicaps > 1 ? STONE::WHITE : STONE::BLACK );
				_pass = 0;
				_sgf.clear();
				_sgf.set_player( SGF::Player::BLACK, black->_login );
				_sgf.set_player( SGF::Player::WHITE, white->_login );
				_sgf.set_info( SGF::Player::BLACK, _gobanSize, _handicaps, _komi, _mainTime );
				broadcast( _out << PROTOCOL::MSG << PROTOCOL::SEP << "The Go match started." << endl << _out );
			}
			after_move();
		}
	}
	return;
	M_EPILOG
}

void HGo::handler_getup( OClientInfo* clientInfo_, HString const& /*message_*/ ) {
	M_PROLOG
	if ( ( contestant( STONE::BLACK ) != clientInfo_ ) && ( contestant( STONE::WHITE ) != clientInfo_ ) )
		throw HLogicException( "you were not sitting" );
	contestant_gotup( clientInfo_ );
	_state = STONE::NONE;
	after_move();
	return;
	M_EPILOG
}

void HGo::handler_select( OClientInfo* clientInfo_, yaal::hcore::HString const& message_ ) {
	M_PROLOG
	if ( ! can_setup( clientInfo_ ) )
		throw HLogicException( GO_MSG[ GO_MSG_INSUFFICIENT_PRIVILEGES ] );
	HTokenizer t( message_, "," );
	HTokenizer::HIterator it( t.begin() );
	HTokenizer::HIterator end( t.end() );
	if ( it == end )
		throw HLogicException( GO_MSG[ GO_MSG_MALFORMED ] );
	int moveNo( 0 );
	try {
		moveNo = lexical_cast<int>( *it );
	} catch ( HLexicalCastException const& ) {
		throw HLogicException( GO_MSG[GO_MSG_MALFORMED] );
	}
	++ it;
	SGF::game_tree_t::const_node_t currentMove( _sgf.game_tree().get_root() );
	if ( currentMove ) {
		for ( int i( 0 ); i < moveNo; ++ i ) {
			int childCount( static_cast<int>( currentMove->child_count() ) );
			if ( childCount == 0 )
				throw HLogicException( GO_MSG[GO_MSG_MALFORMED] );
			int child( 0 );
			if ( childCount > 1 ) {
				if ( it == end )
					throw HLogicException( GO_MSG[GO_MSG_MALFORMED] );
				try {
					child = lexical_cast<int>( *it );
				} catch ( HLexicalCastException const& ) {
					throw HLogicException( GO_MSG[GO_MSG_MALFORMED] );
				}
				++ it;
			}
			currentMove = currentMove->get_child_at( child );
		}
		_sgf.set_current_move( currentMove );
	} else if ( moveNo > 0 )
		throw HLogicException( GO_MSG[GO_MSG_MALFORMED] );
	broadcast( _out << PROTOCOL::SELECT << PROTOCOL::SEP << message_ << endl << _out );
	return;
	M_EPILOG
}

void HGo::handler_put_stone( OClientInfo* clientInfo_, HString const& message_ ) {
	M_PROLOG
	if ( ( ( *_clients.begin() != clientInfo_ ) || ( _state != STONE::NONE ) )
			&& ( _state != STONE::BLACK )
			&& ( _state != STONE::WHITE ) )
		throw HLogicException( GO_MSG[ GO_MSG_INSUFFICIENT_PRIVILEGES ] );
	if ( contestant( _state ) != clientInfo_ )
		throw HLogicException( GO_MSG[ GO_MSG_NOT_YOUR_TURN ] );
	_pass = 0;
	int col( 0 );
	int row( 0 );
	try {
		col = lexical_cast<int>( get_token( message_, ",", 1 ) );
		row = lexical_cast<int>( get_token( message_, ",", 2 ) );
	} catch ( HLexicalCastException const& ) {
		throw HLogicException( GO_MSG[GO_MSG_MALFORMED] );
	}
	int before = count_stones( opponent( _state ) );
	make_move( col, row, _state );
	int after = count_stones( opponent( _state ) );
	get_player_info( contestant( _state ) )->_stonesCaptured += ( before - after );
	_state = opponent( _state );
	_sgf.move( SGF::Coord( col, row ) );
	send_goban();
	return;
	M_EPILOG
}

void HGo::handler_pass( OClientInfo* clientInfo_, HString const& /*message_*/ ) {
	M_PROLOG
	if ( ( _state != STONE::BLACK ) && ( _state != STONE::WHITE ) )
		throw HLogicException( GO_MSG[ GO_MSG_INSUFFICIENT_PRIVILEGES ] );
	if ( contestant( _state ) != clientInfo_ )
		throw HLogicException( GO_MSG[ GO_MSG_NOT_YOUR_TURN ] );
	_state = opponent( _state );
	++ _pass;
	if ( _pass == 3 ) {
		_state = STONE::MARK;
		broadcast( _out << PROTOCOL::MSG << PROTOCOL::SEP << "The match has ended." << endl << _out );
		broadcast_contestants( _out << *this << PROTOCOL::MSG << PROTOCOL::SEP << "Select your dead stones." << endl << _out );
		mark_teritory();
		for ( int i = 0; i < ( _gobanSize * _gobanSize ); ++ i ) {
			int x = i / _gobanSize;
			int y = i % _gobanSize;
			if ( goban( x, y ) == STONE::TERITORY_BLACK )
				_sgf.add_position( SGF::Position::BLACK_TERITORY, SGF::Coord( x, y ) );
			else if ( goban( x, y ) == STONE::TERITORY_WHITE )
				_sgf.add_position( SGF::Position::WHITE_TERITORY, SGF::Coord( x, y ) );
		}
		send_goban();
	}
	return;
	M_EPILOG
}

void HGo::handler_dead( OClientInfo* clientInfo_, HString const& message_ ) {
	M_PROLOG
	if ( _state != STONE::MARK )
		throw HLogicException( GO_MSG[ GO_MSG_INSUFFICIENT_PRIVILEGES ] );
	HString str;
	for ( int i = 1; ! ( str = get_token( message_, ",", i ) ).is_empty() ; i += 2 ) {
		int col( 0 );
		int row( 0 );
		try {
			col = lexical_cast<int>( str );
			row = lexical_cast<int>( get_token( message_, ",", i + 1 ) );
		} catch ( HLexicalCastException const& ) {
			throw HLogicException( GO_MSG[GO_MSG_MALFORMED] );
		}
		out << "dead: " << col << "," << row << endl;
		ensure_coordinates_validity( col, row );
		STONE::stone_t stone = goban( col, row );
		if( ! ( ( stone == STONE::BLACK ) || ( stone == STONE::WHITE )
					|| ( stone == STONE::DEAD_BLACK ) || ( stone == STONE::DEAD_WHITE ) ) )
			throw HLogicException( "no stone here" );
		if ( contestant( goban( col, row ) ) != clientInfo_ )
			throw HLogicException( "not your stone" );
		mark_stone_dead( col, row );
	}
	send_goban();
	return;
	M_EPILOG
}

void HGo::handler_accept( OClientInfo* clientInfo_ ) {
	M_PROLOG
	if ( _state != STONE::MARK )
		throw HLogicException( GO_MSG[ GO_MSG_INSUFFICIENT_PRIVILEGES ] );
	OPlayerInfo& info = *get_player_info( clientInfo_ );
	if ( info._byoYomiPeriods == ACCEPTED )
		throw HLogicException( "you already accepted stones" );
	info._byoYomiPeriods = ACCEPTED;
	if ( ( get_player_info( _contestants[ 0 ] )->_byoYomiPeriods == ACCEPTED )
			&& ( get_player_info( _contestants[ 1 ] )->_byoYomiPeriods == ACCEPTED ) ) {
		count_score();
		_state = STONE::NONE;
	}
	return;
	M_EPILOG
}

void HGo::handler_newgame( OClientInfo* clientInfo_, HString const& ) {
	M_PROLOG
	if ( ! can_setup( clientInfo_ ) )
		throw HLogicException( GO_MSG[ GO_MSG_INSUFFICIENT_PRIVILEGES ] );
	_start = 0;
	_move = 0;
	set_handicaps( _handicaps ); /* calls _sgf.clear() */
	_sgf.set_info( SGF::Player::BLACK, _gobanSize, _handicaps, _komi, _mainTime );
	send_goban();
	return;
	M_EPILOG
}

void HGo::handler_undo( OClientInfo* /* clientInfo_ */ ) {
	M_PROLOG
	return;
	M_EPILOG
}

void HGo::mark_teritory( void ) {
	M_PROLOG
	for ( int i = 0; i < ( _gobanSize * _gobanSize ); ++ i ) {
		int x = i / _gobanSize;
		int y = i % _gobanSize;
		if ( goban( x, y ) == STONE::NONE ) {
			STONE::stone_t teritory = mark_teritory( x, y );
			STONE::stone_t mark = STONE::TERITORY_NONE;
			switch ( teritory ) {
				case ( STONE::BLACK ): mark = STONE::TERITORY_BLACK; break;
				case ( STONE::WHITE ): mark = STONE::TERITORY_WHITE; break;
				case ( STONE::TERITORY_NONE ): mark = STONE::TERITORY_NONE; break;
				default:
					out << "teritory: '" << teritory << "'" << endl;
					M_ASSERT( ! "bug in count_score switch" );
			}
			replace_stones( static_cast<char>( toupper( STONE::TERITORY_NONE ) ), mark );
		}
	}
	M_EPILOG
}

void HGo::count_score( void ) {
	M_PROLOG
	mark_teritory();
	replace_stones( static_cast<char>( toupper( STONE::DEAD_BLACK ) ), STONE::DEAD_BLACK );
	replace_stones( static_cast<char>( toupper( STONE::DEAD_WHITE ) ), STONE::DEAD_WHITE );
	commit();
	int blackTeritory = count_stones( STONE::TERITORY_BLACK );
	int whiteTeritory = count_stones( STONE::TERITORY_WHITE );
	int blackCaptures = count_stones( STONE::DEAD_BLACK );
	int whiteCaptures = count_stones( STONE::DEAD_WHITE );
	blackTeritory += whiteCaptures;
	whiteTeritory += blackCaptures;
	broadcast( _out << PROTOCOL::MSG << PROTOCOL::SEP
			<< "The game results are: " << endl << _out );
	OPlayerInfo& b = *get_player_info( _contestants[ 0 ] );
	OPlayerInfo& w = *get_player_info( _contestants[ 1 ] );
	b._stonesCaptured += whiteCaptures;
	w._stonesCaptured += blackCaptures;
	b._score = b._stonesCaptured + blackTeritory;
	w._score += ( w._stonesCaptured + whiteTeritory );
	broadcast( _out << PROTOCOL::MSG << PROTOCOL::SEP
			<< "Black teritory: " << blackTeritory
			<< ", captutes: " << b._stonesCaptured << "." << endl << _out );
	broadcast( _out << PROTOCOL::MSG << PROTOCOL::SEP
			<< "White teritory: " << whiteTeritory
			<< ", captutes: " << w._stonesCaptured
			<< ", and " << _komi << " of komi." << endl << _out );
	broadcast( _out << PROTOCOL::MSG << PROTOCOL::SEP
			<< ( b._score > w._score ? "Black" : "White" )
			<< " wins by " << ( b._score > w._score ? b._score - w._score : w._score - b._score ) + .5
			<< endl << _out );
	send_goban();
	return;
	M_EPILOG
}

HGo::STONE::stone_t HGo::mark_teritory( int x, int y ) {
	STONE::stone_t teritory = static_cast<char>( toupper( STONE::TERITORY_NONE ) );
	STONE::stone_t stone = goban( x, y );
	if ( ( stone == STONE::NONE ) || ( stone == STONE::DEAD_BLACK ) || ( stone == STONE::DEAD_WHITE ) ) {
		if ( stone == STONE::NONE )
			goban( x, y ) = teritory;
		else
			goban( x, y ) = static_cast<char>( toupper( stone ) );
		int blackNeighbour = 0;
		int whiteNeighbour = 0;
		int bothNeighbour = 0;
		int directs[][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
		for ( int i = 0; i < 4; ++ i ) {
			int nx = x + directs[ i ][ 0 ];
			int ny = y + directs[ i ][ 1 ];
			if ( ( nx >= 0 ) && ( nx < _gobanSize )
				&& ( ny >= 0 ) && ( ny < _gobanSize ) ) {
				STONE::stone_t neighbour = goban( nx, ny );
				if ( ( neighbour != teritory )
						&& ( neighbour != toupper( STONE::DEAD_BLACK ) )
						&& ( neighbour != toupper( STONE::DEAD_WHITE ) ) ) {
					STONE::stone_t NewTeritory = mark_teritory( nx, ny );
					if ( NewTeritory == STONE::BLACK )
						++ blackNeighbour;
					else  if ( NewTeritory == STONE::WHITE )
						++ whiteNeighbour;
					else
						++ bothNeighbour;
				}
			}
		}
		if ( bothNeighbour || ( blackNeighbour && whiteNeighbour ) )
			teritory = STONE::TERITORY_NONE;
		else if ( blackNeighbour )
			teritory = STONE::BLACK;
		else
			teritory = STONE::WHITE;
	} else if ( ( stone == STONE::BLACK ) || ( stone == STONE::WHITE ) || ( stone == toupper ( STONE::TERITORY_NONE ) ) )
		teritory = stone;
	else
		M_ASSERT( ( stone == toupper( STONE::DEAD_BLACK ) ) || ( stone == toupper( STONE::DEAD_WHITE ) ) );
	return ( teritory );
}

void HGo::handler_play( OClientInfo* clientInfo_, HString const& message_ ) {
	M_PROLOG
	HLock l( _mutex );
	HString item = get_token( message_, ",", 0 );
	if ( ! can_play( clientInfo_ ) )
		throw HLogicException( "you are not playing" );
	if ( item == PROTOCOL::PUTSTONE )
		handler_put_stone( clientInfo_, message_ );
	else if ( item == PROTOCOL::PASS )
		handler_pass( clientInfo_, message_ );
	else if ( item == PROTOCOL::DEAD )
		handler_dead( clientInfo_, message_ );
	else if ( item == PROTOCOL::ACCEPT )
		handler_accept( clientInfo_ );
	else if ( item == PROTOCOL::UNDO )
		handler_undo( clientInfo_ );
	else
		throw HLogicException( GO_MSG[ GO_MSG_MALFORMED ] );
	after_move();
	return;
	M_EPILOG
}

void HGo::after_move( void ) {
	M_PROLOG
	broadcast( _out << PROTOCOL::TOMOVE << PROTOCOL::SEP
			<< static_cast<char>( _state ) << endl << _out );
	if ( ( _state == STONE::BLACK ) || ( _state == STONE::WHITE ) )
		update_clocks();
	send_contestants();
	return;
	M_EPILOG
}

HGo::players_t::iterator HGo::find_player( OClientInfo* clientInfo_ ) {
	M_PROLOG
	players_t::iterator it;
	for ( it = _players.begin(); it != _players.end(); ++ it )
		if ( it->first == clientInfo_ )
			break;
	return ( it );
	M_EPILOG
}

HGo::OPlayerInfo* HGo::get_player_info( OClientInfo* clientInfo_ ) {
	M_PROLOG
	players_t::iterator it = find_player( clientInfo_ );
	M_ASSERT( it != _players.end() );
	return ( &it->second );
	M_EPILOG
}

bool HGo::do_accept( OClientInfo* clientInfo_ ) {
	out << "new candidate " << clientInfo_->_login << endl;
	return ( false );
}

void HGo::do_post_accept( OClientInfo* clientInfo_ ) {
	M_PROLOG
	HLock l( _mutex );
	if ( _players.size() == 0 )
		*clientInfo_->_socket << *this
			<< PROTOCOL::SETUP << PROTOCOL::SEP << PROTOCOL::ADMIN << endl;
	*clientInfo_->_socket
		<< *this << PROTOCOL::SETUP << PROTOCOL::SEP
		<< PROTOCOL::GOBAN << PROTOCOL::SEPP << _gobanSize << endl
		<< *this << PROTOCOL::SETUP << PROTOCOL::SEP
		<< PROTOCOL::KOMI << PROTOCOL::SEPP << _komi << endl
		<< *this << PROTOCOL::SETUP << PROTOCOL::SEP
		<< PROTOCOL::HANDICAPS << PROTOCOL::SEPP << _handicaps << endl
		<< *this << PROTOCOL::SETUP << PROTOCOL::SEP
		<< PROTOCOL::MAINTIME << PROTOCOL::SEPP << _mainTime << endl
		<< *this << PROTOCOL::SETUP << PROTOCOL::SEP
		<< PROTOCOL::BYOYOMIPERIODS << PROTOCOL::SEPP << _byoYomiPeriods << endl
		<< *this << PROTOCOL::SETUP << PROTOCOL::SEP
		<< PROTOCOL::BYOYOMITIME << PROTOCOL::SEPP << _byoYomiTime << endl;
	player_t info;
	info.first = clientInfo_;
	_players.push_back( info );
	for ( clients_t::HIterator it = _clients.begin(); it != _clients.end(); ++ it ) {
		if ( *it != clientInfo_ ) {
			*clientInfo_->_socket << *this
					<< PROTOCOL::PLAYER << PROTOCOL::SEP
					<< (*it)->_login << endl;
			*clientInfo_->_socket << *this
					<< PROTOCOL::MSG << PROTOCOL::SEP
					<< "Player " << (*it)->_login << " approached this table." << endl;
		}
	}
	broadcast( _out << PROTOCOL::PLAYER << PROTOCOL::SEP
			<< clientInfo_->_login << endl << _out );
	broadcast( _out << PROTOCOL::MSG << PROTOCOL::SEP
			<< "Player " << clientInfo_->_login << " approached this table." << endl << _out );
	send_contestants( clientInfo_ );
	send_goban( clientInfo_ );
	return;
	M_EPILOG
}

void HGo::do_kick( OClientInfo* clientInfo_ ) {
	M_PROLOG
	HLock l( _mutex );
	bool newadmin = false;
	players_t::iterator it = find_player( clientInfo_ );
	if ( it == _players.begin() )
		newadmin = true;
	_players.erase( it );
	if ( newadmin ) {
		it = _players.begin();
		while ( ( it != _players.end() ) && ! it->first->_valid )
			++ it;
		if ( it != _players.end() ) {
			try {
				*it->first->_socket << *this
					<< PROTOCOL::SETUP << PROTOCOL::SEP << PROTOCOL::ADMIN << endl;
			} catch ( HOpenSSLException const& ) {
				drop_client( it->first );
			}
		}
	}
	if ( ( contestant( STONE::BLACK ) == clientInfo_ )
			|| ( contestant( STONE::WHITE ) == clientInfo_ ) ) {
		STONE::stone_t stone = ( contestant( STONE::BLACK ) == clientInfo_ ? STONE::BLACK : STONE::WHITE );
		contestant_gotup( contestant( stone ) );
		send_contestants();
	}
	broadcast( _out << PROTOCOL::MSG << PROTOCOL::SEP
			<< "Player " << clientInfo_->_login << " left this match." << endl << _out );
	return;
	M_EPILOG
}

yaal::hcore::HString HGo::do_get_info() const {
	HLock l( _mutex );
	return ( HString( "go," ) + get_comment() + "," + _gobanSize + "," + _komi + "," + _handicaps + "," + _mainTime + "," + _byoYomiPeriods + "," + _byoYomiTime );
}

void HGo::reschedule_timeout( void ) {
	M_PROLOG
	HAsyncCaller::get_instance().register_call( 0, call( &HGo::schedule_timeout, this ) );
	return;
	M_EPILOG
}

void HGo::schedule_timeout( void ) {
	M_PROLOG
	++ _move;
	OPlayerInfo& p = *get_player_info( contestant( _state ) );
	if ( p._byoYomiPeriods < _byoYomiPeriods )
		p._timeLeft = _byoYomiTime;
	HScheduledAsyncCaller::get_instance().register_call( time( NULL ) + p._timeLeft, call( &HGo::on_timeout, this ) );
	return;
	M_EPILOG
}

void HGo::on_timeout( void ) {
	M_PROLOG
	HLock l( _mutex );
	OPlayerInfo& p = *get_player_info( contestant( _state ) );
	-- p._byoYomiPeriods;
	if ( p._byoYomiPeriods < 0 ) {
		_state = STONE::NONE;
		broadcast( _out << PROTOCOL::TOMOVE << PROTOCOL::SEP
				<< static_cast<char>( _state ) << endl << _out );
		broadcast( _out << PROTOCOL::MSG << PROTOCOL::SEP << "End of time." << endl << _out );
	} else {
		p._timeLeft = _byoYomiTime;
		reschedule_timeout();
		send_contestants();
	}
	return;
	M_EPILOG
}

void HGo::set_handicaps( int handicaps_ ) {
	M_PROLOG
	if ( ( handicaps_ > 9 ) || ( handicaps_ < 0 ) )
		throw HLogicException( _out << "bad handicap value: " << handicaps_ << _out );
	_sgf.clear();
	_sgf.set_info( SGF::Player::BLACK, _gobanSize, _handicaps, _komi, _mainTime );
	::memset( _game.raw(), STONE::NONE, _gobanSize * _gobanSize );
	_game.get<char>()[ _gobanSize * _gobanSize ] = 0;
	::memset( _koGame.raw(), STONE::NONE, _gobanSize * _gobanSize );
	_koGame.get<char>()[ _gobanSize * _gobanSize ] = 0;
	if ( handicaps_ != _handicaps ) {
		if ( handicaps_ > 0 )
			_komi = 0;
		else
			_komi = setup._komi;
		_sgf.set_handicap( _handicaps = handicaps_ );
	}
	if ( _handicaps > 1 )
		set_handi( _handicaps );
	broadcast( _out << PROTOCOL::SETUP << PROTOCOL::SEP
			<< PROTOCOL::KOMI << PROTOCOL::SEPP << _komi << endl << _out );
	send_goban();
	return;
	M_EPILOG
}

void HGo::set_handi( int handi_ ) {
	M_PROLOG
	int hoshi( 3 - ( _gobanSize == GOBAN_SIZE::TINY ? 1 : 0 ) );
	int col( 0 );
	int row( 0 );
	switch ( handi_ ) {
		case ( 9 ):
			put_stone( col = _gobanSize / 2, row = _gobanSize / 2, STONE::BLACK );
			_sgf.add_position( SGF::Position::BLACK, SGF::Coord( col, row ) );
		case ( 8 ):
			set_handi( 6 );
			put_stone( col = _gobanSize / 2, row = hoshi, STONE::BLACK );
			_sgf.add_position( SGF::Position::BLACK, SGF::Coord( col, row ) );
			put_stone( col = _gobanSize / 2, row = ( _gobanSize - hoshi ) - 1, STONE::BLACK );
			_sgf.add_position( SGF::Position::BLACK, SGF::Coord( col, row ) );
		break;
		case ( 7 ):
			put_stone( col = _gobanSize / 2, row = _gobanSize / 2, STONE::BLACK );
			_sgf.add_position( SGF::Position::BLACK, SGF::Coord( col, row ) );
		case ( 6 ):
			set_handi( 4 );
			put_stone( col = hoshi, row = _gobanSize / 2, STONE::BLACK );
			_sgf.add_position( SGF::Position::BLACK, SGF::Coord( col, row ) );
			put_stone( col = ( _gobanSize - hoshi ) - 1, row = _gobanSize / 2, STONE::BLACK );
			_sgf.add_position( SGF::Position::BLACK, SGF::Coord( col, row ) );
		break;
		case ( 5 ):
			put_stone( col = _gobanSize / 2, row = _gobanSize / 2, STONE::BLACK );
			_sgf.add_position( SGF::Position::BLACK, SGF::Coord( col, row ) );
		case ( 4 ):
			put_stone( col = ( _gobanSize - hoshi ) - 1, row = ( _gobanSize - hoshi ) - 1, STONE::BLACK );
			_sgf.add_position( SGF::Position::BLACK, SGF::Coord( col, row ) );
		case ( 3 ):
			put_stone( col = hoshi, row = hoshi, STONE::BLACK );
			_sgf.add_position( SGF::Position::BLACK, SGF::Coord( col, row ) );
		case ( 2 ):
			put_stone( col = hoshi, row = ( _gobanSize - hoshi ) - 1, STONE::BLACK );
			_sgf.add_position( SGF::Position::BLACK, SGF::Coord( col, row ) );
			put_stone( col = ( _gobanSize - hoshi ) - 1, row = hoshi, STONE::BLACK );
			_sgf.add_position( SGF::Position::BLACK, SGF::Coord( col, row ) );
		break;
		default:
			M_ASSERT( ! "unhandled case" );
	}
	return;
	M_EPILOG
}

void HGo::put_stone( int col_, int row_, STONE::stone_t stone_ ) {
	M_PROLOG
	_game.get<char>()[ row_ * _gobanSize + col_ ] = stone_;
	return;
	M_EPILOG
}

void HGo::send_goban( OClientInfo* clientInfo_ ) {
	M_PROLOG
	_out << PROTOCOL::SGF << PROTOCOL::SEP;
	_sgf.save( _out, true );
	_out << endl;
	if ( clientInfo_ )
		*clientInfo_->_socket << *this << _out.consume();
	else
		broadcast( _out.consume() );
	_path.clear();
	SGF::game_tree_t::const_node_t n( _sgf.get_current_move() );
	int moveNo( 0 );
	while ( n ) {
		SGF::game_tree_t::const_node_t p( n->get_parent() );
		if ( p ) {
			if ( p->child_count() > 1 ) {
				int idx( 0 );
				for ( SGF::game_tree_t::HNode::const_iterator it( p->begin() ); &*it != n; ++ it, ++ idx )
					;
				_path.push_back( idx );
			}
			++ moveNo;
		}
		n = p;
	}
	_out << PROTOCOL::SELECT << PROTOCOL::SEP << moveNo;
	for ( int i( static_cast<int>( _path.get_size() - 1 ) ); i >= 0; -- i )
		_out << ',' << _path[i];
	_out << endl;
	if ( clientInfo_ )
		*clientInfo_->_socket << *this << _out.consume();
	else
		broadcast( _out.consume() );
	return;
	M_EPILOG
}

char& HGo::goban( int col_, int row_ ) {
	return ( _koGame.get<char>()[ row_ * _gobanSize + col_ ] );
}

bool HGo::have_liberties( int col_, int row_, STONE::stone_t stone ) {
	if ( ( col_ < 0 ) || ( col_ > ( _gobanSize - 1 ) )
			|| ( row_ < 0 ) || ( row_ > ( _gobanSize - 1 ) ) )
		return ( false );
	if ( goban( col_, row_ ) == STONE::NONE )
		return ( true );
	if ( goban( col_, row_ ) == stone ) {
		goban( col_, row_ ) = static_cast<char>( toupper( stone ) );
		return ( have_liberties( col_, row_ - 1, stone )
				|| have_liberties( col_, row_ + 1, stone )
				|| have_liberties( col_ - 1, row_, stone )
				|| have_liberties( col_ + 1, row_, stone ) );
	}
	return ( false );
}

void HGo::clear_goban( bool removeDead ) {
	for ( int i = 0; i < _gobanSize; i++ ) {
		for ( int j = 0; j < _gobanSize; j++ ) {
			if ( goban( i, j ) != STONE::NONE ) {
				if ( removeDead && isupper( goban( i, j ) ) )
					goban( i, j ) = STONE::NONE;
				else
					goban( i, j ) = static_cast<char>( tolower( goban( i, j ) ) );
			}
		}
	}
}

HGo::STONE::stone_t HGo::opponent( STONE::stone_t stone ) {
	return ( stone == STONE::WHITE ? STONE::BLACK : STONE::WHITE );
}

bool HGo::have_killed( int x, int y, STONE::stone_t stone ) {
	bool killed = false;
	STONE::stone_t foeStone = opponent( stone );
	goban( x, y ) = stone;
	if ( ( x > 0 ) && ( goban( x - 1, y ) == foeStone ) && ( ! have_liberties( x - 1, y, foeStone ) ) )
		clear_goban( killed = true );
	else
		clear_goban( false );
	if ( ( x < ( _gobanSize - 1 ) ) && ( goban( x + 1, y ) == foeStone ) && ( ! have_liberties( x + 1, y, foeStone ) ) )
		clear_goban( killed = true );
	else
		clear_goban( false );
	if ( ( y > 0 ) && ( goban( x, y - 1 ) == foeStone ) && ( ! have_liberties( x, y - 1, foeStone ) ) )
		clear_goban( killed = true );
	else
		clear_goban( false );
	if ( ( y < ( _gobanSize - 1 ) ) && ( goban( x, y + 1 ) == foeStone ) && ( ! have_liberties( x, y + 1, foeStone ) ) )
		clear_goban( killed = true );
	else
		clear_goban( false );
	goban( x, y ) = STONE::NONE;
	return ( killed );
}

bool HGo::is_ko( void ) {
	return ( ::memcmp( _koGame.raw(), _oldGame.raw(), _gobanSize * _gobanSize ) == 0 );
}

bool HGo::is_suicide( int x, int y, STONE::stone_t stone ) {
	bool suicide = false;
	goban( x, y ) = stone;
	if ( ! have_liberties( x, y, stone ) )
		suicide = true;
	clear_goban( false );
	goban( x, y ) = STONE::NONE;
	return ( suicide );
}	

void HGo::ensure_coordinates_validity( int x, int y ) {
	if ( ( x < 0 ) || ( x > ( _gobanSize - 1 ) )
			|| ( y < 0 ) || ( y > ( _gobanSize - 1 ) ) )
		throw HLogicException( "move outside goban" );
}

void HGo::make_move( int x, int y, STONE::stone_t stone ) {
	M_PROLOG
	ensure_coordinates_validity( x, y );
	::memcpy( _koGame.raw(), _game.raw(), _gobanSize * _gobanSize );
	if ( goban( x, y ) != STONE::NONE )
		throw HLogicException( "position already occupied" );
	if ( ! have_killed( x, y, stone ) ) {
		if ( is_suicide( x, y, stone ) ) {
			clear_goban( false );
			throw HLogicException( "suicides forbidden" );
		}
	}	
	goban( x, y ) = stone;
	if ( is_ko() )
		throw HLogicException( "forbidden by ko rule" );
	::memcpy( _oldGame.raw(), _game.raw(), _gobanSize * _gobanSize );
	commit();
	return;
	M_EPILOG
}

void HGo::commit( void ) {
	M_PROLOG
	::memcpy( _game.raw(), _koGame.raw(), _gobanSize * _gobanSize );
	return;
	M_EPILOG
}

void HGo::mark_stone_dead( int col, int row ) {
	M_PROLOG
	STONE::stone_t stone = goban( col, row );
	switch ( stone ) {
		case ( STONE::BLACK ) : stone = STONE::DEAD_BLACK; break;
		case ( STONE::WHITE ) : stone = STONE::DEAD_WHITE; break;
		case ( STONE::DEAD_BLACK ) : stone = STONE::BLACK; break;
		case ( STONE::DEAD_WHITE ) : stone = STONE::WHITE; break;
		default:
			M_ASSERT( ! "predicate error for switch( stone )" );
	}
	goban( col, row ) = stone;
	commit();
	return;
	M_EPILOG
}

OClientInfo*& HGo::contestant( STONE::stone_t stone ) {
	M_ASSERT( ( stone == STONE::BLACK ) || ( stone == STONE::WHITE )
			|| ( stone == STONE::DEAD_BLACK ) || ( stone == STONE::DEAD_WHITE ) );
	return ( ( stone == STONE::BLACK ) || ( stone == STONE::DEAD_BLACK ) ? _contestants[ 0 ] : _contestants[ 1 ] );
}

OClientInfo const* HGo::contestant( STONE::stone_t stone ) const {
	M_ASSERT( ( stone == STONE::BLACK ) || ( stone == STONE::WHITE )
			|| ( stone == STONE::DEAD_BLACK ) || ( stone == STONE::DEAD_WHITE ) );
	return ( ( stone == STONE::BLACK ) || ( stone == STONE::DEAD_BLACK ) ? _contestants[ 0 ] : _contestants[ 1 ] );
}

void HGo::contestant_gotup( OClientInfo* clientInfo_ ) {
	STONE::stone_t stone = ( contestant( STONE::BLACK ) == clientInfo_ ? STONE::BLACK : STONE::WHITE );
	OClientInfo* foe = NULL;
	if ( ( _state != STONE::NONE )
			&& ( ( foe = contestant( STONE::BLACK ) ) || ( foe = contestant( STONE::WHITE ) ) ) )
		broadcast( _out << PROTOCOL::MSG << PROTOCOL::SEP
				<< clientInfo_->_login << " resigned - therefore " << foe->_login << " wins." << endl << _out );
	contestant( stone ) = NULL;
	_state = STONE::NONE;
	return;
}

void HGo::send_contestants( OClientInfo* clientInfo_ ) {
	M_PROLOG
	send_contestant( STONE::BLACK, clientInfo_ );
	send_contestant( STONE::WHITE, clientInfo_ );
	return;
	M_EPILOG
}

void HGo::send_contestant( char stone, OClientInfo* clientInfo_ ) {
	M_PROLOG
	OClientInfo* cinfo = contestant( stone );
	char const* name = "";
	int captured = 0;
	int time = 0;
	int byoyomi = 0;
	int score = 0;
	if ( cinfo ) {
		OPlayerInfo& info = *get_player_info( cinfo );
		name = cinfo->_login.raw();
		captured = info._stonesCaptured;
		time = static_cast<int>( info._timeLeft );
		byoyomi = info._byoYomiPeriods;
		score = info._score;
	}
	_out << PROTOCOL::CONTESTANT << PROTOCOL::SEP
			<< stone << PROTOCOL::SEPP
			<< name << PROTOCOL::SEPP
			<< captured << PROTOCOL::SEPP
			<< score << PROTOCOL::SEPP
			<< time << PROTOCOL::SEPP
			<< byoyomi << endl;
	if ( clientInfo_ )
		*clientInfo_->_socket << *this << _out.consume();
	else
		broadcast( _out.consume() );
	return;
	M_EPILOG
}

int HGo::count_stones( STONE::stone_t stone ) {
	M_PROLOG
	return ( static_cast<int>( count( _game.get<char>(), _game.get<char>() + _gobanSize * _gobanSize, stone ) ) );
	M_EPILOG
}

void HGo::replace_stones( STONE::stone_t which, STONE::stone_t with ) {
	M_PROLOG
	replace( _koGame.get<char>(), _koGame.get<char>() + _gobanSize * _gobanSize, which, with );
	return;
	M_EPILOG
}

bool HGo::can_play( OClientInfo* clientInfo_ ) const {
	M_PROLOG
	return ( ( _state != STONE::NONE ) && ( ( contestant( STONE::BLACK ) == clientInfo_ ) || ( contestant( STONE::WHITE ) == clientInfo_ ) ) );
	M_EPILOG
}

bool HGo::can_setup( OClientInfo* clientInfo_ ) const {
	M_PROLOG
	return ( ( _state == STONE::NONE ) && ! _players.is_empty() && ( _players.begin()->first == clientInfo_ ) );
	M_EPILOG
}

void HGo::update_clocks( void ) {
	M_PROLOG
	revoke_scheduled_tasks();
	int long now = time( NULL );
	OPlayerInfo& p = *get_player_info( contestant( opponent( _state ) ) );
	if ( _start )
		p._timeLeft -= ( now - _start );
	schedule_timeout();
	_start = now;
	return;
	M_EPILOG
}

void HGo::revoke_scheduled_tasks( void ) {
	M_PROLOG
	HAsyncCaller::get_instance().flush( this );
	HScheduledAsyncCaller::get_instance().flush( this );
	return;
	M_EPILOG
}

}

namespace logic_factory {

class HGoCreator : public HLogicCreatorInterface {
protected:
	virtual HLogic::ptr_t do_new_instance( HServer*, HLogic::id_t const&, HString const& );
	virtual HString do_get_info( void ) const;
} goCreator;

HLogic::ptr_t HGoCreator::do_new_instance( HServer* server_, HLogic::id_t const& id_, HString const& argv_ ) {
	M_PROLOG
	out << "creating logic: " << argv_ << endl;
	HString name = get_token( argv_, ",", 0 );
	return ( make_pointer<go::HGo>( server_, id_, name ) );
	M_EPILOG
}

HString HGoCreator::do_get_info( void ) const {
	M_PROLOG
	HString setupMsg;
	setupMsg.format( "go:%d,%d,%d,%d,%d,%d", setup._gobanSize, setup._komi, setup._handicaps,
			setup._mainTime, setup._byoYomiPeriods, setup._byoYomiTime );
	out << setupMsg << endl;
	return ( setupMsg );
	M_EPILOG
}

namespace {

bool registrar( void ) {
	M_PROLOG
	bool volatile failed = false;
	HLogicFactory& factory = HLogicFactoryInstance::get_instance();
	factory.register_logic_creator( HTokenizer( goCreator.get_info(), ":" )[0], &goCreator );
	return ( failed );
	M_EPILOG
}

bool volatile registered = registrar();

}

}

}

