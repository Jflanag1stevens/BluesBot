#ifndef HARMONICA_PACKET_H
#define HARMONICA_PACKET_H

#include <stdint.h>

typedef struct{
    uint32_t time;
    uint16_t shutter;
    uint8_t direction;
} harmonica_packet_t;

uint32_t harmonica_packet_encoder(const harmonica_packet_t* packet);
harmonica_packet_t* harmonica_packet_decode_buffer(size_t size, const uint32_t *array);
void harmonica_packet_decoder(const uint32_t packet, harmonica_packet_t *instruction);


#endif