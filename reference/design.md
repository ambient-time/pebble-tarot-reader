# From the table to the watch

The supplied Celtic Cross and its occult variation share the same 78-card
packet, ten positions, shuffle, and 28% reversal probability. Their different
backgrounds do not require separate apps. This version keeps green felt, white
cards, a brass border, and purple backs on color watches. Monochrome watches use
black and white throughout.

The ten-card arrangement serves as a navigation map. Select opens an individual
card at a readable size; its meaning has a separate scrolling screen. A single
card draw and a three-card spread offer shorter readings. The open cast retains
five-to-seven-card choices, separated random positions, 50% reversals, and the
source's left/center/right and upper/lower interpretation. This first watch version
omits the source's loose rotations and pair-distance commentary.

The app waits for Select before each reveal. It replaces the browser's automatic
dealing sequence with deliberate button input. Reveals give a short vibration;
the artwork stays still afterward. Readings persist when the app closes.

## Pebble app research

The [official button conventions](https://developer.repebble.com/guides/events-and-services/buttons/)
map Up/Down to navigation, Select to opening or accepting, and Back to returning.
The [recommended interface patterns](https://developer.repebble.com/guides/design-and-interaction/recommended/)
also call for immediate feedback. Those conventions fit a card reader directly.

The [Spring 2026 contest gallery](https://developer.repebble.com/spring2026contest)
includes pinball, Go, miniature golf, adventure games, and virtual pets. The
contest has ended; the gallery remains useful for inspecting small-screen apps.

For a later arcade game, [Harrison Allen's GBC Graphics](https://github.com/HarrisonAllen/pebble-gbc-graphics)
offers tile and sprite rendering, a starter, and the documented Tiny Pilot game.
Its README recommends the basic engine for Aplite and the advanced engine when
memory permits. No repository-wide license appeared in the GitHub metadata
checked for this review; inspect the relevant files and permissions before reuse.

[PebblePinball](https://github.com/Bod9001/PebblePinball) is an AGPL-3.0 game with
phone-loaded custom maps. [Neographics](https://github.com/pebble-dev/neographics)
is another graphics-library reference, with its own license file to review.
Neither is a dependency here. Native drawing and button handlers cover this
reader without a game engine.

[ekelen/tarot-api](https://github.com/ekelen/tarot-api) is directly relevant: it
packages Waite's text and explicitly invites reuse of its JSON data. The packet
already appears in the supplied examples. The native app uses only that data.
