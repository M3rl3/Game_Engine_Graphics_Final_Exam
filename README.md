# Game_Engine_Graphics_Final_Exam
Repository for the Final Exam of Graphics-1 and Game Engine Frameworks and Patterns (INFO-6028 & INFO-6044)

Both the Graphics and the Frameworks parts of this exam are in a single solution.

<h3/> Build Instructions:
<ul>
<li> Built in Visual Studio 17 (2022) - Retarget solution if necessary.
<li> All libraries and files use relative paths, the application should run right out of the box upon build.
</ul>

<p>This application was designed using C++ and FMOD Studio API, while the GUI was drawn using imgui and opengl.</p>

<h3/> General Info:
<ol>
<li/> Both the Graphics and the Frameworks parts of the exam are in a single solution.
<li/> Initially, the camera spawnpoint will be right in front of the "dungeon" looking at the mountains behind.
<li/> The user has an option to enable or disable the mouse by pressing the 'F1' key.
<li/> The moon is placed high above the terrain/mountain that dimly lights the scene from a distance.
<li/> Panning the camera a few units up with the mouse should make the moon visible from the spawnpoint.
<li/> Three beholders patrol the scene and all of them have the vision cone as a child mesh.
<li/> These beholders will go about their day "patrolling" the room that are supposed to be in.
<li/> Hold on, before you press 'F6', please disable the mouse by pressing the 'F1' key as the mouse gets in the way of the camera targeting.
<li/> The 'F6' key will enable the spectator mode where you can switch between beholders in the scene on each keypress.
<li/> This will result in the camera slowing moving up to each of these guys and following them until you press 'F6' again.
<li/> At any point the user can press the 'C' key to get out of 'spectator' mode.
<li/> Now, the FIGHT scene, please enable the mouse again for this one (F1), also, enable the camera mode (C). 
<li/> Maybe even move the camera around a little bit with the 'W,A,S,D,Q,E' keys to reorient yourself in order to witness the showdown.
<li/> The fight scene starts by pressing the 'F7' key.
<li/> All the positioning and orientation data is being picked up from an external text document called "sceneDescription.txt" located in the project directory.
<li/> The permalink and the dungeon floor plan image from the generator website can be found in the "assets" folder in solution directory.
<li/> This dungeon is apparently called "Castle of the Dark Witch".
</ol>

<h3/> Controls and user inputs:
<ol>
<li/> Mouse can be enabled or disabled using the 'F1' key.
<li/> Camera mode can be accessed by pressing the 'C' key.
<li/> The camera can be moved around on the 3 axes pressing by the 'W,A,S,D,Q,E' keys in combination.
<li/> The pitch and yaw of the camera can be controlled with the mouse, if it is enabled.
<li/> 'F6' enables the spectator mode.
<li/> (spoiler) 'F7' starts a fight between the beholders where they all kill each other at the end.
<li/> The user can iterate between all the 312 objects/meshes in the scene by pressing the 'O' key to enable object mode.
<li/> While in the object mode, pressing 'NUM2' will target the next mesh in the scene based on their positon on the drawing array/vector, 'NUM3' will target the previous object, 'NUM1' will return focus to the origin of the scene.
<li/> The selected object can also be moved around the scene by pressing the 'W,A,S,D,Q,E' keys in combination.
<li/> User can press the 'U' key to return all objects to their original positions and orientations.
<li/> Pressing and holding the 'X' key will turn all meshes in the scene to wireframe mode.

</ol>