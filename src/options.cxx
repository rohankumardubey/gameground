/*
---       `gameground' 0.0.0 (c) 1978 by Marcin 'Amok' Konarski         ---

	rc_options.cxx - this file is integral part of `gameground' project.

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

#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <yaal/yaal.hxx>
M_VCSID( "$Id: "__ID__" $" )

#include "options.hxx"
#include "version.hxx"
#include "setup.hxx"

using namespace yaal;
using namespace yaal::hcore;
using namespace yaal::tools;
using namespace yaal::tools::util;

namespace gameground
{

bool set_variables( HString& a_roOption, HString& a_roValue )
	{
	::fprintf( stdout, "option: [%s], value: [%s]\n",
			a_roOption.raw(), a_roValue.raw() );
	return ( false );
	}

/* Set all the option flags according to the switches specified.
   Return the index of the first non-option argument.                    */

void usage( void* ) __attribute__((__noreturn__));
void usage( void* arg )
	{
	OOptionInfo* info = static_cast<OOptionInfo*>( arg );
	info->PROC( info->_opt, setup.f_pcProgramName,
			"GameGround - universal networked multiplayer game server", NULL );
	throw ( setup.f_bHelp ? 0 : 1 );
	}

void version( void* ) __attribute__ ( ( __noreturn__ ) );
void version( void* )
	{
	printf ( "`gameground' %s\n", VER );
	throw ( 0 );
	}

void set_verbosity( void* )
	{
	++ setup.f_iVerbose;
	return;
	}

namespace
{

HProgramOptionsHandler::simple_callback_t v( set_verbosity, NULL );
HProgramOptionsHandler::simple_callback_t help( usage, NULL );
HProgramOptionsHandler::simple_callback_t dump( usage, NULL );
HProgramOptionsHandler::simple_callback_t version_call( version, NULL );

}

int handle_program_options( int a_iArgc, char** a_ppcArgv )
	{
	M_PROLOG
	HProgramOptionsHandler po;
	po( "log_path", program_options_helper::option_value( setup.f_oLogPath ), NULL, HProgramOptionsHandler::OOption::TYPE::D_REQUIRED, "path", "path pointing to file for application logs", NULL )
		( "aspell_lang", program_options_helper::option_value( setup.f_oAspellLang ), NULL, HProgramOptionsHandler::OOption::TYPE::D_REQUIRED, "language", "language used for spell checking", NULL )
		( "board", program_options_helper::option_value( setup.f_iBoardSize ), "B", HProgramOptionsHandler::OOption::TYPE::D_REQUIRED, "size", "size of the galaxy board", NULL )
		( "client", program_options_helper::option_value( setup.f_bClient ), "C", HProgramOptionsHandler::OOption::TYPE::D_NONE, NULL, "connect to server", NULL )
		( "console_charset", program_options_helper::option_value( setup.f_oConsoleCharset ), NULL, HProgramOptionsHandler::OOption::TYPE::D_REQUIRED, "charset", "charset encoding for current terminal", NULL )
		( "emperors", program_options_helper::option_value( setup.f_iEmperors ), "E", HProgramOptionsHandler::OOption::TYPE::D_REQUIRED, "count", "set number of players", NULL )
		( "host", program_options_helper::option_value( setup.f_oHost ), "H", HProgramOptionsHandler::OOption::TYPE::D_REQUIRED, "addr", "select host to connect", NULL )
		( "join", program_options_helper::option_value( setup.f_oGame ), "J", HProgramOptionsHandler::OOption::TYPE::D_REQUIRED, "name", "join to game named {name}", NULL )
		( "login", program_options_helper::option_value( setup.f_oLogin ), "L", HProgramOptionsHandler::OOption::TYPE::D_REQUIRED, "nane", "set your player name", NULL )
		( "new", program_options_helper::option_value( setup.f_oGameType ), "N", HProgramOptionsHandler::OOption::TYPE::D_REQUIRED, "type,name", "create game of type {type} and named {name}", NULL )
		( "port", program_options_helper::option_value( setup.f_iPort ), "P", HProgramOptionsHandler::OOption::TYPE::D_REQUIRED, "num", "set port number", NULL )
		( "server", program_options_helper::option_value( setup.f_bServer ), "G", HProgramOptionsHandler::OOption::TYPE::D_NONE, NULL, "setup a game server", NULL )
		( "systems", program_options_helper::option_value( setup.f_iSystems ), "S", HProgramOptionsHandler::OOption::TYPE::D_REQUIRED, "count", "set number of neutral systems", NULL )
		( "quiet", program_options_helper::option_value( setup.f_bQuiet ), "q", HProgramOptionsHandler::OOption::TYPE::D_NONE, NULL, "inhibit usual output", NULL )
		( "silent", program_options_helper::option_value( setup.f_bQuiet ), "q", HProgramOptionsHandler::OOption::TYPE::D_NONE, NULL, "inhibit usual output", NULL )
		( "verbose", program_options_helper::option_value( setup.f_iVerbose ), "v", HProgramOptionsHandler::OOption::TYPE::D_NONE, NULL, "print more information", &v )
		( "help", program_options_helper::option_value( setup.f_bHelp ), "h", HProgramOptionsHandler::OOption::TYPE::D_NONE, NULL, "display this help and exit", &help )
		( "dump-configuration", program_options_helper::no_value, "W", HProgramOptionsHandler::OOption::TYPE::D_NONE, NULL, "dump current configuration", &dump )
		( "version", program_options_helper::no_value, "V", HProgramOptionsHandler::OOption::TYPE::D_NONE, NULL, "output version information and exit", &version_call );
	po.process_rc_file( "gameground", "", NULL );
	if ( setup.f_oLogPath.is_empty() )
		setup.f_oLogPath = "gameground.log";
	int l_iUnknown = 0, l_iNonOption = 0;
	OOptionInfo info( po.get_options(), util::show_help );
	OOptionInfo infoConf( po.get_options(), util::dump_configuration );
	help.second = &info;
	dump.second = &infoConf;
	l_iNonOption = po.process_command_line( a_iArgc, a_ppcArgv, &l_iUnknown );
	if ( l_iUnknown > 0 )
		usage( &info );
	return ( l_iNonOption );
	M_EPILOG
	}

}

