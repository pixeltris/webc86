#include <stdio.h>
#include <stdint.h>

// This file contains some very rudimentary tests for the minimum required opcodes to compile TCC with TCC.

// Try and keep the tests in opcode order (0F XX should come after 0E, not after the last single byte opcode (FF))

#define TEST_ON_WEB 1
#define TEST_FLOAT 0

#if TEST_ON_WEB
void wc86_assert(char* test, int32_t val);
void wc86_assertI32(char* test, int32_t val, int32_t expected);
void wc86_assertU32(char* test, uint32_t val, uint32_t expected);
#else
void wc86_assert(char* test, int32_t val)
{
    if (val)
    {
        printf("pass %s\n", test);
    }
    else
    {
        printf("fail %s\n", test);
    }
}

void wc86_assertI32(char* test, int32_t val, int32_t expected)
{
    wc86_assert(test, val == expected);
}

void wc86_assertU32(char* test, uint32_t val, uint32_t expected)
{
    wc86_assert(test, val == expected);
}
#endif

// Define these after main so that main is generated at 0x004010000 (so we can just point the entry point to that address)
void testRetVoid();
int8_t testRetI8();
uint8_t testRetU8();
int32_t testRetI32();
uint32_t testRetU32();
int64_t testRetI64();
uint64_t testRetU64();

int main()
{
    char* PC_1 = NULL;
    char* PC_2 = NULL;
    
    int8_t I8_1 = 0;
    int8_t I8_2 = 0;
    int8_t* PI8_1 = NULL;
    int8_t* PI8_2 = NULL;
    uint8_t U8_1 = 0;
    uint8_t U8_2 = 0;
    uint8_t* PU8_1 = NULL;
    uint8_t* PU8_2 = NULL;
    
    int16_t I16_1 = 0;
    int16_t I16_2 = 0;
    int16_t* PI16_1 = NULL;
    int16_t* PI16_2 = NULL;
    uint16_t U16_1 = 0;
    uint16_t U16_2 = 0;
    uint16_t* PU16_1 = NULL;
    uint16_t* PU16_2 = NULL;
    
    int32_t I32_1 = 0;
    int32_t I32_2 = 0;
    int32_t* PI32_1 = NULL;
    int32_t* PI32_2 = NULL;    
    uint32_t U32_1 = 0;
    uint32_t U32_2 = 0;
    uint32_t* PU32_1 = NULL;
    uint32_t* PU32_2 = NULL;
    
    int64_t I64_1 = 0;
    int64_t I64_2 = 0;
    int64_t* PI64_1 = NULL;
    int64_t* PI64_2 = NULL;
    uint64_t U64_1 = 0;
    uint64_t U64_2 = 0;
    uint64_t* PU64_1 = NULL;
    uint64_t* PU64_2 = NULL;
    
#if TEST_FLOAT
    float F32_1 = 0;
    float F32_2 = 0;
    float* PF32_1 = NULL;
    float* PF32_2 = NULL;
    
    double F64_1 = 0;
    double F64_2 = 0;
    double* PF64_1 = NULL;
    double* PF64_2 = NULL;
    
    long double LF64_1 = 0;
    long double LF64_2 = 0;
    long double* PLF64_1 = NULL;
    long double* PLF64_2 = NULL;
#endif

    /*long double a = 9999999999999999.0L;
    long double b = 9999999999999995.0L;
    float c = 845.68f;
    int64_t d = -3000;
    uint64_t f = (uint64_t)d;
    char* g = "hello world one two three";
    int isa = a - b == 4;
    wc86_assert("9999999999999999.0L-9999999999999995.0L==4", isa);
    //printf("%d\n", 99876);
    
    char buff[512];
    sprintf(buff, "%Lf %f %lld %llu %s\n", b, c, d, f, g);
    wc86_assert(buff, 1);
    printf("%s hi", buff);
    printf("%s hi", buff);
    printf("%s hi", buff);
    printf("%s hi", buff);
    printf("%s hi", buff);*/
    
    //printf("%Lf %f %lld %llu %s\n", b, c, d, f, g);
    //printf("%Lf\n", a - b);
    
    //////////////////////////////////////////////////////
    // Opcode: 00-05
    //////////////////////////////////////////////////////
    // Instructions:
    // ADD
    //////////////////////////////////////////////////////
    
    //00401317   01C8             ADD EAX,ECX
    I32_1 = -2;
    I32_2 = 5;
    I32_1 = I32_1 + I32_2;
    wc86_assertI32("-2+5==3", I32_1, 3);
    
    //////////////////////////////////////////////////////
    // Opcode: 08-0D
    //////////////////////////////////////////////////////
    // Instructions:
    // OR
    //////////////////////////////////////////////////////
    
    //00401277   09C8             OR EAX,ECX
    I32_1 = 0x40;
    I32_2 = 0x08;
    I32_1 = I32_1 | I32_2;
    wc86_assertI32("0x40|0x08==0x48", I32_1, 0x48);

    //////////////////////////////////////////////////////
    // Opcode: 0F 80 - 0F 8F
    //////////////////////////////////////////////////////
    // Instructions:
    // JO(80), JNO(81), JB(82), JNB(83), JE(84), JNZ(85),
    // JBE(86), JA(87), JS(88), JNS(89), JPE(8A), JPO(8B),
    // JL(8C), JGE(8D), JLE(8E), JG(8F)
    //////////////////////////////////////////////////////
    // Notes:
    // - TCC doesn't use JS/JNS/JP/JNP/JO/JNO/JPO?
    // - Overflow detection is broken so JO/JNO/JPO wont work
    //////////////////////////////////////////////////////
    
    wc86_assert("========= Conditional jump =========", 1);
    
    //004012AC   0F82 0D000000    JB main.004012BF
    U32_1 = 101;
    U32_2 = 100;
    if (U32_1 >= U32_2 && 100 >= U32_2) { I8_1 = 1; } else { I8_1 = 0; }
    wc86_assert("101>=100", I8_1);
    
    //004012F2   0F83 0D000000    JNB main.00401305
    U32_1 = 100;
    U32_2 = 101;
    if (U32_1 < U32_2) { I8_1 = 1; } else { I8_1 = 0; }
    wc86_assert("100<101", I8_1);
    
    //00401338   0F84 0D000000    JE main.0040134B
    U32_1 = 105;
    U32_2 = 110;
    if (U32_1 != U32_2) { I8_1 = 1; } else { I8_1 = 0; }
    wc86_assert("105!=110", I8_1);
    
    //0040137E   0F85 0D000000    JNZ main.00401391
    U32_1 = 120;
    U32_2 = 120;
    if (U32_1 == U32_2) { I8_1 = 1; } else { I8_1 = 0; }
    wc86_assert("120==120", I8_1);
    
    //004013D0   0F86 0D000000    JBE main.004013E3
    PC_1 = (char*)2;
    PC_2 = (char*)1;
    if (PC_1 > PC_2) { I8_1 = 1; } else { I8_1 = 0; }
    wc86_assert("2>1", I8_1);
    
    //00401420   0F87 0D000000    JA main.00401433
    PC_1 = (char*)5;
    PC_2 = (char*)5;
    if (PC_1 <= PC_2 && (char*)4 <= PC_2) { I8_1 = 1; } else { I8_1 = 0; }
    wc86_assert("5<=5", I8_1);
    
    // 88-8B aren't handled (see above)
    
    //00401466   0F8C 0D000000    JL main.00401479
    I32_1 = 20;
    I32_2 = 15;
    if (I32_1 >= I32_2 && 15 >= I32_2) { I8_1 = 1; } else { I8_1 = 0; }
    wc86_assert("20>=15", I8_1);
    
    //004014DC   0F8D 0D000000    JGE main.004014EF
    I32_1 = -30;
    I32_2 = 40;
    if (I32_1 < I32_2) { I8_1 = 1; } else { I8_1 = 0; }
    wc86_assert("-30<40", I8_1);
    
    //00401522   0F8E 0D000000    JLE main.00401535
    I32_1 = 40;
    I32_2 = -30;
    if (I32_1 > I32_2) { I8_1 = 1; } else { I8_1 = 0; }
    wc86_assert("40>-30", I8_1);
    
    //00401568   0F8F 1D000000    JG main.0040158B
    I32_1 = 50;
    I32_2 = 75;
    if (I32_1 <= I32_2 && 75 <= I32_2) { I8_1 = 1; } else { I8_1 = 0; }
    wc86_assert("50<=75", I8_1);
    
    //////////////////////////////////////////////////////
    // Opcode: 0F 90 - 0F 9F
    //////////////////////////////////////////////////////
    // Instructions:
    // SETNO(90), SETNO(91), SETB(92), SETNB(93), SETE(94), SETNZ(95),
    // SETBE(96), SETA(97), SETS(98), SETNS(99), SETPE(9A), SETPO(9B),
    // SETL(9C), SETGE(9D), SETLE(9E), SETG(9F)
    //////////////////////////////////////////////////////
    // Notes:
    // - Various conditionals aren't supported (see conditional jumps above)
    //////////////////////////////////////////////////////
    
    wc86_assert("========= Conditional set =========", 1);
    
    //004015EB   0F92C0           SETB AL
    U32_1 = 210;
    U32_2 = 220;
    I8_1 = U32_1 < U32_2;
    wc86_assert("210<220", I8_1);
    
    //004015EB   0F93C0           SETNB AL
    U32_1 = 350;
    U32_2 = 100;
    I8_1 = U32_1 >= U32_2;
    I8_2 = 100 >= U32_2;
    wc86_assert("350>=100", I8_1 && I8_2);
    
    //004015EB   0F94C0           SETE AL
    U32_1 = 520;
    U32_2 = 520;
    I8_1 = U32_1 == U32_2;
    I8_2 = U32_1 != 521;
    wc86_assert("520==520", I8_1 && I8_2);
    
    //004015EB   0F95C0           SETNE AL
    U32_1 = 900;
    U32_2 = 902;
    I8_1 = U32_1 != U32_2;
    I8_2 = 902 == U32_2;
    wc86_assert("900!=902", I8_1 && I8_2);
    
    //004015EB   0F96C0           SETBE AL
    U32_1 = 50;
    U32_2 = 100;
    I8_1 = U32_1 <= U32_2;
    I8_2 = 100 <= U32_2;
    wc86_assert("50<=100", I8_1 && I8_1);
    
    //00401621   0F97C0           SETA AL
    U32_1 = (uint32_t)-100;// Should be some large value
    U32_2 = 200;
    I8_1 = U32_1 > U32_2;
    I8_2 = U32_2 > 199;
    wc86_assert("(uint32_t)-100 >= 100", I8_1 && I8_2);
    
    // 98-9B aren't handled (see above)
    
    //00401840   0F9CC0           SETL AL
    I32_1 = -30;
    I32_2 = 40;
    I8_1 = I32_1 < I32_2;
    wc86_assert("-30<40", I8_1);
    
    //00401855   0F9DC0           SETGE AL
    I32_1 = 20;
    I32_2 = 15;
    I8_1 = I32_1 >= I32_2;
    I8_2 = 15 >= I32_2;
    wc86_assert("20>=15", I8_1 && I8_2);
    
    //004018AD   0F9EC0           SETLE AL
    I32_1 = 50;
    I32_2 = 75;
    I8_1 = I32_1 <= I32_2;
    I8_2 = 75 <= I32_2;
    wc86_assert("50<=75", I8_1 && I8_2);
    
    //00401950   0F9FC0           SETG AL
    I32_1 = 40;
    I32_2 = -30;
    I8_1 = I32_1 > I32_2;
    wc86_assert("40>-30", I8_1);
    
    wc86_assert("===================================", 1);
    
    //////////////////////////////////////////////////////
    // Opcode: 0F AF
    //////////////////////////////////////////////////////
    // Instructions:
    // IMUL
    //////////////////////////////////////////////////////
    // Notes:
    // - There are additional instructions under 0F AX (SHRD / SHLD) but these aren't used by TCC
    //////////////////////////////////////////////////////
    
    //00401993   0FAFC1           IMUL EAX,ECX
    I32_1 = 3;
    I32_2 = 10;
    I32_1 = I32_1 * I32_2;
    wc86_assertI32("3*10==30", I32_1, 30);
        
    // Negative
    //004019C7   0FAFC1           IMUL EAX,ECX
    I32_1 = 5;
    I32_2 = -100;
    I32_1 = I32_1 * I32_2;
    wc86_assertI32("5*-100==-500", I32_1, -500);
    
    // Large value
    //004019FB   0FAFC1           IMUL EAX,ECX
    I32_1 = 1951677952;
    I32_2 = 1219798720;
    I32_1 = I32_1 * I32_2;
    wc86_assertI32("1951677952*1219798720==2002944000", I32_1, 2002944000);
    
    // Large negative value
    //00401A2F   0FAFC1           IMUL EAX,ECX
    I32_1 = -1951677952;
    I32_2 = 2019798720;
    I32_1 = I32_1 * I32_2;
    wc86_assertI32("-1951677952*2019798720==-2069004288", I32_1, -2069004288);
    
    // Large negative value (both negative)
    //00401A63   0FAFC1           IMUL EAX,ECX
    I32_1 = -1951677952;
    I32_2 = -2019798720;
    I32_1 = I32_1 * I32_2;
    wc86_assertI32("-1951677952*-2019798720==2069004288", I32_1, 2069004288);
    
    // Unsigned
    //00401AC0   0FAFC1           IMUL EAX,ECX
    U32_1 = 3;
    U32_2 = 10;
    U32_1 = U32_1 * U32_2;
    wc86_assertU32("3*10==30", U32_1, 30);
    
    // Unsigned large value
    //00401AF4   0FAFC1           IMUL EAX,ECX
    U32_1 = 4051677952;
    U32_2 = 4119798720;
    U32_1 = U32_1 * U32_2;
    wc86_assertU32("4051677952*4119798720==2453618688", U32_1, 2453618688);
    
    // I16 test
    //00401B2C   0FAFC1           IMUL EAX,ECX
    I16_1 = 32000;
    I16_2 = 100;
    I16_1 = I16_1 * I16_2;
    wc86_assertI32("(I16) 32000*100==-11264", I16_1, -11264);
    
    // U16 test
    //00401B66   0FAFC1           IMUL EAX,ECX
    U16_1 = 65472;
    U16_2 = 64000;
    U16_1 = U16_1 * U16_2;
    wc86_assertI32("(U16) 65472*64000==32768", U16_1, 32768);
    
    // U64 test (three muls for this 1 operation?)
    //00401BAC   F7E1             MUL ECX
    //00401BBA   0FAFC8           IMUL ECX,EAX
    //00401BC9   0FAFC2           IMUL EAX,EDX
    U64_1 = 0x1000000000000000ULL;
    U64_2 = 1000;
    U64_1 = U64_1 * U64_2;
    wc86_assert("(U64) 0x1000000000000000ULL*1000==9223372036854775808", U64_1 == 9223372036854775808ULL);
    
    //////////////////////////////////////////////////////
    // Opcode: 0F B6
    //////////////////////////////////////////////////////
    // Instructions:
    // MOVZX (Move with Zero-Extend)
    //////////////////////////////////////////////////////
    
    //00401B96   0FB645 EB        MOVZX EAX,BYTE PTR SS:[EBP-15]
    U32_1 = 0x02020202;
    U8_1 = 0x66;
    U32_1 = U8_1;
    wc86_assertU32("U32_1=U8_1=0x66==0x66", U32_1, 0x66);

    //////////////////////////////////////////////////////
    // Opcode: 0F B7
    //////////////////////////////////////////////////////
    // Instructions:
    // MOVZX (Move with Zero-Extend)
    //////////////////////////////////////////////////////
    
    //00401BC6   0FB745 D2        MOVZX EAX,WORD PTR SS:[EBP-2E]
    U32_1 = 0x02020202;
    U16_1 = 0x7788;
    U32_1 = U16_1;
    wc86_assertU32("U32_1=U16_1=0x7788==0x7788", U32_1, 0x7788);
    
    //////////////////////////////////////////////////////
    // Opcode: 0F BD
    //////////////////////////////////////////////////////
    // Instructions:
    // BSR (Bit Scan Reverse)
    //////////////////////////////////////////////////////
    
    // There is a lot which happens here. From what I can tell TCC only generates BSR functions when dealing with int64
    // numbers. When we have a long division like this, a whole bunch of code gets generated (including a BSR instruction).
    // - This happens within its own seperate function (there are 1-2 calls just for this operation).
    // - The generated code occurs for division of int64 / int64 (non immediate values).
    // NOTE: This currently isn't hitting the BSR instruction. TODO: Find the code path which hits the instruction.
    //00401D8E   0FBDC0           BSR EAX,EAX
    U64_1 = 0x1000000000000000ULL * 2;
    U64_2 = 0x400;
    U64_1 = U64_1 / U64_2;
    wc86_assert("(0x1000000000000000ULL*2)/0x400==0x8000000000000ULL", U64_1 == 0x8000000000000ULL);
    
    //////////////////////////////////////////////////////
    // Opcode: 0F BE
    //////////////////////////////////////////////////////
    // Instructions:
    // MOVSX (Move with Sign-Extension)
    //////////////////////////////////////////////////////
    
    //00401C69   0FBE45 F7        MOVSX EAX,BYTE PTR SS:[EBP-9]
    I32_1 = 0x02020202;
    I8_1 = -120;
    I32_1 = I8_1;
    wc86_assertI32("I32_1=I8_1=-120==-120", I32_1, -120);
    
    //00401C98   0FBE45 F7        MOVSX EAX,BYTE PTR SS:[EBP-9]
    I32_1 = 0x02020202;
    I8_1 = 110;
    I32_1 = I8_1;
    wc86_assertI32("I32_1=I8_1=110==110", I32_1, 110);

    //////////////////////////////////////////////////////
    // Opcode: 0F BF
    //////////////////////////////////////////////////////
    // Instructions:
    // MOVSX (Move with Sign-Extension)
    //////////////////////////////////////////////////////
    
    //00401CC8   0FBF45 DE        MOVSX EAX,WORD PTR SS:[EBP-22]
    I32_1 = 0x02020202;
    I16_1 = -31244;
    I32_1 = I16_1;
    wc86_assertI32("I32_1=I16_1=-31244==-31244", I32_1, -31244);    
    
    //00401CF8   0FBF45 DE        MOVSX EAX,WORD PTR SS:[EBP-22]
    I32_1 = 0x02020202;
    I16_1 = 27358;
    I32_1 = I16_1;
    wc86_assertI32("I32_1=I16_1=27358==27358", I32_1, 27358);
    
    //////////////////////////////////////////////////////
    // Opcode: 10-15
    //////////////////////////////////////////////////////
    // Instructions:
    // ADC (Add with Carry)
    //////////////////////////////////////////////////////
    
    // I'm not certain this is even validating anything
    //00401D45   11D1             ADC ECX,EDX
    I64_1 = -1;
    I64_2 = 2;
    I64_1 += I64_2;
    wc86_assert("I64_1=-1+2==1", I64_1 == 1);
    
    //00401DAE   11D1             ADC ECX,EDX
    U64_1 = 0xFFFFFFFFFFFFFFFFULL;
    U64_2 = 4;
    U64_1 += U64_2;
    wc86_assert("U64_1=0xFFFFFFFFFFFFFFFFULL+4==3", U64_1 == 3);
    
    //////////////////////////////////////////////////////
    // Opcode: 18-1D
    //////////////////////////////////////////////////////
    // Instructions:
    // SBB (Integer Subtraction with Borrow)
    //////////////////////////////////////////////////////
    
    //00401E17   19D1             SBB ECX,EDX
    I64_1 = 6;
    I64_2 = 10;
    I64_1 -= I64_2;
    wc86_assert("I64_1=6-10==4", I64_1 == -4);
    
    //00401E80   19D1             SBB ECX,EDX
    U64_1 = 20;
    U64_2 = 0xFFFFFFFFFFFFFFFFULL;
    U64_1 -= U64_2;
    wc86_assert("U64_1=20-0xFFFFFFFFFFFFFFFFULL==21", U64_1 == 21);
    
    //////////////////////////////////////////////////////
    // Opcode: 20-25
    //////////////////////////////////////////////////////
    // Instructions:
    // AND (Logical AND)
    //////////////////////////////////////////////////////
    
    //00401ED1   21C8             AND EAX,ECX
    I32_1 = 0x04821;
    I32_2 = 0x14001;
    I32_1 = I32_1 & I32_2;
    wc86_assertI32("I32_1=0x04821&0x14001==0x04001", I32_1, 0x04001);
    
    //////////////////////////////////////////////////////
    // Opcode: 28-2D
    //////////////////////////////////////////////////////
    // Instructions:
    // SBB (Integer Subtraction with Borrow)
    //////////////////////////////////////////////////////
    
    //00401F04   29C8             SUB EAX,ECX
    I32_1 = 11;
    I32_2 = 24;
    I32_1 -= I32_2;
    wc86_assertI32("I32_1=11-24==-13", I32_1, -13);
    
    //00401FC9   29C8             SUB EAX,ECX
    I32_1 = 333;
    I32_2 = -444;
    I32_1 -= I32_2;
    wc86_assertI32("I32_1=333--444==777", I32_1, 777);
    
    //00401F37   29C8             SUB EAX,ECX
    U32_1 = 50;
    U32_2 = 0xFFFFFFFFU;
    U32_1 -= U32_2;
    wc86_assertU32("I32_1=50-0xFFFFFFFFU==51", U32_1, 51);
    
    //////////////////////////////////////////////////////
    // Opcode: 30-35
    //////////////////////////////////////////////////////
    // Instructions:
    // XOR (Logical Exclusive OR)
    //////////////////////////////////////////////////////
    
    //00401F6A   31C8             XOR EAX,ECX
    I32_1 = 0x403520;
    I32_2 = 0x413408;
    I32_1 = I32_1 ^ I32_2;
    wc86_assertI32("I32_1=0x403520^0x413408==0x010128", I32_1, 0x010128);
    
    //////////////////////////////////////////////////////
    // Opcode: 38-3D
    //////////////////////////////////////////////////////
    // Instructions:
    // CMP (Compare Two Operands)
    //////////////////////////////////////////////////////    
    
    // Skipping CMP opcodes as these should have already been covered by the conditional jumps (0F 80 - 0F 8F)
    
    //////////////////////////////////////////////////////
    // Opcode: 40-47
    //////////////////////////////////////////////////////
    // Instructions:
    // INC (Increment by 1)
    //////////////////////////////////////////////////////
    
    //00401F94   40               INC EAX
    I32_1 = -1;
    I32_1++;
    I32_1++;
    wc86_assertI32("I32_1=-1 ++ ++ ==1", I32_1, 1);
    
    //00401FD8   40               INC EAX
    U32_1 = 4294967294;// 1 off uint32 max
    U32_1++;
    U32_1++;
    U32_1++;
    U32_1++;
    wc86_assertU32("U32_1=4294967294 ++ ++ ++ ++ ==2", U32_1, 2);
    
    //////////////////////////////////////////////////////
    // Opcode: 48-4F
    //////////////////////////////////////////////////////
    // Instructions:
    // DEC (Decrement by 1)
    //////////////////////////////////////////////////////
    // Notes:
    // - TCC seems to generate 'ADD EAX,-1' for 'val--'
    //////////////////////////////////////////////////////
    
    // NOTE: val-- seems to generate ADD REG,-1
    
    //0040209A   48               DEC EAX
    //004020A1   48               DEC EAX
    I32_1 = 1;
    I32_1 = I32_1 - 1;
    I32_1 = I32_1 - 1;
    wc86_assertI32("I32_1=1 -- -- ==-1", I32_1, -1);
    
    //004020C8   48               DEC EAX
    //004020CF   48               DEC EAX
    U32_1 = 1;
    U32_1 = U32_1 - 1;
    U32_1 = U32_1 - 1;
    wc86_assertU32("U32_1=1 -- -- ==4294967295", U32_1, 4294967295);
    
    //////////////////////////////////////////////////////
    // Opcode: 50-57 (PUSH)
    // Opcode: 58-5F (POP)
    //////////////////////////////////////////////////////
    // Instructions:
    // PUSH (Push Word or Doubleword Onto the Stack)
    // POP (Pop a Value from the Stack)
    //////////////////////////////////////////////////////
    
    // Skipping PUSH / POP as they are used implicitly in a lot of places
    
    //////////////////////////////////////////////////////
    // Opcode: 64 (prefix: FS segment override)
    // Opcode: 66 (prefix: Precision size override)
    //////////////////////////////////////////////////////
    // Instructions:
    // These are prefixes and don't have their own instruction name
    //////////////////////////////////////////////////////
    
    // Skipping prefixes as they are used implicitly in a lot of places
    
    //////////////////////////////////////////////////////
    // Opcode: 6A
    //////////////////////////////////////////////////////
    // Instructions:
    // PUSH (Push imm8)
    //////////////////////////////////////////////////////
    // Notes:
    // - In TCC this only seems to be used as support codegen for floating point operations
    //////////////////////////////////////////////////////
    
    // Skipping this type of PUSH as it's only used as support for floating point operations
    
    //////////////////////////////////////////////////////
    // Opcode: 70 - 7F
    //////////////////////////////////////////////////////
    // Instructions:
    // Conditional jumps (see 0F 80 - 0F 8F)
    //////////////////////////////////////////////////////
    
    // Conditional jumps should have already been covered by the other conditional jump tests (0F 80 - 0F 8F)
    
    //////////////////////////////////////////////////////
    // Opcode: 80-83
    //////////////////////////////////////////////////////
    // Instructions:
    // ADD, OR, ADC, SBB, AND, SUB, XOR, CMP (immediate values)
    //////////////////////////////////////////////////////
    // Notes:
    // - Opcode 80 seems to only be used as part of floating point code gen?
    //////////////////////////////////////////////////////
    
    //004020F7   83C0 03          ADD EAX,3
    I8_1 = 127;
    I8_1 += 3;
    wc86_assertI32("I8_1=127+3==-126", I8_1, -126);
    
    //00402121   83C0 03          ADD EAX,3
    I32_1 = 2147483647;
    I32_1 += 3;
    wc86_assertI32("I32_1=2147483647+3==-2147483646", I32_1, -2147483646);
    
    //0040214A   83C0 03          ADD EAX,3
    U32_1 = 0xFFFFFFFE;
    U32_1 += 3;
    wc86_assertU32("U32_1=0xFFFFFFFE+3==1", U32_1, 1);
    
    //0040217B   83C0 04          ADD EAX,4
    U64_1 = 0xFFFFFFFFFFFFFFFEULL;
    U64_1 += 4;
    wc86_assert("U64_1=0xFFFFFFFFFFFFFFFEULL+4==2", U64_1 == 2);
    
    //00402122   81C0 01010000    ADD EAX,101
    I8_1 = 100;
    I8_1 += 257;
    wc86_assertI32("I8_1=100+257==101", I8_1, 101);
    
    //004021F7   83C0 C4          ADD EAX,-3C
    I8_1 = 50;
    I8_1 += -60;
    wc86_assertI32("I8_1=50+-60==-10", I8_1, -10);
    
    //004021F6   81C0 2C010000    ADD EAX,12C
    U32_1 = 0xFFFFFFFE;
    U32_1 += 300;
    wc86_assertU32("U32_1=0xFFFFFFFE+300==298", U32_1, 298);
    
    //00402224   C1F9 1F          SAR ECX,1F
    //00402227   81C0 AAFAFFAA    ADD EAX,AAFFFAAA
    //0040222D   81D1 AAFAFFFF    ADC ECX,-556
    I32_1 = 1000;
    I32_1 += 0xFFFFFAAAAAFFFAAAULL;
    wc86_assertI32("I32_1=1000+0xFFFFFAAAAAFFFAAAULL==-1426063726", I32_1, -1426063726);
    
    //00402259   81E8 FDFFFF7F    SUB EAX,7FFFFFFD
    I32_1 = 117483647;
    I32_1 -= 2147483645;
    wc86_assertI32("I32_1=117483647-2147483645==-2029999998", I32_1, -2029999998);
    
    //00402285   83E8 7D          SUB EAX,7D
    I32_1 = -2147483646;
    I32_1 -= 125;
    wc86_assertI32("I32_1=-2147483646-125==2147483525", I32_1, 2147483525);
    
    //004022AE   83E8 83          SUB EAX,-7D
    I32_1 = 2147483640;
    I32_1 -= -125;
    wc86_assertI32("I32_1=2147483640--125==-2147483531", I32_1, -2147483531);
    
    //004022D7   83E8 F6          SUB EAX,-0A
    U32_1 = 0xFFFFFFFE;
    U32_1 -= -10;
    wc86_assertU32("U32_1=0xFFFFFFFE--10==8", U32_1, 8);
    
    //////////////////////////////////////////////////////
    // Opcode: 84-85 (TEST)
    // Opcode: 86-87 (XCHG)
    //////////////////////////////////////////////////////
    // Instructions:
    // TEST / XCHG
    //////////////////////////////////////////////////////
    // Notes:
    // - TEST is only used for floating point operations (gen_opf) (and only the F6 opcode version)
    // - XCHG is only used by manually assembled code?
    //////////////////////////////////////////////////////
    
    // Skipping TEST / XCHG
    
    //////////////////////////////////////////////////////
    // Opcode: 88-8D
    //////////////////////////////////////////////////////
    // Instructions:
    // MOV (88-8C) / LEA (8D)
    //////////////////////////////////////////////////////    
    
    //00402358   8845 F6          MOV BYTE PTR SS:[EBP-A],AL
    //0040235B   8D45 F7          LEA EAX,DWORD PTR SS:[EBP-9]
    //0040235E   8945 F0          MOV DWORD PTR SS:[EBP-10],EAX
    //00402361   8B45 F0          MOV EAX,DWORD PTR SS:[EBP-10]
    //00402364   0FBE4D F6        MOVSX ECX,BYTE PTR SS:[EBP-A]
    //00402368   8808             MOV BYTE PTR DS:[EAX],CL
    //0040236A   8B45 C4          MOV EAX,DWORD PTR SS:[EBP-3C]
    I8_2 = 120;
    PI8_1 = &I8_1;
    PI8_1[0] = I8_2;
    wc86_assertI32("*PI8_1=I8_2=120==120", I8_1, 120);
    
    //////////////////////////////////////////////////////
    // Opcode: 90
    //////////////////////////////////////////////////////
    // Instructions:
    // NOP
    //////////////////////////////////////////////////////
    
    // No test required
    
    //////////////////////////////////////////////////////
    // Opcode: 99
    //////////////////////////////////////////////////////
    // Instructions:
    // CDQ
    //////////////////////////////////////////////////////

    //00402399   99               CDQ
    //0040239A   F7F9             IDIV ECX
    I32_1 = 8;
    I32_2 = 2;
    I32_1 = I32_1 / I32_2;
    wc86_assertI32("I32_1=8/2==4", I32_1, I32_1);
    
    //004023CB   99               CDQ
    //004023CC   F7F9             IDIV ECX
    I32_1 = 2147483647;
    I32_2 = 2;
    I32_1 = I32_1 / I32_2;
    wc86_assertI32("I32_1=2147483647/2==1073741823", I32_1, 1073741823);
    
    //004023FF   99               CDQ
    //00402400   F7F9             IDIV ECX
    I32_1 = 2147483647+1;
    I32_2 = 2;
    I32_1 = I32_1 / I32_2;
    wc86_assertI32("I32_1=(2147483647+1)/2==-1073741824", I32_1, -1073741824);
    
    //////////////////////////////////////////////////////
    // Opcode: B0-B7
    // Opcode: B8-BF
    //////////////////////////////////////////////////////
    // Instructions:
    // MOV
    //////////////////////////////////////////////////////
    // Notes:
    // - B0-B7 are unused by TCC?
    //////////////////////////////////////////////////////
    
    //00402425   B8 01000000      MOV EAX,1
    I8_1 = 1;
    wc86_assertI32("I8_1=1==1", I8_1, 1);
    
    //0040243E   B8 FFFFFFFF      MOV EAX,-1
    I8_1 = -1;
    wc86_assertI32("I8_1=1==1", I8_1, -1);

    //0040245F   B8 FFFFFFFF      MOV EAX,-1
    //00402464   B9 FFFFFFFF      MOV ECX,-1
    U64_1 = -1;
    wc86_assert("U64_1=-1==18446744073709551615", U64_1 == 18446744073709551615ULL);
    
    //////////////////////////////////////////////////////
    // Opcode: C0-C1
    //////////////////////////////////////////////////////
    // Instructions:
    // ROL, ROR, RCL, RCR, SHL, SHR, SAR
    //////////////////////////////////////////////////////
    
    //004024AE   C1E0 02          SHL EAX,2
    I8_1 = 3;
    I8_1 <<= 2;
    wc86_assertI32("I8_1=3<<2==12", I8_1, 12);
    
    //004024D9   C1E0 05          SHL EAX,5
    I8_1 = 3;
    I8_1 <<= 5;
    wc86_assertI32("I8_1=3<<5==96", I8_1, 96);
    
    //00402504   C1E0 06          SHL EAX,6
    I8_1 = 3;
    I8_1 <<= 6;
    wc86_assertI32("I8_1=3<<6==-64", I8_1, -64);
    
    //0040252F   C1E0 07          SHL EAX,7
    I8_1 = 3;
    I8_1 <<= 7;
    wc86_assertI32("I8_1=3<<7==-128", I8_1, -128);
    
    //0040255A   C1E0 08          SHL EAX,8
    I8_1 = 3;
    I8_1 <<= 8;
    wc86_assertI32("I8_1=3<<8==0", I8_1, 0);
    
    //00402585   C1E0 09          SHL EAX,9
    I8_1 = 3;
    I8_1 <<= 9;
    wc86_assertI32("I8_1=3<<9==0", I8_1, 0);
    
    //0040251B   C1F8 0D          SAR EAX,0D
    I16_1 = 16384;
    I16_1 >>= 13;
    wc86_assertI32("I16_1=16384>>13==2", I16_1, 2);
    
    //00402548   C1F8 0E          SAR EAX,0E
    I16_1 = 16384;
    I16_1 >>= 14;
    wc86_assertI32("I16_1=16384>>14==1", I16_1, 1);
    
    //00402575   C1F8 0F          SAR EAX,0F
    I16_1 = 16384;
    I16_1 >>= 15;
    wc86_assertI32("I16_1=16384>>15==0", I16_1, 0);
    
    //004025AA   C1E9 18          SHR ECX,18
    U64_1 = 0x8000000000000000;
    U64_1 >>= 24;
    wc86_assert("U64_1=0x8000000000000000>>24==0x8000000000", U64_1 == 0x8000000000);
    
    //00402609   C1F9 03          SAR ECX,3
    I64_1 = 1LL << 63;//-9223372036854775808
    I64_1 >>= 3;
    wc86_assert("I64_1=-9223372036854775808>>3==-1152921504606846976", I64_1 == -1152921504606846976);
    
    //00402666   C1F8 09          SAR EAX,9
    //00402669   89C1             MOV ECX,EAX
    //0040266B   C1F9 1F          SAR ECX,1F
    I64_1 = 1LL << 63;//-9223372036854775808
    I64_1 >>= 41;
    wc86_assert("I64_1=-9223372036854775808>>41==-4194304", I64_1 == -4194304);
    
    //004026BD   C1F8 1E          SAR EAX,1E
    //004026C0   89C1             MOV ECX,EAX
    //004026C2   C1F9 1F          SAR ECX,1F
    I64_1 = 1LL << 63;//-9223372036854775808
    I64_1 >>= 62;
    wc86_assert("I64_1=-9223372036854775808>>62==-2", I64_1 == -2);
    
    //00402711   C1F8 01          SAR EAX,1
    //00402714   89C1             MOV ECX,EAX
    //00402716   C1F9 1F          SAR ECX,1F
    // This isn't really "correct" behaviour but this the native result on an x86 machine (when compiled with TCC)
    // It's probably worth keeping this test to ensure compatibility with the native result (which is all we care about)
    I64_1 = 1LL << 63;//-9223372036854775808
    I64_1 >>= 65;
    wc86_assert("I64_1=-9223372036854775808>>65==-1073741824 (remove this test?)", I64_1 == -1073741824);
    
    //////////////////////////////////////////////////////
    // Opcode: C2-C3 (RETN)
    // Opcode: C9 (LEAVE)
    //////////////////////////////////////////////////////
    // Instructions:
    // RETN, LEAVE
    //////////////////////////////////////////////////////
    
    testRetVoid();
    wc86_assert("testRetVoid()", 1);
    wc86_assert("testRetI8()==-12", testRetI8() == -12);
    wc86_assert("testRetU8()==254", testRetU8() == 254);
    wc86_assert("testRetI32()==-1600", testRetI32() == -1600);
    wc86_assert("testRetU32()==2", testRetU32() == 2);
    wc86_assert("testRetI64()==-140737488355328LL", testRetI64() == -140737488355328LL);
    wc86_assert("testRetU64()==140737488355328ULL", testRetU64() == 140737488355328ULL);
    
    //////////////////////////////////////////////////////
    // Opcode: D0-D3
    //////////////////////////////////////////////////////
    // Instructions:
    // ROL, ROR, RCL, RCR, SHL, SHR, SAR
    //////////////////////////////////////////////////////
    
    // Repeat the tests from C0-C1 but without using immediate values (store in a temp variable first)
    
    I8_1 = 3;
    I8_2 = 2;
    I8_1 <<= I8_2;
    wc86_assertI32("I8_1=3<<2==12", I8_1, 12);
    
    I8_1 = 3;
    I8_2 = 5;
    I8_1 <<= I8_2;
    wc86_assertI32("I8_1=3<<5==96", I8_1, 96);
    
    I8_1 = 3;
    I8_2 = 6;
    I8_1 <<= I8_2;
    wc86_assertI32("I8_1=3<<6==-64", I8_1, -64);
    
    I8_1 = 3;
    I8_2 = 7;
    I8_1 <<= I8_2;
    wc86_assertI32("I8_1=3<<7==-128", I8_1, -128);
    
    I8_1 = 3;
    I8_2 = 8;
    I8_1 <<= I8_2;
    wc86_assertI32("I8_1=3<<8==0", I8_1, 0);
    
    I8_1 = 3;
    I8_2 = 9;
    I8_1 <<= I8_2;
    wc86_assertI32("I8_1=3<<9==0", I8_1, 0);

    I16_1 = 16384;
    I16_2 = 13;
    I16_1 >>= I16_2;
    wc86_assertI32("I16_1=16384>>13==2", I16_1, 2);
    
    I16_1 = 16384;
    I16_2 = 14;
    I16_1 >>= I16_2;
    wc86_assertI32("I16_1=16384>>14==1", I16_1, 1);
    
    I16_1 = 16384;
    I16_2 = 15;
    I16_1 >>= I16_2;
    wc86_assertI32("I16_1=16384>>15==0", I16_1, 0);
    
    U64_1 = 0x8000000000000000;
    U64_2 = 24;
    U64_1 >>= U64_2;
    wc86_assert("U64_1=0x8000000000000000>>24==0x8000000000", U64_1 == 0x8000000000);
    
    I64_1 = 1LL << 63;//-9223372036854775808
    I64_2 = 3;
    I64_1 >>= I64_2;
    wc86_assert("I64_1=-9223372036854775808>>3==-1152921504606846976", I64_1 == -1152921504606846976);
    
    I64_1 = 1LL << 63;//-9223372036854775808
    I64_2 = 41;
    I64_1 >>= I64_2;
    wc86_assert("I64_1=-9223372036854775808>>41==-4194304", I64_1 == -4194304);
    
    I64_1 = 1LL << 63;//-9223372036854775808
    I64_2 = 62;
    I64_1 >>= I64_2;
    wc86_assert("I64_1=-9223372036854775808>>62==-2", I64_1 == -2);

    I64_1 = 1LL << 63;//-9223372036854775808
    I64_2 = 65;
    I64_1 >>= I64_2;
    wc86_assert("I64_1=-9223372036854775808>>65==-1073741824 (remove this test?)", I64_1 == -1073741824);
    
#if TEST_FLOAT
    
    // TODO: Floating point value tests
    // D8    | FSUB
    // D9    | FLDS / FSTS / FLD / FSTP / FXCH
    // DD    | FLDL / FSTPL / FSTP
    // DB    | FLDT / FSTPT
    // DA    | FUCOMPP
    // DE    | FCOMPP
    // DF    | FNSTSW / FILDLL / FILD
    // DC    | FMUL
    
#endif

    //////////////////////////////////////////////////////
    // Opcode: E8 (CALL)
    // Opcode: E9-EB (JMP)
    //////////////////////////////////////////////////////
    // Instructions:
    // CALL, JMP, JMPF, JMP NEAR
    //////////////////////////////////////////////////////
    // Notes:
    // - I don't think TCC uses JMPF? (0xEA)
    //////////////////////////////////////////////////////
    
    // Skipping as we tested CALL when testing RETN and there are various generated JMP instructions already    
    
    //////////////////////////////////////////////////////
    // Opcode: F6-F7
    //////////////////////////////////////////////////////
    // Instructions:
    // TEST, NOT, NEG, MUL, IMUL, DIV, IDIV
    //////////////////////////////////////////////////////
    // Notes:
    // - NOT / NEG aren't used by TCC?
    // - TEST is only used for floating point operations (gen_opf)
    //////////////////////////////////////////////////////
    
    // Most multiplication operations seem to generate the 0FAF opcode?
    
    //00402E8A   F7E1             MUL ECX
    U64_1 = (1ULL << 5);//32ULL
    U64_1 *= (U64_2 = (1ULL << 50));//1125899906842624ULL
    wc86_assert("U64_1=32*1125899906842624==36028797018963968", U64_1 == 36028797018963968ULL);
    
    //00402D35   F7F9             IDIV ECX
    I8_1 = -122;
    I8_1 /= -3;
    wc86_assertI32("I8_1=-122/-3==40", I8_1, 40);
    
    //00402D65   F7F9             IDIV ECX
    U8_1 = 122;
    U8_1 /= 5;
    wc86_assertI32("U8_1=122/5==24", U8_1, 24);
    
    //00402D95   F7F9             IDIV ECX
    U8_1 = 122;
    U8_1 /= 3;
    wc86_assertI32("U8_1=122/3==40", U8_1, 40);
    
    //00402DC5   F7F9             IDIV ECX
    I8_1 = -122;
    I8_1 /= -2;
    wc86_assertI32("U8_1=-122/-2==61", I8_1, 61);
    
    //00402DF4   F7F9             IDIV ECX
    I32_1 = 973741824;
    I32_1 /= -3741824;
    wc86_assertI32("I32_1=973741824/-3741824==-260", I32_1, -260);
    
    //00402E32   F7F1             DIV ECX
    U32_1 = 3333;
    U32_1 /= (U32_2 = 11);
    wc86_assertU32("U32_1=3333/11==303", U32_1, 303);
    
    //00402E70   F7F1             DIV ECX
    U32_1 = 4026531840;
    U32_1 /= (U32_2 = 99);
    wc86_assertU32("U32_1=4026531840/99==40672038", U32_1, 40672038);
    
    //00402EA4   F7F9             IDIV ECX
    //00402EA6   B9 05000000      MOV ECX,5
    //00402EAB   99               CDQ
    //00402EAC   F7F9             IDIV ECX
    I32_1 = 0x7FFFFFFF;
    I32_2 = 3;
    I32_1 = I32_1 / I32_2 / 5;
    wc86_assertU32("I32_1=0x7FFFFFFF/3/5==143165576", I32_1, 143165576);
    
    //////////////////////////////////////////////////////
    // Opcode: FF
    //////////////////////////////////////////////////////
    // Instructions:
    // CALLN, JUMPN, PUSH
    //////////////////////////////////////////////////////
    
    // Skipping as these should be covered by API calls such as printf (this is possibly only true for JUMPN)
    
    //////////////////////////////////////////////////////
    // Additional misc tests
    //////////////////////////////////////////////////////
    
    // NOT test (this doesn't use a NOT opcode)
    I8_1 = 100;
    I8_1 = !I8_1;
    wc86_assertI32("I8_1=!100==0", I8_1, 0);
    
    // NOT test (this doesn't use a NOT opcode)
    I8_1 = -100;
    I8_1 = !I8_1;
    wc86_assertI32("I8_1=!-100==0", I8_1, 0);
    
    // NOT test (this doesn't use a NOT opcode)
    I8_1 = 0;
    I8_1 = !I8_1;
    wc86_assertI32("I8_1=!0==1", I8_1, 1);
    
    // NEG test (this doesn't use a NEG opcode)
    I8_1 = 50;
    I8_1 = -I8_1;
    wc86_assertI32("I8_1=50=-==-50", I8_1, -50);
    
    // NEG test (this doesn't use a NEG opcode)
    I8_1 = -50;
    I8_1 = -I8_1;
    wc86_assertI32("I8_1=-50=-==50", I8_1, 50);
    
    // NEG test (this doesn't use a NEG opcode)
    I8_1 = 0;
    I8_1 = -I8_1;
    wc86_assertI32("I8_1=0=-==0", I8_1, 0);
    
    wc86_assert("============== Tests finished ==============", 1);
    
    return 0;
}

void testRetVoid()
{
}

int8_t testRetI8()
{
    int8_t I8_1 = 5;
    int8_t I8_2 = 100;
    return I8_1 * I8_2;//-12
}

uint8_t testRetU8()
{
    uint8_t U8_1 = 127;
    return U8_1 * 2;//254
}

int32_t testRetI32()
{
    int32_t I32_1 = 800;
    int32_t I32_2 = -2;
    return I32_1 * I32_2;//-1600
}

uint32_t testRetU32()
{
    uint32_t U32_1 = 0xFFFFFFFF;
    uint32_t U32_2 = 3;
    return U32_1 + U32_2;//2
}

int64_t testRetI64()
{
    return -140737488355328LL;
}

uint64_t testRetU64()
{
    return 140737488355328ULL;
}