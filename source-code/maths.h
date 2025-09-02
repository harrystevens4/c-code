#ifndef _MATH_H
#define _MATH_H

#include <stdint.h>
#include <stddef.h>

#define INFINITY (1.0/0.0)
#define NAN (0.0f/0.0f)

double m_sqrt(double val);
double m_ln(double val);
double m_logab(double base,double val);
size_t fibonacci(size_t n);

#endif
