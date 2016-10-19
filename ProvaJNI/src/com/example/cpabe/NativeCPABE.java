package com.example.cpabe;

public class NativeCPABE {

	public static final int TYPE1R_160Q_512 	= 1;
	public static final int TYPE1R_224Q_1024 	= 2;
	public static final int TYPE1R_256Q_1536 	= 3;
	
	static {
		System.loadLibrary("cpabe");
	}
	
	public native float getTick();
	
	public native double setup(
		String pubFile, 
		String mskFile,
		int parameters_type);
	
	public native double keygen (
		String pubFile, 
		String mskFile, 
		String prvFile, 
		String attributes,
		int numAttr); 
	
	public native double enc ( 
		String pubFile, 
		String jpolicy,
		String inFile);
	
	public native double dec ( 
		String pubFile, 
		String prvFile, 
		String inFile,
		String outFile); 
	
}
