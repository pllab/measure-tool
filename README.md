# measure-tool
A tool for measuring memory consumption in the JVM

# Dependencies

- version of Java >=1.7 is needed to build.
- g++ 6+
- make

# Install

1. Check the Makefile to see whether the JVM home location is correct (probably not, depending on the Java version), so change that appropriately.

2. Run `make`

3a. When running a Java program, run it with an additional `-agentpath:libmeasure.so` argument to use the tool. 

3b. Create a jar file out of the whole folder and include it in your Java project's classpath. Now you can use `edu.ucsb.pllab.tools.Measure` methods `void start()` and `long stop()` to measure memory only within that region.

# Notes

The currently important use case was measuring the maximum amount of heap allocations found _after_ a garbage collection pass, so it may report `0` for cases where that didn't happen yet. This strategy is a time-saver for our current experiments, and isn't supposed to stay.
