extern void func_0282c0(), func_00cc82(), WaitForNextVBlank(), SpriteLoadingStuff();
extern char VBLANK_FLAG_2;

/* Keep the selector order: the repeated target is hoisted into a preserved
 * address register, and the original combines the argument stack cleanups. */
void func_02836c()
{
    func_0282c0(0);
    func_0282c0(4);
    func_0282c0(1);
    func_0282c0(2);
    func_0282c0(5);
    func_0282c0(6);
    func_0282c0(7);
    VBLANK_FLAG_2 = 0;
    func_00cc82();
    WaitForNextVBlank(1);
    SpriteLoadingStuff(0, 0);
}
