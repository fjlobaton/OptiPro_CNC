# OptiPro_CNC
CNC machine production optimizer for AYDA class B2025


# Requirements
The project requires SDL2 and OpenGL3 installed as dependencies

the program uses Dear Imgui as its ui library and SDL2 and OpenGl3 as rendering backends

# Architecture
The system will be implemented in 3 parts,
a main ui thread running Imgui, with a thread running a event thread alongside the core optimizer thread