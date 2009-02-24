/*
---           `gameground' 0.0.0 (c) 1978 by Marcin 'Amok' Konarski            ---

	gamegroundd.cxx - this file is integral part of `galaxy' project.

	i.  You may not make any changes in Copyright information.
	ii. You must attach Copyright information to any part of every copy
	    of this software.

Copyright:

 You are free to use this program as is, you can redistribute binary
 package freely but:
  1. You cannot use any part of sources of this software.
  2. You cannot redistribute any part of sources of this software.
  3. No reverse engineering is allowed.
  4. If you want redistribute binary package you cannot demand any fees
     for this software.
     You cannot even demand cost of the carrier (CD for example).
  5. You cannot include it to any commercial enterprise (for example 
     as a free add-on to payed software or payed newspaper).
 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE. Use it at your own risk.
*/

#include <yaal/yaal.hxx>
M_VCSID( "$Id: "__ID__" $" )
#include "galaxyd.hxx"

#include "setup.hxx"
#include "clientinfo.hxx"
#include "logicfactory.hxx"

using namespace yaal;
using namespace yaal::hcore;
using namespace yaal::hconsole;
using namespace yaal::tools;
using namespace yaal::tools::util;

namespace gameground
{

namespace galaxy
{

int n_piColors [ ] =
	{
	( COLORS::D_FG_BRIGHTBLUE | COLORS::D_BG_BLACK ),
	( COLORS::D_FG_BRIGHTGREEN | COLORS::D_BG_BLACK ),
	( COLORS::D_FG_BRIGHTRED | COLORS::D_BG_BLACK ),
	( COLORS::D_FG_BRIGHTCYAN | COLORS::D_BG_BLACK ),
	( COLORS::D_FG_BRIGHTMAGENTA | COLORS::D_BG_BLACK ),
	( COLORS::D_FG_YELLOW | COLORS::D_BG_BLACK ),
	( COLORS::D_FG_BLUE | COLORS::D_BG_BLACK ),
	( COLORS::D_FG_GREEN | COLORS::D_BG_BLACK ),
	( COLORS::D_FG_RED | COLORS::D_BG_BLACK ),
	( COLORS::D_FG_CYAN | COLORS::D_BG_BLACK ),
	( COLORS::D_FG_MAGENTA | COLORS::D_BG_BLACK ),
	( COLORS::D_FG_BROWN | COLORS::D_BG_BLACK ),
	( COLORS::D_FG_LIGHTGRAY | COLORS::D_BG_BLACK ),
	( COLORS::D_FG_LIGHTGRAY | COLORS::D_BG_BLACK ),
	( COLORS::D_FG_LIGHTGRAY | COLORS::D_BG_BLACK ),
	( COLORS::D_FG_LIGHTGRAY | COLORS::D_BG_BLACK )
	};

HSystem::HSystem ( void )
	: f_iId( -1 ), f_iCoordinateX( -1 ), f_iCoordinateY( -1 ),
	f_iProduction( -1 ), f_iFleet( -1 ), f_poEmperor( NULL ), f_oAttackers()
	{
	M_PROLOG
	return;
	M_EPILOG
	}

HSystem::HSystem( HSystem const& s )
	: f_iId( s.f_iId ), f_iCoordinateX( s.f_iCoordinateX ), f_iCoordinateY( s.f_iCoordinateY ),
	f_iProduction( s.f_iProduction ), f_iFleet( s.f_iFleet ), f_poEmperor( s.f_poEmperor ), f_oAttackers( s.f_oAttackers )
	{
	M_ASSERT( ! "wrong execution path" );
	}

HSystem& HSystem::operator = ( HSystem const& s )
	{
	if ( &s != this )
		{
		HSystem tmp( s );
		swap( tmp );
		}
	return ( *this );
	}

void HSystem::swap( HSystem& other )
	{
	if ( &other != this )
		{
		using yaal::swap;
		swap( f_iId, other.f_iId );
		swap( f_iCoordinateX, other.f_iCoordinateX );
		swap( f_iCoordinateY, other.f_iCoordinateY );
		swap( f_iProduction, other.f_iProduction );
		swap( f_iFleet, other.f_iFleet );
		swap( f_poEmperor, other.f_poEmperor );
		swap( f_oAttackers, other.f_oAttackers );
		M_ASSERT( ! "wrong execution path" );
		}
	return;
	}

void HSystem::do_round( HGalaxy& a_roGalaxy )
	{
	M_PROLOG
	int l_iColor = 0;
	HString l_oMessage;
	if ( f_poEmperor != NULL )
		f_iFleet += f_iProduction;
	else
		f_iFleet = f_iProduction;
	if ( f_oAttackers.size() )
		{
		int ec = 0;
		for ( attackers_t::iterator it = f_oAttackers.begin(); it != f_oAttackers.end(); )
			{
			it->f_iArrivalTime --;
			if ( it->f_iArrivalTime <= 0 )
				{
				l_iColor = f_poEmperor ? a_roGalaxy.get_color( f_poEmperor ) : 0;
				if ( it->f_poEmperor == f_poEmperor ) /* reinforcements */
					{
					f_iFleet += it->f_iSize;
					l_oMessage.format ( "glx:play:system_info=%c,%d,%d,%d,%d\n",
							'r', f_iId, f_iProduction, l_iColor, f_iFleet );
					f_poEmperor->f_oSocket->write_until_eos ( l_oMessage );
					}
				else if ( it->f_iSize <= f_iFleet ) /* failed attack */
					{
					f_iFleet -= it->f_iSize;
					l_iColor = a_roGalaxy.get_color ( it->f_poEmperor );
					l_oMessage.format ( "glx:play:system_info=%c,%d,%d,%d,%d\n",
							's', f_iId, f_iProduction,
							l_iColor, f_iFleet );
					if ( f_poEmperor )
						f_poEmperor->f_oSocket->write_until_eos ( l_oMessage ); /* defender */
					l_oMessage.format ( "glx:play:system_info=%c,%d,%d,%d,%d\n",
							'f', f_iId, f_iProduction, f_poEmperor ? a_roGalaxy.get_color ( f_poEmperor ) : -1, -1 );
					it->f_poEmperor->f_oSocket->write_until_eos ( l_oMessage ); /* attacker */
					}
				else if ( it->f_iSize > f_iFleet )
					{
					f_iFleet = it->f_iSize - f_iFleet;
					l_iColor = a_roGalaxy.get_color ( it->f_poEmperor );
					l_oMessage.format ( "glx:play:system_info=%c,%d,%d,%d,%d\n",
							'd', f_iId, f_iProduction, l_iColor, - 1 );
					if ( f_poEmperor )
						f_poEmperor->f_oSocket->write_until_eos ( l_oMessage ); /* loser */
					f_poEmperor = it->f_poEmperor;
					l_oMessage.format ( "glx:play:system_info=%c,%d,%d,%d,%d\n",
							'c', f_iId, f_iProduction, l_iColor, f_iFleet );
					it->f_poEmperor->f_oSocket->write_until_eos ( l_oMessage ); /* winer */
					}
				attackers_t::iterator done = it;
				++ it;
				log_trace << "ec: " << ec << endl;
				M_ASSERT( f_oAttackers.size() > 0 );
				f_oAttackers.erase( done );
				++ ec;
				}
			else
				{
				a_roGalaxy.mark_alive ( it->f_poEmperor );
				++ it;
				}
			}
		}
	if ( f_poEmperor != NULL )
		{
		l_oMessage.format ( "glx:play:system_info=%c,%d,%d,%d,%d\n",
				'i', f_iId, - 1, a_roGalaxy.get_color ( f_poEmperor ), f_iFleet );
		f_poEmperor->f_oSocket->write_until_eos ( l_oMessage );
		}
	if ( f_poEmperor )
		a_roGalaxy.mark_alive( f_poEmperor );
	return;
	M_EPILOG
	}

HGalaxy::HGalaxy( HString const& a_oName, int a_iBoardSize, int a_iSystems, int a_iEmperors )
	: HLogic( "glx", a_oName ), f_iBoardSize( a_iBoardSize ), f_iSystems( a_iSystems ),
	f_iEmperors( a_iEmperors ), f_iRound( -1 ), f_iReady( 0 ),
	f_oSystems( a_iSystems + a_iEmperors ), f_oEmperors()
	{
	M_PROLOG
	int l_iCtr = 0, l_iCtrLoc = 0;
	HRandomizer l_oRandom;
	HSystem * l_poSystem = NULL;
	l_oRandom.set ( time ( NULL ) );
	for ( l_iCtr = 0; l_iCtr < ( a_iEmperors + a_iSystems ); l_iCtr ++ )
		{
		l_poSystem = &f_oSystems[ l_iCtr ];
		l_poSystem->f_iId = l_iCtr;
		l_poSystem->f_iCoordinateX = l_oRandom.rnd( f_iBoardSize );
		l_poSystem->f_iCoordinateY = l_oRandom.rnd( f_iBoardSize );
		if ( l_iCtr )
			{
			for ( l_iCtrLoc = 0; l_iCtrLoc < l_iCtr; l_iCtrLoc ++ )
				if ( ( l_poSystem->f_iCoordinateX
							== f_oSystems [ l_iCtrLoc ].f_iCoordinateX )
						&& ( l_poSystem->f_iCoordinateY
							== f_oSystems [ l_iCtrLoc ].f_iCoordinateY ) )
					break;
			if ( l_iCtrLoc < l_iCtr )
				{
				l_iCtr --;
				continue;
				}
			}
		}
	for ( l_iCtr = 0; l_iCtr < ( a_iEmperors + a_iSystems ); l_iCtr ++ )
		f_oSystems [ l_iCtr ].f_iProduction = f_oSystems [ l_iCtr ].f_iFleet = l_oRandom.rnd ( 16 );
	f_oHandlers[ "play" ] = static_cast<handler_t>( &HGalaxy::handler_play );
	f_oHandlers[ "say" ] = static_cast<handler_t>( &HGalaxy::handler_message );
	return;
	M_EPILOG
	}

HGalaxy::~HGalaxy ( void )
	{
	M_PROLOG
	return;
	M_EPILOG
	}

bool HGalaxy::do_accept( OClientInfo* )
	{
	return ( false );
	}

void HGalaxy::do_post_accept( OClientInfo* a_poClientInfo )
	{
	M_PROLOG
	int l_iColor = -1, l_iSysNo = -1;
	HString l_oMessage;
	/*
	 * Send basic setup info.
	 */
	l_oMessage.format ( "glx:setup:board_size=%d\n", f_iBoardSize );
	a_poClientInfo->f_oSocket->write_until_eos( l_oMessage ); /* send setup info to new emperor */
	l_oMessage.format ( "glx:setup:systems=%d\n", f_iEmperors + f_iSystems );
	a_poClientInfo->f_oSocket->write_until_eos( l_oMessage );
	for ( int l_iCtr = 0; l_iCtr < ( f_iEmperors + f_iSystems ); ++ l_iCtr )
		{
		l_oMessage.format( "glx:setup:system_coordinates=%d,%d,%d\n",
				l_iCtr, f_oSystems[ l_iCtr ].f_iCoordinateX,
				f_oSystems[ l_iCtr ].f_iCoordinateY );
		a_poClientInfo->f_oSocket->write_until_eos ( l_oMessage );
		if ( ( f_iRound >= 0 ) && ( f_oSystems[ l_iCtr ].f_poEmperor ) )
			{
			l_oMessage.format ( "glx:play:system_info=%c,%d,%d,%d,%d\n",
					'i', l_iCtr, f_oSystems[ l_iCtr ].f_iProduction, get_color( f_oSystems[ l_iCtr ].f_poEmperor ),
					f_oSystems[ l_iCtr ].f_iFleet );
			a_poClientInfo->f_oSocket->write_until_eos ( l_oMessage );
			}
		}
	if ( ( f_iRound < 0 ) && ( f_iReady < f_iEmperors ) )
		{
		l_iSysNo = assign_system( a_poClientInfo ); /* assign mother system for new emperor */
		l_iColor = get_color( a_poClientInfo );
		l_oMessage.format ( "glx:setup:emperor=%d,%s\n",
				l_iColor, a_poClientInfo->f_oName.raw() );
		broadcast( l_oMessage ); /* send setup information about new rival to all emperors */
		f_oSystems[ l_iSysNo ].f_iProduction = 10;
		f_oSystems[ l_iSysNo ].f_iFleet = 10;
		l_oMessage.format ( "glx:play:system_info=%c,%d,%d,%d,%d\n",
				'c', l_iSysNo, f_oSystems [ l_iSysNo ].f_iProduction, l_iColor,
				f_oSystems [ l_iSysNo ].f_iFleet );
		a_poClientInfo->f_oSocket->write_until_eos ( l_oMessage );
		l_oMessage.format ( "glx:msg:$12;Emperor ;$%d;", l_iColor );
		l_oMessage += a_poClientInfo->f_oName;
		l_oMessage += ";$12; invaded the galaxy.\n";
		f_iReady ++;
		}
	else
		{
		f_oEmperors[ a_poClientInfo ] = OEmperorInfo();
		l_oMessage.format ( "glx:setup:emperor=%d,%s\n",
				-1, a_poClientInfo->f_oName.raw() );
		a_poClientInfo->f_oSocket->write_until_eos( l_oMessage );
		l_oMessage = "glx:msg:$12;Spectator " + a_poClientInfo->f_oName + " is visiting this galaxy.\n";
		}
	broadcast( l_oMessage ); /* inform every emperor about new rival */
	for ( emperors_t::iterator it = f_oEmperors.begin(); it != f_oEmperors.end(); ++ it )
		{
		if ( it->first != a_poClientInfo )
			{
			int l_iClr = it->second.f_iColor;
			if ( l_iClr >= 0 )
				{
				l_oMessage.format ( "glx:setup:emperor=%d,%s\n",
						l_iClr, it->first->f_oName.raw() );
				a_poClientInfo->f_oSocket->write_until_eos ( l_oMessage );
				l_oMessage.format ( "glx:msg:$12;Emperor ;$%d;", l_iClr );
				l_oMessage += it->first->f_oName;
				l_oMessage += ";$12; invaded the galaxy.\n";
				}
			else
				l_oMessage = "glx:msg:$12;Spectator " + it->first->f_oName + " is visiting this galaxy.\n";
			a_poClientInfo->f_oSocket->write_until_eos ( l_oMessage );
			}
		}
	if ( ( f_iRound < 0 ) && ( f_iReady >= f_iEmperors ) )
		{
		f_iRound = 0;
		l_oMessage = "glx:setup:ok\n";
		broadcast ( l_oMessage );
		f_iReady = 0;
		}
	return;
	M_EPILOG
	}

int HGalaxy::assign_system( OClientInfo* a_poClientInfo )
	{
	M_PROLOG
	OEmperorInfo info;
	typedef HSet<int> integer_set_t;
	integer_set_t l_oUsed;
	for ( emperors_t::iterator it = f_oEmperors.begin(); it != f_oEmperors.end(); ++ it )
		l_oUsed.insert( it->second.f_iColor );
	int l_iCtr = 0;
	for ( integer_set_t::HIterator it = l_oUsed.begin(); it != l_oUsed.end(); ++it, ++ l_iCtr )
		if ( *it != l_iCtr )
			break;
	info.f_iColor = l_iCtr;
	info.f_iSystems = 1;
	int l_iRivals = static_cast<int>( f_oEmperors.size() );
	HRandomizer l_oRnd;
	randomizer_helper::init_randomizer_from_time( l_oRnd );
	int l_iMotherSystem = l_oRnd.rnd( f_iEmperors + f_iSystems - l_iRivals );
	f_oEmperors[ a_poClientInfo ] = info;
	l_iCtr = 0;
	for ( int i = 0; i < l_iMotherSystem; ++ i, ++ l_iCtr )
		while ( f_oSystems[ l_iCtr ].f_poEmperor != NULL )
			++ l_iCtr;
	while ( f_oSystems[ l_iCtr ].f_poEmperor != NULL )
		++ l_iCtr;
	M_ASSERT( ! f_oSystems[ l_iCtr ].f_poEmperor );
	f_oSystems[ l_iCtr ].f_poEmperor = a_poClientInfo;
	return ( l_iCtr );
	M_EPILOG
	}

void HGalaxy::handler_message ( OClientInfo* a_poClientInfo, HString const& a_roMessage )
	{
	M_PROLOG
	HString l_oMessage;
	l_oMessage = "glx:msg:$";
	int color = get_emperor_info( a_poClientInfo )->f_iColor;
	if ( color < 0 )
		color = 12;
	l_oMessage += color;
	l_oMessage += ';';
	l_oMessage += a_poClientInfo->f_oName;
	l_oMessage += ";$12;: ";
	l_oMessage += a_roMessage;
	l_oMessage += '\n';
	broadcast( l_oMessage );
	return;
	M_EPILOG
	}

void HGalaxy::handler_play ( OClientInfo* a_poClientInfo, HString const& a_roCommand )
	{
	M_PROLOG
	if ( get_color( a_poClientInfo ) == -1 )
		{
		*a_poClientInfo->f_oSocket << "err:Illegal message!\n" << endl;
		return;
		}
	int l_iSource = - 1, l_iDestination = - 1, l_iDX = 0, l_iDY = 0;
	HString l_oVariable;
	HString l_oValue;
	HFleet l_oFleet;
	l_oVariable = a_roCommand.split ( "=", 0 );
	l_oValue = a_roCommand.split ( "=", 1 );
	if ( l_oVariable == "move" )
		{
		l_oFleet.f_poEmperor = a_poClientInfo;
		l_iSource = lexical_cast<int>( l_oValue.split ( ",", 0 ) );
		l_iDestination = lexical_cast<int>( l_oValue.split ( ",", 1 ) );
		l_oFleet.f_iSize = lexical_cast<int>( l_oValue.split ( ",", 2 ) );
		if ( ( l_iSource == l_iDestination )
				&& ( f_oSystems [ l_iSource ].f_poEmperor != a_poClientInfo )
				&& ( f_oSystems [ l_iSource ].f_iFleet < l_oFleet.f_iSize ) )
			kick_client( a_poClientInfo );
		else
			{
			f_oSystems [ l_iSource ].f_iFleet -= l_oFleet.f_iSize;
			l_iDX = f_oSystems [ l_iSource ].f_iCoordinateX - f_oSystems [ l_iDestination ].f_iCoordinateX;
			l_iDY = f_oSystems [ l_iSource ].f_iCoordinateY - f_oSystems [ l_iDestination ].f_iCoordinateY;
			l_iDX = ( l_iDX >= 0 ) ? l_iDX : - l_iDX;
			l_iDY = ( l_iDY >= 0 ) ? l_iDY : - l_iDY;
			l_iDX = ( ( f_iBoardSize - l_iDX ) < l_iDX ) ? f_iBoardSize - l_iDX : l_iDX;
			l_iDY = ( ( f_iBoardSize - l_iDY ) < l_iDY ) ? f_iBoardSize - l_iDY : l_iDY;
			l_oFleet.f_iArrivalTime = static_cast<int>( ::sqrt( l_iDX * l_iDX + l_iDY * l_iDY ) + 0.5 );
			f_oSystems[ l_iDestination ].f_oAttackers.push_front( l_oFleet );
			}
		}
	else if ( l_oVariable == "end_round" )
		{
		OEmperorInfo* l_poInfo = get_emperor_info( a_poClientInfo );
		M_ASSERT( l_poInfo );
		if ( l_poInfo->f_iSystems >= 0 )
			f_iReady ++;
		out << "emperors: " << f_oEmperors.size() << endl;
		out << "ready: " << f_iReady << endl;
		if ( f_iReady >= f_oEmperors.size() )
			end_round();
		}
	return;
	M_EPILOG
	}
	
void HGalaxy::end_round( void )
	{
	M_PROLOG
	int l_iCtr = 0, l_iDead = 0;
	f_iReady = 0;
	for ( emperors_t::iterator it = f_oEmperors.begin(); it != f_oEmperors.end(); ++ it )
		if ( it->second.f_iSystems > 0 )
			it->second.f_iSystems = 0;
	for ( l_iCtr = 0; l_iCtr < ( f_iSystems + f_iEmperors ); l_iCtr ++ )
		f_oSystems [ l_iCtr ].do_round( *this );
	HString l_oMessage;
	for ( emperors_t::iterator it = f_oEmperors.begin(); it != f_oEmperors.end(); ++ it )
		{
		if ( ! it->second.f_iSystems )
			{
			it->second.f_iSystems = -1;
			l_oMessage.format( "glx:msg:$12;Once mighty empire of ;$%d;%s;$12; fall in ruins.\n",
					it->second.f_iColor, it->first->f_oName.raw() );
			broadcast( l_oMessage );
			}
		if ( it->second.f_iColor >= 0 ) /* not spectator */
			{
			if ( it->second.f_iSystems >= 0 )
				f_iReady ++;
			else
				l_iDead ++;
			}
		}
	if ( f_iReady == 1 )
		{
		for ( emperors_t::iterator it = f_oEmperors.begin(); it != f_oEmperors.end(); ++ it )
			{
			if ( it->second.f_iSystems > 0 )
				{
				l_oMessage.format( "glx:msg:$12;The invincible ;$%d;%s;$12; crushed the galaxy.\n",
						it->second.f_iColor, it->first->f_oName.raw() );
				broadcast( l_oMessage );
				}
			}
		}
	for ( l_iCtr = 0; l_iCtr < ( f_iEmperors + f_iSystems ); l_iCtr ++ )
		{
		l_oMessage.format ( "glx:play:system_info=%c,%d,%d,%d,%d\n",
				'i', l_iCtr, f_oSystems[ l_iCtr ].f_iProduction, get_color( f_oSystems[ l_iCtr ].f_poEmperor ),
				f_oSystems[ l_iCtr ].f_iFleet );
		for ( emperors_t::iterator it = f_oEmperors.begin(); it != f_oEmperors.end(); ++ it )
			{
			if ( it->second.f_iColor < 0 )
				it->first->f_oSocket->write_until_eos ( l_oMessage );
			}
		}
	f_iRound ++;
	l_oMessage.format ( "glx:play:round=%d\n", f_iRound );
	broadcast( l_oMessage );
	f_iReady = l_iDead;
	return;
	M_EPILOG
	}

int HGalaxy::get_color( OClientInfo* a_poClientInfo )
	{
	M_PROLOG
	OEmperorInfo* info = get_emperor_info( a_poClientInfo );
	return ( info ? info->f_iColor : -1 );
	M_EPILOG
	}

void HGalaxy::mark_alive( OClientInfo* a_poClientInfo )
	{
	M_PROLOG
	get_emperor_info( a_poClientInfo )->f_iSystems ++;
	return;
	M_EPILOG
	}

HGalaxy::OEmperorInfo* HGalaxy::get_emperor_info( OClientInfo* a_poClientInfo )
	{
	M_PROLOG
	OEmperorInfo* info = NULL;
	if ( a_poClientInfo )
		{
		emperors_t::iterator it = f_oEmperors.find( a_poClientInfo );
		M_ASSERT( it != f_oEmperors.end() );
		info = &it->second;
		}
	return ( info );
	M_EPILOG
	}

void HGalaxy::do_kick( OClientInfo* a_poClientInfo )
	{
	M_PROLOG
	for ( int l_iCtr = 0; l_iCtr < ( f_iSystems + f_iEmperors ); ++ l_iCtr )
		{
		HSystem::attackers_t& l_oAttackers = f_oSystems[ l_iCtr ].f_oAttackers;
		for ( HSystem::attackers_t::iterator it = l_oAttackers.begin();
				it != l_oAttackers.end(); ++ it )
			{
			if ( it->f_poEmperor == a_poClientInfo )
				{
				HSystem::attackers_t::iterator kicked = it;
				++ it;
				l_oAttackers.erase( kicked );
				}
			}
		if ( f_oSystems[ l_iCtr ].f_poEmperor == a_poClientInfo )
			f_oSystems[ l_iCtr ].f_poEmperor = NULL;
		}
	int color = get_color( a_poClientInfo );
	f_oEmperors.remove( a_poClientInfo );
	if ( ( color >= 0 ) && ( f_iRound < 0 ) )
		-- f_iReady;
	if ( color >= 0 )
		broadcast( HString( "glx:msg:$12;Emperor ;$" ) + color + ";" + a_poClientInfo->f_oName + ";$12; fleed from the galaxy.\n" );
	else
		broadcast( HString( "glx:msg:$12;Spectator " ) + a_poClientInfo->f_oName + " left this universum.\n" );
	out << "galaxy: dumping player: " << a_poClientInfo->f_oName << endl;
	if ( f_iReady >= f_oEmperors.size() )
		end_round();
	out << "ready: " << f_iReady << ", emperors: " << f_oEmperors.size() << endl;
	return;
	M_EPILOG
	}

yaal::hcore::HString HGalaxy::get_info() const
	{
	return ( HString( "glx," ) + f_oName + "," + f_oEmperors.size() + "," + f_iEmperors + "," + f_iBoardSize + "," + f_iSystems );
	}

}

namespace logic_factory
{

class HGalaxyCreator : public HLogicCreatorInterface
	{
protected:
	virtual HLogic::ptr_t do_new_instance( HString const& );
public:
	} galaxyCreator;

HLogic::ptr_t HGalaxyCreator::do_new_instance( HString const& a_oArgv )
	{
	M_PROLOG
	HString l_oName = a_oArgv.split( ",", 0 );
	int l_iEmperors = lexical_cast<int>( a_oArgv.split( ",", 1 ) );
	int l_iBoardSize = lexical_cast<int>( a_oArgv.split( ",", 2 ) ); 
	int l_iSystems = lexical_cast<int>( a_oArgv.split( ",", 3 ) );
	char* l_pcMessage = NULL;
	out << "new glx: ( " << l_oName << " ) {" << endl;
	cout << "emperors = " << l_iEmperors << endl;
	cout << "board_size = " << l_iBoardSize << endl;
	cout << "systems = " << l_iSystems << endl;
	cout << "};" << endl;
	if ( OSetup::test_glx_emperors( l_iEmperors, l_pcMessage )
			|| OSetup::test_glx_emperors_systems( l_iEmperors, l_iSystems, l_pcMessage )
			|| OSetup::test_glx_systems( l_iSystems, l_pcMessage )
			|| OSetup::test_glx_board_size( l_iBoardSize, l_pcMessage ) )
		throw HLogicException( l_pcMessage );
	return ( HLogic::ptr_t( new galaxy::HGalaxy( l_oName,
					l_iBoardSize,
					l_iSystems,
					l_iEmperors ) ) );
	M_EPILOG
	}

namespace
{

bool registrar( void )
	{
	M_PROLOG
	bool volatile failed = false;
	HLogicFactory& factory = HLogicFactoryInstance::get_instance();
	HString l_oSetup;
	l_oSetup.format( "%s:%d,%d,%d", "glx", setup.f_iEmperors, setup.f_iBoardSize, setup.f_iSystems );
	factory.register_logic_creator( l_oSetup, &galaxyCreator );
	return ( failed );
	M_EPILOG
	}

bool volatile registered = registrar();

}

}

}

