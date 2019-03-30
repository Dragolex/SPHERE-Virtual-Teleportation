# SPHERE: Virtual Teleportation in Real-Time

SPHERE was my Bachelor Thesis project about generating 3D models from 2D cameras input in real time.

I developed a DLL in C++ that accesses the camera streams via OpenCV and computers triangle coordinates of the 3D output (including texture UV coordinates).
The DLL is usable by an UNITY project which handles the rendering.

The software part as well as the associated bachelor thesis (the .pdf file) have been graded with an 1.0 (European grading system; 4.0 American grading system).
  

# Folders Overview

## Records
Contains sample input data for a hand virtualized with two cameras.
Those files are set as default input.

## VSphere
Is a complete Unity project and part of the example.
The project uses a DLL for accessing cameras and performing most tasks which result in the final polygons. Unity is basically only used for rendering.
The DLL can be found in: VSphere\Assets\VSpherePlugin\x86_64
To use, install Unity: https://unity3d.com/ and chose this folder as the project to open.

## VSphere_DLL
Contains the Source folder with the header and cpp files of the C++ project forming the dll.  
A visual studio project can be found in:   VSphere_DLL\VisualStudioProject\VSpherePlugin  

To be able to compile the project, the external dependency OpenCV (http://opencv.org/releases.html ) in version 3.1 has to be installed on the computer and linked correctly with system paths (OPENCV_DIR is important!). Version 3.2 MIGHT work. 2.4 however would require a different way of linking into the project.  
Instructions can be found here:  
http://docs.opencv.org/master/d3/d52/tutorial_windows_install.html
http://docs.opencv.org/master/d6/d8a/tutorial_windows_visual_studio_Opencv.html



# Important Files to Start

The Visual Studio Solution contains a tiny example project besides the DLL, called "DLL_Test".  
It's source file DLL_Test.cpp demonstrates how the DLL can be used. It is commented throughly and uses the sample data from the records folder.  
It does not contain actual 3D rendering but performs all computations and shows a 2D preview.

The entry point in the main dll project (VSpherePlugin) is the file PluginInterface/VSpherePlugin.cpp.  
The classes have generally rather descriptive names and a summary of their purpose at the very top of every cpp file.

The Unity project contains several unrelated files only used for the GUI (I re-used a slightly more complex environment written once for a Software project in Artificial Inteligence).  
The only script of importance is directly in   Assets/VSpherePlugin/VSphere(.cs).  
It is used by the instance "SphereControler" in the scene Assts/Scenes/MainScene. That instance only uses this script and nothing else. The other instances in that scene are for camera and GUI.  
The VSphere.cs script initializes and uses the DLL. It also contains the system for generating the 3D model from the output coordinates received from the DLL.


# Things to Note

The project uses post-build scripts to copy the compiled DLL directly into the Assets folder of Unity.  
However because DLLs cannot be unloaded from the Editor once tested, the Unity Editor has to be closed before compiling (and thus overwriting) the DLL again. This only works as longs as folders are not renamed.

For a simple way of testing own camera input, comment the first call of the function "ConfigureRecordHandler()" out (either in the DLL_Test.cpp or the VSphere.cs file through Unity).  
Without it, the system will try to use real input for the associated camera instead of the defined file. pay attention to the command line whether it says that the Camera has been opened successfully, or not.  
The first argument of the "ConfigureCamera()" calls beforehand tell which hardware camera to use if multiple ones are accessible. 0 is usually the first camera but note that screen-capture and streaming software sometimes pretends to be camera-inputs and could be accessed.
