# Standalone Welcome Shweta Smart Lock

This is a complete copy of the repository's `smart_lock` project, stored separately from the original `smart_lock` folder.

## Function change
When the existing face-recognition result contains the registered name `Shweta`, the copied UI displays **Welcome Shweta**. Other recognized users retain the normal **Recognition Successful** message.

## Run on SLN-VIZN3D-IOT
Open this folder as an MCUXpresso project, build with the same SDK/toolchain used by the original project, flash the generated image to the kit, register a face with the name `Shweta`, and test recognition.

The original NXP source tree is not modified.
