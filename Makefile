
CXX=g++ -std=c++11 -g
JVM_HOME=/usr/lib/jvm/java-1.11.0-openjdk-amd64/
JVMTI_HEADER=-I$(JVM_HOME)/include/ -I$(JVM_HOME)/include/linux
EXPORT_LIB_NAME=libmeasure.so

measure:
	javac edu/ucsb/pllab/tools/Measure.java
	g++ -c -fPIC $(JVMTI_HEADER) edu_ucsb_pllab_tools_Measure.cc -o edu_ucsb_pllab_tools_Measure.o
	g++ -shared -fPIC -o libmeasure.so edu_ucsb_pllab_tools_Measure.o -lc

%.o: %.cc
	$(CXX) $(JVMTI_HEADER) -Wall -c -fPIC $< -lc

clean:
	rm -rf $(EXPORT_LIB_NAME) *.o
