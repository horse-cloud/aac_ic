/* Shared definitions for supported sensors and data ordering.

This header file is shared between the sensor chip MCU and its host MCU. Note that everytime this header changes, the
developer has to copy this header to the both codebases.

IS_TPZ macro should be defined when this header is included in a chip-MCU source file.

** UltraSense System Confidential **
*/

#ifndef SHARED_BRAHMS_SHARED_H_
#define SHARED_BRAHMS_SHARED_H_


/* An enum that defines bit masks to indicate supported sensors in current FW build of TPZ MCU.

The resulting bit masks are of the form MASK_SENSOR_xxx where xxx is replaced by the sensor names. For searching
reference purposes, they are MASK_SENSOR_CASP, MASK_SENSOR_USP, MASK_SENSOR_CAP, and MASK_SENSOR_TEMP.

The bit mask, when masking with a communication front-door GPR, tells if the sensor (i.e., CASP sensor, USP sensor,
etc.) is supported by the TPZ FW.

A combined mask indicating the supported sensors is stored in SWREG_SupportedSensors.
*/
#define x(l) \
  SENSOR_ ## l, \
  MASK_SENSOR_ ## l = (1 << SENSOR_ ## l), \
  t_SENSOR_ ## l = SENSOR_ ## l
enum {
  x(CASP),
  x(USP),
  x(CAP),
  x(TEMP),
  SENSOR_COUNT,
};
#undef x  // undefine the macro used in defining the enum above.


/* An enum that defines bit masks for CASP operation modes.

The resulting bit masks are of the form MASK_CASP_xxx where xxx is replaced by the CASP mode names. For searching
reference purposes, they are MASK_CASP_COMPARATOR_MODE, MASK_CASP_SECONDARY_CHANNELS, and MASK_CASP_DYNAMIC_POLARITY.

A combined mask indicating the current CASP operation configuration is stored in SWREG_SupportedCasp.
*/
#define x(l) \
  s_casp_ ## l, \
  MASK_CASP_ ## l = (1 << s_casp_ ## l), \
  t_casp_ ## l = s_casp_ ## l
enum {
  /* enable using the 'split' aspect of casp to sense and compare.
   * this is a power savings of xxx per read.
   */
  x(COMPARATOR_MODE),

  /* enable channel read parameters.
   * if the secondary channel is configured then
   * the primary channel must be configured.
   * the data is packed as zsum and the configured channels.
   * note: changes to register 4b require a large (>x00ms)
   *       settling time. this means back-to-back accesses
   *       may not be correlated w/ the same 'press' state.
   *       this means that it is not configurable.
   */
  x(SECONDARY_CHANNELS),

  /* enabled  - either polarity can be the leading edge of a press.
   * disabled - upstream can set the desired leading edge polarity.
   *            the state machine tracks the transaction so that
   *            the trailing edge doesn't trigger the 'correct' polarity.
   */
  x(DYNAMIC_POLARITY),
};
#undef x  // undefine the macro used in defining the enum above.


/* An enum that defines bit masks for sensor control communication.

The resulting bit masks are of the form SENSOR_CONTROL_xxx where xxx is replaced by the sensor operation mode names.
For searching reference purposes, they are SENSOR_CONTROL_HOST_UPDATE, SENSOR_CONTROL_HOLD_MODE,
SENSOR_CONTROL_ENDLESS_MODE, SENSOR_CONTROL_DEVELOPMENT_MODE, SENSOR_CONTROL_Z_TRIMMING,
SENSOR_CONTROL_POWER_DOWN_ENABLE, and SENSOR_CONTROL_Z_COMPARATOR.

This communication protocol is through GPR 0x57 for the host to tell TPZ which mode to go into.
*/
enum {
#define x(__t) \
  b_ ## __t, \
  SENSOR_CONTROL_ ## __t = (1 << (b_ ## __t)), \
  n_ ## __t = b_ ## __t
  /* signal host update to sensor parameters through SWREG_THS */
  x(HOST_UPDATE),
  /* signal sensor to stop processing and wait for release */
  x(HOLD_MODE),
  /* signal sensor to enter calibration mode for a sensor */
  x(ENDLESS_MODE),
  /* signal sensor to enter development mode */
  x(DEVELOPMENT_MODE),
  /* casp trimming mode */
  x(Z_TRIMMING),
  /* host wants power-down signalling (if the sensor suports it) */
  x(POWER_DOWN_ENABLE),
  /* signal sensor to enable z comparator (if the sensor supports it */
  x(Z_COMPARATOR),
#undef x
};


/* A struct that defines the content of the first one of the reporting bytes.*/
typedef struct FDRep{
  unsigned char overRunCNT:3;
  unsigned char watchDogCNT:3;
  unsigned char AlgoState:2;
} FDRepTyp;


#endif  // SHARED_BRAHMS_SHARED_H_
