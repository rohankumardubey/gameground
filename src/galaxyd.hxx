/*
---           `gameground' 0.0.0 (c) 1978 by Marcin 'Amok' Konarski            ---

	gamegroundd.hxx - this file is integral part of `galaxy' project.

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

#ifndef GALAXYD_HXX_INCLUDED
#define GALAXYD_HXX_INCLUDED

#include <yaal/yaal.hxx>

#include "logic.hxx"

namespace gameground
{

namespace galaxy
{

class HSystem;
class HGalaxy;
class HFleet
	{
protected:
	/*{*/
	int f_iSize;
	OClientInfo* f_poEmperor;
	int f_iArrivalTime;
	/*}*/
public:
	/*{*/
	/*}*/
protected:
	/*{*/
	/*}*/
	friend class HSystem;
	friend class HGalaxy;
	};

class HGalaxy;
class HSystem
	{
protected:
	/*{*/
	typedef yaal::hcore::HList<HFleet> attackers_t;
	int f_iId;
	int f_iCoordinateX;
	int f_iCoordinateY;
	int f_iProduction;
	int f_iFleet;
	OClientInfo* f_poEmperor;
	attackers_t f_oAttackers;
	/*}*/
public:
	/*{*/
	HSystem ( void );
	HSystem( HSystem const& ) __attribute__(( __noreturn__ ));
	HSystem& operator = ( HSystem const& );
	void do_round( HGalaxy & );
	void swap( HSystem& );
	/*}*/
protected:
	friend class HGalaxy;
	};

inline void swap( HSystem& a, HSystem& b )
	{ a.swap( b ); }

class HGalaxy : public HLogic
	{
	struct OEmperorInfo
		{
		int f_iColor;
		int f_iSystems;
		OEmperorInfo( void ) : f_iColor( -1 ), f_iSystems( -1 ) {}
		};
	typedef yaal::hcore::HMap<OClientInfo*, OEmperorInfo> emperors_t;
protected:
	/*{*/
	int f_iBoardSize;
	int f_iSystems;
	int f_iEmperors;
	int f_iRound;
	int f_iReady;
	yaal::hcore::HArray<HSystem> f_oSystems;
	emperors_t f_oEmperors;
	/*}*/
public:
	/*{*/
	HGalaxy( yaal::hcore::HString const&, int, int, int );
	virtual ~HGalaxy( void );
	int get_color( OClientInfo* );
	void mark_alive( OClientInfo* );
	virtual yaal::hcore::HString get_info() const;
	/*}*/
protected:
	/*{*/
	OEmperorInfo* get_emperor_info( OClientInfo* );
	int assign_system( OClientInfo* );
	virtual bool do_accept( OClientInfo* );
	virtual void do_post_accept( OClientInfo* );
	virtual void do_kick( OClientInfo* );
	void handler_message( OClientInfo*, yaal::hcore::HString const& );
	void handler_play( OClientInfo*, yaal::hcore::HString const& );
	void end_round( void );
	/*}*/
private:
	/*{*/
	HGalaxy( HGalaxy const& );
	HGalaxy& operator = ( HGalaxy const& );
	/*}*/
	friend class HServer;
	};

}

}

#endif /* not GALAXYD_HXX_INCLUDED */
