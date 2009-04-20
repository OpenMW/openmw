/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (math.d) is part of the Monster script language
  package.

  Monster is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */


// Simple math functions
module monster.modules.math;

import monster.monster;
import std.math;
import monster.vm.mclass;

const char[] moduleDef =
"module math;

double E = 2.7182818284590452354;
double PI = 3.141592653589793238;
double DEGTORAD = 3.141592653590/180;
double RADTODEG = 180/3.141592653590;

native double sin(double x);
native double cos(double x);
native double tan(double x);
native double asin(double x);
native double acos(double x);
native double atan(double x);
native double atan2(double y, double x); // = atan(y/x)
native double sinh(double x);
native double cosh(double x);
native double tanh(double x);
native double asinh(double x);
native double acosh(double x);
native double atanh(double x);

native double sqrt(double x);
native double exp(double x);  // e^x
native double exp2(double x); // 2^x
native double log(double x);  // base e
native double log10(double x);// base 10
native double log2(double x); // base 2

native double pow(double x, double y); // x^y
native double ipow(double x, int n);   // x^n (faster than pow)

native int abs(int x);
native double fabs(double x);
native double ceil(double x);
native double floor(double x);
native double round(double x); // rounds to nearest integer
native double trunc(double x);

native double hypot(double x, double y); // = sqrt(x*x+y*y)
native double cbrt(double x); // cube root

// Calculates polynomial a0 + x*a1 + x^2*a2 + x^3*a3 + ...
native double poly(double x, float[] A);
"; //"

version(Tango)
{
  double fabs(double x) { return abs(x); }
}

void initMathModule()
{
  static MonsterClass mc;
  if(mc !is null) return;

  mc = vm.loadString(moduleDef, "math");

  mc.bind("sin", { stack.pushDouble(sin(stack.popDouble)); });
  mc.bind("cos", { stack.pushDouble(cos(stack.popDouble)); });
  mc.bind("tan", { stack.pushDouble(tan(stack.popDouble)); });
  mc.bind("asin", { stack.pushDouble(asin(stack.popDouble)); });
  mc.bind("acos", { stack.pushDouble(acos(stack.popDouble)); });
  mc.bind("atan", { stack.pushDouble(atan(stack.popDouble)); });

  mc.bind("sinh", { stack.pushDouble(sinh(stack.popDouble)); });
  mc.bind("cosh", { stack.pushDouble(cosh(stack.popDouble)); });
  mc.bind("tanh", { stack.pushDouble(tanh(stack.popDouble)); });
  mc.bind("asinh", { stack.pushDouble(asinh(stack.popDouble)); });
  mc.bind("acosh", { stack.pushDouble(acosh(stack.popDouble)); });
  mc.bind("atanh", { stack.pushDouble(atanh(stack.popDouble)); });

  mc.bind("atan2",
  {
    // Remember to pop the variables in the reverse order
    auto x = stack.popDouble;
    stack.pushDouble(atan2(stack.popDouble, x));
  });

  mc.bind("sqrt", { stack.pushDouble(sqrt(stack.popDouble)); });
  mc.bind("exp", { stack.pushDouble(exp(stack.popDouble)); });
  mc.bind("exp2", { stack.pushDouble(exp2(stack.popDouble)); });
  mc.bind("log", { stack.pushDouble(log(stack.popDouble)); });
  mc.bind("log2", { stack.pushDouble(log2(stack.popDouble)); });
  mc.bind("log10", { stack.pushDouble(log10(stack.popDouble)); });

  mc.bind("pow",
  {
    auto x = stack.popDouble;
    stack.pushDouble(pow(stack.popDouble, x));
  });
  mc.bind("ipow",
  {
    auto x = stack.popInt;
    stack.pushDouble(pow(cast(real)stack.popDouble, x));
  });

  mc.bind("abs", { stack.pushDouble(abs(stack.popDouble)); });
  mc.bind("fabs", { stack.pushDouble(fabs(stack.popDouble)); });
  mc.bind("ceil", { stack.pushDouble(ceil(stack.popDouble)); });
  mc.bind("floor", { stack.pushDouble(floor(stack.popDouble)); });
  mc.bind("round", { stack.pushDouble(round(stack.popDouble)); });
  mc.bind("trunc", { stack.pushDouble(trunc(stack.popDouble)); });

  // Order doesn't matter here
  mc.bind("hypot", { stack.pushDouble(hypot(stack.popDouble,
                                            stack.popDouble)); });

  mc.bind("cbrt", { stack.pushDouble(cbrt(stack.popDouble)); });

  mc.bind("poly", &npoly);
}

// Implement this ourselves, since phobos doesn't use the types we
// want
double poly(double x, float A[])
{
  // Use 'real' internally for higher precision
  real r = A[$-1];
  foreach_reverse(c; A[0..$-1])
    {
      r *= x;
      r += c;
    }
  return r;
}

// double poly(double x, float[] A);
void npoly()
{
  auto arf = stack.popArray();
  assert(arf.elemSize == 1);
  stack.pushDouble(poly(stack.popDouble, arf.farr));
}
