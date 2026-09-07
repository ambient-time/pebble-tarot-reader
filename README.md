# Tarot Reader

A pocket reading table for Pebble. Draw a card, turn it over, and spend a moment
with its meaning. The complete 78-card Rider–Waite–Smith deck lives on the watch,
along with A. E. Waite's upright and reversed meanings. Everything works offline.

Choose a single card, a past/present/emerging three-card spread, the ten-card
Celtic Cross, or an open table with five, six, or seven cards. The Celtic Cross
keeps the arrangement from my web reader. Open Table scatters the cards and
reads their positions as past, present, or emerging influences, with visible
influences above and quieter ones below.

Up and Down move between cards. Select opens a card, reveals it, then opens its
meaning. A single-card draw goes straight to the card. Hold Select on revealed
artwork to enlarge it; Up and Down pan the image, and Back restores the full
card. Select also opens the meaning from the enlarged view. Up and Down scroll
the text; Back returns one step. A filled dot marks
a revealed card in the overview. The current selection has a light background.
Menus use larger, bold type. Meanings appear as black text on a white page;
color is limited to the brass card borders on color watches.
Hold Back to leave the app.

The last reading stays on the watch, including its revealed cards and their
orientations. Choose **Continue** to return. A new draw replaces that
reading. Turn reversals off in the menu to make future draws upright only.

Meanings quote Waite's historical text, including its dated language. The spread
notes use shorter, gender-neutral wording. Readings offer prompts for reflection.

## Credits

App and adaptation: Luke Steuber. Illustrations: Pamela Colman Smith. Meanings:
A. E. Waite, *The Pictorial Key to the Tarot* (1910). The original illustrations
and text are public domain.

The artwork comes from the [Wikimedia Commons Pictorial Key collection](https://commons.wikimedia.org/wiki/Category:Pictorial_Key_to_the_Tarot),
through the local deck at [datapoems.io](https://datapoems.io/clocks/shared/tarot/rws/).
The text packet uses [ekelen/tarot-api](https://github.com/ekelen/tarot-api).
The app bundles the packet and never calls that service.

## Build and validation

Requires Pebble SDK 4.33.1, Python with Pillow, and a C compiler for the draw tests.

```sh
python3 tools/assets.py
pebble build
cc -std=c99 -Wall -Wextra -fsanitize=address,undefined \
  test/reading.c src/c/reading.c -o build/test-reading
build/test-reading
python3 test/assets.py
# Run with the Python interpreter that has pebble-tool installed:
python test/emulator.py diorite --fresh
```

Declared targets: Basalt, Chalk, Diorite, Emery, Flint, and Gabbro. This package
is a watchapp, with button input and no background clock loop. It does not target
the original Aplite watches. The store listing has not been published.

`reference/cards.json` pins the complete supplied text. `reference/art/` holds
the 78 original snapshots. `reference/sources.json` records their hashes and
the three supplied HTML examples. `tools/assets.py` creates monochrome artwork
at each display size without cropping the original card. Card names also appear
as native text; the reading screen always shows the full name.

Only the current card and its text enter memory. There is no network permission,
phone component, account, analytics, or collection of questions. The watch stores
one reading and the reversals preference locally.

The draw uses a seeded Fisher–Yates shuffle without replacement. Celtic Cross
preserves the web example's 28% reversal chance; other spreads use 50%. Draws are
pseudorandom. The same seed reproduces a reading for testing.

The original 0.1.0 menu was installed and captured on a Time 2.
The 0.2.0 readability changes are covered in [the review](reference/readability-review.md).

See [evidence/readability-v0.2/validation.json](evidence/readability-v0.2/validation.json) for the tested binary,
native targets, and screenshots. Emulator checks do not establish physical-watch
readability or battery performance.
