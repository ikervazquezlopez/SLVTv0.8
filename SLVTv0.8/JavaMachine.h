#include "stdafx.h"
#include <jni.h>



class JavaMachine{

public:
	JavaMachine();
	~JavaMachine();

	JavaVM *jvm;				// Pointer to the JVM (Java Virtual Machine)
	JNIEnv *env;				// Pointer to native interface

	/* Initialize Java Virtual Machine */
	int JavaMachine::initialize();

	/* Classify the instance */
	const char * JavaMachine::classify(std::vector<double> raw_instance);




};