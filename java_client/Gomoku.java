import java.awt.Container;
import java.awt.event.ActionEvent;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.lang.reflect.Method;
import java.net.URL;
import java.util.HashMap;
import java.util.SortedMap;
import java.util.TreeMap;
import java.util.Vector;
import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JList;
import javax.swing.JTextField;
import javax.swing.JTextPane;
import javax.swing.table.AbstractTableModel;
import javax.swing.text.DefaultStyledDocument;
import javax.swing.text.Style;
import java.util.Comparator;
import org.swixml.SwingEngine;
import javax.swing.table.TableRowSorter;
import javax.swing.RowSorter;
import javax.swing.DefaultListModel;
import java.util.Collections;
import java.util.Map;
import java.util.Date;
import java.text.SimpleDateFormat;
import java.util.Calendar;

class GomokuPlayer {
	public JLabel _name;
	public JLabel _score;
	public JButton _sit;
	public void clear() {
		_name.setText( "" );
		_score.setText( "" );
		_sit.setText( Gomoku.HGUILocal.SIT );
	}
}

class Gomoku extends HAbstractLogic implements Runnable {
	public static final class PROTOCOL {
		public static final String SEP = ":";
		public static final String SEPP = ",";
		public static final String CMD = "cmd";
		public static final String SAY = "say";
		public static final String NAME = "gomoku";
		public static final String PLAY = "play";
		public static final String PASS = "pass";
		public static final String SETUP = "setup";
		public static final String ADMIN = "admin";
		public static final String CONTESTANT = "contestant";
		public static final String SIT = "sit";
		public static final String GETUP = "get_up";
		public static final String PUTSTONE = "put_stone";
		public static final String STONES = "stones";
		public static final String STONE = "stone";
		public static final String TOMOVE = "to_move";
		public static final String PLAYER = "player";
		public static final String PLAYERQUIT = "player_quit";
		public static final String PREFIX = PROTOCOL.CMD + PROTOCOL.SEP + PROTOCOL.NAME + PROTOCOL.SEP;
	}
	public static final class GOBAN_SIZE {
		public static final int NORMAL = 19;
	}
	public static final class STONE {
		public static final char BLACK = 'b';
		public static final char WHITE = 'w';
		public static final char NONE	= ' ';
		public static final char MARK = 'm';
		public static final char DEAD_BLACK = 's';
		public static final char DEAD_WHITE = 't';
		public static final char TERITORY_BLACK = 'p';
		public static final char TERITORY_WHITE = 'q';
		public static final char DAME = 'x';
		public static final char INVALID = 'N';
		public static final char WAIT = 'Z';
		public static final String NONE_NAME = "None";
		public static final String BLACK_NAME = "Black";
		public static final String WHITE_NAME = "White";
	}
	public class HGUILocal extends HGUIface {
		public static final long serialVersionUID = 17l;
		public static final String SIT = "Sit !";
		public static final String GETUP = "Get up !";
		public JTextField _messageInput;
		public JTextField _wordInput;
		public JTextPane _logPad;
		public JLabel _blackName;
		public JLabel _blackScore;
		public JButton _blackSit;
		public JLabel _whiteName;
		public JLabel _whiteScore;
		public JButton _whiteSit;
		public JList _visitors;
		public Goban _board;
		public DummyConfigurator _conf;
		public JLabel _toMove;
		public JLabel _move;
		public String _toolTip;
		public String _passText;
		public HGUILocal( String $resource ) {
			super( $resource );
		}
		public void updateTagLib( XUL $xul ) {
			$xul.getTaglib().registerTag( "goban", Goban.class );
			$xul.getTaglib().registerTag( "setup", DummyConfigurator.class );
		}
		public void init() {
			super.init();
		}
		public void reinit() {
			clearLog();
			((DefaultListModel)_visitors.getModel()).clear();
		}
		public JTextPane getLogPad() {
			return ( _logPad );
		}
		public HAbstractConfigurator getConfigurator() {
			return ( _conf );
		}
		public void onMessage() {
			String msg = _messageInput.getText();
			if ( msg.matches( ".*\\S+.*" ) ) {	
				_client.println( PROTOCOL.PREFIX + PROTOCOL.SAY + PROTOCOL.SEP + msg );
				_messageInput.setText( "" );
			}
		}
		public void onExit() {
			_app.setFace( HBrowser.LABEL );
		}
		public void onBlack() {
			_client.println( PROTOCOL.CMD + PROTOCOL.SEP
					+ PROTOCOL.PLAY + PROTOCOL.SEP
					+ ( ( _stone == Gomoku.STONE.NONE ) ? PROTOCOL.SIT + PROTOCOL.SEPP + STONE.BLACK : PROTOCOL.GETUP ) );
			_blackSit.setEnabled( _stone != Gomoku.STONE.NONE );
			_whiteSit.setEnabled( _stone != Gomoku.STONE.NONE );
		}
		public void onWhite() {
			_client.println( PROTOCOL.CMD + PROTOCOL.SEP
					+ PROTOCOL.PLAY + PROTOCOL.SEP
					+ ( ( _stone == Gomoku.STONE.NONE ) ? PROTOCOL.SIT + PROTOCOL.SEPP + STONE.WHITE : PROTOCOL.GETUP ) );
			_blackSit.setEnabled( _stone != Gomoku.STONE.NONE );
			_whiteSit.setEnabled( _stone != Gomoku.STONE.NONE );
		}
	}
//--------------------------------------------//
	public static final long serialVersionUID = 17l;
	public static final String LABEL = "gomoku";
	public HGUILocal _gui;
	public HClient _client;
	private char _stone = STONE.NONE;
	private char _toMove = STONE.NONE;
	private long _start = 0;
	private boolean _admin = false;
	private int _move = 0;
	private String _stones = null;
	private SortedMap<Character,GomokuPlayer> _contestants = java.util.Collections.synchronizedSortedMap( new TreeMap<Character,GomokuPlayer>() );
//--------------------------------------------//
	public Gomoku( GameGround $applet ) throws Exception {
		super( $applet );
		init( _gui = new HGUILocal( LABEL ) );
		_handlers.put( PROTOCOL.NAME, Gomoku.class.getDeclaredMethod( "handlerGomoku", new Class[]{ String.class } ) );
		_handlers.put( PROTOCOL.STONES, Gomoku.class.getDeclaredMethod( "handlerStones", new Class[]{ String.class } ) );
		_handlers.put( PROTOCOL.PLAYER, Gomoku.class.getDeclaredMethod( "handlerPlayer", new Class[]{ String.class } ) );
		_handlers.put( PROTOCOL.TOMOVE, Gomoku.class.getDeclaredMethod( "handlerToMove", new Class[]{ String.class } ) );
		_handlers.put( PROTOCOL.CONTESTANT, Gomoku.class.getDeclaredMethod( "handlerContestant", new Class[]{ String.class } ) );
		_handlers.put( PROTOCOL.PLAYERQUIT, Gomoku.class.getDeclaredMethod( "handlerPlayerQuit", new Class[]{ String.class } ) );
		_info = new HLogicInfo( "gomoku", "gomoku", "Gomoku" );
		GoImages images = new GoImages();
		//_gui._board.setGui( this );
		_gui._board.setImages( images );
		GomokuPlayer black = new GomokuPlayer();
		black._name = _gui._blackName;
		black._score = _gui._blackScore;
		black._sit = _gui._blackSit;
		_contestants.put( new Character( STONE.BLACK ), black );
		GomokuPlayer white = new GomokuPlayer();
		white._name = _gui._whiteName;
		white._score = _gui._whiteScore;
		white._sit = _gui._whiteSit;
		_contestants.put( new Character( STONE.WHITE ), white );
	}
	void handlerGomoku( String $command ) {
		processMessage( $command );
	}
	void handlerStones( String $command ) {
		_gui._board.setStones( ( _stones = $command ).getBytes() );
		_gui._board.repaint();
	}
	void handlerContestant( String $command ) {
		String[] tokens = $command.split( ",", 6 );
		char stone = STONE.NONE;
		GomokuPlayer contestant = _contestants.get( new Character( stone = tokens[ 0 ].charAt( 0 ) ) );
		contestant._name.setText( tokens[ 1 ] );
		contestant._score.setText( tokens[ 3 ] );
		if ( tokens[ 1 ].equals( _app.getName() ) ) {
			_gui._board.setStone( _stone = stone );
			contestant._sit.setText( HGUILocal.GETUP );
			contestant._sit.setEnabled( true );
			GomokuPlayer foe = _contestants.get( _gui._board.oponent( _stone ) );
			foe._sit.setEnabled( false );
		} else {
			if ( ( stone == _stone ) && ( _stone != STONE.NONE ) ) {
				contestant._sit.setText( HGUILocal.SIT );
				GomokuPlayer foe = _contestants.get( _gui._board.oponent( _stone ) );
				foe._sit.setEnabled( "".equals( foe._name.getText() ) );
				_gui._board.setStone( _stone = STONE.NONE );
			}
			contestant._sit.setEnabled( "".equals( tokens[ 1 ] ) && ( _stone == STONE.NONE ) );
		}
		_start = new Date().getTime();
	}
	void handlerToMove( String $command ) {
		_toMove = $command.charAt( 0 );
		++ _move;
		String toMove = STONE.NONE_NAME;
		if ( _toMove == STONE.BLACK )
			toMove = STONE.BLACK_NAME;
		else if ( _toMove == STONE.WHITE )
			toMove = STONE.WHITE_NAME;
		else
			_move = 0;
		_gui._move.setText( "" + _move );
		_gui._toMove.setText( toMove );
	}
	void handlerPlayer( String $command ) {
		DefaultListModel m = (DefaultListModel)_gui._visitors.getModel();
		m.addElement( $command );
	}
	void handlerPlayerQuit( String $command ) {
		DefaultListModel m = (DefaultListModel)_gui._visitors.getModel();
		m.removeElement( $command );
	}
	public boolean isMyMove() {
		return ( ( _stone != STONE.NONE ) && ( _stone == _toMove ) );
	}
	public char stone() {
		return ( _stone );
	}
	public char stoneDead() {
		return ( _stone == STONE.BLACK ? STONE.DEAD_BLACK : STONE.DEAD_WHITE );
	}
	public char toMove() {
		return ( _toMove );
	}
	public void waitToMove() {
		_toMove = STONE.WAIT;
	}
	public void reinit() {
		_client = _app.getClient();
		_contestants.get( new Character( STONE.BLACK ) ).clear();
		_contestants.get( new Character( STONE.WHITE ) ).clear();
		_stone = STONE.NONE;
		_toMove = STONE.NONE;
		_admin = false;
		_move = 0;
		_app.registerTask( this, 1 );
	}
	public void cleanup() {
		_app.flush( this );
	}
	public void run() {
	}
	static boolean registerLogic( GameGround $app ) {
		try {
			$app.registerLogic( LABEL, new Gomoku( $app ) );
		} catch ( Exception e ) {
			e.printStackTrace();
			System.exit( 1 );
		}
		return ( true );
	}
}

