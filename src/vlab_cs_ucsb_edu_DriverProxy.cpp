/*
 * vlab_cs_ucsb_edu_DriverProxy.cpp
 *
 *  Created on: Aug 26, 2015
 *      Author: baki
 */

#include "vlab_cs_ucsb_edu_DriverProxy.h"
#include "Driver.h"


jfieldID getHandleField(JNIEnv *env, jobject obj)
{
    jclass c = env->GetObjectClass(obj);
    // J is the type signature for long:
    return env->GetFieldID(c, "driverPointer", "J");
}

template <typename T>
T *getHandle(JNIEnv *env, jobject obj)
{
    jlong handle = env->GetLongField(obj, getHandleField(env, obj));
    return reinterpret_cast<T *>(handle);
}

template <typename T>
void setHandle(JNIEnv *env, jobject obj, T *t)
{
    jlong handle = reinterpret_cast<jlong>(t);
    env->SetLongField(obj, getHandleField(env, obj), handle);
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    initABC
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_initABC (JNIEnv *env, jobject obj) {

  Vlab::Driver *abc_driver = new Vlab::Driver();
  abc_driver->initializeABC();
  setHandle(env, obj, abc_driver);
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    isSatisfiable
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_isSatisfiable (JNIEnv *env, jobject obj, jstring constraint) {

  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  std::istringstream input_constraint;
  const char* constraint_str = env->GetStringUTFChars(constraint, JNI_FALSE);
  input_constraint.str(constraint_str);
  abc_driver->parse(&input_constraint);
  env->ReleaseStringUTFChars(constraint, constraint_str);
  abc_driver->initializeSolver();
  abc_driver->solve();
  bool result = abc_driver->isSatisfiable();
  return (jboolean)result;
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    printResultAutomaton
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_printResultAutomaton__ (JNIEnv *env, jobject obj) {
  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  abc_driver->printResult(std::cout);
  std::cout.flush();
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    printResultAutomaton
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_printResultAutomaton__Ljava_lang_String_2 (JNIEnv *env, jobject obj, jstring filePath) {
  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  const char* file_path_str = env->GetStringUTFChars(filePath, JNI_FALSE);
  std::string file_path = file_path_str;
  abc_driver->printResult(file_path);
  env->ReleaseStringUTFChars(filePath, file_path_str);
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    reset
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_reset (JNIEnv *env, jobject obj) {
  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  abc_driver->reset();
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    dispose
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_dispose (JNIEnv *env, jobject obj) {
  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  Vlab::Driver *tmp = abc_driver;
  abc_driver = nullptr;
  setHandle(env, obj, abc_driver);
  delete tmp;
}


