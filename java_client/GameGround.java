import java.util.SortedMap;
import java.util.TreeMap;
import java.util.HashMap;
import java.util.Collections;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import javax.swing.JApplet;
import javax.swing.JFrame;
import org.swixml.SwingEngine;
import java.io.File;
import java.util.Arrays;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.lang.Class;
import java.lang.reflect.Method;
import java.lang.NoSuchMethodException;

class CallStack {
	public static void print() {
		System.out.println("Call Stack:");
		StackTraceElement[] ste = (new Throwable()).getStackTrace();
		for (int i = 1; i < ste.length; ++ i ) {
			System.out.println( "Frame[" + i + "]: " + ste[i].getClassName() + "." + ste[i].getMethodName() + ", at line: " + ste[i].getLineNumber() + ", in: " + ste[i].getFileName() );
		}
	}
}

public class /* Application or applet name: */ GameGround extends JApplet {
	public static final long serialVersionUID = 13l;
	private static JFrame _frame;
	private static SortedMap<String,HAbstractLogic> _logics = java.util.Collections.synchronizedSortedMap( new TreeMap<String,HAbstractLogic>() );
	private static GameGround _instance;

	public void init() {
		try {
			new SwingEngine( this ).render( AppletJDOMHelper.loadResource( "/res/gameground.xml", this ) );
			CallStack.print();

			BufferedReader in = new BufferedReader( new InputStreamReader( getClass().getResourceAsStream( "/class.list" ) ) );
			String line;
			while ( ( line = in.readLine() ) != null ) {
				if ( line.indexOf( "$" ) == -1 ) {
					Class dynamic = Class.forName( line.substring( 0, line.indexOf( ".class" ) ) );
				}
			}
			setFace( "login" );
		} catch ( Exception e ) {
			e.printStackTrace();
		}
	}

	public static GameGround getInstance() {
		return ( _instance );
	}

	public void setFace( String $face ) {
		removeAll();
		HAbstractLogic logic = _logics.get( $face );
		if ( logic != null ) {
			add( logic.getGUI() );
			setFocusable( true );
			requestFocus();
		} else {
			System.out.println( "No such logic: " + $face + "." );
			CallStack.print();
			System.exit( 1 );
		}
		return;
	}

	static public void main( String $argv[] ) {
		_frame = new JFrame();
		_instance = new GameGround();
		_frame.add( _instance );
		_instance.init();
		_instance.start();
		_frame.setSize( _instance.getPreferredSize().width, _instance.getPreferredSize().height );
		class HAppTerminator extends WindowAdapter {
			JApplet _applet;
			public HAppTerminator( JApplet $applet ) {
				_applet = $applet;
			}
		}
		_frame.addWindowListener( new HAppTerminator( _instance ) { // Handle window close requests
			public void windowClosing( WindowEvent e ) {
				_applet.stop();
				_applet.destroy();
				System.exit( 0 );
			}
		} );
		_frame.setVisible( true );
	}

	public void addGlobalKeyListener( java.awt.Component $component, java.awt.event.KeyListener $who ) {
		$component.addKeyListener( $who );
		if ( $component instanceof java.awt.Container ) {
			java.awt.Component[] components = ((java.awt.Container)$component).getComponents();
			for ( int i = 0; i < components.length ; ++ i ) {
				addGlobalKeyListener( components[i], $who );
			}
		}
	}

	public static void registerLogic( String $name, HAbstractLogic $logic ) {
		_logics.put( $name, $logic );
		return;
	}

}

