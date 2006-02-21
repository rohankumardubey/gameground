import java.io.InputStreamReader;
import java.io.InputStream;
import java.net.URL;
import java.awt.Image;
import java.awt.Graphics;
import java.awt.event.MouseEvent;
import java.awt.event.ActionListener;
import java.awt.Color;
import javax.swing.JPanel;
import javax.swing.ImageIcon;
import javax.swing.event.MouseInputListener;

class HImages {
//--------------------------------------------//
	static int PLANETS_ICON_COUNT = 36;
	public Image _background;
	public Image[] _systems;
//--------------------------------------------//
	public HImages() {
		_systems = new Image[PLANETS_ICON_COUNT];
		_background = loadImage( "res/god.png" );
		for ( int i = 0; i < PLANETS_ICON_COUNT; ++ i ) {
			String path = "res/system";
			path += i + ".xpm";
			_systems[ i ] = loadImage( path );
		}
	}
	private Image loadImage( String $path ) {
		try {
			InputStream in = getClass().getResourceAsStream( $path );
			int size = in.available();
			byte buffer[] = new byte[size];
			for ( int i = 0; i < size; ++ i )
				buffer[ i ] = (byte)in.read();
			if ( $path.indexOf( "xpm" ) >= 0 ) {
				return Xpm.XpmToImage( new String( buffer ) );
			} else {
				return new ImageIcon( buffer ).getImage();
			}
		} catch ( Exception e ) {
			e.printStackTrace();
		}
		return null;
	}
}

public class HBoard extends JPanel implements MouseInputListener {
//--------------------------------------------//
	public static final long serialVersionUID = 7l;
	boolean _help;
	int _size;
	int _diameter;
	int _cursorX;
	int _cursorY;
	int _sourceSystem;
	HImages _images;
	HGUIMain _gui;
	HSystem[] _systems;
//--------------------------------------------//
	public HBoard() {
		_help = false;
		_size = -1;
		_cursorX = -1;
		_cursorY = -1;
		_diameter = -1;
		_sourceSystem = -1;
	}
	public void mouseMoved( MouseEvent $event ) {
		int cursorX = $event.getX() / _diameter;
		int cursorY = $event.getY() / _diameter;
		if ( ( _cursorX != cursorX ) || ( _cursorY != cursorY ) ) {
			_cursorX = cursorX;
			_cursorY = cursorY;
			_gui._widgets._systemInfo.setText( "" );
			_gui._widgets._emperorInfo.setText( "" );
			_gui._widgets._productionInfo.setText( "" );
			_gui._widgets._fleetInfo.setText( "" );
			repaint();
		}
	}
	public void mouseDragged( MouseEvent $event ) {
	}
	public void mouseEntered( MouseEvent $event ) {
	}
	public void mouseReleased( MouseEvent $event ) {
		if ( $event.getButton() == MouseEvent.BUTTON3 ) {
			_help = false;
			repaint();
		}
	}
	public void mousePressed( MouseEvent $event ) {
		if ( $event.getButton() == MouseEvent.BUTTON3 ) {
			_help = true;
			repaint();
		}
	}
	public void mouseClicked( MouseEvent $event ) {
		if ( _systems != null ) {
			int sysNo = getSysNo( $event.getX() / _diameter, $event.getY() / _diameter );
			if ( sysNo >= 0 ) {
				if ( _gui.getState() != HGUIMain.State.LOCKED ) {
					if ( _gui.getState() == HGUIMain.State.NORMAL ) {
						if ( _systems[ sysNo ]._color == _gui._client._color ) {
							_gui.setState( HGUIMain.State.SELECT );
							_sourceSystem = sysNo;
						}
					} else if ( _gui.getState() == HGUIMain.State.SELECT ) {
						_gui._widgets._fleet.setEditable( true );
						_gui._widgets._fleet.setText( String.valueOf( _systems[ _sourceSystem ]._fleet ) );
						_gui._widgets._fleet.selectAll();
						_gui.setState( HGUIMain.State.INPUT );
					}
				}
			}
		}
	}
	public void mouseExited( MouseEvent $event ) {
		_cursorX = -1;
		_cursorY = -1;
		repaint();
	}
	int getSysNo( int $coordX, int $coordY ) {
		int systemCount = _gui._client.getSystemCount();
		for ( int i = 0; i < systemCount; ++ i ) {
			if ( ( _systems[ i ]._coordinateX == $coordX ) && ( _systems[ i ]._coordinateY == $coordY ) ) {
				return i;
			}
		}
		return -1;
	}
	void setGui( HGUIMain $gui ) {
		_gui = $gui;
	}
	public void setImages( HImages $images ) {
		_images = $images;
	}
	void setSize( int $size ) {
		_size = $size;
		_diameter = 640 / $size;
	}
	void setSystems( HSystem[] $systems ) {
		_systems = $systems;
		addMouseMotionListener( this );
		addMouseListener( this );
		repaint();
	}
	private void drawSystem( Graphics $gs, int $no, int $coordX, int $coordY, int $color ) {
		$gs.drawImage( _images._systems[$no],
				$coordX * _diameter + ( _diameter - 32 ) / 2,
				$coordY * _diameter + ( _diameter - 32 ) / 2, this );
		if ( ( $color >= 0 ) || ( ( $coordX == _cursorX ) && ( $coordY == _cursorY ) ) ) {
			if ( ( $coordX == _cursorX ) && ( $coordY == _cursorY ) ) {
				$gs.setColor ( _gui._widgets._colors[ HGUIMain.Colors.WHITE ] );
				_gui._widgets._systemInfo.setText( _gui._client._systemNames[ $no ] );
				if ( $color >= 0 ) {
					_gui._widgets._emperorInfo.setText( _gui._client._emperors.get( $color ) );
				} else {
					_gui._widgets._emperorInfo.setText( "" );
				}
				if ( _systems[ $no ]._production >= 0 )
					_gui._widgets._productionInfo.setText( String.valueOf( _systems[ $no ]._production ) );
				else
					_gui._widgets._productionInfo.setText( "?" );
				if ( _systems[ $no ]._fleet >= 0 )
					_gui._widgets._fleetInfo.setText( String.valueOf( _systems[ $no ]._fleet ) );
				else
					_gui._widgets._fleetInfo.setText( "?" );
			}	else {
				$gs.setColor ( _gui._widgets._colors[ $color ] );
			}
			$gs.drawRect ( $coordX * _diameter + 1,
					$coordY * _diameter + 1,
					_diameter - 2, _diameter - 2 );
			$gs.drawRect ( $coordX * _diameter + 2,
					$coordY * _diameter + 2,
					_diameter - 4, _diameter - 4 );
		}
		if ( _help ) {
			$gs.setColor ( _gui._widgets._colors[ HGUIMain.Colors.WHITE ] );
			$gs.drawString( _gui._client._systemNames[ $no ], $coordX * _diameter + 2, ( $coordY + 1 ) * _diameter - 2 );
		}
	}
	protected void paintComponent( Graphics g ) {
		g.drawImage( _images._background, 0, 0, this );
		if ( _size > 0 ) {
			if ( _systems != null ) {
				int systemCount = _gui._client.getSystemCount();
				for ( int i = 0; i < systemCount; ++ i )
					drawSystem( g, i, _systems[ i ]._coordinateX, _systems[ i ]._coordinateY, _systems[ i ]._color );
			}
			/*
			for ( int i = 0; i < 10; ++ i )
				drawSystem( g, i, i + 5, 8, i );
			for ( int i = 0; i < 10; ++ i )
				drawSystem( g, i + 10, i + 5, 9, i );
			for ( int i = 0; i < 10; ++ i )
				drawSystem( g, i + 20, i + 5, 10, i );
			for ( int i = 0; i < 6; ++ i )
				drawSystem( g, i + 30, i + 5, 11, i );
			*/
			g.setColor( Color.darkGray );
			for ( int i = 0; i < _size; ++ i )
				g.drawLine( 0, i * _diameter, 640, i * _diameter );
			for ( int i = 0; i < _size; ++ i )
				g.drawLine( i * _diameter, 0, i * _diameter, 640 );
		}
	}
}
