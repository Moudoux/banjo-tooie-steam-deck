#ifndef TOOIE_CKPACK_ROWS_H
#define TOOIE_CKPACK_ROWS_H
/* The one finite ckpack CPU-deform row predicate (FA12), shared by the CPU625 owner (main draw3
 * and the N3 shadow attachment) and the shadow prepare gate. The actual resource AND state AND
 * the common 0.1f scale word select a row together; a retained binding (for example resource
 * 0625 in state 4) selects none. Row 0 is 0625; rows 1..3 are 060A/061B/0831. Include after
 * silo_bridge.h (SI, u32). */
SI u32 ckpack_row(u32 part) {
    u32 resource=*(volatile u32*)part,state=*(volatile u32*)(part+12u);
    if(*(volatile u32*)(part+16u)!=0x3DCCCCCDu)return 4u;
    if(resource==0x625u && state==3u)return 0u;
    if(resource==0x60Au && state==4u)return 1u;
    if(resource==0x61Bu && state==5u)return 2u;
    if(resource==0x831u && state==6u)return 3u;
    return 4u;
}
#endif
