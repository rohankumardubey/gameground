"use strict"

class Party {
	constructor( app_, id_, name_, configuration_ ) {
		this._id = id_
		this._name = name_
		this._configuration = configuration_
		this._players = []
		this._handlers = {}
		this._app = app_
		this._active = false
		this._handlers["say"] = ( msg ) => this.on_say( msg )
		this._handlers["msg"] = ( msg ) => this.on_msg( msg )
	}
	get name() {
		return ( this._name + ":" + this._id )
	}
	close() {
		this._app.sock.send( "abandon:" + this._id )
		this._app = null
	}
	add_player( player_ ) {
		const idx = this._players.indexOf( player_ );
		if ( idx >= 0 ) {
			return
		}
		this._players.push( player_ )
		this._players.sort()
		if ( player_ == this._app.myLogin ) {
			if ( ! this._active ) {
				this._app.make_visible( this._id )
			}
			this._active = true
		}
	}
	drop_player( player_ ) {
		const idx = this._players.indexOf( player_ );
		if ( idx >= 0 ) {
			this._players.plop( idx )
		}
		if ( ( this._app == null ) || ( player_ == this._app.myLogin ) ) {
			this._active = false
		}
	}
	invoke( message_ ) {
		const message = message_.chop( ":", 2 )
		const handler = this._handlers[message[0]]
		if ( handler !== undefined ) {
			handler( message[1] )
		}
	}
	on_say( message_ ) {
		this._refs.messages.log_message( message_ )
	}
	on_msg( message_ ) {
		this._refs.messages.append_text( message_ )
	}
}
