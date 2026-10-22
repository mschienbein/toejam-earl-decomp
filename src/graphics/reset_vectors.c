/* The first six bytes are untouched. The routine resets two interleaved
 * groups of three 16-bit components, in component order. */
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
