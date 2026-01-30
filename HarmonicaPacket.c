#include <stdint.h>
#include <stdio.h>

#define TIMING_MASK 0xFFFFF
#define TIMING_OFFSET 0
#define SHUTTERS_MASK 0x3FF
#define SHUTTERS_OFFSET 21
#define DIRECTION_MASK 0x1
#define DIRECTION_OFFSET 31

typedef struct {
    uint32_t time;
    uint16_t shutter;
    unsigned char direction;
} harmonica_packet_t;

uint32_t harmonica_packet_encoder(harmonica_packet_t* packet){
    uint32_t returnPacket = 0;
    returnPacket = returnPacket | (packet->time & TIMING_MASK);
    returnPacket = returnPacket | (packet->shutter & SHUTTERS_MASK) << SHUTTERS_OFFSET;
    returnPacket = returnPacket | (packet->direction & DIRECTION_MASK) << DIRECTION_OFFSET;
    return returnPacket;

}


harmonica_packet_t* harmonica_packet_decoder(uint32_t packet){
    harmonica_packet_t* instruction = (harmonica_packet_t*) malloc(sizeof(harmonica_packet_t));
    uint32_t timing = packet & TIMING_MASK;
    instruction->time = timing;
    uint16_t shutters = (packet >> SHUTTERS_OFFSET) & SHUTTERS_MASK;  
    instruction->shutter = shutters;
    unsigned char direction = (packet >> DIRECTION_OFFSET) & DIRECTION_MASK; 
    instruction->direction = direction;
    return instruction;
}


int main(int argc, char **argv){
printf("hello the size is %d", sizeof(harmonica_packet_t));
uint32_t samplePacket = 0;
uint32_t time = 249;
samplePacket = samplePacket | time;
uint16_t shutters = 0b1101101011;
samplePacket = samplePacket | (shutters << 21);
unsigned char direction = 1;
samplePacket = samplePacket | (direction << 31);
    harmonica_packet_t* packetPointer = harmonica_packet_construct(samplePacket);
    printf("For %d millis we will do these shutters %d in this direction %d",packetPointer->time,packetPointer->shutter,packetPointer->direction);
}