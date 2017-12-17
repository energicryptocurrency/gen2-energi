#include "energi.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>


#include "sha3/keccak-tiny.h"

void energi_hash(const char* input, int len, char* output)
{
    sha3_256(output, 32, (uint8_t const *)input, len);
}

