#ifndef LIB_WATER_RIPPLES
#define LIB_WATER_RIPPLES

const float RAIN_RIPPLE_GAPS = 10.0;
const float RAIN_RIPPLE_RADIUS = 0.2;

float scramble(float x, float z)
{
    return fract(pow(fract(x)*3.0+1.0, z));
}

vec2 randOffset(vec2 c, float time)
{
  time = fract(time/1000.0);
  c = vec2(c.x * c.y /  8.0 + c.y * 0.3 + c.x * 0.2,
           c.x * c.y / 14.0 + c.y * 0.5 + c.x * 0.7);
  c.x *= scramble(scramble(time + c.x/1000.0, 4.0), 3.0) + 1.0;
  c.y *= scramble(scramble(time + c.y/1000.0, 3.5), 3.0) + 1.0;
  return fract(c);
}

float randPhase(vec2 c)
{
  return fract((c.x * c.y) /  (c.x + c.y + 0.1));
}

float blip(float x)
{
  x = max(0.0, 1.0-x*x);
  return x*x*x;
}

float blipDerivative(float x)
{
  x = clamp(x, -1.0, 1.0);
  float n = x*x-1.0;
  return -6.0*x*n*n;
}

const float RAIN_RING_TIME_OFFSET = 1.0/6.0;

vec4 circle(vec2 coords, vec2 corner, float adjusted_time)
{
  vec2 center = vec2(0.5,0.5) + (0.5 - RAIN_RIPPLE_RADIUS) * (2.0 * randOffset(corner, floor(adjusted_time)) - 1.0);
  float phase = fract(adjusted_time);
  vec2 toCenter = coords - center;

  float r = RAIN_RIPPLE_RADIUS;
  float d = length(toCenter);
  float ringfollower = (phase-d/r)/RAIN_RING_TIME_OFFSET-1.0; // -1.0 ~ +1.0 cover the breadth of the ripple's ring

#if @rainRippleDetail > 0
// normal mapped ripples
  if(ringfollower < -1.0 || ringfollower > 1.0)
    return vec4(0.0);

  if(d > 1.0) // normalize center direction vector, but not for near-center ripples
    toCenter /= d;

  float height = blip(ringfollower*2.0+0.5); // brighten up outer edge of ring; for fake specularity
  float range_limit = blip(min(0.0, ringfollower));
  float energy = 1.0-phase;

  vec2 normal2d = -toCenter*blipDerivative(ringfollower)*5.0;
  vec3 normal = vec3(normal2d, 0.5);
  vec4 ret = vec4(normal, height);
  ret.xyw *= energy*energy;
  // do energy adjustment here rather than later, so that we can use the w component for fake specularity
  ret.xyz = normalize(ret.xyz) * energy*range_limit;
  ret.z *= range_limit;
  return ret;
#else
// ring-only ripples
  if(ringfollower < -1.0 || ringfollower > 0.5)
    return vec4(0.0);

  float energy = 1.0-phase;
  float height = blip(ringfollower*2.0+0.5)*energy*energy; // fake specularity

  return vec4(0.0, 0.0, 0.0, height);
#endif
}
vec4 rain(vec2 uv, float time)
{
  uv *= RAIN_RIPPLE_GAPS;
  vec2 f_part = fract(uv);
  vec2 i_part = floor(uv);
  float adjusted_time = time * 1.2 + randPhase(i_part);
#if @rainRippleDetail > 0
  vec4 a = circle(f_part, i_part, adjusted_time);
  vec4 b = circle(f_part, i_part, adjusted_time - RAIN_RING_TIME_OFFSET);
  vec4 c = circle(f_part, i_part, adjusted_time - RAIN_RING_TIME_OFFSET*2.0);
  vec4 d = circle(f_part, i_part, adjusted_time - RAIN_RING_TIME_OFFSET*3.0);
  vec4 ret;
  ret.xy = a.xy - b.xy/2.0 + c.xy/4.0 - d.xy/8.0;
  // z should always point up
  ret.z  = a.z  + b.z /2.0 + c.z /4.0 + d.z /8.0;
  //ret.xyz *= 1.5;
  // fake specularity looks weird if we use every single ring, also if the inner rings are too bright 
  ret.w  = (a.w + c.w /8.0)*1.5;
  return ret;
#else
  return circle(f_part, i_part, adjusted_time) * 1.5;
#endif
}

vec2 complex_mult(vec2 a, vec2 b)
{
    return vec2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);
}
vec4 rainCombined(vec2 uv, float time) // returns ripple normal in xyz and fake specularity in w
{
  return
    rain(uv, time)
  + rain(complex_mult(uv, vec2(0.4, 0.7)) + vec2(1.2, 3.0),time)
#if @rainRippleDetail == 2
      + rain(uv * 0.75 + vec2( 3.7,18.9),time)
      + rain(uv * 0.9  + vec2( 5.7,30.1),time)
      + rain(uv * 1.0  + vec2(10.5 ,5.7),time)
#endif
  ;
}

#endif
