extern unsigned short ram_ff91e0;
extern unsigned short PREGEN_RANDOMS[59];
#define VDP_DATA (*(unsigned short *)0xc00000L)

/* Map ASCII and embedded control characters to font glyphs. Some controls
 * emit preceding tile words before returning the final glyph. The question
 * mark cycles through 59 words to choose one of four replacement glyphs.
 * MCC68K 3.3K has no volatile keyword; these explicit MMIO writes are retained
 * by its matching default optimization profile. */
char func_009e2e(character)
char character;
{
    if (character >= 'a' && character <= 'z')
        character += 1 - 'a';
    else if (character >= 'A' && character <= 'Z')
        character += 1 - 'A';
    else if (character >= '0' && character <= '9')
        character += 36 - '0';
    else switch (character) {
    case '?':
        character = (PREGEN_RANDOMS[ram_ff91e0 == 58 ?
                     (ram_ff91e0 = 0) : ++ram_ff91e0] & 3) + 27;
        break;
    case '.': character = 31; break;
    case ',': character = 32; break;
    case ':':
    case '\'': character = 33; break;
    case '-': character = 34; break;
    case '!': character = 35; break;
    case 1: character = 46; break;
    case 2:
        VDP_DATA = 0x85e3;
        VDP_DATA = 0x85e4;
        character = 49;
        break;
    case 3:
        VDP_DATA = 0x85e6;
        character = 51;
        break;
    case 4:
        VDP_DATA = 0x85e8;
        character = 53;
        break;
    case 5:
        VDP_DATA = 0x85ea;
        VDP_DATA = 0x85eb;
        character = 56;
        break;
    case 6:
        VDP_DATA = 0x85ed;
        character = 58;
        break;
    default: character = 0;
    }
    return character;
}
