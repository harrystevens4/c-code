#include "math.h"
#include <stdio.h>

const double golden_ratio = 1.618033988749894;
const double root_5 = 2.236067977499789696409173668731276235440;

double m_sqrt(double val){
	if (val < 0) return NAN;
	if (val == 0) return 0;
	double guess = val/2;
	//====== heron's method ======
	for (int i = 0; i < 9; i++){
		guess = 0.5 * (guess + val/guess);
	}
	return guess;
}
double m_ln(double val){
	//===== cant log a negative ======
	if (val < 0) return -INFINITY;
	double dx = 0.00001*val;
	double total = 0;
	if (val >= 1){
	//====== integral of 1/x between 1 and val ======
		for (double i = 1; i <= val; i += dx){
			total += (1.0/i) * dx;
		}
	}
	if (val < 1){
	//====== integral of 1/x between val and 1 ======
		for (double i = val; i <= 1; i += dx){
			total -= (1.0/i) * dx;
		}
	}
	return total;
}
double m_logab(double base,double val){
	//====== change of base formula ======
	return m_ln(val)/m_ln(base);
}
size_t fibonacci(size_t n){
	if (n < 1000){
		double numerator = pow(golden_ratio,n) - pow(1-golden_ratio,n);
		return nearbyint(numerator/root_5);
	}else{
		return 1;
	}
}
