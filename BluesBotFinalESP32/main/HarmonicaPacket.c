#include <stdint.h>
#include <stdlib.h>
#include "HarmonicaPacket.h"

#define TIMING_MASK 0x7FFFF
#define TIMING_OFFSET 0
#define SHUTTERS_MASK 0x3FF
#define SHUTTERS_OFFSET 21
#define DIRECTION_MASK 0x1
#define DIRECTION_OFFSET 31
#define ARTICULATION_OFFSET 20
#define ARTICULATION_MASK 0x1

uint32_t harmonica_packet_encoder(const harmonica_packet_t* packet){
    uint32_t returnPacket = 0;
    returnPacket = returnPacket | (packet->time & TIMING_MASK);
    returnPacket = returnPacket | (packet->shutter & SHUTTERS_MASK) << SHUTTERS_OFFSET;
    returnPacket = returnPacket | (packet->direction & DIRECTION_MASK) << DIRECTION_OFFSET;
    returnPacket = returnPacket | (packet->articulate & ARTICULATION_MASK) << ARTICULATION_OFFSET;
    return returnPacket;
}


void harmonica_packet_decoder(const uint32_t packet, harmonica_packet_t *instruction){
    uint32_t timing = packet & TIMING_MASK;
    instruction->time = timing;
    uint16_t shutters = (packet >> SHUTTERS_OFFSET) & SHUTTERS_MASK;  
    instruction->shutter = shutters;
    uint8_t direction = (packet >> DIRECTION_OFFSET) & DIRECTION_MASK; 
    instruction->direction = direction;
    uint8_t articulation = (packet >> ARTICULATION_OFFSET) & ARTICULATION_MASK;
    instruction->articulate = articulation;
}


