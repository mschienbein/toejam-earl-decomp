#ifndef TJE_ASSETS_PALETTES_H
#define TJE_ASSETS_PALETTES_H

/* Shared ROM palettes and palette-selection tables. Unknown bounds remain
 * unspecified; asset partitions do not establish logical array bounds. */
extern unsigned short FunkatronPaletteA[];
extern unsigned short FunkatronPaletteB[];
extern unsigned char LevelNumberPalettes[10];
extern short PaletteAnimationFramesA[4][4];
extern short PaletteAnimationFramesB[4][4];
extern unsigned short PaletteGameLine1[];
extern unsigned short PaletteGameLine2[];
extern unsigned short PaletteGameLine3[];
extern unsigned short PaletteGameLine4[];
extern unsigned short PaletteMapLine1[];
extern unsigned short PaletteMapLine2[];
extern unsigned short PaletteMapLine3[];
extern unsigned short PaletteShipScreen_bank3[];
extern unsigned short PaletteTitleScreen_bank1[];
extern unsigned short PaletteAllBlack[];
extern unsigned short PaletteShipLine1[];
extern unsigned short PaletteShipLine2[];
extern unsigned short PaletteShipLine3[];

/* Additional selected final-batch storage and resource interfaces. */
extern unsigned short EndingPaletteA[];
extern unsigned short EndingPaletteB[];
extern unsigned short MenuHeaderPalette[];
extern unsigned short PaletteIntroCollision_bank2[];
extern unsigned short PaletteIntroEarlDriving_bank0[];
extern unsigned short PaletteIntroEarlDriving_bank3[];
extern unsigned short PaletteSpaceScreen_bank2[];
extern unsigned short PaletteSpaceScreen_bank3[];
extern unsigned short PaletteTitleScreen_bank0[];
extern unsigned short PaletteTitleScreen_bank2[];
extern unsigned short PaletteTitleScreen_bank3[];
extern unsigned short SegaLogo_Pal[];

#endif
