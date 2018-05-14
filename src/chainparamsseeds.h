#ifndef ENERGI_CHAINPARAMSSEEDS_H
#define ENERGI_CHAINPARAMSSEEDS_H
/**
 * List of fixed seed nodes for the energi network
 * AUTOGENERATED by contrib/seeds/generate-seeds.py
 *
 * Each line contains a 16-byte IPv6 address and a port.
 * IPv4 as well as onion addresses are wrapped inside a IPv6 address accordingly.
 */
static SeedSpec6 pnSeed6_main[] = {
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x0D,0xD1,0x08,0x96}, 9797}, //13.209.8.150
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x23,0xB2,0x36,0xAF}, 9797}, //35.178.54.175
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x23,0xB2,0x8B,0x02}, 9797}, //35.178.139.2
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x12,0xDA,0x1C,0xE2}, 9797}, //18.218.28.226
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x0D,0xD1,0x0F,0xDE}, 9797}, //13.209.15.222
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x23,0xB1,0xFA,0x41}, 9797}, //35.177.250.65
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x12,0xBC,0x3A,0xD9}, 9797}, //18.188.58.217
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x34,0x38,0xe4,0x83}, 9797}, //52.56.228.131
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x12,0xD8,0x5F,0x4F}, 9797}, //18.216.95.74
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x0D,0x7D,0xCE,0x38}, 9797}, //13.125.206.56
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x22,0xD8,0x3F,0x87}, 9797}, //34.216.63.135
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x12,0xBC,0x42,0xa7}, 9797}, //18.188.66.167
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x0D,0x7D,0xD0,0xE4}, 9797}, //13.125.208.228
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x34,0x38,0xCE,0xEE}, 9797}, //52.56.206.238
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x0D,0xD1,0x0F,0xCF}, 9797}  //13.209.15.207
};

static SeedSpec6 pnSeed6_test[] = {

    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x36,0xCA,0x59,0x6E}, 19797}, // 54.202.89.110
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x22,0xD1,0x30,0xFA}, 19797}, // 34.209.48.250
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x36,0xBB,0x4B,0x5A}, 19797}, // 54.187.75.90
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x36,0xC9,0x37,0xC3}, 19797}, // 54.201.55.195
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x36,0xF5,0xB1,0x6C}, 19797}  // 54.245.177.108
};

#ifdef ENERGI_ENABLE_TESTNET_60X
static SeedSpec6 pnSeed6_test60x[] = {

    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x36,0xf5,0x9e,0x14}, 29797} // 54.245.158.20
};
#endif

#endif // ENERGI_CHAINPARAMSSEEDS_H
