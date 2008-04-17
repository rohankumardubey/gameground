/*
---           `gameground' 0.0.0 (c) 1978 by Marcin 'Amok' Konarski            ---

	server.cxx - this file is integral part of `gameground' project.

	i.  You may not make any changes in Copyright information.
	ii. You must attach Copyright information to any part of every copy
	    of this software.

Copyright:

 You are free to use this program as is, you can redistribute binary
 package freely but:
  1. You can not use any part of sources of this software.
  2. You can not redistribute any part of sources of this software.
  3. No reverse engineering is allowed.
  4. If you want redistribute binary package you can not demand any fees
     for this software.
     You can not even demand cost of the carrier (CD for example).
  5. You can not include it to any commercial enterprise (for example 
     as a free add-on to payed software or payed newspaper).
 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE. Use it at your own risk.
*/

#include <iostream>

#include <yaal/yaal.h>
M_VCSID ( "$Id$" )
#include "server.h"

#include "setup.h"
#include "logicfactory.h"

using namespace std;
using namespace yaal;
using namespace yaal::hcore;
using namespace yaal::hconsole;
using namespace yaal::tools;
using namespace yaal::tools::util;

namespace gameground
{

namespace
{

HString const& mark( int a_iColor )
	{
	static HString buf;
	buf = "$";
	buf += a_iColor;
	buf += ";###;$7;";
	return ( buf );
	}

}

char const* const HServer::PROTOCOL::ABANDON = "abandon";
char const* const HServer::PROTOCOL::CMD = "cmd";
char const* const HServer::PROTOCOL::CREATE = "create";
char const* const HServer::PROTOCOL::ERR = "err";
char const* const HServer::PROTOCOL::GAME = "game";
char const* const HServer::PROTOCOL::GET_GAMES = "get_games";
char const* const HServer::PROTOCOL::GET_GAME_INFO = "get_game_info";
char const* const HServer::PROTOCOL::GET_LOGISTICS = "get_logics";
char const* const HServer::PROTOCOL::GET_PLAYERS = "get_players";
char const* const HServer::PROTOCOL::JOIN = "join";
char const* const HServer::PROTOCOL::KCK = "kck";
char const* const HServer::PROTOCOL::LOGIC = "logic";
char const* const HServer::PROTOCOL::MSG = "msg";
char const* const HServer::PROTOCOL::NAME = "name";
char const* const HServer::PROTOCOL::PLAYER = "player";
char const* const HServer::PROTOCOL::PLAYER_QUIT = "player_quit";
char const* const HServer::PROTOCOL::QUIT = "quit";
char const* const HServer::PROTOCOL::SEP = ":";
char const* const HServer::PROTOCOL::SEPP = ",";
char const* const HServer::PROTOCOL::SHUTDOWN = "shutdown";

HServer::HServer( int a_iConnections )
	: HProcess( a_iConnections ), f_iMaxConnections( a_iConnections ),
	f_oSocket( HSocket::TYPE::D_DEFAULT | HSocket::TYPE::D_NONBLOCKING | HSocket::TYPE::D_SSL_SERVER, a_iConnections ),
	f_oClients(), f_oLogics(), f_oHandlers(), _out()
	{
	M_PROLOG
	return;
	M_EPILOG
	}

HServer::~HServer( void )
	{
	out << brightred << "<<<GameGround>>>" << lightgray << " server finished." << endl;
	}

int HServer::init_server( int a_iPort )
	{
	M_PROLOG
	f_oSocket.listen ( "0.0.0.0", a_iPort );
	register_file_descriptor_handler ( f_oSocket.get_file_descriptor(), &HServer::handler_connection );
	f_oHandlers[ PROTOCOL::SHUTDOWN ] = &HServer::handler_shutdown;
	f_oHandlers[ PROTOCOL::QUIT ] = &HServer::handler_quit;
	f_oHandlers[ PROTOCOL::MSG ] = &HServer::handler_chat;
	f_oHandlers[ PROTOCOL::NAME ] = &HServer::set_client_name;
	f_oHandlers[ PROTOCOL::GET_LOGISTICS ] = &HServer::get_logics_info;
	f_oHandlers[ PROTOCOL::GET_PLAYERS ] = &HServer::get_players_info;
	f_oHandlers[ PROTOCOL::GET_GAMES ] = &HServer::get_games_info;
	f_oHandlers[ PROTOCOL::GET_GAME_INFO ] = &HServer::get_game_info;
	f_oHandlers[ PROTOCOL::CREATE ] = &HServer::create_game;
	f_oHandlers[ PROTOCOL::JOIN ] = &HServer::join_game;
	f_oHandlers[ PROTOCOL::ABANDON ] = &HServer::handler_abandon;
	f_oHandlers[ PROTOCOL::CMD ] = &HServer::pass_command;
	HProcess::init ( 3600 );
	out << brightblue << "<<<GameGround>>>" << lightgray << " server started." << endl;
	return ( 0 );
	M_EPILOG
	}

void HServer::broadcast( HString const& a_roMessage )
	{
	M_PROLOG
	for ( clients_t::iterator it = f_oClients.begin(); it != f_oClients.end(); ++ it )
		*it->second.f_oSocket << a_roMessage << endl;
	return;
	M_EPILOG
	}

void HServer::broadcast_to_interested( HString const& a_roMessage )
	{
	M_PROLOG
	for ( clients_t::iterator it = f_oClients.begin(); it != f_oClients.end(); ++ it )
		if ( ! it->second.f_oLogic )
			*it->second.f_oSocket << a_roMessage << endl;
	return;
	M_EPILOG
	}

void HServer::handler_chat( OClientInfo& a_roInfo, HString const& a_roMessage )
	{
	M_PROLOG
	broadcast_to_interested( _out << PROTOCOL::MSG << PROTOCOL::SEP << a_roInfo.f_oName << ": " << a_roMessage << _out );
	return;
	M_EPILOG
	}

void HServer::set_client_name( OClientInfo& a_roInfo, HString const& a_oName )
	{
	M_PROLOG
	clients_t::iterator it;
	int const D_MINIMUM_NAME_LENGTH = 4;
	for ( it = f_oClients.begin(); it != f_oClients.end(); ++ it )
		if ( ( ! ::strcasecmp( it->second.f_oName, a_oName ) ) && ( it->second.f_oSocket != a_roInfo.f_oSocket ) )
			break;
	if ( a_oName.find_other_than( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_" ) >= 0 )
		*a_roInfo.f_oSocket << "err:Name may only take form of `[a-zA-Z0-9]+'." << endl;
	else if ( a_oName.get_length() < D_MINIMUM_NAME_LENGTH )
		*a_roInfo.f_oSocket << "err:Your name is too short, it needs to be at least 4 character long." << endl;
	else if ( it != f_oClients.end() )
		*a_roInfo.f_oSocket << "err:Name taken." << endl;
	else
		{
		a_roInfo.f_oName = a_oName;
		broadcast_to_interested( _out << PROTOCOL::PLAYER << PROTOCOL::SEP << a_oName << _out );
		broadcast_to_interested( _out << PROTOCOL::MSG << PROTOCOL::SEP
				<< mark( COLORS::D_FG_BLUE ) << " " << a_oName << " entered the GameGround." << _out );
		}
	return;
	M_EPILOG
	}

void HServer::pass_command( OClientInfo& a_roInfo, HString const& a_oCommand )
	{
	M_PROLOG
	if ( ! a_roInfo.f_oLogic )
		a_roInfo.f_oSocket->write_until_eos( "err:Connect to some game first.\n" );
	else
		{
		HString msg;
		try
			{
			if ( a_roInfo.f_oLogic->process_command( &a_roInfo, a_oCommand ) )
				msg = "Game logic could not comprehend your message: ";
			}
		catch ( HLogicException& e )
			{
			msg = e.what();
			}
		if ( ! msg.is_empty() )
			remove_client_from_logic( a_roInfo, msg );
		}
	return;
	M_EPILOG
	}

void HServer::create_game( OClientInfo& a_roInfo, HString const& a_oArg )
	{
	M_PROLOG
	if ( a_roInfo.f_oName.is_empty() )
		kick_client( a_roInfo.f_oSocket, _( "Set your name first." ) );
	else
		{
		HString l_oType = a_oArg.split( ":", 0 );
		HString l_oConfiguration = a_oArg.split( ":", 1 );
		HString l_oName = l_oConfiguration.split( ",", 0 );
		HLogicFactory& factory = HLogicFactoryInstance::get_instance();
		logics_t::iterator it = f_oLogics.find( l_oName );
		if ( it != f_oLogics.end() )
			a_roInfo.f_oSocket->write_until_eos( "err:Game already exists.\n" );
		else if ( ! factory.is_type_valid( l_oType ) )
			kick_client( a_roInfo.f_oSocket, _( "No such game type." ) );
		else if ( l_oName.is_empty() )
			kick_client( a_roInfo.f_oSocket, _( "No game name given." ) );
		else
			{
			HLogic::ptr_t l_oLogic;
			try
				{
				l_oLogic = factory.create_logic( l_oType, l_oConfiguration );
				if ( ! l_oLogic->accept_client( &a_roInfo ) )
					{
					f_oLogics[ l_oName ] = l_oLogic;
					a_roInfo.f_oLogic = l_oLogic;
					out << l_oName << "," << l_oType << endl;
					broadcast_to_interested( _out << PROTOCOL::PLAYER << PROTOCOL::SEP
							<< a_roInfo.f_oName << PROTOCOL::SEPP << l_oLogic->get_info() << _out );
					}
				else
					a_roInfo.f_oSocket->write_until_eos( "err:Specified configuration is inconsistent.\n" );
				}
			catch ( HLogicException& e )
				{
				kick_client( a_roInfo.f_oSocket, e.what() );
				}
			}
		}
	return;
	M_EPILOG
	}

void HServer::join_game( OClientInfo& a_roInfo, HString const& a_oName )
	{
	M_PROLOG
	if ( a_roInfo.f_oName.is_empty() )
		kick_client( a_roInfo.f_oSocket, "Set your name first." );
	else
		{
		logics_t::iterator it = f_oLogics.find( a_oName );
		if ( it == f_oLogics.end() )
			a_roInfo.f_oSocket->write_until_eos( "err:Game does not exists.\n" );
		else if ( !! a_roInfo.f_oLogic )
			kick_client( a_roInfo.f_oSocket, _( "You were already in some game." ) );
		else if ( ! it->second->accept_client( &a_roInfo ) )
			{
			a_roInfo.f_oLogic = it->second;
			broadcast_to_interested( _out << PROTOCOL::PLAYER << PROTOCOL::SEP
					<< a_roInfo.f_oName << PROTOCOL::SEPP << a_roInfo.f_oLogic->get_info() << _out );
			}
		else
			a_roInfo.f_oSocket->write_until_eos( "err:Game is full.\n" );
		}
	return;
	M_EPILOG
	}

int HServer::handler_connection( int )
	{
	M_PROLOG
	HSocket::ptr_t l_oClient = f_oSocket.accept();
	M_ASSERT( !! l_oClient );
	if ( f_oSocket.get_client_count() >= f_iMaxConnections )
		l_oClient->close();
	else
		{
		register_file_descriptor_handler( l_oClient->get_file_descriptor(), &HServer::handler_message );
		f_oClients[ l_oClient->get_file_descriptor() ].f_oSocket = l_oClient;
		}
	out << static_cast<char const* const>( l_oClient->get_host_name() ) << endl;
	return ( 0 );
	M_EPILOG
	}

int HServer::handler_message( int a_iFileDescriptor )
	{
	M_PROLOG
	HString l_oMessage;
	HString l_oArgument;
	HString l_oCommand;
	clients_t::iterator clientIt;
	HSocket::ptr_t l_oClient = f_oSocket.get_client( a_iFileDescriptor );
	try
		{
		HSocket::HStreamInterface::STATUS const* status = NULL;
		if ( ( clientIt = f_oClients.find( a_iFileDescriptor ) ) == f_oClients.end() )
			kick_client( l_oClient );
		else if ( ( status = &l_oClient->read_until( l_oMessage ) )->code == HSocket::HStreamInterface::STATUS::D_OK )
			{
			if ( clientIt->second.f_oName.is_empty() )
				out << "`unnamed'";
			else
				out << clientIt->second.f_oName; 
			cout << "->" << static_cast<char const* const>( l_oMessage ) << endl;
			l_oCommand = l_oMessage.split( ":", 0 );
			l_oArgument = l_oMessage.mid( l_oCommand.get_length() + 1 );
			int l_iMsgLength = l_oCommand.get_length();
			if ( l_iMsgLength < 1 )
				kick_client( l_oClient, _( "Malformed data." ) );
			else
				{
				handlers_t::iterator it = f_oHandlers.find( l_oCommand );
				if ( it != f_oHandlers.end() )
					{
					HLogic::ptr_t l_oLogic = clientIt->second.f_oLogic;
					( this->*it->second )( clientIt->second, l_oArgument );
					if ( ( !! l_oLogic ) && ( ! l_oLogic->active_clients() ) )
						f_oLogics.remove( l_oLogic->get_name() );
					}
				else
					kick_client( l_oClient, _( "Unknown command." ) );
				}
			}
		else if ( status->code == HSocket::HStreamInterface::STATUS::D_ERROR )
			kick_client( l_oClient, "" );
		/* else status->code == HSocket::HStreamInterface::STATUS::D_REPEAT */
		}
	catch ( HOpenSSLException& )
		{
		kick_client( l_oClient );
		}
	return ( 0 );
	M_EPILOG
	}

void HServer::kick_client( yaal::hcore::HSocket::ptr_t& a_oClient, char const* const a_pcReason )
	{
	M_PROLOG
	M_ASSERT( !! a_oClient );
	int l_iFileDescriptor = a_oClient->get_file_descriptor();
	if ( a_pcReason && a_pcReason[0] )
		*a_oClient << PROTOCOL::KCK << PROTOCOL::SEP << a_pcReason << endl;
	f_oSocket.shutdown_client( l_iFileDescriptor );
	unregister_file_descriptor_handler( l_iFileDescriptor );
	clients_t::iterator clientIt = f_oClients.find( l_iFileDescriptor );
	M_ASSERT( clientIt != f_oClients.end() );
	out << "client ";
	if ( clientIt->second.f_oName.is_empty() )
		cout << "`unnamed'";
	else
		cout << clientIt->second.f_oName; 
	if ( ! a_pcReason || a_pcReason[ 0 ] )
		{
		HString reason = " was kicked because of: ";
		reason += ( a_pcReason ? a_pcReason : "connection error" );
		if ( ! clientIt->second.f_oName.is_empty() )
			broadcast_to_interested( _out << PROTOCOL::MSG << PROTOCOL::SEP
					<< mark( COLORS::D_FG_BRIGHTRED ) << " " << clientIt->second.f_oName << reason << _out );
		cout << reason;
		}
	else
		cout << " disconnected from server.";
	cout << endl;
	remove_client_from_logic( clientIt->second );
	HString name;
	if ( ! clientIt->second.f_oName.is_empty() )
		name = clientIt->second.f_oName;
	f_oClients.remove( l_iFileDescriptor );
	if ( ! name.is_empty() )
		broadcast_to_interested( _out << PROTOCOL::PLAYER_QUIT << PROTOCOL::SEP << name << _out );
	return;
	M_EPILOG
	}

void HServer::send_logics_info( OClientInfo& a_roInfo )
	{
	M_PROLOG
	HLogicFactory& factory = HLogicFactoryInstance::get_instance();
	for( HLogicFactory::creators_t::iterator it = factory.begin();
			it != factory.end(); ++ it )
		*a_roInfo.f_oSocket << PROTOCOL::LOGIC << PROTOCOL::SEP << it->second.f_oInfo << endl;
	return;
	M_EPILOG
	}

void HServer::get_logics_info( OClientInfo& a_roInfo, HString const& )
	{
	M_PROLOG
	send_logics_info( a_roInfo );
	return;
	M_EPILOG
	}

void HServer::handler_shutdown( OClientInfo&, HString const& )
	{
	f_bLoop = false;
	return;
	}

void HServer::handler_quit( OClientInfo& a_roInfo, HString const& )
	{
	M_PROLOG
	HString name = a_roInfo.f_oName;
	kick_client( a_roInfo.f_oSocket, "" );
	if ( ! name.is_empty() )
		broadcast_to_interested( _out << PROTOCOL::MSG << PROTOCOL::SEP
				<< mark( COLORS::D_FG_BROWN ) << " " << name << " has left the GameGround." << _out );
	return;
	M_EPILOG
	}

void HServer::handler_abandon( OClientInfo& a_roInfo, HString const& )
	{
	M_PROLOG
	remove_client_from_logic( a_roInfo );
	broadcast_to_interested( _out << PROTOCOL::PLAYER << PROTOCOL::SEP << a_roInfo.f_oName << _out );
	return;
	M_EPILOG
	}

void HServer::remove_client_from_logic( OClientInfo& a_roInfo, char const* const a_pcReason )
	{
	M_PROLOG
	if ( !! a_roInfo.f_oLogic )
		{
		HLogic::ptr_t l_oLogic = a_roInfo.f_oLogic;
		out << "separating logic info from client info for: " << a_roInfo.f_oName << endl;
		l_oLogic->kick_client( &a_roInfo, a_pcReason );
		a_roInfo.f_oLogic = HLogic::ptr_t();
		if ( ! l_oLogic->active_clients() )
			f_oLogics.remove( l_oLogic->get_name() );
		}
	return;
	M_EPILOG
	}

void HServer::get_players_info( OClientInfo& a_roInfo, HString const& )
	{
	M_PROLOG
	send_players_info( a_roInfo );
	return;
	M_EPILOG
	}

void HServer::get_games_info( OClientInfo& a_roInfo, HString const& )
	{
	M_PROLOG
	send_games_info( a_roInfo );
	return;
	M_EPILOG
	}

void HServer::get_game_info( OClientInfo& a_roInfo, HString const& a_oName )
	{
	M_PROLOG
	send_game_info( a_roInfo, a_oName );
	return;
	M_EPILOG
	}

void HServer::send_players_info( OClientInfo& a_roInfo )
	{
	M_PROLOG
	for( clients_t::iterator it = f_oClients.begin();
			it != f_oClients.end(); ++ it )
		{
		if ( ! it->second.f_oName.is_empty() )
			{
			*a_roInfo.f_oSocket << PROTOCOL::PLAYER << PROTOCOL::SEP << it->second.f_oName;
			if ( !! it->second.f_oLogic )
				*a_roInfo.f_oSocket << PROTOCOL::SEPP << it->second.f_oLogic->get_info();
			*a_roInfo.f_oSocket << endl;
			}
		}
	return;
	M_EPILOG
	}

void HServer::send_games_info( OClientInfo& a_roInfo )
	{
	M_PROLOG
	for( logics_t::iterator it = f_oLogics.begin();
			it != f_oLogics.end(); ++ it )
		*a_roInfo.f_oSocket << PROTOCOL::GAME << PROTOCOL::SEP << it->second->get_info() << endl;
	return;
	M_EPILOG
	}

void HServer::send_game_info( OClientInfo& /*a_roInfo*/, HString const& )
	{
	M_PROLOG
	return;
	M_EPILOG
	}


}

