#include <jvmti.h>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <jni.h>

#include "edu_ucsb_pllab_tools_Measure.h"

static bool garbageCollectionDone;
static unsigned long allocated;
static bool active;

static jmethodID mainID;
static jlocation mainEnd;

void check_jvmti_error(jvmtiEnv* env, const jvmtiError& err, const std::string& msg)
{
  if(err != JVMTI_ERROR_NONE)
  {
    char* message = (char*)malloc(256);
    env->GetErrorName(err, &message);

    std::cout << "Error [" << msg << "] ";
    printf("%s\n", message);

    env->Deallocate((unsigned char*)(message));
    exit(err);
  }
}

jint JNICALL heapIterator(jlong class_tag, jlong size, jlong* tag_ptr, jint length, void* user_data)
{
  allocated += size;
  return JVMTI_VISIT_OBJECTS;
}

jint JNICALL primitiveFieldFound
    (jvmtiHeapReferenceKind kind, 
     const jvmtiHeapReferenceInfo* info, 
     jlong object_class_tag, 
     jlong* object_tag_ptr, 
     jvalue value, 
     jvmtiPrimitiveType value_type, 
     void* user_data)
{ return 0; }

jint JNICALL arrayFieldFound
    (jlong class_tag, 
     jlong size, 
     jlong* tag_ptr, 
     jint element_count, 
     jvmtiPrimitiveType element_type, 
     const void* elements, 
     void* user_data)
{ return 0; }

jint stringFound
    (jlong class_tag, 
     jlong size, 
     jlong* tag_ptr, 
     const jchar* value, 
     jint value_length, 
     void* user_data)
{ return 0; }

void startHeapIteration(jvmtiEnv* env)
{
	std::cout << "MEASURE: Heap iteration running..." << std::endl;
	long max = allocated;
	allocated = 0L;
  jvmtiHeapCallbacks callbacks;
  callbacks.heap_iteration_callback = &heapIterator;
  callbacks.primitive_field_callback = &primitiveFieldFound;
  callbacks.array_primitive_value_callback = &arrayFieldFound;
  callbacks.string_primitive_value_callback = &stringFound;
  jvmtiError error = env->IterateThroughHeap(0 , nullptr , &callbacks, nullptr);
  check_jvmti_error(env, error, "Cannot iterate over heap.");

  if(max > allocated)
  	allocated = max;

	std::cout << "MEASURE: Heap iteration ended." << std::endl;
}

void JNICALL measureSingleStep(jvmtiEnv *env, JNIEnv* jni_env, 
  jthread thread, jmethodID method, jlocation location)
{
  if(garbageCollectionDone)
  {
    garbageCollectionDone = false;
    startHeapIteration(env);
  }
  else if(method == mainID && rand() % 100 == 1)
  {
    startHeapIteration(env);
  }
}

static char* name;
static char* sig;

void JNICALL measureMethodEntry(jvmtiEnv *env, JNIEnv* jni_env,
            jthread thread, jmethodID method)
{
  if(mainID == nullptr)
  {
	  env->GetMethodName(method, &name, &sig, nullptr);
  	if((strcmp(name, "main") == 0 && 
  		 strcmp(sig, "([Ljava/lang/String;)V") == 0))
  	{
	  	mainID = method;
	  	jlocation start, end;
			env->GetMethodLocation(method, &start, &end);
			mainEnd = end;
	  }
	}
}

void JNICALL measureGCEnd(jvmtiEnv *env)
{
  if(active)
    garbageCollectionDone = true;
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved)
{
	srand(0);
  mainID = nullptr;
  garbageCollectionDone = false;
  active = true;
  allocated = 0L;

  jvmtiEnv *env = nullptr;
  jint result = vm->GetEnv((void**) &env, JVMTI_VERSION_1_1);
  if(result != JNI_OK) 
  {
    std::cout << "Unable to access JVMTI\n" << std::endl;
    return result;
  }

  jvmtiCapabilities caps;
  jvmtiError error;
  (void)memset(&caps, 0, sizeof(jvmtiCapabilities));

  caps.can_generate_method_entry_events = 1;
  caps.can_generate_method_exit_events = 1;
  caps.can_generate_vm_object_alloc_events = 1;
  caps.can_generate_garbage_collection_events = 1;
  caps.can_generate_object_free_events = 1;
  caps.can_generate_single_step_events = 1;
  caps.can_tag_objects = 1;
  
  error = env->AddCapabilities(&caps);
  check_jvmti_error(env, error, "Cannot set JVMTI capabilities");

  jvmtiEventCallbacks callbacks;
  (void)memset(&callbacks, 0, sizeof(callbacks));

  callbacks.GarbageCollectionFinish = &measureGCEnd;
  callbacks.SingleStep = &measureSingleStep;
  callbacks.MethodEntry = &measureMethodEntry;

  error = env->SetEventCallbacks(&callbacks,(jint)sizeof(callbacks));
  check_jvmti_error(env, error, "Cannot set JVMTI callbacks");

  error = env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_SINGLE_STEP, nullptr);
  check_jvmti_error(env, error, "Cannot enable single step notifications");

  error = env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, nullptr);
  check_jvmti_error(env, error, "Cannot enable garbage collection finished notifications");

  error = env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, nullptr);
  check_jvmti_error(env, error, "Cannot enable method entry notifications");

  name = (char*)malloc(80);
  sig = (char*)malloc(40);

  return JNI_OK;
}

JNIEXPORT void JNICALL Java_edu_ucsb_pllab_tools_Measure_start(JNIEnv *env, jclass thisClass)
{
  std::cout << "activating measurement." << std::endl;
  garbageCollectionDone = false;
  active = true;
  allocated = 0L;
}

JNIEXPORT jlong JNICALL Java_edu_ucsb_pllab_tools_Measure_stop(JNIEnv *env, jclass thisClass)
{
  if(!active)
  {
  	std::cout << "CALLED BEFORE START!" << std::endl;
  	exit(-1);
  }

  active = false;
  std::cout << "ending measurement." << std::endl;
  return allocated;
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm)
{
	std::cout << "Allocated: " << allocated << std::endl;
}