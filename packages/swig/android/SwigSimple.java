package org.swig.simple;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;
import android.widget.ScrollView;
import android.text.method.ScrollingMovementMethod;
import android.content.pm.PackageManager;
import android.content.pm.PackageInfo;
import  android.content.pm.PackageManager.NameNotFoundException; 
import  android.util.Log;
import android.content.res.AssetManager;
import 	android.widget.EditText;

public class SwigSimple extends Activity
{
	 TextView outputText = null;
	ScrollView scroller = null;
	YAPEngine eng = null;
	EditText text;
	String str;
	String buf;

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		String s = null;
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);

		try {
			PackageManager m = getPackageManager();
			s = getPackageName();
			PackageInfo p = m.getPackageInfo(s, 0);
			//s = p.applicationInfo.dataDir;
			AssetManager mgr = getResources().getAssets();
			load(mgr);
		} catch(NameNotFoundException e) { 
			Log.e(TAG, "Couldn't find package  information in PackageManager", e); 
		} 
		Log.i(TAG, "mgr=" +mgr); 

		text = (EditText)findViewById(R.id.EditText01); 
		outputText = (TextView)findViewById(R.id.OutputText);
		outputText.setText("Application " + s + "\nPress 'Run' to start...\n");
		outputText.setMovementMethod(new ScrollingMovementMethod());
		scroller = (ScrollView)findViewById(R.id.Scroller);
		eng = new YAPEngine(   );  
		Log.i(TAG, "engine done");
		JavaCallback callback = new JavaCallback( outputText );
		// set the Java Callback	
		eng.setYAPCallback(callback);
		if (BuildConfig.DEBUG) {
			Log.i(TAG, "callback done");
		} 	
	}

	public void onResetButtonClick(View view)
	{
		if (BuildConfig.DEBUG) {
			Log.i(TAG, "onReseButtonClick called");
		} 	
		// Ensure scroll to end of text
		scroller.post(new Runnable() {
			public void run() {
				scroller.fullScroll(ScrollView.FOCUS_DOWN);
				text.setText("");
			}
		});
	}
	
	public void onRunButtonClick(View view)
	{
		if (BuildConfig.DEBUG) {
			Log.i(TAG, "onRunButtonClick called");
		} 	
		// Ensure scroll to end of text
		scroller.post(new Runnable() {
			public void run() {
				scroller.fullScroll(ScrollView.FOCUS_DOWN);
				str = text.getText().toString();
				outputText.append("?- " + str);
				YAPQuery q = eng.query( str );
				YAPListTerm vs = q.namedVars();
				// text.setText("");
				if (vs.nil()) {
					if (q.next()) {
						outputText.append( "yes\n" );
					} else {
						outputText.append( "no\n" );
					}
				} else {
					while (q.next()) {
						int i=1;
						// outputText.append(Integer.toString(i++) + ": " + vs.text() +"\n");
						while(!vs.nil()){
							YAPTerm eq = vs.car();
							//outputText.append(Integer.toString(i) + ": " + eq.text() );
							outputText.append(Integer.toString(i++) + ":\t" + eq.getArg(1).text() + " = " + eq.getArg(2).text() +"\n" );
							vs = vs.cdr();
						}	
					}
				}
				q.close();
			}
		});
	}

	public void onRunSelectionButtonClick(View view)
	{
		if (BuildConfig.DEBUG) {
			Log.i(TAG, "onRunButtonClick called");
		} 	
		// Ensure scroll to end of text
		scroller.post(new Runnable() {
			public void run() {
				scroller.fullScroll(ScrollView.FOCUS_DOWN);
				int startSelection = text.getSelectionStart();
				int endSelection = text.getSelectionEnd();
				str = text.getText().toString().substring( startSelection, endSelection );
				outputText.append("?- " + str);
				YAPQuery q = eng.query( str );
				YAPListTerm vs = q.namedVars();
				// text.setText("");
				if (vs.nil()) {
					if (q.next()) {
						outputText.append( "yes\n" );
					} else {
						outputText.append( "no\n" );
					}
				} else {
					while (q.next()) {
						int i=1;
						// outputText.append(Integer.toString(i++) + ": " + vs.text() +"\n");
						while(!vs.nil()){
							YAPTerm eq = vs.car();
							//outputText.append(Integer.toString(i) + ": " + eq.text() );
							outputText.append(Integer.toString(i++) + ":\t" + eq.getArg(1).text() + " = " + eq.getArg(2).text() +"\n" );
							vs = vs.cdr();
						}	
					}
				}
				q.close();
			}
		});
	}


	/** static constructor */
	static {
		System.loadLibrary("android");
		System.loadLibrary("log");
		System.loadLibrary("gmp");
		System.loadLibrary("example");
	}

	private static native void load(AssetManager mgr);

	private AssetManager mgr;

	private static final String TAG = "SwigSimple";

}

class JavaCallback extends YAPCallback
{
	TextView output;
	
  public JavaCallback( TextView outputText )
  {
    super();
    output =  outputText;
    Log.i(TAG, "java callback init");
  }


  public void run()
  {
    System.out.println("JavaCallback.run() ");
	Log.i(TAG, "java callback ");
 }
  
  public void displayInWindow(String s)
  {
    System.out.println("JavaCallback.run() ");
	Log.i(TAG, "java callback ");
	output.append(s);
 }
  
  private static final String TAG = "JavaCallback";

}