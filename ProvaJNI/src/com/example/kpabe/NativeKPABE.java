package com.example.kpabe;

public class NativeKPABE {

	public static final int TYPE1R_160Q_512 	= 1;
	public static final int TYPE1R_224Q_1024 	= 2;
	public static final int TYPE1R_256Q_1536 	= 3;
	
	static {
		System.loadLibrary("kpabe");
	}
	
	public native float getTick();
	
	public native double setup(
		String pubFile, 
		String mskFile,
		int type,
		String universe,
		int univSize);
	
	public native double keygen (
		String pubFile, 
		String mskFile, 
		String prvFile, 
		String policy); 
	
	public native double enc ( 
		String pubFile, 
		String inFile,
		String attributes,
		int numAttr);
	
	public native double dec ( 
		String pubFile, 
		String prvFile, 
		String inFile,
		String outFile); 
	
}
