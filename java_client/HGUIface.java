import javax.swing.JPanel;
import javax.swing.JTextPane;
import java.awt.Color;
import javax.swing.text.SimpleAttributeSet;
import javax.swing.text.StyleConstants;
import javax.swing.text.DefaultStyledDocument;

abstract class HGUIface extends JPanel {
	public static final long serialVersionUID = 17l;
	DefaultStyledDocument _log;
	public JTextPane _logPad;
	public Color[] _colors;
	public SimpleAttributeSet _attribute;
	public int _color;
	public String _resource;
	public static final class Style {
		public static final int NORMAL = 0;
		public static final int BOLD = 1;
		public static final int ITALIC = 2;
	}
	public static final class Colors {
		public static final int BLACK = 0;
		public static final int RED = 1;
		public static final int GREEN = 2;
		public static final int BROWN = 3;
		public static final int BLUE = 4;
		public static final int MAGENTA = 5;
		public static final int CYAN = 6;
		public static final int LIGHTGRAY = 7;
		public static final int DARKGRAY = 8;
		public static final int BRIGHTRED = 9;
		public static final int BRIGHTGREEN = 10;
		public static final int YELLOW = 11;
		public static final int BRIGHTBLUE = 12;
		public static final int BRIGHTMAGENTA = 13;
		public static final int BRIGHTCYAN = 14;
		public static final int WHITE = 15;
		public static final int OTHERGRAY = 16;
		public static final int RESET = 256;
		public static final int PALLETE_SIZE = 20;
	}
	public int COLOR_DEFAULT = Colors.RESET;
	public HGUIface( String $resource ) {
		_resource = $resource;
		_attribute = new SimpleAttributeSet();
		_color = COLOR_DEFAULT;
		_colors = new Color[ Colors.PALLETE_SIZE ];

		_colors[ Colors.BLACK ] = Color.black;
		_colors[ Colors.RED ] = Color.red; /* red */
		_colors[ Colors.GREEN ] = new Color( 0x00, 0xa0, 0x00 ); /* green */
		_colors[ Colors.BROWN ] = new Color( 0xa0, 0xa0, 0x00 ); /* brown? */
		_colors[ Colors.BLUE ] = Color.blue; /* blue */
		_colors[ Colors.MAGENTA ] = new Color( 0xa0, 0x00, 0xa0 ); /* magenta */
		_colors[ Colors.CYAN ] = new Color( 0x00, 0xa0, 0xa0 );; /* cyan */
		_colors[ Colors.LIGHTGRAY ] = Color.lightGray;
		_colors[ Colors.DARKGRAY ] = Color.darkGray;
		_colors[ Colors.BRIGHTRED ] = new Color( 0xff, 0x80, 0x80 ); /* bright red */
		_colors[ Colors.BRIGHTGREEN ] = new Color( 0x80, 0xff, 0x80 ); /* bright green */
		_colors[ Colors.YELLOW ] = Color.yellow; /* yellow */
		_colors[ Colors.BRIGHTBLUE ] = new Color( 0x80, 0x80, 0xff ); /* bright blue */
		_colors[ Colors.BRIGHTMAGENTA ] = Color.magenta; /* bright magenta */
		_colors[ Colors.BRIGHTCYAN ] = Color.cyan; /* bright cyan */
		_colors[ Colors.WHITE ] = Color.white;
		_colors[ Colors.OTHERGRAY ] = Color.gray;
	}
	void init() {
		try {
			String res = "/res/" + _resource + ".xml";
			System.out.println( "Loading resources: " + res );
			XUL xul = new XUL( this );
			xul.getTaglib().registerTag( "readlineprompt", ReadlinePrompt.class );
			updateTagLib( xul );
			xul.insert( AppletJDOMHelper.loadResource( res, this ), this );
			mapMembers( xul );
		} catch ( java.lang.Exception e ) {
			e.printStackTrace();
			System.exit( 1 );
		}
		if ( getLogPad() != null ) {
			_logPad = getLogPad();
			_log = ( DefaultStyledDocument )_logPad.getStyledDocument();
		} else
			System.out.println( "No logPad for this face." );
	}
	public abstract void updateTagLib( XUL $se );
	public void mapMembers( XUL $se ) {}
	public void onShow() {}
	public abstract JTextPane getLogPad();
	public static int brightness( Color $color ) {
		double red = (double)$color.getRed();
		double green = (double)$color.getGreen();
		double blue = (double)$color.getBlue();
		double v = Math.sqrt( red * red * 0.299 + green * green * 0.587 + blue * blue * 0.114 );
		return ( (int)v );
	}
	public Color color( int $color ) { return ( color( $color, null ) ); }
	public Color color( int $color, java.awt.Component $on ) {
		if ( $on == null ) {
			$on = _logPad;
		}
		int lum = brightness( $on.getBackground() );
		if ( $color == -1 ) {
			$color = Colors.LIGHTGRAY;
		}
		if ( $color == Colors.RESET ) {
			$color = Colors.LIGHTGRAY;
		}
		if ( lum > 128 ) {
			if ( ( $color == Colors.LIGHTGRAY ) || ( $color == Colors.WHITE ) ) {
				$color = Colors.BLACK;
			} else if ( $color < Colors.DARKGRAY ) {
				$color += Colors.DARKGRAY;
			} else if ( $color >= Colors.DARKGRAY ) {
				$color -= Colors.DARKGRAY;
			}
		}
		return ( _colors[ $color ] );
	}
	public int localColor( int $color ) {
		return ( $color );
	}
	void log( String $message, int $color, int $style ) {
		StyleConstants.setForeground( _attribute, color( localColor( $color ) ) );
		if ( ( $style & Style.BOLD ) != 0 ) {
			StyleConstants.setBold( _attribute, true );
		}
		if ( ( $style & Style.ITALIC ) != 0 ) {
			StyleConstants.setItalic( _attribute, true );
		}
		add( _logPad, $message, _attribute );
	}
	void log( String $message, int $color ) {
		StyleConstants.setForeground( _attribute, color( localColor( $color ) ) );
		StyleConstants.setBold( _attribute, false );
		StyleConstants.setItalic( _attribute, false );
		add( _logPad, $message, _attribute );
	}
	void log( String $message ) {
		StyleConstants.setForeground( _attribute, color( localColor( _color ) ) );
		StyleConstants.setBold( _attribute, false );
		StyleConstants.setItalic( _attribute, false );
		add( _logPad, $message, _attribute );
	}
	void log( int $color ) {
		_color = $color;
	}
	abstract public void onClose();
	public void clearLog() {
		clear( _logPad );
		_log = ( DefaultStyledDocument )_logPad.getStyledDocument();
	}
	public void clear( JTextPane $what ) {
		$what.setStyledDocument( new DefaultStyledDocument() );
	}
	public HAbstractConfigurator getConfigurator() {
		return ( null );
	}
	void add( JTextPane $to, String $what, SimpleAttributeSet $looks ) {
		DefaultStyledDocument txt = (DefaultStyledDocument)$to.getStyledDocument();
		try {
			txt.insertString( txt.getLength(), $what, $looks );
			$to.setCaretPosition( txt.getLength() );
		} catch ( javax.swing.text.BadLocationException e ) {
			e.printStackTrace();
			System.exit( 1 );
		}
	}
	void add( JTextPane $to, String $what ) {
		add( $to, $what, null );
	}
	protected void discardPendingEvents() {
		java.awt.EventQueue q = java.awt.Toolkit.getDefaultToolkit().getSystemEventQueue();
		while ( q.peekEvent() != null ) {
			try {
				q.getNextEvent();
			} catch (InterruptedException e) {
			}
		} 
	}
}
