/*
 * Welcome Shweta demo
 *
 * Standalone helper for the SLN-VIZN3D-IOT Smart Lock project.
 * This file does not modify the original NXP source code.
 */

#include <stdio.h>
#include <string.h>

/*
 * Returns the message that should be displayed after face identification.
 *
 * recognized_name:
 *   Name returned by the existing face-recognition / face database layer.
 *
 * message:
 *   Output buffer supplied by the caller.
 */
void WelcomeShweta_GetMessage(const char *recognized_name, char *message, size_t message_size)
{
    if ((message == NULL) || (message_size == 0))
    {
        return;
    }

    message[0] = '\0';

    if ((recognized_name != NULL) && (strcmp(recognized_name, "Shweta") == 0))
    {
        snprintf(message, message_size, "Welcome Shweta");
    }
    else
    {
        snprintf(message, message_size, "Recognition Successful");
    }
}
