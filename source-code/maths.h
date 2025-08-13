#ifndef _MATH_H
#define _MATH_H

#include <stdint.h>

const uint64_t INF_BYTES = 0x7f800000;
#define INFINITY (*((float *)&INF_BYTES))
#define NAN (0.0f/0.0f)

double m_sqrt(double val);
double m_ln(double val);
double m_logab(double base,double val);

#endif
