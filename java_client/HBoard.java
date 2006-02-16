import java.io.InputStreamReader;
import java.io.InputStream;
import java.net.URL;
import java.awt.Image;
import java.awt.Graphics;
import javax.swing.JPanel;
import javax.swing.ImageIcon;

class HImages {
//--------------------------------------------//
	static int PLANETS_ICON_COUNT = 36;
	public Image _background;
	public Image[] _planets;
//--------------------------------------------//
	public HImages() {
		_planets = new Image[PLANETS_ICON_COUNT];
		_background = loadImage( "res/god.png" );
		for ( int i = 0; i < PLANETS_ICON_COUNT; ++ i ) {
			String path = "res/system";
			path += i + ".xpm";
			_planets[ i ] = loadImage( path );
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

public class HBoard extends JPanel {
//--------------------------------------------//
	public static final long serialVersionUID = 7l;
	int _size;
	HImages _images;
//--------------------------------------------//
	public void setImages( HImages $images ) {
		_images = $images;
	}
	void setSize( int $size ) {
		_size = $size;
	}
	protected void paintComponent( Graphics g ) {
		g.drawImage( _images._background, 0, 0, this );
		for ( int i = 0; i < 10; ++ i )
			g.drawImage( _images._planets[i], 160 + i * 32, 256, this );
		for ( int i = 10; i < 20; ++ i )
			g.drawImage( _images._planets[i], 160 + ( i - 10 ) * 32, 288, this );
		for ( int i = 20; i < 30; ++ i )
			g.drawImage( _images._planets[i], 160 + ( i - 20 ) * 32, 320, this );
		for ( int i = 30; i < 36; ++ i )
			g.drawImage( _images._planets[i], 160 + ( i - 30 ) * 32, 352, this );
	}
}

