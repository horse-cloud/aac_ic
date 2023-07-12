/* Shared definitions for parameter communication.

This header file is shared between the sensor chip MCU and its host MCU. Note that everytime this header changes, the
developer has to copy this header to the both codebases.

IS_TPZ macro should be defined when this header is included in a chip-MCU source file.

We implicitly assume the memory maps are aligned on two devices. We did not ensure the byte alignment by using
`#pragma pack(push, 1)` and `#pragma pack(pop)`.

** UltraSense System Confidential **
*/
#ifndef SHARED_ALGO_1ST_PARAM_H_
#define SHARED_ALGO_1ST_PARAM_H_

#ifndef IS_TPZ
#include <stdint.h>
#endif


/* An enum that defines the supported algorithm mode for determining the press. */
enum {
  ALGO_undef = -1,
  ALGO_USP_DEPTH,
  ALGO_USP_HPF,
  ALGO_Z_USP_DEPTH,
  ALGO_Z_USP_HPF_DEPTH,
  ALGO_Z,
  ALGO_CAP,
  ALGO_CAP_Z,
};


// TODO: remove the tailing comments for the old name of each data member.
/* A struct of parameters for the USP sensing algorithm. */
struct usp_params {
  uint8_t LPF32Weight;  // USPLPF32WEIGHT
  uint8_t OffsetLPF32WEIGHT;  // USPOffsetLPF32WEIGHT
  uint8_t DcmtCntTHS;  // USPWINDecm
  uint8_t MinDepthTHSOF256;  // USPMINDepthTHSOF256
  uint8_t MaxContrast;  // max contrast of this sensor. USPTYPContrast
  uint8_t NoiseRMS;  // noise RMS collected. USPTYPNoiseRMS
  uint8_t DepthTHSJitterOver4;  // USPDeptchTHSJitterOver4
  /* per-sampling interval number of reads for a first-order
   * low-pass-filtering step.
   */
  uint8_t averaging;
};


// TODO: remove the tailing comments for the old name of each data member.
/* A struct of parameters for the CASP sensing algorithm. */
struct casp_params {
  uint8_t SumLPF32WEIGTH;  // ZForceLPF32WEIGTH
  uint8_t SumOffsetTrack256WEIGTH;  // ZForceOFFSETTRACKLPF256WEIGHT
  /* threshold for the leading edge to accept a press.  this is
   * indexed against the current polarity so that we can have
   * different settings for each polarity.
   */
  uint8_t accept_threshold[2];
  /* xxx / yyy factor to scale the 'rising' area for the release. */
  uint8_t ReleaseHoldScale;
  uint8_t SumPlusPeakValueAtStdForcePtn;
  uint8_t ReleasePressScale;
  uint8_t SumNoiseRMS;  // noise RMS for ZForce.  ZForceSumNoiseRMS
  uint8_t SumMinuxPeakExpiredCNTTime;  // ZForceSumMinuxPeakExpiredCNTTime
  uint8_t HoldThreshold;
  uint8_t HoldTimeout;

  /* 0 - positive - rising peak first
   * 1 - negative - falling peak first
   */
  uint8_t ReversedPolarity;

  /* NB: we don't touch 4b because changes there need a separate delay
   * as they flow through the analog amplifier bits.
   */
  struct {
    uint8_t fe_output;
    uint8_t sinc_sum;
  } primary, secondary;
};


/* A struct of parameters for the CAP sensor algorithm. */
struct cap_params {
  /* offset stuff
   * - peak-to-peak during calibration
   */
  uint8_t offset_delta;
  /* baseline tracking decimation 
   * - ascend
   * - descend
   */
  uint8_t offset_decim_ascend, offset_decim_descend;
  /* input filtering */
  uint8_t input_lpf;
  /* thresholds for press/release. */
  uint8_t threshold_press, threshold_release;
  /* time (in xxx multiples of the 20mhz clock)
   * used before reading the raw adc.
   */
  uint8_t leak_count;
  /* amount of delay between each samples
   * this is to mimic frequency hopping
   */
  uint8_t delay_between_samples_us;
};


/* A struct of parameters for the Temperature sensing algorithm. */
struct temp_params {
  /* threshold to saturate to zero on the minimum (adc counts). */
#ifdef IS_TPZ
  uint16_t min;
#else
  uint8_t min[2];
#endif
  uint8_t div;
};


/* A struct of parameters for general configuration. */
struct global_params {
  uint8_t EnabledHostReset;
  uint8_t AlgoSensitivityLevel;  // 1~10 DEFSensLevel10
  /* one of ALGO_xxx */
  uint8_t AlgoMode;

  /* 'ideal' sampling based on the ODR.
   * units ticks (at 32khz) for apc sleep.
   */
#ifdef IS_TPZ
  uint16_t sampling_period;
#else
  uint8_t sampling_period[2];
#endif
};


/* A struct of all parameters for the algorithm and setting.

IMPORTANT NOTE for the chip-MCU codebase: Everytime this struct or any of its components changes, the developer has to
update the constant THS_INIT in Algo1stOdr.c in chip-MCU codebase.
*/
typedef struct ZForceAlgo1stGlobalTHS {
  union {
    struct usp_params s;
    uint8_t space[16];
  } usp;

  union {
    struct casp_params s;
    uint8_t space[16];
  } casp;

  union {
    struct cap_params s;
    uint8_t space[8];
  } cap;

  union {
    struct temp_params s;
    uint8_t space[4];
  } temp;

  struct global_params global;
} ussys_algo1st_ths_t;

#endif  // SHARED_ALGO_1ST_PARAM_H_
