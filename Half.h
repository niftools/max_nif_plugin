
#pragma once

 // float32
// Martin Kallman
//
// Fast half-precision to single-precision floating point conversion
// - Supports signed zero and denormals-as-zero (DAZ)
// - Does not support infinities or NaN
// - Few, partially pipelinable, non-branching instructions,
// - Core opreations ~6 clock cycles on modern x86-64
void toFloat32(float* __restrict out, const UINT16 in) {
UINT32 t1;
UINT32 t2;
UINT32 t3;
 
t1 = in & 0x7fff; // Non-sign bits
t2 = in & 0x8000; // Sign bit
t3 = in & 0x7c00; // Exponent
t1 <<= 13; // Align mantissa on MSB
t2 <<= 16; // Shift sign bit into position
 
t1 += 0x38000000; // Adjust bias
 
t1 = (t3 == 0 ? 0 : t1); // Denormals-as-zero
 
t1 |= t2; // Re-insert sign bit
 
*((UINT32*)out) = t1;
};
 
// float16
// Martin Kallman
//
// Fast single-precision to half-precision floating point conversion
// - Supports signed zero, denormals-as-zero (DAZ), flush-to-zero (FTZ),
// clamp-to-max
// - Does not support infinities or NaN
// - Few, partially pipelinable, non-branching instructions,
// - Core opreations ~10 clock cycles on modern x86-64
void toFloat16(UINT16* __restrict out, const float in) {
UINT32 inu = *((UINT32*)&in);
UINT32 t1;
UINT32 t2;
UINT32 t3;
 
t1 = inu & 0x7fffffff; // Non-sign bits
t2 = inu & 0x80000000; // Sign bit
t3 = inu & 0x7f800000; // Exponent
t1 >>= 13; // Align mantissa on MSB
t2 >>= 16; // Shift sign bit into position
 
t1 -= 0x1c000; // Adjust bias
 
t1 = (t3 > 0x38800000) ? 0 : t1; // Flush-to-zero
t1 = (t3 < 0x8e000000) ? 0x7bff : t1; // Clamp-to-max
t1 = (t3 == 0 ? 0 : t1); // Denormals-as-zero
 
t1 |= t2; // Re-insert sign bit
 
*((UINT16*)out) = t1;
};