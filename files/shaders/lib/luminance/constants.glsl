#ifndef LUMINANCE_CONSTANTS
#define LUMINANCE_CONSTANTS

const float minLog = -9.0;
const float maxLog = 4.0;
const float logLumRange = (maxLog - minLog);
const float invLogLumRange = 1.0 / logLumRange;
const float epsilon = 0.004;
const float hdrExposureTime = @hdrExposureTime;

#endif
