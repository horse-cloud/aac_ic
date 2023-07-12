/**
* @file ussys_tp_cap_calibration.h
*
* UltraSense Systems Touch Point Driver Header File
*
* Copyright (c) 2021 UltraSense Systems, Inc. All Rights Reserved.
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USSYS_TP_CAP_CALIBRATION_H
#define __USSYS_TP_CAP_CALIBRATION_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "ussys_tp_driver.h"

#if USSYS_CAP_ENABLED

/* Exported functions prototypes ---------------------------------------------*/
int ussys_tp_cap_auto_calibration(ussys_tp_dev_t *dev);

#ifdef __cplusplus
}
#endif

#endif

#endif
