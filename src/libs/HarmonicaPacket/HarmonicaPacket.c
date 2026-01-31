#include <stdint.h>
#include <stdlib.h>
#include "HarmonicaPacket.h"

#define TIMING_MASK 0xFFFFF
#define TIMING_OFFSET 0
#define SHUTTERS_MASK 0x3FF
#define SHUTTERS_OFFSET 21
#define DIRECTION_MASK 0x1
#define DIRECTION_OFFSET 31

uint32_t harmonica_packet_encoder(const harmonica_packet_t* packet){
    uint32_t returnPacket = 0;
    returnPacket = returnPacket | (packet->time & TIMING_MASK);
    returnPacket = returnPacket | (packet->shutter & SHUTTERS_MASK) << SHUTTERS_OFFSET;
    returnPacket = returnPacket | (packet->direction & DIRECTION_MASK) << DIRECTION_OFFSET;
    return returnPacket;
}

harmonica_packet_t* harmonica_packet_decode_buffer(size_t size, const uint32_t *array){
    harmonica_packet_t *instruction_buffer = malloc(size * sizeof(harmonica_packet_t));
    if(instruction_buffer == NULL){
        return NULL;
    }
    for(size_t i = 0; i < size; i++){
        harmonica_packet_decoder(array[i], &instruction_buffer[i]);
    }
    return instruction_buffer;
}

void harmonica_packet_decoder(const uint32_t packet, harmonica_packet_t *instruction){
    uint32_t timing = packet & TIMING_MASK;
    instruction->time = timing;
    uint16_t shutters = (packet >> SHUTTERS_OFFSET) & SHUTTERS_MASK;  
    instruction->shutter = shutters;
    unsigned char direction = (packet >> DIRECTION_OFFSET) & DIRECTION_MASK; 
    instruction->direction = direction;
}


