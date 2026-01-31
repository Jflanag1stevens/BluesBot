#ifndef HARMONICA_PACKET_H
#define HARMONICA_PACKET_H

#include <stdint.h>

typedef struct{
    uint32_t time;
    uint16_t shutter;
    uint8_t direction;
} harmonica_packet_t;

/* Returns a uint32 encoded with packet information */
uint32_t harmonica_packet_encoder(const harmonica_packet_t* packet);
/*  Allocates and fills a buffer of harmonica instruction packets after decoding them 
    Caller is responsible for returning memory */
harmonica_packet_t* harmonica_packet_decode_buffer(size_t size, const uint32_t *array);
/*  Decodes a single uint32 into a harmonica instruction packet */
void harmonica_packet_decoder(const uint32_t packet, harmonica_packet_t *instruction);

#endif