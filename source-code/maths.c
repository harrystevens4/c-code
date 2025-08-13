#include "math.h"
#include <stdio.h>

double m_sqrt(double val){
	double guess = val/2;
	//====== heron's method ======
	for (int i = 0; i < 10; i++){
		guess = 0.5 * (guess + val/guess);
	}
	return guess;
}
double m_ln(double val){
	//====== integral of 1/x between 1 and val ======
	double dx = 0.00001*val;
	double total = 0;
	for (double i = 1; i <= val; i += dx){
		total += (1.0/i) * dx;
	}
	return total;
}
double m_logab(double base,double val){
	//====== change of base formula ======
	return m_ln(val)/m_ln(base);
}
