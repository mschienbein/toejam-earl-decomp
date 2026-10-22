/* StopPlayer (name from RyanFae's disassembly) leaves the first six bytes alone and
 * clears two interleaved groups of three 16-bit components, in component
 * order. [INFERENCE] They are the motion components of a player record. */
struct resettable_vectors {
    unsigned char header[6];
    short first[3];
    short second[3];
};

void StopPlayer(state)
struct resettable_vectors *state;
{
    state->first[0] = 0;
    state->second[0] = 0;
    state->first[1] = 0;
    state->second[1] = 0;
    state->first[2] = 0;
    state->second[2] = 0;
}
