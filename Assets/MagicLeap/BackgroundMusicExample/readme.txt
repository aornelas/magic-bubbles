How to use ML Music Service Example:

For automatic setup, click on the Setup Music Service Example button under the MagicLeap drop down menu in the Unity Editor and build and run
the MusicService example without modifying the bundle identifier on the player settings.

For manual setup, see the following steps.

Step 1) Copy following songs from MagicLeap\BackgroundMusicExample\StreamingAssets\BackgroundMusicExample to StreamingAssets\BackgroundMusicExample:
Tune1
Tune2
Tune3

Step 2) Use the Music_Manifest that is in Assets\MagicLeap\BackgroundMusicService, rename it to Manifest.xml, put in Assets\Plugins\Lumin\

Step 3) Enable the following privileges in XR Publishing Settings:
ConnectBackgroundMusicService
ControllerPose
MusicService
RegisterBackgroundMusicService

Step 4) Make sure "example_music_provider" binary file is located at the root of the Unity project
Step 4.A) You can build example_music_provider by calling mabu on the example_music_provider.mabu file located in this folder:
mabu <path/to/your/unityproject/Assets/MagicLeap/BackgroundMusicExample/example_music_provider.mabu> -t device

This will generate the  binary in a .out folder which you can then place in the root folder

Step 5) Follow standard building procedures for a single scene, making sure music service scene is in build settings, and setting your bundle identifier (com.magicleap.unitymusicexample, for instance)

Step 6) modify the project *.package. This only gets generated at build time, so you may have to build once, then modify, then build again.
Add the following under ######### DO NOT EDIT ABOVE #########

DATAS= \
    example_music_provider \


Step 7) After building, open the mpk and verify that "example_music_provider" is in the mpk at the base level. Verify the correct manifest is in there with two components.

Step 8) Launch on device. This will launch both components. If you attempt to terminate this application, the second component (the music provider) will be stuck open and will result in AllocFailed if you try to launch again.
You need to execute terminate twice:
mldb terminate unity.package.name.here .fullscreen
mldb terminate unity.package.name.here .example_music_provider

