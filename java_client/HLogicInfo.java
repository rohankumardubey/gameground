import java.lang.reflect.Method;
import java.util.SortedMap;
import java.util.TreeMap;

public class HLogicInfo {
	public String _symbol;
	public String _face;
	public String _name;
	public String _defaults;
	public HAbstractConfigurator _conf = null;
	private Method _creator = null;
	private SortedMap<String, Party> _partys = java.util.Collections.synchronizedSortedMap( new TreeMap<String, Party>() );

	public HLogicInfo( String $symbol, String $face, String $name, HAbstractConfigurator $conf, Method $creator ) {
		_symbol = $symbol;
		_face = $face;
		_name = $name;
		_conf = $conf;
		_creator = $creator;
		try {
			String res = "/res/" + _face + "-conf.xml";
			System.out.println( "Loading resources: " + res );
			XUL xul = new XUL( _conf );
			xul.insert( AppletJDOMHelper.loadResource( res, _conf ), _conf );
			xul.mapMembers( _conf );
		} catch ( java.lang.Exception e ) {
			e.printStackTrace();
			System.exit( 1 );
		}
	}
	public HAbstractLogic create( GameGround $app ) {
		HAbstractLogic logic = null;
		try {
			logic = (HAbstractLogic)_creator.invoke( null, new Object [] { $app, this } );
		} catch ( java.lang.IllegalAccessException e ) {
			e.printStackTrace();
			System.exit( 1 );
		} catch ( java.lang.reflect.InvocationTargetException e ) {
			e.printStackTrace();
			System.exit( 1 );
		}
		return ( logic );
	}
	public int getPartysCount() {
		return ( _partys.size() );
	}
	public String toString() {
		return ( _name );
	}
	public Party getParty( String $id ) {
		return ( _partys.get( $id ) );
	}
	public void addParty( String $id, Party $party ) {
		_partys.put( $id, $party );
	}
	public void setDefaults( String $defaults ) {
		_conf.setDefaults( $defaults );
	}
	public java.util.Iterator<java.util.Map.Entry<String, Party>> partyIterator() {
		return ( _partys.entrySet().iterator() );
	}
}
