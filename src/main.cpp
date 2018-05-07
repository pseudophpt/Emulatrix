#include <iostream>
#include <fstream>
#include <vector>

// Solution:
// Use the fact that it only checks if the opcode is out of data, not the operands

uint8_t global_opcode_offset = 0;

uint8_t data;
uint8_t addr1;
uint8_t addr2;
uint8_t pc;

int cycles = 0;

bool jr_done = false;

std::ifstream file;
std::vector<uint8_t> stack;

uint8_t code_loc; // 0 = 0x0, 1 = 0x80

#define BUFFER_SIZE 256
#define SECTION_SIZE (BUFFER_SIZE / 2)

uint8_t prog_buffer [256 * 256];

bool check_inst (int loc);
bool check_data (int loc);

void verify ();
void decode ();

int main (int argc, char *argv[]) {
    stack = std::vector<uint8_t>();

    // Open file
    file = std::ifstream("prog", std::ifstream::ate | std::ifstream::binary);

    // Check filesize
    if (file.tellg() != BUFFER_SIZE)
        exit (0);

    // Seek beginning
    file.seekg(file.beg);

    // Read into buffer
    file.read((char *)prog_buffer, BUFFER_SIZE);

    // Checksum
    uint8_t csum = 0;

    for (int i = 0; i < BUFFER_SIZE; i ++) {
        if (i % 2 == 0)
            csum += prog_buffer[i];
        if (i % 2 == 1)
            csum -= prog_buffer[i];
    }

    // If non zero, exit
    if (csum != 0)
        exit(0);

    // Seed RNG
    srand(time(NULL));

    code_loc = rand() % 2;

    if (code_loc == 1) {
        uint8_t intermediate [128];
        for (int i = 0; i < sizeof(intermediate); i ++) {
            // swap
            intermediate[i] = prog_buffer[i];
            prog_buffer[i] = prog_buffer[i + 128];
            prog_buffer[i + 128] = intermediate[i];
        }
    }

    pc = code_loc * SECTION_SIZE;

    addr1 = rand();
    addr2 = rand();
    data = rand();

    while(1) {
        decode();
    }
}

void decode () { // jump back with offset either 0xd4 or 0x7a
    // outside of data section = crash
    if (!check_inst(pc))
        exit(0);

    // opcode offset
    uint8_t opcode = prog_buffer[pc] - global_opcode_offset;

    // add offset
    global_opcode_offset += prog_buffer[pc];

    // do actio
    switch (opcode) {
        // nop
        case 0x33:
        case 0x22:
            pc ++;
            break;
        case 0x73: // shf
            if (addr2 >> 7)
                data ++;
            pc ++;
            break;
        case 0x86: // inca2
            addr2 ++;
            pc ++;
            break;
        case 0x2a: // deca2
            addr2 --;
            pc ++;
            break;
        case 0xd4: // inch1
            addr1 += 0x80;
            pc ++;
            break;
        case 0xae: // inch2
            addr2 += 0x80;
            pc ++;
            break;
        case 0x02: // zrd
            data = 0;
            pc ++;
            break;
        case 0x9b: // tra1a2
            addr2 = addr1;
            pc ++;
            break;
        case 0x37: // sw1
            {
                uint8_t intermediate = addr1;
                addr1 = data;
                data = intermediate;
            }
            pc ++;
            break;
        case 0x57: // sw2
            {
                uint8_t intermediate = addr2;
                addr2 = data;
                data = intermediate;
            }
            pc ++;
            break;
        case 0x6e: // jr
            // only can be done once
            if (jr_done == true)
                exit(0);
            jr_done = true;
            pc += prog_buffer[pc + 1];
            break;
        case 0xd2: // ja1
            pc = prog_buffer[pc + 1] + addr1;
            break;
        case 0x55: // ja2
            pc = prog_buffer[pc + 1] + addr2;
            break;
        case 0x91: // prt
            if(!check_data(addr2))
                exit(0);
            stack.push_back(prog_buffer[addr2]);
            pc ++;
            break;
        case 0xff: // exit
            verify();
        default:
            exit(0);
            break;
    }

    cycles ++;
}

bool check_inst (int loc) {
    return (loc >= (code_loc * SECTION_SIZE) && loc < ((code_loc + 1) * SECTION_SIZE));
}

bool check_data (int loc) {
    int data_loc = (1 - code_loc);
    return (loc >= (data_loc * SECTION_SIZE) && loc < ((data_loc + 1) * SECTION_SIZE));
}

void verify (void) {
    if (stack.size() != 0x78)
        exit(0);

    std::vector<int> sums = std::vector<int>();

    for (int i = 0; i < 0x1E; i ++) {
        int total = 0;
        for (int j = 0; j < 8; j++) {
            int byt = stack[(i >> 1) + (j * 0x0F)];
            byt = (byt >> ((1 - (i & 0x01)) * 4)) & 0x0f;
            if (byt < 5 | byt > 12)
                exit(0);
            total += (byt * byt);
        }
        sums.push_back(total);
    }

    if (abs(sums[0] - sums[1]) < 27)
        exit(0);


    for (int i = 0; i < sums.size() - 2; i ++) {
        if ((sums[i] - sums[i + 1]) != (sums[i + 1] - sums[i + 2]))
            exit(0);
    }

    std::cout << "Cracked successfully in " << std::dec << cycles << " cycles!";
    exit (0);
}