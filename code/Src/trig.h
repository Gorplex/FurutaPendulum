#ifndef TRIG_H
#define TRIG_H
#include <stdint.h>


#define THETA_MAX 4096
#define FIRST_THIRD 2731
#define SECOND_THIRD 1365

uint16_t sinShift03(uint16_t theta);
uint16_t sinShift13(uint16_t theta);
uint16_t sinShift23(uint16_t theta);

#endif
