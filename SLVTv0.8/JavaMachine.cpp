#include "stdafx.h"
#include <iostream>
#include <jni.h>

#include "JavaMachine.h"

using namespace std;

JavaMachine::JavaMachine(){

}

JavaMachine::~JavaMachine(){

}


int JavaMachine::initialize(){

	//==================== prepare loading of Java VM ============================

	JavaVMInitArgs vm_args;                        // Initialization arguments
	JavaVMOption* options = new JavaVMOption[1];   // JVM invocation options
	options[0].optionString = "-Djava.class.path=.;weka.jar";   // where to find java .class
	vm_args.version = JNI_VERSION_1_6;             // minimum Java version
	vm_args.nOptions = 1;                          // number of options
	vm_args.options = options;
	vm_args.ignoreUnrecognized = false;     // invalid options make the JVM init fail

	//================= load and initialize Java VM and JNI interface ===============

	jint rc = JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);  // YES !!
	delete options;    // we then no longer need the initialisation options. 

	//========================= analyse errors if any  ==============================
	// if process interuped before error is returned, it's because jvm.dll can't be 
	// found, i.e.  its directory is not in the PATH. 

	if (rc != JNI_OK) {
		if (rc == JNI_EVERSION)
			cerr << "FATAL ERROR: JVM is oudated and doesn't meet requirements" << endl;
		else if (rc == JNI_ENOMEM)
			cerr << "FATAL ERROR: not enough memory for JVM" << endl;
		else if (rc == JNI_EINVAL)
			cerr << "FATAL ERROR: invalid ragument for launching JVM" << endl;
		else if (rc == JNI_EEXIST)
			cerr << "FATAL ERROR: the process can only launch one JVM an not more" << endl;
		else
			cerr << "FATAL ERROR:  could not create the JVM instance (error code " << rc << ")" << endl;
		cin.get();
		exit(EXIT_FAILURE);
	}

	cout << "JVM load succeeded. \nVersion ";
	jint ver = env->GetVersion();
	cout << ((ver >> 16) & 0x0f) << "." << (ver & 0x0f) << endl;

	return 1;
}




const char * JavaMachine::classify(std::vector<double> raw_instance){

	// Get the java class from where the classification model is called
	jclass javaClass = env->FindClass("WekaModelTest");
	if (javaClass == nullptr){
		cerr << "ERROR: class not found.";
		Sleep(3000);
		return NULL;
	}

	// Get the function ID to call the classification model
	jmethodID method_id = env->GetStaticMethodID(javaClass, "classifyInstance", "([D)Ljava/lang/String;");
	if (method_id == nullptr){
		cerr << "ERROR: method void classifyInstance() not found." << endl;
		Sleep(3000);
		return NULL;
	}

	// Create the argument array to pass to the function in java
	//size_t raw_instance_size = sizeof(raw_instance) / sizeof(*raw_instance);
	size_t raw_instance_size = raw_instance.size();
	jdoubleArray instance = env->NewDoubleArray(raw_instance_size);
	jdouble * buff = (jdouble *)malloc(raw_instance_size * sizeof(jdouble));
	
	for (int i = 0; i < raw_instance_size; i++)
		buff[i] = raw_instance.at(i);

	env->SetDoubleArrayRegion(instance, 0, raw_instance_size, buff);
	free(buff);

	// Get the class of the instance from java
	jstring jvalue = (jstring)env->CallStaticObjectMethod(javaClass, method_id, instance);
	const char * value = env->GetStringUTFChars(jvalue, 0);
	
	return value;
}