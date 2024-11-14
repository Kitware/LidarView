# Rotative LiDAR Interpreter Example

This plugin showcase how to implement a Rotative Lidar Interpreter.

All necessary implementations are illustrated in this example, which
should be kept updated as development progresses.

### File Architecture

It follow the same architecture than [Time Based Lidar Interpreter Architecture](../TimeBasedLidarInterpreter/README.md#file-architecture).
The primary differences are the type of data being received and the conditions used to split frames.

### Testing

To test a new interpreter, one could use `vtkLidarTestTools::TestPacketInterpreter`.
This method performs sanity checks on how LidarView (and its tools such as SLAM)
expect the interpreter to behave.
