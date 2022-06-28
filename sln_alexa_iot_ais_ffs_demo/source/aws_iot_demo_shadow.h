/*
 * Copyright 2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef AWS_IOT_DEMO_SHADOW_H_
#define AWS_IOT_DEMO_SHADOW_H_

typedef void (*shadowUpdateCb)(void *);

/**
 * @brief Send the document as a SHADOW update.
 *
 * @param[in] document Pointer to a (IotShadowUpadte_t *) structure which contains the document.
 *
 * @return `EXIT_SUCCESS` on success, `EXIT_FAILURE` otherwise.
 */
int SendUpdateShadowDemo(void *document);

/**
 * @brief Configure the thing name and the callback for the Shadow.
 *
 * @param[in] pIdentifier       Shadow Thing Name (NULL terminated).
 * @param[in] callbackParamFunc Pointer to the function to be called from the callbacks.
 *
 * @return `EXIT_SUCCESS` on success, `EXIT_FAILURE` otherwise.
 */
int ConfigShadowDemo(const char *pIdentifier, shadowUpdateCb callbackParamCb);

/**
 * @brief Initialize the Shadow stack and register the Delta and Update callbacks.
 *
 * @param[in] deleteShadow Delete the existing shadow document (in case SHADOW_BOOT_RESET is defined)
 *
 * @return `EXIT_SUCCESS` on success, `EXIT_FAILURE` otherwise.
 */
int RunShadowDemo(bool deleteShadow);

#endif /* AWS_IOT_DEMO_SHADOW_H_ */
