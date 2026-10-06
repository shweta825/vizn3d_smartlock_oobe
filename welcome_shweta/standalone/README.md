# Standalone SLN-VIZN3D-IOT Smart Lock Project

This folder is a complete standalone copy of the original `smart_lock` MCUXpresso project.

## Modification
The copied UI recognizes the existing face-recognition result name. If the registered name is exactly `Shweta`, the display shows:

**Welcome Shweta**

All other recognized faces retain the original **Recognition Successful** message.

## Open and build
1. Open MCUXpresso IDE.
2. Import the project from this `standalone` folder.
3. Select the project and verify the MCU/SDK settings.
4. Build the project.
5. Connect the SLN-VIZN3D-IOT board.
6. Flash/debug the generated image.
7. Register a face with the name `Shweta`.
8. Present the registered face and verify the LCD.

## Safety of source
The original NXP `smart_lock` directory and the original NXP repository are not modified by this standalone copy.
