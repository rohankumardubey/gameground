/*
---            `gameground' 0.0.0 (c) 1978 by Marcin 'Amok' Konarski             ---

	main.cxx - this file is integral part of `gameground' project.

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

#include <cstdlib>

#include <yaal/hcore/hfile.hxx>
#include <yaal/hcore/hlog.hxx>
#include <yaal/hconsole/console.hxx>

M_VCSID( "$Id: "__ID__" $" )
#include "setup.hxx"
#include "options.hxx"
#include "galaxy.hxx"

using namespace yaal;
using namespace yaal::hcore;
using namespace yaal::hconsole;
using namespace yaal::tools;
using namespace gameground;

namespace gameground {

OSetup setup;

}

int main( int argc_, char* argv_[] ) {
	M_AT_END_OF_SCOPE( HSignalService::get_instance().stop(); );
	M_PROLOG
/* variables declarations for main loop: */
	int opt = 0;
	HConsole& cons = HConsole::get_instance();
/* end. */
	try {
/* TO-DO: enter main loop code here */
		HSignalService::get_instance();
		setup._programName = argv_[ 0 ];
		opt = handle_program_options( argc_, argv_ );
		setup._logPath.replace( ".log", "_" + setup._login + ".log" );
		hcore::log.rehash( setup._logPath, setup._programName );
		setup.test_setup();
/* *BOOM* */
		if ( ! cons.is_enabled() )
			cons.enter_curses (); /* enabling ncurses ablilities */
		if ( ( cons.get_height() >= 25 ) && ( cons.get_width() >= 80 ) )
			opt = main_client();
		else
			opt = 1;
		if ( cons.is_enabled() )
			cons.leave_curses (); /* ending ncurses sesion */
		if ( opt )
			cout << "Your terminal is too small." << endl;
/* ... there is the place main loop ends. :OD-OT */
	} catch ( ... ) {
		if ( cons.is_enabled ( ) )
			cons.leave_curses (); /* ending ncurses sesion */
		throw;
	}
	cerr << _( "Done" ) << endl;
	return ( opt );
	M_FINAL
}

