
#ifndef OSA_TYPES_H
#define OSA_TYPES_H


/*
 * $copyright$
 *
 * $license$
 *
 */

/*!
 * @file    osa_types.h
 * @brief   Contains base data types for Freescale libosa.
 */

/* Include POSIX type definitions to be used. */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*! Define a version for the Freescale OSA API. */
#define OSA_VERSION 2.0

#ifndef FALSE
    #define FALSE 0
#endif

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef bool
    #define bool boolean
#endif

#endif

