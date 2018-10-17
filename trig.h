#ifndef TRIG_H
#define TRIG_H
#include <stdint.h>


#define THETA_MAX 4096
#define FIRST_THIRD 2731
#define SECOND_THIRD 1365

uint16_t sinShft03(uint16_t theta);
uint16_t sinShft13(uint16_t theta);
uint16_t sinShft23(uint16_t theta);

#endif
