/*
 * Welcome Shweta demo interface.
 */

#ifndef WELCOME_SHWETA_H
#define WELCOME_SHWETA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void WelcomeShweta_GetMessage(const char *recognized_name, char *message, size_t message_size);

#ifdef __cplusplus
}
#endif

#endif /* WELCOME_SHWETA_H */
